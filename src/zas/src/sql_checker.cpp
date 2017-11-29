
#include "connstr.h"
#include "dbug.h"
#include "sql_checker.h"

using namespace stree_types;

/*
 * class sql_checker
 */
#if USE_STL_REGEX==1
sql_checker::sql_checker(void) :
  /* for normal endpoint */
  r_ep(const_ptn_ep),
    /* for constant string */
  r_cs(const_ptn_cs),
    /* for constant integer and floating digit */
  r_cd(const_ptn_cd)
{
}

sql_checker::~sql_checker(void)
{
}
#else
sql_checker::sql_checker(void) 
{
  regcomp(&r_ep,const_ptn_ep,REG_EXTENDED);
  regcomp(&r_cs,const_ptn_cs,REG_EXTENDED);
  regcomp(&r_cd,const_ptn_cd,REG_EXTENDED);
}

sql_checker::~sql_checker(void)
{
  regfree(&r_ep);
  regfree(&r_cs);
  regfree(&r_cd);
}
#endif

inline int sql_checker::check_stmt_list(stxNode *node)
{
  uint16_t i=0;

  if (mget(node->type)!=m_list) {
    return 1;
  }
  /* check illegal alias */
  if (sget(node->type)!=s_sel && 
     sget(node->type)!=s_from &&
     sget(node->type)!=s_upd) {
    for (i=0;i<node->op_lst.size();i++) 
      if (node->op_lst[i]->type==mktype(m_endp,s_alias)) {
        printd("(%d) invalid syntax near '%s' \n",
          __LINE__, sub_type_str(node->type));
        return 0;
      }
  }
  /* TODO:check illegal select list */
  if (sget(node->type)==s_sel) {
    for(i=0;i<node->op_lst.size();i++) {
      /* find if 'wildcast' item */
      if (node->op_lst[i]->type==mktype(m_keyw,s_wildcast))
        break ;
    }
    /* check if wildcast item is the 1st 
     *  in 'select list' */
    if (i>0 && i<node->op_lst.size()) {
      printd("(%d) invalid syntax near '%s' \n",
        __LINE__, sub_type_str(mktype(m_keyw,s_wildcast)));
      return 0;
    }
  }
  /* check illegal statement lists */
  if (node->op_lst.size()<1 && sget(node->type)!=s_arg) {
    printd("(%d) invalid syntax near '%s' \n",
      __LINE__,sub_type_str(node->type));
    return 0;
  }
  /* check illegal 'insert' statement */
  if (sget(node->type)==s_fmt) {
    /* find 'values' list */
    stxNode *nd = find_in_tree(node->parent,
      mktype(m_list,s_val));
    /* alias number */
    size_t num_a = 0;
    if (!nd) {
      /* try if value list has a select statement */
      nd = find_in_tree(node->parent,mktype(m_list,s_sel));
      if (!nd)
        return 1;
      /* sumerise alias count in select list */
      for (i=0;i<nd->op_lst.size();i++)
        if (sget(nd->op_lst[i]->type)==s_alias)
          num_a++ ;
    }
    /* find actual 'format' and 'values' list */
    for (;nd->op_lst[0]->type==mktype(m_list,s_norm);
      nd=nd->op_lst[0]);
    for (;node->op_lst[0]->type==mktype(m_list,s_norm);
      node=node->op_lst[0]);
    /* compare list item counts */
    if (node->op_lst.size()!=(nd->op_lst.size()-num_a)) {
      printd("(%d) invalid syntax near '%s' \n",
        __LINE__,sub_type_str(node->type));
      return 0;
    }
  }
  return 1;
}

int sql_checker::do_check(__regex_t__ &r, char *item)
{
#if USE_STL_REGEX==1
  return std::regex_match(item,r)?0:-1;
#else
  regmatch_t subs[1];
  return regexec(&r,item,0,subs,0);
#endif
}

