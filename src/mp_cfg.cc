
#include <string>
#include <vector>
#include <regex>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp_cfg.h"
#include "dbg_log.h"
#include "dbug.h"
#include "framework.h"

/*
 * keywords in the configure file
 */
namespace {

const char *gsSec[] = {
  "Globals"
};
const char *dnSec[] = {
  "DataNodes"
};
const char *schemaSec[] = {
  "Schemas"
};
const char *tableSec[] = {
  "Tables"
};
const char *mappingSec[] = {
  "Mappings"
};
const char *shardingSec[]= {
  "shardingKey"
};
const char *globalIdSec[] = {
  "globalIdColumn"
} ;
const char *dbAddr[] = {
  "Address"
};
const char *dbSchema[] = {
  "Schema"
};
const char *dbAuth[] = {
  "Auth"
};

const char *ioTypeSec[] = {
  "IoType"
};
const char *tRead[] = {
  "read"
};
const char *tWrite[] = {
  "write"
};

const char *ruleSec[] = {
  "rule"
};
const char *rangeSec[] = {
  "ranges"
};
const char *rRgnMap[] = {
  "rangeMap"
};
const char *rModN[] = {
  "modN"
};
const char *rDefDn[] = {
  "defDn"
};

const char *intervSec[] = {
  "interval"
} ;

const char *threadPoolSec[] = {
  "ThreadPoolSize"
};

const char *cachePoolSec[] = {
  "CachePoolSize"
};

const char *dnGrpSec[] = {
  "DatanodeGroupCount"
};

const char *bndAddr[] = {
  "BindAddr"
};

const char *listenPort[] = {
  "ListenPort"
};

const char *idleSeconds[] = {
  "IdleSeconds"
};

}


mp_cfg::mp_cfg(void)
{
  bzero(&m_globSettings,sizeof(m_globSettings));
}

mp_cfg::~mp_cfg(void)
{
  reset();
}

void mp_cfg::reset(void)
{
  /* clear data node list */
  for (auto pd : m_dataNodes) {
    delete pd ;
  }
  m_dataNodes.clear();

  /* clear schema list */
  for (auto ps : m_schemas) {
    for (auto pt : ps->table_list) {
      for (auto pm : pt->map_list) {
        delete pm ;
      }
      pt->map_list.clear();
#if 0
      for (n=0;n<pt->priKeys.size();n++) 
        delete []pt->priKeys[n];
#endif
      /* delete the table info itself */
      delete pt ;
    }
    ps->table_list.clear();
    delete ps ;
  }

  m_schemas.clear();
}

int mp_cfg::parse_global_settings(void)
{
  jsonKV_t *pj = find(m_root,(char*)gsSec[0]), *tmp = 0 ;

  if (!pj) {
    log_print("global settings entry not found\n");
    return -1;
  }

  for (auto pi : pj->list) {

    /* parse cache pool configs */
    if ((tmp=find(pi,(char*)cachePoolSec[0]))) {
      m_globSettings.szCachePool = atoi(tmp->value.c_str());

      if (m_globSettings.szCachePool<=0 || m_globSettings.szCachePool>50) {
        log_print("cache pool size %zu not valid\n", m_globSettings.szCachePool);
        m_globSettings.szCachePool = 5;
      }
    }

    /* parse datanode group configs */
    if ((tmp=find(pi,(char*)dnGrpSec[0]))) {
      m_globSettings.numDnGrp = atoi(tmp->value.c_str());

      if (m_globSettings.numDnGrp<=0 || m_globSettings.numDnGrp>50) {
        log_print("datanode group size %zu not valid\n", m_globSettings.numDnGrp);
        m_globSettings.numDnGrp = 3;
      }
    }

    /* parse thread pool configs */
    if ((tmp=find(pi,(char*)threadPoolSec[0]))) {
      m_globSettings.szThreadPool = atoi(tmp->value.c_str());

      if (m_globSettings.szThreadPool<=0 || m_globSettings.szThreadPool>64) {
        log_print("thread pool size %zu not valid\n", m_globSettings.szThreadPool);
        m_globSettings.szThreadPool = 10;
      }
    }

    /* parse bind address/port configs */
    if ((tmp=find(pi,(char*)bndAddr[0]))) {
      std::regex r(
        "([1-9]|[1-9][\\d]|1[\\d]{1,2}|2[0-4][\\d]|25[0-5])\\."
        "([\\d]|[1-9][\\d]|1[\\d]{1,2}|2[0-4][\\d]|25[0-5])\\."
        "([\\d]|[1-9][\\d]|1[\\d]{1,2}|2[0-4][\\d]|25[0-5])\\."
        "([\\d]|[1-9][\\d]|1[\\d]{1,2}|2[0-4][\\d]|25[0-5])"
      ); 

      if (!std::regex_match(tmp->value.c_str(),r)) {
        log_print("bind address %s not valid\n", tmp->value.c_str());
        m_globSettings.bndAddr = "127.0.0.1";
      } else {
        m_globSettings.bndAddr = tmp->value;
      }
    }
    if ((tmp=find(pi,(char*)listenPort[0]))) {
      m_globSettings.listenPort = atoi(tmp->value.c_str());

      if (m_globSettings.listenPort<=0 || m_globSettings.listenPort>65535) {
        log_print("listen port %d not valid\n", m_globSettings.listenPort);
        m_globSettings.listenPort = DEFAULT_BUSI_PORT;
      }
    }

    /* parse the idle time configs */
    if ((tmp=find(pi,(char*)idleSeconds[0]))) {
      m_globSettings.idleSeconds = atoi(tmp->value.c_str());

      if (m_globSettings.idleSeconds<=0 || m_globSettings.idleSeconds>3600) {
        log_print("idle seconds %d not valid\n", m_globSettings.idleSeconds);
        m_globSettings.idleSeconds = 30;
      }
    }
  }
  return 0;
}

