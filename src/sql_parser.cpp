
#include "sql_parser.h"
#include "hook.h"
#include "hooks/agg_hooks.h"
#include "dbg_log.h"
#include "env.h"

using namespace STREE_TYPES;
using namespace GLOBAL_ENV;
using namespace global_parser_items;
using namespace AGG_HOOK_ENV;

enum op_type {
  opt_unknown,
  opt_eval, /* operator '=' 'in(...)' */
  opt_lgc_bt, /* logical operator '>' */
  opt_lgc_be, /* logical operator '>=' */
  opt_lgc_lt, /* logical operator '<' */
  opt_lgc_le, /* logical operator '<=' */
  opt_lgc_ne, /* logical operator '!=' '<>' */
} ;

namespace global_parser_items {

  auto parse_opt = [](int t) {
    return (t==mktype(m_expr,s_eql) || t==mktype(m_expr,s_eql1)) ? opt_eval: 
      (t==mktype(m_expr,s_ne)  || t==mktype(m_expr,s_ne1)) ? opt_lgc_le:
      t==mktype(m_expr,s_bt) ? opt_lgc_bt: 
      t==mktype(m_expr,s_be) ? opt_lgc_be: 
      t==mktype(m_expr,s_lt) ? opt_lgc_lt: 
      t==mktype(m_expr,s_le) ? opt_lgc_le: 
      opt_unknown ;
  };

  auto is_binary_opt = [](int t)  {
    return t==opt_eval || 
      t==opt_lgc_bt || 
      t==opt_lgc_be || 
      t==opt_lgc_lt ||
      t==opt_lgc_le ||
      t==opt_lgc_ne ;
  };

  stxNode* parse_tree(char *pSql, size_t szSql)
  {
    stxNode *pTree = 0;
    tree_serializer ts ;
    std::string stmt(pSql,szSql);

    if (!ts.eliminate_comments(stmt)) {
      log_print("error eliminating comments\n");
      return NULL;
    }
    if (!(pTree=ts.build_tree(stmt))) {
      log_print("error building syntax tree\n");
      return NULL;
    }

    /*
     * TODO: do sql adaptions here
     */
    return pTree;
  }

  void del_tree(stxNode *ptree)
  {
    sql_tree st ;

    st.destroy_tree(ptree);
  }

  int do_simple_explain(char *stmt, 
    size_t len, int &cmd)
  {
    cmd = s_na ;

    /*
     * statements that are to be processed by front-end
     */
    if (!strncasecmp(stmt,"select",6)) {
      if (strcasestr(stmt+6,"@@version_comment")) 
        cmd = s_sel_ver_comment;
      else if (strcasestr(stmt+6,"DATABASE()")) 
        cmd = s_sel_cur_db;
    }

    if (!strncasecmp(stmt,"show",4)) {
      if (strcasestr(stmt+4,"databases")) 
        cmd = s_show_dbs ;
      else if (strcasestr(stmt+4,"tables")) 
        cmd = s_show_tbls;
      else if (strcasestr(stmt+4,"processlist")) 
        cmd = s_show_proclst;
    }

    if (cmd!=s_na) {
      return 0;
    }

    /* 
     * commands that need to be executed at backends 
     */
    if (!strncasecmp(stmt,"commit",6) || 
       !strncasecmp(stmt,"rollback",8) ||
       (!strncasecmp(stmt,"set",3) && strcasestr(stmt+3,"autocommit")) ||
       !strncasecmp(stmt,"select",6) ||
       !strncasecmp(stmt,"update",6) ||
       !strncasecmp(stmt,"insert",6) ||
       !strncasecmp(stmt,"delete",6) ||
       !strncasecmp(stmt,"create",6) || 
       !strncasecmp(stmt,"drop",4) ||
       !strncasecmp(stmt,"desc",4)) {
      cmd = s_sharding ;
      return 0;
    }

    /* invalid commands */
    return -1;
  }

} ;

