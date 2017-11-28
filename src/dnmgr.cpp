
#include "dnmgr.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "container_impl.h"
#include "dbg_log.h"
#include "simple_types.h"
#include "env.h"

using namespace GLOBAL_ENV ;

/* 
 * class dnGroups
 */
dnGroups::dnGroups() : m_nDnGroups(0)
{
  /* create datanode groups */
  create_grp_by_conf();
}

dnGroups::~dnGroups(void)
{
}

int dnGroups::create_grp_by_conf(void)
{
  safeDataNodeList *pNodeGroups = 0;

  /* delete managed object */
  if (m_dnGroup.use_count()>0) {
    m_dnGroup.reset();
  }

  m_nDnGroups = m_conf.get_dn_group_count();
  if (m_nDnGroups>=m_maxDnGroup || m_nDnGroups==0) {
    m_nDnGroups = m_defDnGroup;
  }
  log_print("takes %d datanode groups\n", m_nDnGroups);

  /* allocates new datanode groups */
  pNodeGroups = new safeDataNodeList[m_nDnGroups] ;
  m_dnGroup = std::shared_ptr<safeDataNodeList>(pNodeGroups);

  return 0;
}

safeDataNodeList* dnGroups::get_specific_group(int gid)
{
  safeDataNodeList *pNodeGroups = 0;

  if (gid>=m_nDnGroups) {
    return NULL;
  }

  if (m_dnGroup.use_count()<=0) {
    return NULL ;
  }

  pNodeGroups = m_dnGroup.get();
  if (!pNodeGroups) {
    return NULL ;
  }

  return &pNodeGroups[gid];
}

int dnGroups::get_free_group(safeDataNodeList* &nodes, int &gid)
{
  safeDataNodeList *pNodeGroups = 0;

  /* no free groups available */
  if (m_freeGroupIds.pop(gid)) {
    return -1;
  }

  if (m_dnGroup.use_count()<=0) {
    return -1 ;
  }

  pNodeGroups = m_dnGroup.get();
  if (!pNodeGroups) {
    return -1 ;
  }

  nodes = &pNodeGroups[gid] ;

  log_print("acquired datanode group %d\n",gid);

  return 0;
}

int dnGroups::return_group(int gid)
{
  if (gid>=0&&gid<m_nDnGroups) {
    m_freeGroupIds.push(gid);

    log_print("release datanode group %d\n",gid);
  }
  return 0;
}

int dnGroups::get_num_groups(void) const
{
  return m_nDnGroups ;
}


/*
 * class dnmgr
 */
namespace {

  dnmgr *m_gDnMgr = 0 ;

  void reload(int signo)
  {
    if (m_conf.reload()) {
      /* TODO: should throw an exception here */
      return ;
    }

    /* update the datanodes */
    if (m_gDnMgr && m_gDnMgr->initialize()) {
      return ;
    }

    //printf("updated configs\n");
  }

}

dnmgr::dnmgr() 
{
  /* FIXME: I know it's ugly, but HOW? */
  m_gDnMgr = this ;

  initialize();

  /* register posix signals */
  if (signal(SIGUSR2,reload)==SIG_ERR) {
    return ;
  }
}

dnmgr::~dnmgr() 
{
}

int dnmgr::initialize(void)
{

  if (get_tables_by_conf()) {
    /* TODO: error messages here */
    return -1;
  }
  if (get_datanodes_by_conf()) {
    /* TODO: error messages here */
    return -1;
  }

  /* update connections */
  if (update_dn_conn()) {
    /* TODO: error messages here */
    return -1;
  }    

  /* refresh table structs */
  if (refresh_tbl_info()) {
    return -1;
  }

  return 0;
}

int dnmgr::get_tables_by_conf(void)
{
  uint16_t i=0, n=0, m=0;
  SCHEMA_BLOCK *sch ;
  TABLE_INFO *tbl ;
  MAPPING_INFO *mi = 0 ;
  int idn = 0;

  /* reset table items */
  m_tables.clear();

  /* iterate schema list */
  for (i=0;i<m_conf.get_num_schemas();i++) {
    sch = m_conf.get_schema(i);
    /* iterate table list of schema */
    for (n=0;n<m_conf.get_num_tables(sch);n++) {
      tbl = m_conf.get_table(sch,n);
      /* add to table list */
      m_tables.add((char*)sch->name.c_str(),
        (char*)tbl->name.c_str());
      log_print("db table `%s.%s` is added to list\n",
         sch->name.c_str(),tbl->name.c_str());
      /* add data node mapping info */
      for (m=0;m<tbl->map_list.size();m++) {
        mi = tbl->map_list[m] ;
        idn= m_conf.get_dataNode((char*)mi->dataNode.c_str());
        if (idn<0) {
          log_print("data node index of %s not found!!\n",
            mi->dataNode.c_str());
          continue ;
        }
        m_tables.add((char*)sch->name.c_str(),
          (char*)tbl->name.c_str(),idn,mi->io_type);
        log_print("added data node %d(%s) io type %d to "
          "`%s.%s` in table list\n", idn,mi->dataNode.c_str(),
          mi->io_type,sch->name.c_str(),tbl->name.c_str());
      }
    }
    log_print("table count %zu\n",m_tables.size());
  } /* end for(i=0) */

  return 0;
}