size_t mp_cfg::get_cache_pool_size(void) const
{
  return m_globSettings.szCachePool ;
}

size_t mp_cfg::get_dn_group_count(void) const
{
  return m_globSettings.numDnGrp ;
}

size_t mp_cfg::get_thread_pool_size(void) const
{
  return m_globSettings.szThreadPool ;
}

char* mp_cfg::get_bind_address(void) const
{
  return (char*)m_globSettings.bndAddr.c_str() ;
}

int mp_cfg::get_listen_port(void) const
{
  return m_globSettings.listenPort ;
}

int mp_cfg::get_idle_seconds(void) const
{
  return m_globSettings.idleSeconds ;
}

uint32_t mp_cfg::parse_ipv4(std::string &str)
{
  int i0,i1,i2,i3 ;

  sscanf(str.c_str(),"%d.%d.%d.%d",&i0,&i1,&i2,&i3);
  return (i0<<24) | (i1<<16) | (i2<<8) | i3 ;
}

int mp_cfg::parse_data_nodes(void)
{
  size_t np = 0;
  uint16_t i = 0;
  DATA_NODE *node = 0;
  jsonKV_t *pj = find(m_root,(char*)dnSec[0]), 
    *pi = 0, *tmp = 0 ;

  if (!pj) {
    log_print("data node entry not found\n");
    return -1;
  }
  /* create datanode list */
  for (i=0;i<pj->list.size();i++) {

    /* search duplicated tables */
    if (check_duplicate(__func__,pj->list,i)) {
      continue ;
    }

    pi = pj->list[i] ;
    /* get suitable data node */
    if (i>=m_dataNodes.size()) {
      node = new DATA_NODE ;
      //m_dataNodes.push_back(node);
      m_dataNodes.emplace_back(node);
    }

    node = m_dataNodes.back();

    node->name = pi->key ;
    /* parse address:port */
    if (!(tmp=find(pi,(char*)dbAddr[0]))) {
      return -1;
    }
    np = tmp->value.find(":");
    if (np==std::string::npos) {
      node->address = parse_ipv4(tmp->value);
      node->port    = 3306 ;
    } else {
      std::string s = std::string(
        tmp->value.substr(0,np));
      node->address = parse_ipv4(s);
      node->port = atoi(tmp->value.substr(np+1,
        tmp->value.size()).c_str());
    }
    /* schema */
    if (!(tmp=find(pi,(char*)dbSchema[0]))) {
      return -1;
    }
    node->schema = tmp->value ;
    /* authentication */
    if (!(tmp=find(pi,(char*)dbAuth[0]))) {
      return -1;
    }
    node->auth.usr = tmp->list[0]->key ;
    node->auth.pwd = tmp->list[0]->value ;

  }
  return 0;
}

SCHEMA_BLOCK* mp_cfg::get_schema(const char *sch)
{
  for (auto ps : m_schemas) {
    if (ps->name==sch)
      return ps;
  }
  return 0;
}

SCHEMA_BLOCK* mp_cfg::get_schema(uint16_t idx)
{
  if (idx>=m_schemas.size() || idx>=m_schemas.size())
    return NULL;
  return m_schemas[idx];
}

