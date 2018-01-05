
#ifndef __MYPROXY_CONFIG_H__
#define __MYPROXY_CONFIG_H__

#include "json_parser.h"
#include "container_impl.h"



/* 
 * proxy config structures 
 */
using AUTH_BLOCK = struct auth_block_t {
  std::string usr;
  std::string pwd;
} ;

/* io type of data nodes */
enum ioTypes {
  it_read,
  it_write,
  it_both
} ;
using MAPPING_INFO = struct mapping_info_t {
  std::string dataNode; /* target data node name */
  uint8_t io_type ; /* 0 read, 1 write, 2 both */
} ;

/* distribution rules */
enum tRules {
  t_dummy, /* dummy rule */
  t_modN, /* get data node by MOD(data-node-count) */
  t_rangeMap, /* column value range and data node mapping */
  t_maxRules,
} ;

using TABLE_INFO = struct table_info_t {
  std::string name ; /* target table name */
  std::vector<MAPPING_INFO*> map_list ;
  int num_maps;
#if 0
  std::vector<char*> priKeys ; /* primary key list */
  int num_priKeys;
  uint8_t rule ; /* the distribution rule id */
#endif
} ;
/*
 * schema block definition
 */
using SCHEMA_BLOCK = struct schema_block_t {
  std::string name; /* the schema name */
  std::vector<AUTH_BLOCK*> auth_list ;
  int num_auths;
  std::vector<TABLE_INFO*> table_list ;
  int num_tbls ;
} ;
/* 
 * data node definition 
 */
using DATA_NODE = struct data_node_t {
  std::string name;
  uint32_t address;
  uint16_t port ;
  std::string schema;
  AUTH_BLOCK auth ;
} ;
/*
 * global settings
 */
using GLOBAL_SETTINGS = struct global_settings_t {
  size_t szCachePool ;
  size_t numDnGrp ;
  size_t szThreadPool ;
} ;

class myproxy_config : public SimpleJsonParser 
{
public:
  myproxy_config(void);
  ~myproxy_config(void);

protected:
  std::string m_confFile ;

public:
  /*
   * global settings item
   */
  GLOBAL_SETTINGS m_globSettings ;
  /* 
   * the data node list 
   */
  std::vector<DATA_NODE*> m_dataNodes ;
  int num_dataNodes;
  /* 
   * the schema list 
   */
  std::vector<SCHEMA_BLOCK*> m_schemas;
  int num_schemas;
  /*
   * the sharding column list
   */
  safeShardingColumnList m_shds ;

  /* the global id list */
  safeGlobalIdColumnList m_gidConf ;

public:
  int read_conf(const char*);
  int reload(void);

  void dump(void);

  char* get_pwd(char*,char*);

  bool is_db_exists(char*);

  DATA_NODE* get_dataNode(uint16_t);
  int get_dataNode(char*);

  SCHEMA_BLOCK* get_schema(uint16_t);

  size_t get_num_dataNodes(void);

  size_t get_num_schemas(void);

  SCHEMA_BLOCK* get_schema(const char*);

  TABLE_INFO* get_table(SCHEMA_BLOCK*,
    uint16_t);
    
  TABLE_INFO* get_table(SCHEMA_BLOCK *sch,
    const char *tbl);

  size_t get_num_tables(SCHEMA_BLOCK*);

  size_t get_cache_pool_size(void);

  size_t get_dn_group_count(void);

  size_t get_thread_pool_size(void);

protected:

  int parse(std::string&);

  int parse_global_settings(void);

  int parse_data_nodes(void);

  int parse_schemas(void);

  int parse_mapping_list(jsonKV_t*, TABLE_INFO*);

  int parse_shardingkey_list(jsonKV_t*, char*, char*);

  int parse_globalid_list(jsonKV_t*, char*, char*);

  uint32_t parse_ipv4(std::string&);

  void reset(void);

  int check_duplicate(const char*,std::vector<struct tJsonParserKeyVal*>&,size_t);
} ;

#endif /* __MYPROXY_CONFIG_H__*/