int dnmgr::update_tbl_extra_info(tDNInfo *pd, tTblDetails *pt)
{
  uint8_t kt = 0;
  bool nullable = false;
  char buf[128], *dtype=0, *cname=0, *def=0, *extra=0;
  uint16_t n=0;
  MYSQL_RES *mr = 0;
  char **results = 0;

  /* only query table structure, no need result set */
  sprintf(buf,"desc %s.%s",pt->phy_schema.c_str(),
    pt->table.c_str());
  /* execute the sql */
  if (mysql_query(pd->mysql,buf)) {
    log_print("execute sql %s failed: %s\n",
      buf,mysql_error(pd->mysql));
    return -1;
  }
  log_print("********extra info of table %s.%s*********\n",
    pt->schema.c_str(),pt->table.c_str());
  /* get query result */
  mr = pd->mysql->res ;
  /* initiates the iteration */
  mysql_store_result(pd->mysql);
  while ((results=mysql_fetch_row(mr))) {
    for (n=0;n<mysql_num_fields(mr);n++) {
      switch (n) {
        /* column name */
        case 0: cname = results[n] ;
          break ;
        /* display type name */
        case 1: dtype = results[n] ;
          break ;
        /* null-able */
        case 2: nullable = strcasecmp(results[n],"no") ;
          break ;
        /* key type */
        case 3:
          kt = !strncasecmp(results[n],"pri",3)?0:
            !strncasecmp(results[n],"mul",3)?1:
            !strncasecmp(results[n],"uni",3)?2:0xff;
          break;
        /* default value */
        case 4: 
          def = !strcasecmp(results[n],"null") ||
            results[n][0]=='\0'?NULL:results[n];
          break ;
        /* extra */
        case 5: 
          extra = !strcasecmp(results[n],"null") ||
            results[n][0]=='\0'?NULL:results[n];
          break ;
      }
      log_print("column %d: %s\n", n,
        results[n][0]=='\0'?"null":results[n]);
    }
    /* add extra infos to table list */
    m_tables.add(const_cast<char*>(pt->schema.c_str()),
      const_cast<char*>(pt->table.c_str()),cname,dtype,
      nullable,kt,def,extra);
  }
  log_print("******** %zu columns in %s ********\n", 
    pt->num_cols, pt->table.c_str());
  return 0;
}

int dnmgr::update_tbl_struct(tDNInfo *pd, tTblDetails *pt)
{
  uint16_t nCol=0;
  char buf[128];
  MYSQL_FIELD *mf ;

  /* only query table structure, no need result set */
  sprintf(buf,"select *from %s.%s where 1<>1",
    pt->phy_schema.c_str(),pt->table.c_str());
  /* execute the sql */
  if (mysql_query(pd->mysql,buf)) {
    log_print("execute sql %s failed: %s\n",
      buf,mysql_error(pd->mysql));
    return -1;
  }
  log_print("********struct of table %s.%s*********\n",
    pt->schema.c_str(),pt->table.c_str());
  for (nCol=0,pt->num_cols=0;nCol<pd->mysql->columns.number;nCol++) {
    /* extract column details */
    mf = ((MYSQL_FIELD*)pd->mysql->columns.c)+nCol ;
    log_print("column %d: %s, charset: %d, type: %d, "
      "max len: %d, flags: 0x%x\n",nCol,mf->name,
      mf->charset,mf->type,mf->len,mf->flags);
    /* add column to table list */
    m_tables.add(const_cast<char*>(pt->schema.c_str()),mf->tbl,
      mf->name,mf->charset,mf->len,mf->type,mf->flags);
  }
  log_print("******** %zu columns in %s ********\n", 
    pt->num_cols, pt->table.c_str());
  return 0;
}