size_t mp_cfg::get_num_schemas(void)
{
  return m_schemas.size() ;
}

size_t mp_cfg::get_num_tables(SCHEMA_BLOCK *sch)
{
  return sch->table_list.size();
}

TABLE_INFO* mp_cfg::get_table(SCHEMA_BLOCK *sch,
  uint16_t idx)
{
  if (idx>=sch->table_list.size())
    return NULL;
  return sch->table_list[idx];
}

TABLE_INFO* mp_cfg::get_table(SCHEMA_BLOCK *sch,
  const char *tbl)
{
  for (auto i : sch->table_list) {
    if (i->name==tbl) 
      return i ;
  }
  return NULL;
}

int 
mp_cfg::check_duplicate(
    const char *func, 
    std::vector<struct tJsonParserKeyVal*> &lst, 
    size_t pos)
{
  size_t pn = 0;

  for (; pn<pos && pn < lst.size(); pn++) {
    if (lst[pn]->key == lst[pos]->key)
      break;
  }

  if (pn<pos) {
    log_print("warning(%s): duplicated key item '%s'\n", 
      func,lst[pn]->key.c_str());
    return -1 ;
  }

  return 0;
}

int mp_cfg::parse_mapping_list(jsonKV_t *jsonTbl, TABLE_INFO *pt)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  MAPPING_INFO *pm = 0;

  if (!(p1=find(jsonTbl,(char*)mappingSec[0]))) {
    log_print("found NO mappings for table %s\n", 
      pt->name.c_str());
    return 0;
  }

  for (m=0;m<p1->list.size();m++) {

    /* search duplicated mappings */
    if (check_duplicate(__func__,p1->list,m)) {
      continue ;
    }

    /* save mapping to m_conf */
    if (m>=pt->map_list.size()) {
      pm = new MAPPING_INFO ;
      //pt->map_list.push_back(pm);
      pt->map_list.emplace_back(pm);
    }

    auto pm = pt->map_list.back();
    /* data node name */
    pm->dataNode = p1->list[m]->key ;
    /* data node io type */
    if ((p2=find(p1->list[m],(char*)ioTypeSec[0]))) {
      pm->io_type = p2->value==tRead[0]?it_read:
        p2->value==tWrite[0]?it_write:it_both;
    } else {
      pm->io_type = it_both ;
    }

  }

  return 0;
}

int
mp_cfg::save_range_maps(jsonKV_t *s_map, SHARDING_EXTRA &se)
{
  int rStart = 0, rEnd = 0, dn = 0;

  for (auto pm : s_map->list) {
    dn = atoi(pm->key.c_str());
    sscanf(pm->value.c_str(),"%d,%d",&rStart,&rEnd);

    /* the range beginning should less than ending */
    if (rStart>rEnd) {
      log_print("invalid range for dn %d: %d - %d\n",
        dn,rStart,rEnd);
      return -1;
    }

    se.map[dn] = std::pair<int,int>(rStart,rEnd);

    log_print("dn %d, range %d - %d\n", dn, rStart, rEnd);
  }

  /* check for overlapped ranges */
  for (auto pm= se.map.begin(); pm!=se.map.end(); pm++) {
    auto cmp = pm ;
    rStart = pm->second.first ;
    rEnd = pm->second.second ;

    for (auto pm2=++cmp;pm2!=se.map.end();pm2++) {

      int rstart1 = pm2->second.first ;
      int rend1 = pm2->second.second ;

      /* overlapped ranges */
      if (!(rEnd<rstart1 || rend1<rStart)) {
        log_print("invalid ranges for dn %d: %d - %d\n",
          pm2->first,rstart1,rend1);
        return -1;
      }
    }

  }

  return 0;
}

