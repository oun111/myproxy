
#include <string>
#include <vector>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myproxy_config.h"
#include "dbg_log.h"
#include "dbug.h"

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
const char *rRgnMap[] = {
  "rangeMap"
};
const char *rModN[] = {
  "modN"
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

}


myproxy_config::myproxy_config(void)
{
}

myproxy_config::~myproxy_config(void)
{
  reset();
}

void myproxy_config::reset(void)
{
  uint16_t i=0, m=0, n=0;
  TABLE_INFO *pt=0;
  SCHEMA_BLOCK *ps = 0;

  /* clear data node list */
  for (i=0;i<m_dataNodes.size();i++) 
    delete m_dataNodes[i] ;
  /* clear schema list */
  for (i=0;i<m_schemas.size();i++) {
    ps = m_schemas[i] ;
    for (m=0;m<ps->table_list.size();m++) {
      pt = ps->table_list[m] ;
      for (n=0;n<pt->map_list.size();n++) 
        delete pt->map_list[n] ;
#if 0
      for (n=0;n<pt->priKeys.size();n++) 
        delete []pt->priKeys[n];
#endif
      /* delete the table info itself */
      delete pt ;
    }
  }
}

int myproxy_config::parse_global_settings(void)
{
  uint16_t i = 0;
  jsonKV_t *pj = find(m_root,(char*)gsSec[0]), 
    *pi = 0, *tmp = 0 ;

  if (!pj) {
    log_print("global settings entry not found\n");
    return -1;
  }

  for (i=0;i<pj->list.size();i++) {
    pi = pj->list[i] ;

    /* parse cache pool configs */
    if ((tmp=find(pi,(char*)cachePoolSec[0]))) {
      m_globSettings.szCachePool = atoi(tmp->value.c_str());
    }

    /* parse datanode group configs */
    if ((tmp=find(pi,(char*)dnGrpSec[0]))) {
      m_globSettings.numDnGrp = atoi(tmp->value.c_str());
    }

    /* parse thread pool configs */
    if ((tmp=find(pi,(char*)threadPoolSec[0]))) {
      m_globSettings.szThreadPool = atoi(tmp->value.c_str());
    }
  }
  return 0;
}

size_t myproxy_config::get_cache_pool_size(void)
{
  return m_globSettings.szCachePool ;
}

size_t myproxy_config::get_dn_group_count(void)
{
  return m_globSettings.numDnGrp ;
}

size_t myproxy_config::get_thread_pool_size(void)
{
  return m_globSettings.szThreadPool ;
}

uint32_t myproxy_config::parse_ipv4(std::string &str)
{
  int i0,i1,i2,i3 ;

  sscanf(str.c_str(),"%d.%d.%d.%d",&i0,&i1,&i2,&i3);
  return (i0<<24) | (i1<<16) | (i2<<8) | i3 ;
}

int myproxy_config::parse_data_nodes(void)
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
  for (num_dataNodes=0,i=0;i<pj->list.size();
     i++,num_dataNodes++) {
    pi = pj->list[i] ;
    /* get suitable data node */
    if (i>=m_dataNodes.size()) {
      node = new DATA_NODE ;
      m_dataNodes.push_back(node);
    }
    node = m_dataNodes[i];
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

SCHEMA_BLOCK* myproxy_config::get_schema(char *sch)
{
  uint16_t i=0;

  for (;i<num_schemas;i++) {
    if (m_schemas[i]->name==sch)
      return m_schemas[i];
  }
  return 0;
}

SCHEMA_BLOCK* myproxy_config::get_schema(uint16_t idx)
{
  if (idx>=m_schemas.size() || idx>=num_schemas)
    return NULL;
  return m_schemas[idx];
}

size_t myproxy_config::get_num_schemas(void)
{
  return num_schemas ;
}

size_t myproxy_config::get_num_tables(SCHEMA_BLOCK *sch)
{
  return sch->num_tbls;
}

TABLE_INFO* myproxy_config::get_table(SCHEMA_BLOCK *sch,
  uint16_t idx)
{
  if (idx>=sch->num_tbls)
    return NULL;
  return sch->table_list[idx];
}

int myproxy_config::parse_mapping_list(jsonKV_t *jsonTbl, TABLE_INFO *pt)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  MAPPING_INFO *pm = 0;

  if (!(p1=find(jsonTbl,(char*)mappingSec[0]))) {
    log_print("found NO mappings for table %s\n", 
      pt->name.c_str());
    return -1;
  }
  for (m=0,pt->num_maps=0;m<p1->list.size();
     m++,pt->num_maps++) {
    if (m>=pt->map_list.size()) {
      pm = new MAPPING_INFO ;
      pt->map_list.push_back(pm);
    }
    pm = pt->map_list[m];
    /* data node name */
    pm->dataNode = p1->list[m]->key ;
    /* data node io type */
    if ((p2=find(p1->list[m],(char*)ioTypeSec[0]))) 
      pm->io_type = p2->value==tRead[0]?it_read:
        p2->value==tWrite[0]?it_write:it_both;
  }
  return 0;
}