sql_parser::sql_parser(): 
  m_shds(m_conf.m_shds)
{
}

sql_parser::~sql_parser() 
{
}

int sql_parser::enum_sharding_configs(
  tSqlParseItem *sp,  /* tree parsing result */
  stxNode *node, /* tree node where the requesting column locates */
  char *strCol, /* column name to try */
  char *db /* default schema name */
  )
{
  TABLE_NAME *pt = 0;
  SHARDING_KEY *ps = 0;

  for (pt=sp->m_tblKeyLst.next(true);pt;(pt=sp->m_tblKeyLst.next())) {
    /* try to match the sharding list */
    ps = m_shds.get((char*)pt->sch.c_str(),
      (char*)pt->tbl.c_str(),strCol);
    if (!ps) {
      continue ;
    }
    /* the schema NOT matches */
    if (ps->sch!=db) {
      continue ;
    }
    /* process the operator node */
    deal_operator(sp,node/*->parent*/,ps);
    /* TODO: save the matched one */
    return 0;
  }
  return 1;
}

int sql_parser::deal_binary_sv(int idx, tSqlParseItem *sp, stxNode *nd1)
{
  /* the operand is constant digit */
  if (nd1->type==mktype(m_endp,s_c_int) || nd1->type==mktype(m_endp,s_c_float)) {
    SHARDING_VALUE *psv = sp->get_sv(idx);
    char svbuf[32] = "";

    if (!psv) {
      log_print("no sharding value found for index %d\n",idx);
      return -1;
    }
    /* float/double type */
    if (psv->type==MYSQL_TYPE_FLOAT || psv->type==MYSQL_TYPE_DOUBLE ||
       psv->type==MYSQL_TYPE_DECIMAL || psv->type==MYSQL_TYPE_NEWDECIMAL) {
      *(float*)svbuf = atof(nd1->name);
      //printf("vf: %f\n",*(float*)(nd1->name));
    /* short integer */
    } else if (psv->type==MYSQL_TYPE_TINY || psv->type==MYSQL_TYPE_SHORT) {
      *(int*)svbuf = atoi(nd1->name);
      //printf("vd: %d\n",*(int*)(nd1->name));
    /* long integer */
    } else {
      *(long*)svbuf = atol(nd1->name);
      //printf("vl: %ld\n",*(long*)(nd1->name));
    }
    sp->update_sv(idx,svbuf,-1);

  } else if (!strcmp(nd1->name,"?")) {
    /* the operand is place holder */
    pholder_map::iterator itr ;

    /* get place holder's number */
    itr = m_maps.find(nd1);
    if (itr==m_maps.end()) {
      log_print("no place holder number found by PH node %p\n",nd1);
      return -1;
    }
    sp->update_sv(idx,NULL,itr->second);

  } else {
    /* the operand is not the types above, removing it */
    sp->drop_sv(idx);
    log_print("fatal: no matching opt node(type %s - %s), droping it\n",
      main_type_str(nd1->type), sub_type_str(nd1->type));
    return -1;
  }
  return 0;
}

