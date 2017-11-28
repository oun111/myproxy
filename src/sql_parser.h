
#ifndef __SQL_PARSER_H__
#define __SQL_PARSER_H__

#include "container_impl.h"
/* 
 * sql parsing basement support 
 */
#include "connstr.h"
#include "sql_tree.h"


/* statement types */
enum stmt_type {
  s_na,

  /* set xxx statements */
  s_set_xxx,

  /* remotely sql that are to be splited 
   *  and pass over to processor side */
  s_sharding,

  /* the 'show databases' statements */
  s_show_dbs,

  /* the 'show tables' statements */
  s_show_tbls,

  /* 'select version-comment' statement */
  s_sel_ver_comment,

  /* 'select DATABASE()' statement */
  s_sel_cur_db,

  /* 'desc table' statement */
  s_desc_tbl,

  /* 'set autocommit' statement */
  s_set_autocommit,

  s_max,
} ;


namespace global_parser_items {

  extern stxNode* parse_tree(char *pSql, size_t szSql);

  extern void del_tree(stxNode *ptree);

  /* do a simple parse on statment */
  extern int do_simple_explain(char*,size_t,int&);
} ;

class sql_parser {
protected:
  /* sharding column infos */
  safeShardingColumnList &m_shds;
  /* targeting table list */
  unsafeTblKeyList m_tblKeyLst ;
  /* the tree node - place holder number map */
  using pholder_map = std::unordered_map<stxNode*,int> ;
  pholder_map m_maps;
  tree_serializer *m_ts ;

public:
  sql_parser();
  ~sql_parser();

public:
#if 0
  stxNode* parse_tree(char *req, size_t sz);
  void del_tree(stxNode *ptree);
  /* do a simple parse on statment */
  static int do_simple_explain(char*,size_t,int&);
#endif

  int get_stmt_type(stxNode *tree, stxNode* &out, int &type);

  /* deals with the sharding column value 
   *  of binary operators' */
  int deal_binary_sv(int,tSqlParseItem*,stxNode*);
  /* digging informations from tree */
  int do_dig(char*,size_t,tSqlParseItem*,char*,tContainer&,stxNode* =NULL);
  /* collects tables names in the statement */
  int collect_target_tbls(stxNode*,tSqlParseItem*,char*,tContainer&);
  /* collects sharding columns in the statement */
  int collect_sharding_columns(stxNode*,tSqlParseItem*,char*);
  /* collects normal sharding columns  */
  int collect_normal_shd_cols(stxNode*,tSqlParseItem*,char*);
  /* collects normal sharding columns in 'select'/'update'/'delete' statements */
  int collect_sharding_cols(stxNode*,tSqlParseItem*,char*);
  /* collects normal sharding columns in 'insert' statements */
  int collect_sharding_col_4_insert(stxNode*,tSqlParseItem*,char*);
  /* collects normal sharding columns in normal 'insert' statements */
  int collect_shd_cols_4_normal_insert(stxNode*,tSqlParseItem*,char*);
  /* enumerates the sharding column configs */
  int enum_sharding_configs(tSqlParseItem*,stxNode*,char*,char*);
  /* process endpoint-type tree nodes */
  int deal_endpoints(stxNode*,tSqlParseItem*,char*);
  /* process the operator node */
  int deal_operator(tSqlParseItem*,stxNode*,SHARDING_KEY*);
  /* collect place-holder location info */
  int collect_place_holders(stxNode*,int&);
  /* collect aggregation informations */
  bool is_agg_func(char *name, int &t);
  int collect_agg_info(stxNode *node, tSqlParseItem *sp);
} ;

#endif /* __SQL_PARSER_H__*/
