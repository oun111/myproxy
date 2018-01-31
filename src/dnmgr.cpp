
#include "dnmgr.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "container_impl.h"
#include "dbg_log.h"
#include "simple_types.h"
#include "env.h"
#include "sock_toolkit.h"

using namespace GLOBAL_ENV ;

/* 
 * class dnGroups
 */
dnGroups::dnGroups() : m_nDnGroups(0), m_idleCount(0)
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

  /* init free groups */
  release_all_groups();

  return 0;
}

int dnGroups::acquire_all_groups(void)
{
  safeDataNodeList *plst = 0;
  int gid = 0, i=0;

  /* init free groups */
  for (i=0;i<m_nDnGroups;i++) {
    get_free_group(plst,gid);
  }

  if (i<m_nDnGroups) {
    log_print("warning: not all groups can be acquired\n");
    return -1;
  }

  return 0;
}

int dnGroups::release_all_groups(void)
{
  for (int i=0;i<m_nDnGroups;i++) {
    return_group(i);
  }

  return 0;
}

safeDataNodeList* dnGroups::get_specific_group(int gid)
{
  safeDataNodeList *pNodeGroups = 0;

  if (gid>=m_nDnGroups) {
    return NULL;
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

  pNodeGroups = m_dnGroup.get();
  if (!pNodeGroups) {
    return -1 ;
  }

  nodes = &pNodeGroups[gid] ;

  /* reset the idle counter */
  m_idleCount = 0;

  log_print("acquired datanode group %d\n",gid);

  return 0;
}

int dnGroups::return_group(int gid)
{
  if (gid>=0&&gid<m_nDnGroups) {

    m_freeGroupIds.push(gid);

    /* reset the idle counter */
    m_idleCount = 0;

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

  int do_send_stmt_close(int myfd, int stmtid)
  {
    char outb[32];
    size_t sz = 0;

    sz = mysqls_gen_stmt_close(outb,stmtid);

    do_send(myfd,outb,sz);

    return 0;
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

  /* init the updater task */
  m_thread.bind(&dnmgr::datanode_idle_task,this,0);
  m_thread.start();
}

dnmgr::~dnmgr() 
{
}

void dnmgr::datanode_idle_task(int arg)
{
  int idle_s = m_conf.get_idle_seconds();

  log_print("idle seconds: %d\n",idle_s);

  while (1) {

    /* the datanode groups are idle */
    if (m_nDnGroups==m_freeGroupIds.size()) {
      m_idleCount ++ ;
    } else {
      /* reset 'idle seconds' if any group(s) 
       *  are found not idle*/
      m_idleCount = 0;
    }

    /* all datanode groups are idle over 'idle_s' seconds */
    if (m_idleCount>idle_s) {
      m_idleCount = 0;

      log_print("refreshing...\n");

      /* keep connections with backend mysql servers */
      keep_dn_conn();

      /* dont update table structures */
      refresh_tbl_info(true);

      /* send stmt_close command by myfd -> stmtid map */
      batch_stmt_close();
    }

    sleep(1);

  } // end while()

}

int dnmgr::initialize(void)
{
  if (get_datanodes_by_conf()) {
    /* TODO: error messages here */
    return -1;
  }

  /* update connections */
  if (update_dn_conn()) {
    /* TODO: error messages here */
    return -1;
  }    

  if (get_tables_by_conf()) {
    /* TODO: error messages here */
    return -1;
  }

  /* refresh table structs */
  if (refresh_tbl_info()) {
    return -1;
  }

  return 0;
}

int dnmgr::add_dn_tbl_relations(auto sch, auto tbl)
{
  int gid = 0;
  safeDataNodeList *m_nodes = 0 ;

  if (get_free_group(m_nodes,gid)) {
    log_print("fatal: found no free datanode groups\n");
    return -1;
  }

  /* no mapping config, map to all datanodes by default */
  if (!tbl->map_list.size()) {

    int iot = it_both ;

    safe_container_base<int,tDNInfo*>::ITR_TYPE itr ;

    for (tDNInfo *pd=m_nodes->next(itr,true);pd;pd=m_nodes->next(itr)) {

      /* check validation of target datanode */
      if (!m_nodes->is_alive(pd)) {
        log_print("warning: data node %d is in-active!\n",pd->no);
      }

      m_tables.add_map((char*)sch->name.c_str(),
        (char*)tbl->name.c_str(),pd->no,iot);

      log_print("added DEFAULT data node %d(%s) io type %d to "
        "`%s.%s` in table list\n", pd->no,"",
        iot,sch->name.c_str(),tbl->name.c_str());
    }

  }

  /* add data nodes by mappings configs */
  else {

    for (auto mi : tbl->map_list) {

      int idn= m_conf.get_dataNode((char*)mi->dataNode.c_str());

      if (idn<0) {
        log_print("data node index of %s not found!!\n",
          mi->dataNode.c_str());
        continue ;
      }

      /* check validation of target datanode */
      if (!m_nodes->is_alive(idn)) {
        log_print("warning: data node %d is in-active!\n",idn);
      }

      m_tables.add_map((char*)sch->name.c_str(),
        (char*)tbl->name.c_str(),idn,mi->io_type);

      log_print("added data node %d(%s) io type %d to "
        "`%s.%s` in table list\n", idn,mi->dataNode.c_str(),
        mi->io_type,sch->name.c_str(),tbl->name.c_str());
    }

  } // end else{}

  return_group(gid);

  return 0;
}

int dnmgr::get_tables_by_conf(void)
{
  /* reset table items */
  m_tables.clear();

  /* iterate schema list */
  for (size_t i=0;i<m_conf.get_num_schemas();i++) {
    SCHEMA_BLOCK *sch = m_conf.get_schema(i);

    /* iterate table list of schema */
    for (size_t n=0;n<m_conf.get_num_tables(sch);n++) {
      TABLE_INFO *tbl = m_conf.get_table(sch,n);

      /* add to table list */
      m_tables.add((char*)sch->name.c_str(),
        (char*)tbl->name.c_str());
      log_print("db table `%s.%s` is added to list\n",
         sch->name.c_str(),tbl->name.c_str());

      /* add 'datanode - table' relationships to table list */
      add_dn_tbl_relations(sch,tbl);
    }
  } /* end for(i=0) */

  log_print("table count %zu\n",m_tables.size());

  return 0;
}

int dnmgr::update_tbl_extra_info(tDNInfo *pd, tTblDetails *pt, bool check)
{
  uint8_t kt = 0;
  bool nullable = false;
  char buf[128], *dtype=0, *cname=0, *def=0, *extra=0;
  char *pnull = 0, *pkt=0;
  uint16_t n=0;
  MYSQL_RES *mr = 0;
  char **results = 0;

  /* only query table structure, no need result set */
  sprintf(buf,"desc %s.%s",pd->schema,pt->table.c_str());
  /* execute the sql */
  if (mysql_query(pd->mysql,buf)) {
    log_print("execute sql %s failed: %s\n",
      buf,mysql_error(pd->mysql));
    return -1;
  }

  /* check result of SQL exection above only */
  if (check) {
    return 0;
  }

  log_print("********extra info of table %s.%s*********\n",
    pt->schema.c_str(),pt->table.c_str());

  /* get query result */
  mr = pd->mysql->res ;
  /* initiates the iteration */
  mysql_store_result(pd->mysql);

  for (size_t nRow=0;(results=mysql_fetch_row(mr)) && nRow<mysql_num_rows(mr);nRow++) {

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
          pnull = results[n];
          break ;
        /* key type */
        case 3:
          kt = !strncasecmp(results[n],"pri",3)?0:
            !strncasecmp(results[n],"mul",3)?1:
            !strncasecmp(results[n],"uni",3)?2:0xff;
          pkt = results[n];
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
    }

    log_print("column %zu: name: %s, type: %s, null-able: %s, key: %s, default: %s, extra: %s\n",
      nRow,cname,dtype,pnull,pkt,def,extra);

    /* add extra infos to table list */
    m_tables.add_col_extra(const_cast<char*>(pt->schema.c_str()),
      const_cast<char*>(pt->table.c_str()),cname,dtype,
      nullable,kt,def,extra);
  }

  log_print("%zu columns in %s\n", pt->columns.size(), pt->table.c_str());

  return 0;
}

int dnmgr::update_tbl_struct(tDNInfo *pd, tTblDetails *pt, bool check)
{
  uint16_t nCol=0;
  char buf[128];
  MYSQL_FIELD *mf ;

  /* only query table structure, no need result set */
  sprintf(buf,"select *from %s.%s where 1<>1",
    pd->schema,pt->table.c_str());
  /* execute the sql */
  if (mysql_query(pd->mysql,buf)) {
    log_print("execute sql %s failed: %s\n",
      buf,mysql_error(pd->mysql));
    return -1;
  }

  /* check result of SQL exection above only */
  if (check) {
    return 0;
  }

  log_print("********struct of table %s.%s*********\n",
    pt->schema.c_str(),pt->table.c_str());

  for (nCol=0;nCol<pd->mysql->columns.number;nCol++) {

    /* extract column details */
    mf = ((MYSQL_FIELD*)pd->mysql->columns.c)+nCol ;
    log_print("column %d: %s, charset: %d, type: %d, "
      "max len: %d, flags: 0x%x\n",nCol,mf->name,
      mf->charset,mf->type,mf->len,mf->flags);

    /* add column to table list */
    m_tables.add_col(const_cast<char*>(pt->schema.c_str()),mf->tbl,
      mf->name,mf->charset,mf->len,mf->type,mf->flags);
  }

  log_print("%zu columns in %s\n", 
    pt->columns.size(), pt->table.c_str());

  return 0;
}

tDNInfo* dnmgr::get_valid_datanode(safeDataNodeList *nodes, tTblDetails *pt)
{
  safeTableDetailList::dnMap_itr i;

  log_print("trying table `%s - %s`\n",pt->schema.c_str(),pt->table.c_str());
  for (tDnMappings *pm=m_tables.next_map(pt,i,true);pm;pm=m_tables.next_map(pt,i)) {

    tDNInfo *pd = nodes->get(pm->dn);

    if (!pd) {
      log_print("data node %d is NOT found!!\n",pm->dn);
      continue ;
    }

    if (!nodes->is_alive(pd)) {
      struct in_addr ia ;

      ia.s_addr = htonl(pd->addr) ;
      log_print("data node %d(@%s:%d) is not active!!\n",
        pd->no,inet_ntoa(ia),pd->port);
      continue ;
    }

    return pd ;

  } // end for()

  return NULL ;
}

int dnmgr::refresh_tbl_info(bool check)
{
  safe_container_base<uint64_t,tTblDetails*>::ITR_TYPE itr ;
  safeDataNodeList *nodes = 0;
  int gid = 0;

  /* get a free datanode group */
  if (get_free_group(nodes,gid)) {
    log_print("fatal: found no free datanode groups\n");
    return -1;
  }

  /* query structure of all tables in list */
  for (tTblDetails *pt=m_tables.next(itr,true);pt;pt=m_tables.next(itr)) {

    //tDNInfo *pd = get_valid_datanode(nodes,pt);
    safeTableDetailList::dnMap_itr i;
    tDnMappings *pm = 0;

    log_print("trying table `%s - %s`\n",pt->schema.c_str(),pt->table.c_str());

    /* iterate every single mapping nodes for table 'pt' */
    for (pm=m_tables.next_map(pt,i,true);pm;pm=m_tables.next_map(pt,i)) {
      tDNInfo *pd = nodes->get(pm->dn);

      /* check & update table state */
      if (!pd) {
        m_tables.set_invalid(pt);

        log_print("no valid datanode found for `%s.%s`\n",
          pt->schema.c_str(),pt->table.c_str());
        continue ;
      }

      if (!nodes->is_alive(pd)) {
        struct in_addr ia ;

        ia.s_addr = htonl(pd->addr) ;
        log_print("data node %d(@%s:%d) is not active!!\n",
          pd->no,inet_ntoa(ia),pd->port);
        continue ;
      }

#if 0
      log_print("try query structure of table `%s.%s` on dn %d ...\n",
        pt->schema.c_str(),pt->table.c_str(),pd->no);
#endif

      /* set block mode to fetch the query results */
      set_block(pd->mysql->sock);

      /* get table structure/extra info by the given datanode, 
       *  try other datanodes if failed */
      if (update_tbl_struct(pd,pt,check) || 
         update_tbl_extra_info(pd,pt,check)) {
        continue ;
      }

      m_tables.set_valid(pt);
      /* the table is ok */
      log_print("OK!\n");
      break ;

    } // end for()

    if (!pm) {
      m_tables.set_invalid(pt);
      log_print("FAIL!\n");
    }

  } // end for()

  /* return back the gropu */
  return_group(gid);

  return 0;
}

int dnmgr::get_datanodes_by_conf(void)
{
  uint16_t i=0, n=0 ;
  DATA_NODE *pd = 0;
  size_t cnt = m_conf.get_num_dataNodes();

  for (n=0;n<get_num_groups();n++) {

    auto m_nodes = get_specific_group(n);

    /* clean up the list first */
    m_nodes->clear();

    /* iterates the data node configs */
    for (i=0;i<cnt;i++) {
      pd = m_conf.get_dataNode(i) ;
      if (!pd)
        break ;
      log_print("processing data node %d******\n",i);
      log_print("addr: %x:%d, auth %s(%s), schema: %s\n", 
        pd->address,pd->port,pd->auth.usr.c_str(),
        pd->auth.pwd.c_str(),pd->schema.c_str());
      /* update or add data node 'no' */
      m_nodes->add(i,pd->address,pd->port,(char*)pd->schema.c_str(),
        (char*)pd->auth.usr.c_str(),(char*)pd->auth.pwd.c_str());
    }

    /* add to the free groups container */
    //return_group(n);
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
  log_print("trying connect\n");
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
    m_nodes->reset_stats();

    /* update all connections by configs */
    for (pd=m_nodes->next(itr,true);pd;pd=m_nodes->next(itr)) {

      /* TODO: do a validation check for the socket */

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
        m_nodes->reset_single_stat(pd);
        log_print("login to '%s' of '%s'@'0x%x:%d' fail\n",
          pd->schema,pd->usr,pd->addr,pd->port);
        continue ;
      }

      m_nodes->set_alive(pd);

      log_print("login to '%s' of '%s'@'0x%x:%d' dn %d fd %d ok\n",
        pd->schema,pd->usr, pd->addr,pd->port,pd->no,pd->mysql->sock);
    } /* for (pd) */ 

  } /* for (n) */
  return 0;
}

int dnmgr::keep_dn_conn(void)
{
  uint16_t n=0;
  tDNInfo *pd = 0;
  safe_container_base<int,tDNInfo*>::ITR_TYPE itr ;

  /* XXX: occupy all groups to forbid the collision that 
   *  others may acquire the same group  */
  acquire_all_groups();

  for (n=0;n<get_num_groups();n++) {

    auto m_nodes = get_specific_group(n);

    /* update all connections by configs */
    for (pd=m_nodes->next(itr,true);pd;pd=m_nodes->next(itr)) {

      if (pd->mysql) {

        /* set block mode to fetch the 'ping' replies */
        set_block(pd->mysql->sock);

        /* test connection by ping */
        if (!test_conn(pd->mysql)) {
          continue ;
        }

        //log_print("datanode %d is in-active!!\n",pd->no);

        mysql_close(pd->mysql);
      }

      /* try connect to new mysql server */
      if (new_connection(pd)) {
        log_print("login to '%s' of '%s'@'0x%x:%d' fail\n",
          pd->schema,pd->usr,pd->addr,pd->port);
        m_nodes->reset_single_stat(pd);
        continue ;
      }

      m_nodes->set_alive(pd);

      log_print("login to '%s' of '%s'@'0x%x:%d' ok\n",
        pd->schema,pd->usr, pd->addr,pd->port);
    } /* for (pd) */ 

  } /* for (n) */

  release_all_groups();

  return 0;
}

int dnmgr::batch_stmt_close(void)
{
  /* occupy all groups */
  acquire_all_groups();

  /* force to free all prepared statements 
   *  at backend servers */
  m_mfMaps.do_iterate(
     do_send_stmt_close
    );

  m_mfMaps.clear();

  release_all_groups();

  return 0;
}