int 
myproxy_config::parse_shardingkey_list(jsonKV_t *jsonTbl, char *strTbl, char *strSchema)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  uint8_t rule = 0;

  if (!(p1=find(jsonTbl,(char*)shardingSec[0]))) {
    log_print("found NO sharding keys for table %s\n", 
      strTbl);
    return -1;
  }
  for (m=0;m<p1->list.size();m++) {
    SHARDING_EXTRA se ;

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
    /* add to sharding key list */
    m_shds.add(strSchema,strTbl,
      (char*)p1->list[m]->key.c_str(),
      rule, &se);
  } /* end for(...) */
  return 0;
}

int 
myproxy_config::parse_globalid_list(jsonKV_t *jsonTbl, char *strTbl, char *strSchema)
{
  jsonKV_t *p1, *p2 ;
  uint16_t m=0;
  uint8_t intv = 0;

  if (!(p1=find(jsonTbl,(char*)globalIdSec[0]))) {
    log_print("found NO global id columns for table %s\n", 
      strTbl);
    return -1;
  }
  for (m=0;m<p1->list.size();m++) {

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

int myproxy_config::parse_schemas(void)
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
  for (i=0,num_schemas=0;i<pj->list.size();
     i++,num_schemas++) {
    if (i>=m_schemas.size()) {
      ps = new SCHEMA_BLOCK ;
      m_schemas.push_back(ps);
    }
    ps = m_schemas[i];
    /* schema name */
    pi = pj->list[i] ;
    ps->name = pi->key;
    /* auth list */
    if (!(tmp=find(pi,(char*)"Auth"))) {
      log_print("fatal: no 'auth' entry found for schema %s\n", 
        ps->name.c_str());
      return -1;
    }
    for (n=0,ps->num_auths=0;n<tmp->list.size();
       n++,ps->num_auths++) {
      if (n>=ps->auth_list.size()) {
        pa = new AUTH_BLOCK ;
        ps->auth_list.push_back(pa);
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
    for (n=0,ps->num_tbls=0;n<tmp->list.size();
       n++,ps->num_tbls++) {
      if (n>=ps->table_list.size()) {
        pt = new TABLE_INFO ;
        ps->table_list.push_back(pt);
      }
      pt = ps->table_list[n] ;
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

int myproxy_config::parse(std::string &s_in)
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

int myproxy_config::read_conf(const char *infile)
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

int myproxy_config::reload(void)
{
  return read_conf(m_confFile.c_str());
}

char* myproxy_config::get_pwd(char *db, char *usr)
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

bool myproxy_config::is_db_exists(char *db)
{
  uint16_t i=0;

  /* find suitable schema */
  for (;i<m_schemas.size();i++) {
    if (m_schemas[i]->name==db) 
      return true; ;
  }
  return false ;
}

void myproxy_config::dump(void)
{
  uint16_t i=0,m=0,n=0;
  DATA_NODE *pd = 0;
  SCHEMA_BLOCK *ps = 0;
  AUTH_BLOCK *pa = 0;
  TABLE_INFO *pt = 0;
  MAPPING_INFO *pm = 0;

  log_print("dumping data nodes*****: \n");
  for (i=0;i<num_dataNodes;i++) {
    pd = m_dataNodes[i] ;
    log_print("name: %s\n", pd->name.c_str());
    log_print("addr: 0x%x:%d\n", pd->address,pd->port);
    log_print("schema: %s\n", pd->schema.c_str());
    log_print("auth: %s:%s\n",pd->auth.usr.c_str(),
      pd->auth.pwd.c_str());
  }
  log_print("dumping schemas*****: \n");
  for (i=0;i<num_schemas;i++) {
    ps = m_schemas[i];
    log_print("name: %s\n",ps->name.c_str());
    log_print("dumping auth list*****: \n");
    for (m=0;m<ps->num_auths;m++) {
      pa = ps->auth_list[m] ;
      log_print("usr: %s, pwd: %s\n",pa->usr.c_str(),
        pa->pwd.c_str());
    }
    log_print("dumping table list*****: \n");
    for (m=0;m<ps->num_tbls;m++) {
      pt = ps->table_list[m];
      log_print("name: %s\n",pt->name.c_str());
      log_print("dumping mapping list*****: \n");
      for (n=0;n<pt->num_maps;n++) {
        pm = pt->map_list[n] ;
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

int myproxy_config::get_dataNode(char *name)
{
  uint16_t i=0;

  for (i=0;i<num_dataNodes;i++) {
    if (m_dataNodes[i]->name==name)
      return i;
  }
  return -1 ;
}

DATA_NODE* myproxy_config::get_dataNode(uint16_t idx)
{
  if (idx>=m_dataNodes.size() || idx>=num_dataNodes)
    return NULL;
  return m_dataNodes[idx] ;
}

size_t myproxy_config::get_num_dataNodes(void)
{
  return num_dataNodes;
}