int sql_parser::deal_operator(tSqlParseItem *sp, 
  stxNode *nd0, SHARDING_KEY *psk)
{
  uint8_t ct = 0;
  tColDetails *pc = 0;
  int i = -1;
  stxNode *pParent = 0,  /* the parent node */
    *nd1 = 0; /* another operand node */

  if (!nd0) {
    return -1;
  }
  /* get column details */
  pc = m_tables.get_col((char*)psk->sch.c_str(),(char*)psk->tbl.c_str(),
    (char*)psk->col.c_str());
  if (!pc) {
    log_print("no such `%s` found in `%s.%s`\n",psk->col.c_str(),
      psk->sch.c_str(),psk->tbl.c_str());
    return -1;
  }
  /* get parent node */
  pParent = nd0->parent ;
  if (pParent->op_lst.size()<2) {
    log_print("fatal: operand count is %zu\n",pParent->op_lst.size());
    return -1;
  }
  /* get another operand node */
  i = m_ts.get_parent_pos(nd0)==1?0:1;
  nd1 = pParent->op_lst[i] ;
  /* save sharding column type */
  ct = parse_opt(pParent->type) ;
  /* save sharding value */
  sp->add_sv(i,psk,pc->col.type,ct);
  /*log_print("sharding col: %s.%s.%s, rule: %d, "
    "op: %s '%s', column type %d, index %d\n", 
    psk->sch.c_str(),psk->tbl.c_str(),psk->col.c_str(),
    psk->rule,main_type_str(pParent->type),
    sub_type_str(pParent->type),pc->col.type,i);*/
  /* 
   * process the values of binary sharding column 
   */
  if (is_binary_opt(ct) && deal_binary_sv(i,sp,nd1)) {
    return -1;
  }
  /* 
   * TODO: process the values of unary sharding column 
   */
  return 0;
}

int sql_parser::deal_endpoints(
  stxNode *node, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
  )
{
  SHARDING_KEY *ps = 0;
  TABLE_NAME *pt = 0;

  //log_print("endpoint %p\n",node);
  /* 'schema+table+column' type */
  if (node->type==mktype(m_endp,s_schema)) {
    /* try match the sharding-column table directly */
    ps = m_shds.get(node->name,node->op_lst[0]->name,
      node->op_lst[0]->op_lst[0]->name);
    /* NOT the sharding column */
    if (!ps) {
      return 0;
    }
    /* process the operator node */
    if (deal_operator(sp,node,ps)) {
      return -1;
    }
    /* TODO: save the sharding column values */
    return 0;
  }
  /* 'table/alias+column' type */
  if (node->type==mktype(m_endp,s_tbl)) {
    /* 
     * 1# treat it as an alias 
     */
    pt = sp->m_tblKeyLst.get(node->name);
    if (pt) {
      ps = m_shds.get((char*)pt->sch.c_str(),(char*)pt->tbl.c_str(),
        node->op_lst[0]->name);
      /* just the sharding column */
      if (ps) {
        /* process the operator node */
        if (deal_operator(sp,node,ps))
          return -1;
        /* TODO: save the sharding column values */
        return 0;
      }
    }
    /* 
     * 2# treat it as a table name 
     */
    ps = m_shds.get(db,node->name,node->op_lst[0]->name);
    if (ps) {
      /* process the operator node */
      deal_operator(sp,node/*->parent*/,ps);
      /* TODO: save the sharding column values */
      return 0;
    }
    /* 
     * 3# try only the 'col' node 
     */
    return enum_sharding_configs(sp,node,node->op_lst[0]->name,db);
  }
  /* 'column only' type */
  if (node->type==mktype(m_endp,s_col)) {
    return enum_sharding_configs(sp,node,node->name,db);
  }
  return 1;
}

int sql_parser::collect_normal_shd_cols(
  stxNode *node, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
)
{
  uint16_t i = 0;
  int ret = 0;

  /* deal the sub-query */
  if (mget(node->type)==m_stmt) {
    return collect_sharding_cols(node,sp,db);
  }
  /* try to process endpoint types */
  ret = deal_endpoints(node,sp,db);
  if (ret!=1) {
    return ret;
  }
  for (i=0;i<node->op_lst.size();i++) {
    ret = collect_normal_shd_cols(node->op_lst[i],sp,db);
    if (ret<0) return -1;
  }
  return 1;
}

/* collect sharding column infos from normal 'insert' statements,
 *  in 'format list' + 'value list' form */