inline int sql_checker::check_endpoint(stxNode *node)
{
  uint16_t i=0;
  char m[32]="";
  int nAttr=0;
  bool bSz = false;
  stxNode *p = 0;

  if (mget(node->type)!=m_endp && 
     mget(node->type)!=m_pha) {
    return 1;
  }
  /* check place holders */
  if (sget(node->type)==s_ph) {
    for (i=0;i<node->op_lst.size();i++) {
      p = node->op_lst[i] ;
      if (p->type==mktype(m_pha,s_pha_t) &&
         !strcasecmp(p->name,"char")) {
        bSz = true ;
      }
      nAttr |= (1<<sget(p->type));
    }
    /* check missing attributes */
    if ((bSz && (nAttr&0x7)!=0x7) || 
       (!bSz && ((nAttr&0x3)!=0x3))) {
      printd("(%d) invalid syntax near '%s'\n",
        __LINE__, node->name);
      return 0;
    }
  }
  if (*node->name=='('|| *node->name==')' ||
     *node->name==',' || *node->name==';' ||
     *node->name=='.' || *node->name==':' ||
     *node->name=='[' || *node->name==']' ||
     //!strcasecmp(node->name,"in")  ||
     !strcasecmp(node->name,"like")||
     !strcasecmp(node->name,"not") ||
     !strcasecmp(node->name,"is")  ||
     !strcasecmp(node->name,"and") ||
     !strcasecmp(node->name,"or")) {
    printd("(%d) invalid syntax near '%s' \n", 
      __LINE__, node->name);
    return 0;
  }
  /* check validation of normal endpoint */
  if (mget(node->type)==m_endp   && 
     (sget(node->type)==s_tbl    ||
      sget(node->type)==s_schema ||
      sget(node->type)==s_col)) {
    if (do_check(r_ep,node->name)) {
      printd("(%d) invalid syntax near '%s' %s\n", 
        __LINE__, node->name, m);
      return 0;
    }
  }
  /* check validation of constant string */
  if (node->type==mktype(m_endp,s_c_str)) {
    if (do_check(r_cs,node->name)) {
      printd("(%d) invalid syntax near '%s' %s\n", 
        __LINE__, node->name, m);
      return 0;
    }
  }
  /* check alias, may be normal endpoint or 
   *  constant string */
  if (node->type==mktype(m_endp,s_alias)) {
    if (do_check(r_ep,node->name) && do_check(r_cs,node->name)) {
      printd("(%d) invalid syntax near '%s'\n",
        __LINE__, node->name);
      return 0;
    }
  }
  /* check validation of constant digits */
  if (node->type==mktype(m_endp,s_c_int) ||
     node->type==mktype(m_endp,s_c_float)) {
    if (do_check(r_cd,node->name)) {
      printd("(%d) invalid syntax near '%s' %s\n", 
        __LINE__, node->name, m);
      return 0;
    }
  }
  return 1;
}

inline int sql_checker::check_expr(stxNode *node)
{
  if (mget(node->type)!=m_expr) {
    return 1;
  }
  if (sget(node->type)==s_in) {
    if ((node->op_lst.size()<2) ||
       (mget(node->op_lst[1]->type)==m_endp&&
        strlen(node->op_lst[1]->name)<=0) ||
       (mget(node->op_lst[1]->type)==m_list&&
       node->op_lst[1]->op_lst.size()<=0)) {
      printd("(%d) invalid syntax near 'in' expr\n",
        __LINE__);
      return 0;
    }
  }
  return 1;
}

int sql_checker::do_checking(stxNode *node)
{
  uint16_t i=0;

  if (!node) {
    return 1;
  }
  /* do various checking on node */
  if (!check_endpoint(node)) {
    return 0;
  }
  if (!check_stmt_list(node)) {
    return 0;
  }
  if (!check_expr(node)) {
    return 0;
  }
  for (i=0;i<node->op_lst.size();i++) {
    if (!do_checking(node->op_lst[i])) 
      return 0;
  }
  return 1;
}