int 
mp_cfg::parse_shardingkey_list(jsonKV_t *jsonTbl, char *strTbl, char *strSchema)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  uint8_t rule = 0;

  if (!(p1=find(jsonTbl,(char*)shardingSec[0]))) {
    log_print("found NO sharding keys for table %s.%s\n", 
      strSchema,strTbl);
    return -1;
  }

  for (m=0;m<p1->list.size();m++) {

    SHARDING_EXTRA se ;

    /* search duplicated sharding keys */
    if (check_duplicate(__func__,p1->list,m)) {
      continue ;
    }

    /* rules */
    if (!(p2=find(p1->list[m],(char*)ruleSec[0]))) {
      log_print("no rules found for sharding key %s\n",
        p1->list[m]->key.c_str());
      return -1;
    }

    /* TODO: more rule types */
    rule = p2->value==rRgnMap[0]?t_rangeMap:
      p2->value==rModN[0]?t_modN:
      t_dummy;

    /* save the 'range map' rules */
    if (rule==t_rangeMap) {
      jsonKV_t *tmp = find(p1->list[m],(char*)rangeSec[0]);

      if (!tmp || save_range_maps(tmp,se)) {
        log_print("no range maps, use rule=modN by default\n");
        rule = t_modN ;
      }
    }

    /* find default datanode */
    {
      jsonKV_t *tmp = find(p1->list[m],(char*)rDefDn[0]);

      if (!tmp) {
        se.def_dn = 0;
      } else {
        se.def_dn = atoi(tmp->value.c_str());
      }
      log_print("default datanode %d used for %s.%s\n", 
        se.def_dn,strSchema,strTbl);
    }

    /* add to sharding key list */
    m_shds.add(strSchema,strTbl, (char*)p1->list[m]->key.c_str(),
      rule, se);

  } /* end for(...) */

  return 0;
}

int 
mp_cfg::parse_globalid_list(jsonKV_t *jsonTbl, char *strTbl, char *strSchema)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  uint8_t intv = 0;

  if (!(p1=find(jsonTbl,(char*)globalIdSec[0]))) {
    log_print("found NO global id columns for table %s.%s\n", 
      strSchema,strTbl);
    return 0;
  }

  for (m=0;m<p1->list.size();m++) {

    /* search duplicated global id */
    if (check_duplicate(__func__,p1->list,m)) {
      continue ;
    }

    /* interval */
    if (!(p2=find(p1->list[m],(char*)intervSec[0]))) {
      log_print("no interval found for sharding key %s\n",
        p1->list[m]->key.c_str());
      return -1;
    }
    intv = atoi(p2->value.c_str());

    /* add to sharding key list */
    m_gidConf.add(strSchema,strTbl,
      (char*)p1->list[m]->key.c_str(),
      intv);

  } /* end for(...) */

  return 0;
}

int mp_cfg::parse_schemas(void)
{
  //char *pk = 0;
  uint16_t i=0, n=0/*, m=0*/;
  //uint8_t rule = 0;
  SCHEMA_BLOCK *ps = 0;
  AUTH_BLOCK *pa = 0;
  TABLE_INFO *pt = 0;
  //MAPPING_INFO *pm = 0;
  jsonKV_t *pj = find(m_root,(char*)schemaSec[0]), 
    *pi = 0, *tmp = 0/*, *p1 = 0, *p2 = 0*/ ;

  if (!pj) {
    log_print("schema entry not found\n");
    return -1;
  }
  for (i=0;i<pj->list.size();i++) {

    /* search duplicated tables */
    if (check_duplicate(__func__,pj->list,i)) {
      continue ;
    }

    if (i>=m_schemas.size()) {
      ps = new SCHEMA_BLOCK ;
      //m_schemas.push_back(ps);
      m_schemas.emplace_back(ps);
    }

    ps = m_schemas.back();
    /* schema name */
    pi = pj->list[i] ;
    ps->name = pi->key;
    /* auth list */
    if (!(tmp=find(pi,(char*)"Auth"))) {
      log_print("fatal: no 'auth' entry found for schema %s\n", 
        ps->name.c_str());
      return -1;
    }
    for (n=0;n<tmp->list.size();n++) {
      if (n>=ps->auth_list.size()) {
        pa = new AUTH_BLOCK ;
        //ps->auth_list.push_back(pa);
        ps->auth_list.emplace_back(pa);
      }
      pa = ps->auth_list[n] ;
      pa->usr = tmp->list[n]->key ;
      pa->pwd = tmp->list[n]->value ;
    }
    /* 
     * table list 
     */
    if (!(tmp=find(pi,(char*)tableSec[0]))) {
      log_print("found NO tables in schema %s\n", 
        ps->name.c_str());
      continue;
    }
    for (n=0;n<tmp->list.size();n++) {

      /* search duplicated tables */
      if (check_duplicate(__func__,tmp->list,n)) {
        continue ;
      }

      if (n>=ps->table_list.size()) {
        pt = new TABLE_INFO ;
        //ps->table_list.push_back(pt);
        ps->table_list.emplace_back(pt);
      }

      pt = ps->table_list.back();
      /* table name */
      pt->name = tmp->list[n]->key ;
      /* 
       * mapping list 
       */
      if (parse_mapping_list(tmp->list[n],pt))
        continue ;
      /* 
       * sharding key list 
       */
      if (parse_shardingkey_list(tmp->list[n],
         const_cast<char*>(pt->name.c_str()),
         const_cast<char*>(ps->name.c_str()))) 
      {
        continue ;
      }
      /*
       * global id column
       */
      if (parse_globalid_list(tmp->list[n],
         const_cast<char*>(pt->name.c_str()),
         const_cast<char*>(ps->name.c_str())))
      {
        continue ;
      }
      /* TODO: more attributes here */

    }

  }
  return 0;
}