int dnmgr::refresh_tbl_info(void)
{
  tDNInfo *pd = 0;
  tTblDetails *pt = 0;
  safe_container_base<uint64_t,tTblDetails*>::ITR_TYPE itr ;
  struct in_addr ia ;
  safeDataNodeList *nodes = 0;
  int gid = 0;

  /* get a free datanode group */
  get_free_group(nodes,gid);

  /* query structure of all tables in list */
  for (pt=m_tables.next(itr,true);pt;(pt=m_tables.next(itr))) {
    /*log_print("try querying structure of %s.%s at data node %d\n",
      pt->phy_schema.c_str(),pt->table.c_str(),pt->dn);*/

    if (!(pd=nodes->get(pt->dn))) {
      log_print("data node %d is NOT found!!\n",
        pt->dn);
      continue ;
    }
    //log_print("fetching table info from datanode %d\n",pt->dn);
    if (pd->stat==s_invalid) {
      ia.s_addr = htonl(pd->addr) ;
      log_print("data node %d(@%s:%d) is not active!!\n",
        pt->dn,inet_ntoa(ia),pd->port);
      continue ;
    }
    //log_print("sql: %s\n", buf);
    update_tbl_struct(pd,pt);
    /* get extra table info */
    update_tbl_extra_info(pd,pt);
  }

  /* return back the gropu */
  return_group(gid);

  return 0;
}

int dnmgr::get_datanodes_by_conf(void)
{
  uint16_t i=0, n=0 ;
  DATA_NODE *pd = 0;
  size_t cnt = m_conf.get_num_dataNodes();
  tDNInfo /**pd = 0,*/ di ;

  for (n=0;n<get_num_groups();n++) {

    auto m_nodes = get_specific_group(n);

    /* clean up the list first */
    m_nodes->clear();
    /* iterates the data node configs */
    log_print("data node count: %zu\n",cnt);
    for (i=0;i<cnt;i++) {
      pd = m_conf.get_dataNode(i) ;
      if (!pd)
        break ;
      log_print("processing data node %d******\n",i);
      di.no = i;
      di.addr = pd->address;
      di.port = pd->port ;
      strcpy(di.usr,pd->auth.usr.c_str());
      strcpy(di.pwd,pd->auth.pwd.c_str());
      strcpy(di.schema,pd->schema.c_str());
      log_print("data node addr: %x:%d, auth %s(%s), "
        "schema: %s\n", di.addr,di.port,di.usr,di.pwd,
        di.schema);
      /* set data node invalid */
      di.stat = s_invalid ;
      di.add_ep= 0;
      di.mysql= 0;
      /* update or add data node 'no' */
      m_nodes->add(di.no,&di);
    }

    /* add to the free groups container */
    return_group(n);
  }

  /* get total data node count */
  //num_dataNodes = cnt ;

  return 0;
}

int dnmgr::new_connection(tDNInfo *pd)
{
  /* do new connection to db servers */
  char host[32] ;

  pd->mysql = mysql_init(0);
  /* ip address string */
  ul2ipv4(host,pd->addr);
  //log_print("trying connect\n");
  /* try connect to new mysql server */
  if (!mysql_real_connect(pd->mysql,host,pd->usr,
     pd->pwd,pd->schema,pd->port,NULL,0)) {
    log_print("connect fail: %s\n",pd->mysql->last_error);
    return -1;
  }
  /* set auto commit */
  mysql_autocommit(pd->mysql,false);
  return 0;
}

int dnmgr::update_dn_conn(void)
{
  uint16_t n=0;
  tDNInfo *pd = 0;
  safe_container_base<int,tDNInfo*>::ITR_TYPE itr ;

  for (n=0;n<get_num_groups();n++) {

    auto m_nodes = get_specific_group(n);

    /* make all connections inactive */
    m_nodes->update_batch_stats(s_invalid);

    /* update all connections by configs */
    for (pd=m_nodes->next(itr,true);pd;pd=m_nodes->next(itr)) {

      /* TODO: do a validation check for the socket */
      //log_print("mysql: %p,group %d, dn no %d\n",pd->mysql,n,pd->no);

      if (pd->mysql) {
#if 0
        /* XXX: should I keep the old connections alive ? */
        if (pd->mysql->sock>0) 
          continue ;
        /* abort abnormal connections */
        if (pd->mysql->sock<=0) 
#endif
          mysql_close(pd->mysql);
      }
      /* try connect to new mysql server */
      if (new_connection(pd)) {
        /* TODO: failed, should do a retry? */
        pd->stat = s_invalid ;
        log_print("login to '%s' of '%s'@'0x%x:%d' fail\n",
          pd->schema,pd->usr,pd->addr,pd->port);
        continue ;
      }
      pd->stat = s_free ;
      log_print("login to '%s' of '%s'@'0x%x:%d' ok\n",
        pd->schema,pd->usr, pd->addr,pd->port);
    } /* for (pd) */ 

  } /* for (n) */
  return 0;
}