int sql_parser::collect_shd_cols_4_normal_insert(
  stxNode *root, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
  )
{
  auto eliminate_norm_lst = [&](auto &nd) {
    for (;nd && nd->op_lst.size()==1 &&  
      nd->op_lst[0]->type==mktype(m_list,s_norm);  
      nd=nd->op_lst[0]) ; 
  } ;

  stxNode *pVal = 0, *pTbl = 0, *pFmt = 0;
  char *tbl = 0;
  uint16_t i=0;
  SHARDING_KEY *ps = 0;
  int idx= 0 ;
  tColDetails *pc = 0;

  /* get target table sub tree */
  pTbl = root->op_lst[0] ;
  /* 'table' only form */
  if (pTbl->type==mktype(m_endp,s_col)) {
    tbl = pTbl->name;
  } else {
    /* the 'schema' + 'table' form */
    db = pTbl->name;
    tbl= pTbl->op_lst[0]->name;
  }
  //log_print("schema %s, table %s\n",db,tbl);
  /* get the format list */
  pFmt = m_ts.find_in_tree(root,mktype(m_list,s_fmt));
  /* eliminate '()' */
  eliminate_norm_lst(pFmt) ;
  /*
   * it has 'format' list
   */
  if (pFmt) {
    /* test for sharding columns */
    for (i=0;i<pFmt->op_lst.size();i++) {
      ps = m_shds.get(db,tbl,pFmt->op_lst[i]->name);
      if (ps)
        break ;
    }
    /* no sharding column found in statement */
    if (i>=pFmt->op_lst.size()) {
      log_print("found no sharding columns\n");
      return 0;
    }
    /* get column details */
    pc = m_tables.get_col(db,tbl,(char*)ps->col.c_str());
    if (!pc) {
      log_print("no such `%s` found in `%s.%s`\n",db,
        tbl,ps->tbl.c_str());
      return 0;
    }
  } else {
    /* 
     * no format list 
     */
    /* iterates all table columns and test 
     *  for sharding columns */
    for (pc=m_tables.get_col(db,tbl,(tColDetails*)NULL),i=0;pc;
       pc=m_tables.get_col(db,tbl,pc),i++) 
    {
      ps = m_shds.get(db,tbl,(char*)pc->col.name);
      if (ps)
        break ;
    }
    /* found no sharding columns */
    if (!ps) {
      log_print("found no sharding columns1\n");
      return 0;
    }
  }
  /* 
   * save sharding value 
   */
  sp->add_sv(idx,ps,pc->col.type,opt_eval);
  /* the value list should be existing */
  pVal = m_ts.find_in_tree(root,mktype(m_list,s_val));
  /* eliminate '()' */
  eliminate_norm_lst(pVal) ;
  //print_tree(pVal,0);
  if (deal_binary_sv(idx,sp,pVal->op_lst[i])) {
    return -1;
  }
  return 0;
}

/* collect sharding column infos from 'insert' statements */
int sql_parser::collect_sharding_col_4_insert(
  stxNode *root, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
  )
{
  /* if there's 'select' sub statement, treat it as normal statement */
  if (m_ts.find_in_tree(root,mktype(m_stmt,s_select))) {
    return collect_sharding_cols(root,sp,db);
  }
  /* deal with 'insert statements' in 'format + values' form */
  return collect_shd_cols_4_normal_insert(root,sp,db);
}

/* collect sharding column infos from 'select'/'update'/'delete' statements */
int sql_parser::collect_sharding_cols(
  stxNode *node, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
  )
{
  uint16_t i=0;

  /* deal sharding columns from the 'where' list */
  if (node->type==mktype(m_list,s_where)) {
    return collect_normal_shd_cols(node,sp,db);
  }
  for (i=0;i<node->op_lst.size();i++) {
    collect_sharding_cols(node->op_lst[i],sp,db);
  }
  return 0;
}