int mp_cfg::parse(std::string &s_in)
{
  /* 
   * parse raw json content 
   */
  if (SimpleJsonParser::parse(s_in)) {
    return -1;
  }
  /*
   * parse global settings
   */
  if (parse_global_settings()) {
    return -1;
  }
  /* 
   * parse data nodes 
   */
  if (parse_data_nodes()) {
    return -1;
  }
  /*
   * parse schema list
   */
  if (parse_schemas()) {
    return -1;
  }
  /* XXX: test */
  //dump();
  return 0;
}

int mp_cfg::read_conf(const char *infile)
{
  int fd = 0, ret = 0;
  char buf[65];
  std::string content = "" ;

  if ((fd=open(infile,O_RDONLY))<=0) {
    log_print("error open %s(%s)\n",infile,strerror(errno));
    return -1;
  }
  m_confFile = infile ;

  while ((ret = read(fd,buf,64))==64) {
    buf[ret] = '\0';
    content += buf ;
  }
  buf[ret] = '\0';
  content += buf ;
  close(fd);

  /* parse the configs */
  if (parse(content)) {
    return -1;
  }

  return 0;
}

int mp_cfg::reload(void)
{
  reset();

  return read_conf(m_confFile.c_str());
}

char* mp_cfg::get_pwd(char *db, char *usr)
{
  uint16_t i=0;
  SCHEMA_BLOCK *ps = 0;
  AUTH_BLOCK *pa = 0;

  /* find suitable schema */
  for (;i<m_schemas.size();i++) {
    if (m_schemas[i]->name==db) 
      break ;
  }
  if (i>=m_schemas.size())
    return NULL ;
  ps = m_schemas[i] ;
  /* find matching password from list */
  for (i=0;i<ps->auth_list.size();i++) {
    pa = ps->auth_list[i];
    if (pa->usr==usr)
      return (char*)pa->pwd.c_str() ;
  }
  return NULL ;
}

bool mp_cfg::is_db_exists(char *db)
{
  /* find suitable schema */
  for (auto ps : m_schemas) {
    if (ps->name==db) 
      return true; ;
  }
  return false ;
}

void mp_cfg::dump(void)
{
  log_print("dumping data nodes*****: \n");

  for (auto pd : m_dataNodes) {
    log_print("name: %s\n", pd->name.c_str());
    log_print("addr: 0x%x:%d\n", pd->address,pd->port);
    log_print("schema: %s\n", pd->schema.c_str());
    log_print("auth: %s:%s\n",pd->auth.usr.c_str(),
      pd->auth.pwd.c_str());
  }
  log_print("dumping schemas*****: \n");
  for (auto ps : m_schemas) {
    log_print("name: %s\n",ps->name.c_str());
    log_print("dumping auth list*****: \n");
    for (auto pa : ps->auth_list) {
      log_print("usr: %s, pwd: %s\n",pa->usr.c_str(),
        pa->pwd.c_str());
    }
    log_print("dumping table list*****: \n");
    for (auto pt : ps->table_list) {
      log_print("name: %s\n",pt->name.c_str());
      log_print("dumping mapping list*****: \n");
      for (auto pm : pt->map_list) {
        log_print("node: %s\n",pm->dataNode.c_str());
        log_print("io: %d\n",pm->io_type);
      }
#if 0
      log_print("dumping pri keys*****: \n");
      for (n=0;n<pt->num_priKeys;n++) 
        log_print("key: %s\n",pt->priKeys[n]);
      log_print("rule: %d\n", pt->rule);
#endif
    }
  }
}

int mp_cfg::get_dataNode(char *name)
{
  for (size_t i=0;i<m_dataNodes.size();i++) {
    if (m_dataNodes[i]->name==name)
      return i;
  }
  return -1 ;
}

DATA_NODE* mp_cfg::get_dataNode(uint16_t idx)
{
  if (idx>=m_dataNodes.size() || idx>=m_dataNodes.size())
    return NULL;
  return m_dataNodes[idx] ;
}

size_t mp_cfg::get_num_dataNodes(void)
{
  return m_dataNodes.size();
}