int sql_parser::collect_sharding_columns(
  stxNode *root, /* tree root */ 
  tSqlParseItem *sp, /* the tree parsing results */
  char *db /* the default database name */
  )
{
  stxNode *nd = root ;
  int type = 0;
  
  /*
   * test for statement type
   */
  if (get_stmt_type(root,nd,type)) {
    return -1;
  }
  sp->stmt_type = type ;
  /* statements: the 'select',  'delete', 'update'  */
  if (sget(type)==s_select || sget(type)==s_delete ||
     sget(type)==s_update) {
    //log_print("trying select type\n");
    return collect_sharding_cols(nd,sp,db);
  }
  /* the 'insert' statements */
  else if (type==mktype(m_stmt,s_insert)) {
    return collect_sharding_col_4_insert(nd,sp,db);
  } 
  /* rollback/commit */
  else if (sget(type)==s_commit || sget(type)==s_rollback) {
    return 0;
  }
  /* create/drop */
  else if (sget(type)==s_cTbl || sget(type)==s_cTbl_cond ||
     sget(type)==s_dTbl || sget(type)==s_dTbl_cond) {
    return 0;
  }
  /* show xxx */
  else if (sget(type)==s_show) {
    return 0;
  }
  /* desc xxx */
  else if (sget(type)==s_desc) {
    return 0;
  }
  /* set xxx */
  else if (sget(type)==s_setparam) {
    return 0;
  }

  /* 
   * TODO: more statement types 
   */

  /* errors */
  log_print("unknown statement type %d\n",sget(type));
  sp->stmt_type = -1;
  return -1;
}

int sql_parser::collect_target_tbls(
  stxNode *node, 
  tSqlParseItem *sp, 
  char *def_db, /* default schema */
  tContainer &err
)
{
  uint16_t i=0;
  stxNode *nd = 0;
  char *tbl_a = 0, *tbl = 0, *pdb = 0 ;
  tTblDetails *td =0 ;
  int ret = 0;

  /* deal table names from the 'from' list or 'update' list*/
  if (node->type==mktype(m_list,s_from) || node->type==mktype(m_list,s_upd)) {
    for (i=0;i<node->op_lst.size();i++) {
      nd = node->op_lst[i];
      /* check for table alias name */
      tbl_a = 0;
      if ((size_t)(i+1)<node->op_lst.size() && 
         node->op_lst[i+1]->type==mktype(m_endp,s_alias)) {
        tbl_a = node->op_lst[i+1]->name ;
      }
      /* 'schema.table' endpoint type */
      if (nd->type==mktype(m_endp,s_tbl)) {
        pdb = nd->name ;
        tbl = nd->op_lst[0]->name;
      } 
      /* 'table' endpoint type */
      else if (nd->type==mktype(m_endp,s_col)) {
        pdb = def_db;
        tbl = nd->name ;
      }
      /* check for existence of table */
      td = m_tables.get(m_tables.gen_key(pdb,tbl));
      if (!td) {
        size_t sz = 0;
        
        err.tc_resize(1024);
        sz = do_err_response(0,err.tc_data(),ER_NO_SUCH_TABLE,
          ER_NO_SUCH_TABLE,pdb,tbl);
        err.tc_update(sz);
        log_print("error: %s.%s dose NOT exists\n",pdb,tbl);
        return -1;
      }
      //log_print("table: %s.%s, alias %s\n",pdb,tbl,tbl_a);
      sp->m_tblKeyLst.add(pdb,tbl,tbl_a);
    } /* end for(...) */
  }

  /* deal table names from the 'show create table' */
  else if (node->type==mktype(m_stmt,s_cTbl)) {
    sp->m_tblKeyLst.add(def_db,node->name,NULL);
    return 0;
  }

  /* deal table names from the 'desc xxx'/ 'drop table' */
  else if (node->type==mktype(m_stmt,s_desc) ||
     node->type==mktype(m_stmt,s_dTbl) ||
     node->type==mktype(m_stmt,s_dTbl_cond)) {
    stxNode *tmp = m_ts.find_in_tree(node,mktype(m_endp,s_col));
    sp->m_tblKeyLst.add(def_db,tmp->name,NULL);
    return 0;
  }

  for (i=0;i<node->op_lst.size();i++) {
    ret = collect_target_tbls(node->op_lst[i],sp,def_db,err) ;
    if (ret) 
      return ret ;
  }
  return 0;
}

int sql_parser::collect_place_holders(stxNode *node, int &num_phs)
{
  uint16_t i=0;

  /* it's the place-holder node */
  if (node->type==mktype(m_endp,s_col)&&!strcmp(node->name,"?")) {
    m_maps[node] = num_phs++ ;
    return 0;
  }
  for (i=0;i<node->op_lst.size();i++) {
    collect_place_holders(node->op_lst[i],num_phs);
  }
  return 0;
}

bool sql_parser::is_agg_func(char *name, int &t)
{
  const std::map<std::string,int> ag_list {
    {"sum",agt_sum}, 
    {"count",agt_count}, 
    {"max",agt_max}, 
    {"min",agt_min},
  } ;

  for (auto i : ag_list) {
    if (i.first==name) {
      t = i.second ;
      return true;
    }
  }
  return false;
}

int sql_parser::collect_agg_info(stxNode *node, tSqlParseItem *sp)
{
  stxNode *nd = 0;
  int t = 0;
  size_t i=0;

  if (get_stmt_type(node,nd,t) || sget(t)!=s_select) {
    //log_print("not 'select' statement\n");
    return 0;
  }

  /* get 'select' list */
  nd = nd->op_lst[0] ;

  /* search for aggregation functions */
  for (i=0;i<nd->op_lst.size();i++) {
    auto p = m_ts.find_in_tree(nd->op_lst[i],mktype(m_endp,s_func));

    if (!p || !is_agg_func(p->name,t)) {
      continue ;
    }

    sp->add_agg_info(i,t);
  }

  return 0;
}

int sql_parser::get_stmt_type(stxNode *tree, stxNode* &out, int &type)
{
  stxNode *nd = tree ;
  
  /*
   * test for statement type
   */
  while (nd&&mget(nd->type)!=m_stmt) {
    nd = nd->op_lst.size()>0?nd->op_lst[0]:NULL ;
  }

  /* not found any statement types */
  if (!nd) {
    log_print("no statement types found\n");
    return -1;
  }

  type = nd->type ;
  out  = nd ;

  return 0;
}

/* do a deep digging on statement */
int sql_parser::scan(char *sql, size_t sz, 
  tSqlParseItem *sp, char *db, tContainer &err,
  stxNode *pTree
  )
{
#define RETURN(__rc__) do {\
  if (!pTree) m_ts.destroy_tree(root); \
  return (__rc__); \
}while(0)
  stxNode* root = pTree?pTree:parse_tree(sql,sz);
  int num_phs = 0; /* place holder number */
  auto gen_err = [](auto &err, auto rc) {
    err.tc_resize(1024);
    size_t sz = do_err_response(0,err.tc_data(),rc,rc);
    err.tc_update(sz);
  } ;

  /* summary all targeting tables */
  sp->m_tblKeyLst.clear();
  if (collect_target_tbls(root,sp,db,err)) {
    log_print("error collect targeting table names\n");
    RETURN(-1);
  }

  /* collect place-holders */
  m_maps.clear();
  if (collect_place_holders(root,num_phs)) {
    gen_err(err,ER_INTERNAL_SCAN_PLACEHOLDER);
    log_print("error collect place-holders info\n");
    RETURN(-1);
  }

  /* reset content of the parse item */
  sp->reset();
  /* fetch sharding key info from statement */
  if (collect_sharding_columns(root,sp,db)) {
    gen_err(err,ER_INTERNAL_SCAN_SHARDING_COL);
    log_print("error collect sharding columns\n");
    RETURN(-1);
  }

  /* collect aggregated informations, for example, sum(), count() */
  if (collect_agg_info(root,sp)) {
    gen_err(err,ER_INTERNAL_SCAN_AGGREGATES);
    log_print("error collect aggregated infos\n");
    RETURN(-1);
  }

  RETURN(0);
}

