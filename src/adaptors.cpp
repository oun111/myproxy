
#include <memory>
#include "connstr.h"
#include "adaptors.h"
#include "sql_checker.h"
#include "attrib_list.h"
#include "dbug.h"

using namespace stree_types;

/* 
 * class mysql_adpt
 */
mysql_adpt::mysql_adpt():root(0)
{
  register_adpt_actions();
}

mysql_adpt::~mysql_adpt()
{
  reset();
  unregister_adpt_actions();
}

void mysql_adpt::reset(void)
{
  /* release older tree */
  if (root) {
    destroy_tree(root);
    root = 0;
  }
}

int mysql_adpt::do_adaption(
  std::string &stmt, 
  class attribute_list* plist,
  const bool mp)
{
  reset();
  /* mysql prepare mode or not */
  bPrep = mp ;
  /* remove all existing comments */
  if (!eliminate_comments(stmt)) {
    return 0;
  }
  /* build syntax tree */
  if (!(root=build_tree(stmt))) {
    return 0;
  }
  /* XXX: debug */
  //print_tree(root,1);

  /* do sql syntax checking */
  {
    sql_checker t_chk ;

    if (!t_chk.do_checking(root))
      return 0;
  }
  /* 
   * gather place holder informations from
   *  tree
   */
  plist->reset();
  plist->reset_counters();
  if (!get_placeholder_info(root,plist)) {
    return 0;
  }
  /* create place holder orders */
  plist->build_order_map();
  /* 
   * translate the tree to mysql style 
   */
  mysqlize_tree(root,0);
  /* XXX: debug */
  //print_tree(root,0);

  /* after syntax adaptions,
   *  re-gather place holder informations 
   *  from attribute list, cause the number
   *  and order of place holders may have been
   *  changed
   */
  plist->reset_counters();
  if (!get_placeholder_info(root,plist)) {
    return 0;
  }
  /* re-create place holder orders */
  plist->rebuild_order_map();
  /* for prepare mode, adapts the 
   *  place holders here */
  if (bPrep) {
    do_placeholder_adpt(root);
    //do_mysqlization(root,0,ac_ph);
    //mysqlize_tree(root,0,true);
  }
  /* serialize the syntax tree */
  do_serialize(root,stmt);
  /* yzhou added: release the tree */
  reset();
  return 1;
}

int mysql_adpt::do_placeholder_adpt(stxNode *root)
{
  return do_mysqlization(root,0,ac_ph);
}

void mysql_adpt::unregister_adpt_actions(void)
{
  a_list.clear();
}

inline int mysql_adpt::do_action_register(uint32_t t, adpt_cb func)
{
  tAdptAct *act = 0;
  auto act_ptr = make_shared<tAdptAct>();

  act = act_ptr.get() ;
  act->sel = t_type ;
  act->u.t = t;
  act->func=func;
  a_list.push_back(act_ptr);
  return 1;
}

inline int mysql_adpt::do_action_register(char *val, adpt_cb func)
{
  tAdptAct *act = 0;
  auto act_ptr = make_shared<tAdptAct>();

  act = act_ptr.get() ;
  act->sel = t_key ;
  strcpy(act->u.k,val);
  act->func=func;
  a_list.push_back(act_ptr);
  return 1;
}

void mysql_adpt::register_adpt_actions(void)
{
  /* oracle 'nextval' */
  do_action_register((char*)"nextval",
    &mysql_adpt::adpt_nextval);

  /* oracle 'curval' */
  do_action_register((char*)"currval",
    &mysql_adpt::adpt_currval);

  /* oracle 'nvl' */
  do_action_register((char*)"nvl",
    &mysql_adpt::adpt_nvl);

  /* oracle 'sysdate' */
  do_action_register((char*)"sysdate",
    &mysql_adpt::adpt_sysdate);

  /* oracle function 'trunc' */
  do_action_register((char*)"trunc",
    &mysql_adpt::adpt_trunc);

  /* oracle function 'to_date' */
  do_action_register((char*)"to_date",
    &mysql_adpt::adpt_trunc);

  /* yzhou modified 2016.8.5 */
  /* oracle function 'to_char' */
  do_action_register((char*)"to_char",
    &mysql_adpt::adpt_tochar);

  /* oracle function 'to_number' */
  do_action_register((char*)"to_number",
    &mysql_adpt::adpt_to_num);

  /* oracle function 'substr' */
  do_action_register((char*)"substr",
    &mysql_adpt::adpt_substr);

  /* oracle function 'regexp_like' */
  do_action_register((char*)"regexp_like",
    &mysql_adpt::adpt_relike);

  /* oracle object name 'dbms_lob' */
  do_action_register((char*)"dbms_lob",
    &mysql_adpt::adpt_dbms_lob);

  /* oracle 'dummy' */
  do_action_register((char*)"dummy",
    &mysql_adpt::adpt_dummy);

  /* add quote onto mysql keyword 'range' */
  do_action_register((char*)"range",
    &mysql_adpt::adpt_add_quote);

  /* add quote onto mysql keyword 'key' */
  do_action_register((char*)"key",
    &mysql_adpt::adpt_add_quote);

  /* add quote onto mysql keyword 'minvalue' */
  do_action_register((char*)"minvalue",
    &mysql_adpt::adpt_add_quote);

  /* add quote onto mysql keyword 'maxvalue' */
  do_action_register((char*)"maxvalue",
    &mysql_adpt::adpt_add_quote);

  /* add quote onto mysql keyword 'rownum' */
  do_action_register((char*)"rownum",
    &mysql_adpt::adpt_rownum);

  /* oracle 'start with'/'connect by' */
  do_action_register(mktype(m_list,s_cb),
    &mysql_adpt::adpt_hierarchal);

  /* oracle '+=' operator */
  do_action_register(mktype(m_expr,s_peql),
    &mysql_adpt::adpt_peql);

  /* oracle 'for update wait/nowait/of' */
  do_action_register(mktype(m_keyw,s_fu),
    &mysql_adpt::adpt_4update_attr);

  /* oracle 'limit' */
  do_action_register(mktype(m_list,s_limit),
    &mysql_adpt::adpt_limit);

  /* oracle 'insert' */
  do_action_register(mktype(m_stmt,s_insert),
    &mysql_adpt::adpt_insert);

  /* oracle expression 'in' */
  do_action_register(mktype(m_expr,s_in),
    &mysql_adpt::adpt_expr_in);

  /* oracle 'delete' */
  do_action_register(mktype(m_stmt,s_delete),
    &mysql_adpt::adpt_delete);

  /* oracle 'update' */
  do_action_register(mktype(m_stmt,s_update),
    &mysql_adpt::adpt_update);

  /* oracle 'with...as' */
  do_action_register(mktype(m_stmt,s_withas),
    &mysql_adpt::adpt_withas);

  /* oracle 'union/union all' */
  do_action_register(mktype(m_expr,s_union),
    &mysql_adpt::adpt_union);
  do_action_register(mktype(m_expr,s_uniona),
    &mysql_adpt::adpt_union);

  /* oracle 'select'(subqueries) */
  do_action_register(mktype(m_stmt,s_select),
    &mysql_adpt::adpt_select);

  /* oracle '(+)' left/right join */
  do_action_register(mktype(m_expr,s_olj),
    &mysql_adpt::adpt_ora_joins);
  do_action_register(mktype(m_expr,s_orj),
    &mysql_adpt::adpt_ora_joins);

  /* oracle 'merge into' */
  do_action_register(mktype(m_stmt,s_merge),
    &mysql_adpt::adpt_merge_into);

  /* adapts the normal list */
  do_action_register(mktype(m_list,s_norm),
    &mysql_adpt::adpt_normal_list);

  /* XXX: adapts place holders, shold be at the
   *  end of adaption list */
  do_action_register(mktype(m_endp,s_ph),
    &mysql_adpt::adpt_place_holder);
  /* save last one(place holder adaption) 
   *  for later use */
  ac_ph = (a_list[a_list.size()-1]).get() ;
}

/* process all place holders in tree */
#if TREE_NON_RECURSION==0
int mysql_adpt::get_placeholder_info(stxNode *node,
  attribute_list *plist)
{
  uint16_t i = 0;
  stxNode *nd = 0;
  char *pname=0, *ptype=0, *pSz = 0;

  if (!node) {
    return 1;
  }
  if (node->type==mktype(m_endp,s_ph)) {
    for (i=0;i<node->op_lst.size();i++) {
      nd = node->op_lst[i] ;
      /* place holder's name and type */
      if (nd->type==mktype(m_pha,s_pha_name))
        pname = nd->name ;
      else if (nd->type==mktype(m_pha,s_pha_t)) 
        ptype = nd->name ;
      else if (nd->type==mktype(m_pha,s_pha_sz))
        pSz = nd->name ;
    }
    /* process attribute item */
    if (ptype && pname) {
      if (!plist->add_attr(pname,ptype,pSz))
        return 0;
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    if (!get_placeholder_info(node->op_lst[i],plist)) 
      return 0;
  }
  return 1;
}
#else
int mysql_adpt::get_placeholder_info(stxNode *node,
  attribute_list *plist)
{
  uint16_t i = 0;
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *ptr = node, *nd = 0 ;
  char *pname=0, *ptype=0, *pSz = 0;
  int _np = -1;
  std::stack<int> stk ;

  for (;ptr&&(dir||_np>=0);) {
    /*  find place holder from tree */
    if (dir && ptr->type==mktype(m_endp,s_ph)) {
      for (i=0;i<ptr->op_lst.size();i++) {
        nd = ptr->op_lst[i] ;
        /* place holder's name and type */
        if (nd->type==mktype(m_pha,s_pha_name))
          pname = nd->name ;
        else if (nd->type==mktype(m_pha,s_pha_t)) 
          ptype = nd->name ;
        else if (nd->type==mktype(m_pha,s_pha_sz))
          pSz = nd->name ;
      }
      /* process attribute item */
      if (ptype && pname && 
         !plist->add_attr(pname,ptype,pSz)) 
        return 0; ;
    }
    /* try next node */
    _np = _np<((int)ptr->op_lst.size()-1)?
      (_np+1):-1;
    if (_np<0) {
      if (!stk.empty()) {
        _np = stk.top();
        stk.pop();
      }
      ptr = ptr->parent ;
      dir = 0;
    } else {
      ptr = ptr->op_lst[_np];
      stk.push(_np);
      _np = -1;
      dir = 1;
    }
  } /* end for */
  return 1;
}
#endif

/* split the 'start with' tree into 
 *  2 sub trees */
int mysql_adpt::__split_sw_tree(
  stxNode *tree, 
  stxNode **st0,
  stxNode **st1
  )
{
  int pos = 0;
  stxNode *ptr = 0, *tmp = 0, *p;

  /* get first sub tree */
  for (ptr=tree;
    ptr && ptr->op_lst.size()>0 &&
    (ptr->type==mktype(m_list,s_sw) ||
    ptr->type==mktype(m_expr,s_and) ||
    ptr->type==mktype(m_expr,s_or));
    ptr=ptr->op_lst[0]) ;
  /* split the 1st sub tree */
  if (!ptr) {
    printd("fatal: cant split the tree\n");
    return 0;
  }
  *st0 = ptr ;
  /* detach from parent */
  p  = ptr->parent ;
  pos= get_parent_pos(ptr);
  p->op_lst.erase(p->op_lst.begin()+pos);
  /* no sub tree left */
  if (tree->op_lst.size()<=0) {
    *st1 = 0;
    return 1;
  }
  /* attach a dummy node */
  tmp = create_node(0,m_expr,s_eql);
  ptr = create_node((char*)"1",m_endp,s_c_str);
  attach(tmp,ptr);
  ptr = create_node((char*)"1",m_endp,s_c_str);
  attach(tmp,ptr);
  attach(p,tmp,pos);
  /* detach the whole sub tree from
   *  'start with' node */
  *st1 = tree->op_lst[0] ;
  tree->op_lst.erase(tree->op_lst.begin());
  return 1;
}

stxNode* mysql_adpt::__create_empty_select_tree(
  bool bSel, /* with 'select' list item */
  bool bFrm, /* with 'from' list */
  bool bW    /* with 'where' list */
  )
{
  stxNode *p= 0, *tmp = 0, *pstmt = 0;

  /* select list */
  tmp = create_node(0,m_list,s_sel);
  if (bSel) {
    p = create_node(0,m_keyw,s_wildcast);
    attach(tmp,p);
  }
  /* statement */
  pstmt = create_node(0,m_stmt,s_select);
  attach(pstmt,tmp);
  /* from list */
  if (bFrm) {
    tmp = create_node(0,m_list,s_from);
    attach(pstmt,tmp);
  }
  /* where list */
  if (bW) {
    tmp = create_node(0,m_list,s_where);
    attach(pstmt,tmp);
  }
  return pstmt ;
}

stxNode* mysql_adpt::__create_inherit_nodes(char *parent,
  char *child)
{
  stxNode *p0, *p1 ;

  p0 = create_node(child,m_endp,s_col);
  p1 = create_node(parent,m_endp,s_tbl);
  attach(p1,p0);
  return p1;
}

stxNode* mysql_adpt::__create_binary_nodes(int op,
  stxNode *op0, stxNode *op1)
{
  stxNode *p = create_node(0,0,0);

  p->type = op ;
  attach(p,op0);
  attach(p,op1);
  return p;
}

stxNode* mysql_adpt::__gen_where_stuff(stxNode *node,
  char *biTbl,bool bReve)
{
  stxNode *pstmt=0, *p=0, *tmp=0, *p1=0;

  /* create the upper 'select' statement */
  pstmt = __create_empty_select_tree(false,true,true);
  /* add item to 'select' list */
  attach(pstmt->op_lst[0],
    __create_inherit_nodes((char*)"b",(char*)"id"));
  /* add items to 'from' list */
  tmp = create_node(biTbl,m_endp,s_col);
  attach(pstmt->op_lst[1],tmp);
  tmp = create_node((char*)"a",m_endp,s_alias);
  attach(pstmt->op_lst[1],tmp);
  /* add 'select *from xx' sub statement to
   *  'from' list of the statement created above */
  p = __create_empty_select_tree(true,true,false);
  tmp = create_node(biTbl,m_endp,s_col);
  attach(p->op_lst[1],tmp);
  attach(pstmt->op_lst[1],p);
  p = create_node((char*)"b",m_endp,s_alias);
  attach(pstmt->op_lst[1],p);
  /*
   * 'where' list
   */
  tmp = __create_inherit_nodes((char*)"a",
    (char*)"id");;
  /* 'node' is 'xx in[=] {value...}' */
  p1= node->op_lst[0] ;
  node->op_lst.erase(node->op_lst.begin());
  /* recreate the node as 'a.id in[=] {value...}' */
  attach(node,tmp,0);
  tmp = node;
  /* b.lkey>=a.lkey and b.rkey<=a.rkey */
  p = __create_binary_nodes(
    mktype(m_expr,s_and),
    /* b.lkey<=[>=]a.lkey */
    /* yzhou modified: 2015.12.25 */
    __create_binary_nodes(mktype(m_expr,!bReve?s_be:s_le),
      __create_inherit_nodes((char*)"b",(char*)"lkey"),
      __create_inherit_nodes((char*)"a",(char*)"lkey")),
    /* b.rkey>=[<=]a.rkey */
    /* yzhou modified: 2015.12.25 */
    __create_binary_nodes(mktype(m_expr,!bReve?s_le:s_be),
      __create_inherit_nodes((char*)"b",(char*)"rkey"),
      __create_inherit_nodes((char*)"a",(char*)"rkey"))
    );
  /* a.id in[=] {value...} and 
   *  b.lkey>=a.lkey and b.rkey<=a.rkey */
  p = __create_binary_nodes(
    mktype(m_expr,s_and),tmp,p) ;
  /* add into 'where' list */
  attach(pstmt->op_lst[2],p);
  /* construct stuffs in the toppest 
   *  'where' list */
  p = __create_binary_nodes(
    mktype(m_expr,s_in),p1,pstmt);
  return p;
}

stxNode* mysql_adpt::__create_dummy_start_with(stxNode *nd, 
  char *name)
{
  stxNode *p0 = 0, *p = 0;

  p = dup_tree(nd);
  /* rename terminate leaf with 'name' */
  for (p0=p;p&&p->op_lst.size()>0;
    p=p->op_lst[0]);
  strcpy(p->name,name);
  /* return the 'xxx=1' node */
  return __create_binary_nodes(
    mktype(m_expr,s_eql), p0,
    create_node((char*)"1",m_endp,s_col));
}

int mysql_adpt::__join_where_clouse(stxNode *pw,
  stxNode *pcb, stxNode *st0, stxNode *st1)
{
  bool bReve = false /* do reverse hierarchal or not */ ;
  stxNode *p = 0, *pir = 0, *tmp = 0;

  if (!pcb) {
    printd("'connect by' not found\n");
    return -1;
  }
  /* find 'prior' node from 'connect by' */
#if 0
  pir = pcb->op_lst[0] ;
  pir = pir->op_lst[0]->type==
    mktype(m_uexpr,s_pir)?pir->op_lst[0]:
    pir->op_lst[1];
#else
  pir = find_in_tree(pcb,mktype(m_uexpr,s_pir));
#endif
  /* save sub tree below 'prior' node */
  tmp = pir->op_lst[0] ;
  /* find terminal leaf of 'prior' node */
  for (;pir && pir->op_lst.size()>0;
    pir=pir->op_lst[0]);
  /* generate stuffs in 'where' clouse */
  if (strcasestr(pir->name,"orgid")) {
    /* create dummy 'start with' if null */
    if (!st0) 
      st0 = __create_dummy_start_with(tmp,
        (char*)"orgid");
    /* it's 'pg_info_org' table */
    bReve = pir&&!!strcasestr(pir->name,"parent");
    p = __gen_where_stuff(st0,
      (char*)"pgInfoOrg_bi_table",
      bReve);
  } else if (strcasestr(pir->name,"userid")) {
    /* create dummy 'start with' if null */
    if (!st0) 
      st0 = __create_dummy_start_with(tmp,
        (char*)"masteruserid");
    /* it's 'cm_rl_user2user' table */
    bReve = pir&&!!strcasestr(pir->name,"salve");
    p = __gen_where_stuff(st0,
      (char*)"user2user_bi_table",
      bReve);
  } else {
    printd("un-supported target\n");
    return -1;
  }
  if (pw->op_lst.size()>0) {
    tmp = pw->op_lst[0] ;
    pw->op_lst.erase(pw->op_lst.begin());
    p = __create_binary_nodes(
      mktype(m_expr,s_and),p,tmp);
  }
  if (st1) {
    p = __create_binary_nodes(
      mktype(m_expr,s_and),p,st1);
  }
  /* try to test and merge extended part of 'connect by'
   *  clouse to under new stuff of 'where' */
  if (pcb->op_lst[0]->type==mktype(m_expr,s_and) ||
     pcb->op_lst[0]->type==mktype(m_expr,s_or)) {
    /*
     * process connect by clouse like: 
     *
     *   connect by a=b and x=y
     *                  ^^^^^^^
     */
    p = __create_binary_nodes(
      mktype(m_expr,s_and),
      /* choose the extended sub tree */
      dup_tree(pcb->op_lst[0]->op_lst[1]),
      p);
  }
  /* add all stuffs to 'where' list */
  attach(pw,p);
  return 1;
}

int mysql_adpt::__find_in_from_list(stxNode *pFrom, char *pname)
{
  uint16_t i = 0;
  stxNode *nd = 0;

  for (i=0;i<pFrom->op_lst.size();i++) {
    nd = pFrom->op_lst[i] ;
    if (mget(nd->type)==m_endp && 
       find_in_tree(nd,pname)) 
      return i ;
  }
  return -1;
}

int mysql_adpt::__find_in_join_expr(stxNode *pFrom, char *pname)
{
  stxNode *p = 0;
  uint16_t i=0;

  /* find 'join' expr in 'from' list */
  for (;i<pFrom->op_lst.size();i++) {
    p = pFrom->op_lst[i];
    if ((p->type==mktype(m_expr,s_ljoin)|| 
       p->type==mktype(m_expr,s_rjoin)  ||
       p->type==mktype(m_expr,s_ijoin)  ||
       p->type==mktype(m_expr,s_cjoin)  ||
       p->type==mktype(m_expr,s_join)) &&
       find_in_tree(p,pname)) 
      return i;
  }
  printd("not found\n");
  return -1;
}

/* convert 'oracle join' operand to
 *  normal 'join' expression */
int mysql_adpt::__ojoin2join(stxNode *pFrom, 
  stxNode *pJoinExpr, 
  int op,
  bool bFront /* insert op to the front
                of op list of 'join' */
  )
{
  uint16_t sz = pFrom->op_lst.size()  ;
  stxNode *nd = 0, *pa = 0 ;

  if (!pFrom) {
    return 0;
  }
  if (pFrom->op_lst[op]->type!=mktype(m_endp,s_alias)) {
    nd = pFrom->op_lst[op] ;
    /* also get alias */
    if ((op+1)<sz && pFrom->op_lst[op+1]->type
       ==mktype(m_endp,s_alias)) {
      pa = pFrom->op_lst[op+1];
    }
  } else {
    /* get alias */
    pa = pFrom->op_lst[op] ;
    /* get node */
    nd = pFrom->op_lst[op-1];
    op-= 1;
  }
  /* detach from 'from' list */
  pFrom->op_lst.erase(pFrom->op_lst.begin()+op);
  if (pa) {
    pFrom->op_lst.erase(pFrom->op_lst.begin()+op);
  }
  /* add the operand of 'left join', also
   *  its possible alias */
  attach(pJoinExpr,nd,bFront?0:-1);
  if (pa) {
    attach(pJoinExpr,pa,bFront?1:-1);
  }
  return 1;
}

int mysql_adpt::__join_proc_method1(stxNode *pFrom, 
  stxNode *pExpr, int p0, int p1)
{
  stxNode *node = 0, *tmp = 0 ;
  uint16_t sz = pFrom->op_lst.size();

  if (p0>=sz || p1>=sz) {
    printd("pos out of range\n");
    return -1;
  }
  /* build the 'join' node */
  node = create_node(0,0,0);
  node->type = pExpr->type==mktype(m_expr,s_olj)?
    mktype(m_expr,s_ljoin):
    mktype(m_expr,s_rjoin);
  /* add both of operands of 'oracle join' and
   *  their possible alias */
  if (p0>p1) {
    __ojoin2join(pFrom,node,p0);
    __ojoin2join(pFrom,node,p1);
  } else {
    __ojoin2join(pFrom,node,p1);
    __ojoin2join(pFrom,node,p0,true);
  }
  /* process the 'on' list */
  tmp = create_node(0,m_list,s_on);
  sset(pExpr->type,s_eql);
  attach(tmp,pExpr);
  attach(node,tmp);
  /* attach new 'join' expression to 
   *  'from' list */
  attach(pFrom,node);
  return 1;
}

int mysql_adpt::__join_proc_method2(
  stxNode *pFrom,
  stxNode *pExpr, 
  uint32_t pos /* position of 'join' expr in
            'from' list */
  )
{
  uint32_t i = 0;
  stxNode *node = 0, *p = 0;
  
  /* get related 'join' expr in 
   *  'from' list */
  if (pos>=pFrom->op_lst.size() ||
     !(node=pFrom->op_lst[pos])) {
    printd("found no 'join' expr\n");
    return -1;
  }
  /* find 'on' list */
  for (i=0;i<node->op_lst.size();i++) {
    if (node->op_lst[i]->type==
       mktype(m_list,s_on))
      break ;
  }
  if (i>=node->op_lst.size() ||
     node->op_lst[i]->op_lst.size()<=0) {
    printd("found no 'on' list from "
      "'join' expr\n");
    return -1;
  }
  /* detach stuffs in 'on' list */
  node = node->op_lst[i] ;
  p = node->op_lst[0] ;
  node->op_lst.erase(node->op_lst.begin());
  /* change expr type of 'pExpr' */
  sset(pExpr->type,s_eql);
  /* construct new 'on' stuff */
  p = __create_binary_nodes(
    mktype(m_expr,s_and),
    p,pExpr);
  /* attach to 'on' list */
  attach(node,p);
  return 1; 
}

int mysql_adpt::__process_ora_join(stxNode *pFrom, 
  stxNode *pExpr)
{
  /* endpoint state */
  enum epst {
    s_na, /* not found */
    s_frm,/* in from list */
    s_join/* in join expr */
  } ;
  char *pname = 0;
  uint8_t st0 = s_na, st1 = s_na ;
  int p0 = 0, p1=0 ;

  /* test if operand of 'join' is in 'from' list
   *  or other 'join' expr */
  /*
   * #1 
   */
  pname = pExpr->op_lst[0]->name ;
  st0 = (p0=__find_in_from_list(pFrom,pname))>=0?s_frm:
    (p0=__find_in_join_expr(pFrom,pname))>=0?s_join:s_na ;
  if (st0==s_na) {
    printd("definition of %s missing\n", pname);
    return -1;
  }
  //printd("node %s, st %d, p %d\n",pname,st0,p0);
  /*
   * #2 
   */
  pname = pExpr->op_lst[1]->name ;
  st1 = (p1=__find_in_from_list(pFrom,pname))>=0?s_frm:
    (p1=__find_in_join_expr(pFrom,pname))>=0?s_join:s_na ;
  if (st1==s_na) {
    printd("definition of %s missing\n", pname);
    return -1;
  }
  //printd("node %s, st %d, p %d\n",pname,st1,p1);
  /* 
   * attach the join item to suitable 
   *  place of 'from' list 
   */
  /* case 1: both definitions of 2 ops are 
   *  in 'from' list but not 'join' expr */
  if (st0==s_frm && st0==st1) {
    __join_proc_method1(pFrom,pExpr,p0,p1);
  } 
  /* case 2: one op is in 'from' list and another
   *  in 'join' expr */
  else if ((st0==s_frm && st1==s_join)) { 
    sset(pExpr->type, sget(pExpr->type)==s_orj
      ?s_olj:s_orj);
    __join_proc_method1(pFrom,pExpr,p1,p0);
  } else if (st0==s_join && st1==s_frm) {
    __join_proc_method1(pFrom,pExpr,p0,p1);
  }
  /* case 3: both ops are in a single 'join' expr */
  else if (st0==s_join && st1==st0 && p0==p1) {
    __join_proc_method2(pFrom,pExpr,p0);
  }
  /* case 4: 2 ops are in different 'join' exprs */
  else if (st0==s_join && st1==st0 && p0!=p1) {
    __join_proc_method1(pFrom,pExpr,p0,p1);
  }
  return 0;
}

int mysql_adpt::adpt_ora_joins(stxNode *node, int pos)
{
  uint16_t i = 0;
  stxNode *nd = 0, *ptr = 0;

  /* operands is in the form like: 
   *  prefix.col(+)=<int_const>  or
   *  <int_const>=prefix.col(+) 
   *
   *  set to normal 'a=b' expr
   */
  if (node->op_lst[0]->op_lst.size()<=0 ||
     node->op_lst[1]->op_lst.size()<=0) {
    sset(node->type,s_eql);
    return 0;
  }
  /* get parent stmt */
  for(ptr=node->parent;
    ptr&&mget(ptr->type)!=m_stmt;
    ptr=ptr->parent);
  if (!ptr) {
    printd("parent statement not found\n");
    return -1;
  }
  /* find 'from' list from parent */
  for (i=0;i<ptr->op_lst.size();i++) {
    if (ptr->op_lst[i]->type==mktype(m_list,s_from))
      break ;
  }
  if (i>=ptr->op_lst.size()) {
    printd("no 'from' list found\n");
    return -1;
  }
  /* the 'from' list */
  ptr = ptr->op_lst[i] ;
  /* detach the 'join' expr from parent */
  nd = node->parent ;
  i  = get_parent_pos(node);
  nd->op_lst.erase(nd->op_lst.begin()+i);
  /* insert a dummy expr at its 
   *  original position */
  attach(nd,
    __create_binary_nodes(
      mktype(m_expr,s_eql),
      create_node((char*)"1",m_endp,s_col),
      create_node((char*)"1",m_endp,s_col)),
    i);
  /* process the 'join' operation */
  __process_ora_join(ptr,node);
  return /*1*/REDO_HIERARCH;
}

int mysql_adpt::__remove_prefix(stxNode *lst,int pos)
{
  stxNode *p = lst->op_lst[pos];

  if (p->op_lst.size()<=0)
    return 0 ;
  /* get terminate leaf of item */
  for (;p && p->op_lst.size()>0;
    p=p->op_lst[0]) ;
  /* detach the leaf */
  p->parent->op_lst.erase(
    p->parent->op_lst.begin());
  /* remove the parent nodes */
  detach(lst->op_lst[pos],pos);
  attach(lst,p,pos);
  return 1;
}

int mysql_adpt::adpt_normal_list(stxNode *node, int pos)
{
  uint16_t i = 0;
  stxNode *p = 0, *nd = 0;

#if 0
  if (node->op_lst.size()!=1) {
    return 0;
  }
#endif
  p = node->parent ;
  if (!((p->type==mktype(m_expr,s_union) ||
     p->type==mktype(m_expr,s_uniona))&&
     pos==0) &&
     mget(p->type)!=m_list && 
     mget(p->type)!=m_root) {
    return 0;
  } 
  if (p->type==mktype(m_list,s_from) ||
     p->type==mktype(m_list,s_sel)) {
    return 0;
  }
  /* detach from top node */
  p->op_lst.erase(p->op_lst.begin()+pos);
  for (i=0;i<node->op_lst.size();i++) {
    nd = node->op_lst[i] ;
    attach(p,nd);
  }
  node->op_lst.clear();
  destroy_node(node);
  return REDO_HIERARCH;
}

int mysql_adpt::adpt_merge_into(stxNode *node, int pos)
{
  uint16_t i = 0/*, n=0*/;
  stxNode *pInst = 0, *p = 0, *nd = 0, *ps = 0,
    *pu = 0;

  /* 
   * create the 'insert' statement 
   */
  pInst = create_node(0,m_stmt,s_insert);
  /* 
   * the insert target 
   */
  p = find_in_tree(node,mktype(m_mia,s_mia_dst));
  if (!p) {
    printd("no 'dst' section found\n");
    return -1;
  }
  attach(pInst,dup_tree(p->op_lst[0]));
  /* 
   * the 'format' list 
   */
  pu = p = find_in_tree(node,mktype(m_mia,
    s_mia_unmatch));
  if (!p) {
    printd("no 'unmatch' section found\n");
    return -1;
  }
  nd = dup_tree(p->op_lst[0]);
  attach(pInst,nd);
  /* remove possible prefixes from
   *  'format' list items */
  for (i=0;i<nd->op_lst.size();i++) {
    __remove_prefix(nd,i);
  }
  /* 
   * create the 'value' section by 
   *  adding a new 'select' statement 
   */
  ps = __create_empty_select_tree(false,true,true);
  /* 'select' list */
  for (i=0;i<nd->op_lst.size();i++) {
    attach(ps->op_lst[0], 
      dup_tree(nd->op_lst[i]));
  }
  p = find_in_tree(node,mktype(m_mia,s_mia_src));
  if (!p) {
    printd("no 'src' section found\n");
    return -1;
  }
  /* 'from' list */
  attach(ps->op_lst[1],dup_tree(
    p->op_lst[0]));
  /* also attach alias */
  if (p->op_lst.size()>1) {
    attach(ps->op_lst[1],dup_tree(
      p->op_lst[1]));
  }
  /* 'where' list */
  attach(ps->op_lst[2],dup_tree(
    pu->op_lst[2]->op_lst[0]));
  attach(pInst,ps);
  /* 
   * the 'on duplicate key' section 
   */
  p  = find_in_tree(node,mktype(m_mia,
    s_mia_match));
  if (!p) {
    printd("no 'match' section found\n");
    return -1;
  }
  nd = dup_tree(p->op_lst[0]) ;
  sset(nd->type,s_odku);
  attach(pInst,nd);
  /* remove possible prefixes of 'update' 
   *  list items */
  for (i=0;i<nd->op_lst.size();i++) {
    /* the 'a=b' list item  */
    p = nd->op_lst[i] ;
    __remove_prefix(p,0);
  }
  /* XXX: test */
  p = node->parent ;
  /* replace 'merge' statement with new 
   *  'insert' statement */
  detach(node,pos);
  attach(/*node->parent*/p,pInst,pos);
  return REDO_HIERARCH;
}

/* adapts with oracl 'start with'/'connect by' */
int mysql_adpt::adpt_hierarchal(stxNode *node,int pos)
{
  uint16_t i = 0, frm_pos = 0;
  stxNode *p = 0, *pw = 0, 
   *psw = 0, *pcb = node,
   *p1 = 0, *p2 = 0; 

  /* find 'where'/'start with'/'connect by'
   *  clouses */
  for (p=node->parent,i=0;i<p->op_lst.size();
      i++) {
    if (p->op_lst[i]->type==mktype(
       m_list,s_where)) {
      /* found 'where' */
      pw = p->op_lst[i]  ;
    } else if (p->op_lst[i]->type==mktype(
       m_list,s_sw)) {
      /* found 'start with' */
      psw = p->op_lst[i]  ;
    }
    /* XXX: 'connect by' is the 
     *  current node */
    else if (p->op_lst[i]->type==mktype(
       m_list,s_from)) {
      /* find position of 'from' clouse in statement */
      frm_pos = i ;
    }
  }
  /* get parent */
  p = node->parent ;
  /* split the 'start with' tree */
  if (psw) {
    __split_sw_tree(psw,&p1,&p2);
    /* detach the 'start with' node */
    i = get_parent_pos(psw);
    detach(psw,i);
  }
  /* detach the 'connect by' tree */
  if (pcb) {
    i = get_parent_pos(pcb);
    p->op_lst.erase(p->op_lst.begin()+i);
  }
  /* if no 'where' list, then create one */
  if (!pw) {
    pw = create_node(0,m_list,s_where);
    /* attach new where clouse to behind the 'from' list */
    attach(p,pw,frm_pos>0?(frm_pos+1):-1);
  }
  /* join the 'where' tree with the sub trees
   *  above */
  __join_where_clouse(pw,pcb,p1,p2);
  destroy_tree(pcb);
  return REDO_HIERARCH;
  //return 1;
}

int mysql_adpt::adpt_place_holder(stxNode *node,int pos)
{
  /* 
   * only work-able in prepare mode
   */
  if (bPrep) {
    /* replace the name domain with '?' */
    strcpy(node->name,"?");
    destroy_tree(node,false);
    node->type = mktype(m_endp,s_col);
  }
  /* XXX: the query mode is processed within
   *  serialize_ph() function */
  return 1;
}

int mysql_adpt::adpt_union(stxNode *node,int pos)
{
  stxNode *prt = node->parent, *tmp = 0;

  /* if this union* expression is the 
   *  toppest of the whole statement, then
   *  no need to add '()' */
  if (prt->type==mktype(m_list,s_norm) ||
     mget(prt->type)==m_root) {
    return 1;
  }
  /* remove 'union*' node from parent */ 
  prt->op_lst.erase(prt->op_lst.begin()+pos);
  /* create '()' node under parent node */
  tmp = create_node(0,m_list,s_norm);
  attach(prt,tmp,pos);
  attach(tmp,node);
#if 0
  /* add alias for the expr if in 
   *  'from' list */
  for (tmp=prt;tmp && tmp->type!=mktype(m_list,s_from);
    tmp=tmp->parent) ;
  /* dont add alias if it already has one */
  if (0&&tmp && prt->op_lst[pos+1]->type
     !=mktype(m_endp,s_alias)) {
    tmp = create_node(0,m_endp,s_alias);
    sprintf(tmp->name,"t%lld",(uint64_t)node);
    attach(prt,tmp,pos+1);
  }
#endif
  return 1;
}

/* adpation with oracle 'limit' */
int mysql_adpt::adpt_limit(stxNode *node,int pos)
{
  bool bFu = false ; /* has 'for update' in statement */
  int last = 0;
  stxNode *prt = node->parent;

  if (!prt) {
    return 0;
  }
  last = (int)prt->op_lst.size()-1;
  /* don't do adaption if 'limit' sub statement
   *  is at lastest position of parent statement
   *  or stays next to last and the last is the
   *  'for update' keyword */
  bFu = prt->op_lst[last]->type==mktype(m_keyw,s_fu);
  if ((pos==last) || ((pos==last-1) && bFu)) {
    return 1;
  }
  /* move 'limit' sub statement to last 
   *  of parent statement */
  prt->op_lst.erase(prt->op_lst.begin()+pos);
  /* if 'for update' is at last of statement,then put
   *  'limit' to before it, otherwise, bring it to 
   *  the end */
  attach(prt,node,bFu?last-1:last);
  return 1;
  //return REDO_HIERARCH;
}

/* adpation with oracle 'for update wait/nowait/of' */
int mysql_adpt::adpt_4update_attr(stxNode *node,int pos)
{
  stxNode *prt = node->parent/*, *nd = 0*/;

  if (!prt) {
    return 0;
  }
  /* remove all attribute sub nodes */
  if (node->op_lst.size()>0) {
    destroy_tree(node,false);
  }
  /* 'for update' sub node shold be
   *  at last of statement*/
  if (pos==(int)(prt->op_lst.size()-1)) {
    return 1;
  }
  /* move to end of statement */
  prt->op_lst.erase(prt->op_lst.begin()+pos);
  attach(prt,node);
  return REDO_HIERARCH;
}

int mysql_adpt::adpt_dummy(stxNode *node,int pos)
{
  stxNode *pstmt = 0, *nd = 0;

  if (!node->parent) {
    return 0;
  }
  /* check if statement is in the form like: 
   *  select dummy frm dual */
  if (node->parent->type!=mktype(m_list,s_sel)) {
    return 0;
  }
  if (!(pstmt=node->parent->parent)) {
    return 0;
  }
  if (pstmt->op_lst[1]->type!=mktype(m_list,s_from) ||
     strcasecmp(pstmt->op_lst[1]->op_lst[0]->name,"dual")) {
    return 0;
  }
  /* add alias for the node */
  strcpy(node->name,"'x'");
  nd = create_node((char*)"_dummy",m_endp,s_alias);
  attach(node,nd);
  return 1;
}

int mysql_adpt::__date_fmt_o2m(char *str)
{
  using fpair = struct pfPair {
    char match[32]; /* pattern to be matched */
    char toRepl[32]; /* to be replaced with */
  } ;
  fpair rp_list[] = {
    {"dd", "%d"},
    {"mm", "%m"},
    {"yyyy", "%Y"},
    {"mi", "%i"},
    {"ss", "%s"},
    {"hh24", "%H"},
    /* TODO: more format translations here */
  } ; 
  std::string::size_type pos = 0;
  uint16_t ln = 0, i=0;
  std::string tmp = str;
  fpair *rp = 0;

  for (i=0; i<sizeof(rp_list)/sizeof(rp_list[0]);i++) {
    rp = &rp_list[i] ;
    ln = strlen(rp->match);
    for (pos=0;(pos=tmp.find(rp->match,pos))
       !=std::string::npos;) {
      tmp.replace(pos,ln,rp->toRepl);
    }
  }
  strcpy(str,tmp.c_str());
  return 1;
}

/* adapts with oracle object 'dbms_lob' */
int mysql_adpt::adpt_dbms_lob(stxNode *node,int pos)
{
  stxNode *prt = node->parent, *nd = 0 ;

  /* detach from parent */
  prt->op_lst.erase(prt->op_lst.begin()+pos);
  nd = node->op_lst[0] ;
  node->op_lst.erase(node->op_lst.begin());
  /* move chil to parent */
  attach(prt,nd,pos);
  destroy_node(node);
  return REDO_HIERARCH;
}

int mysql_adpt::adpt_substr(stxNode *node,int p)
{
  int nArg = 0;
  stxNode *nd = 0;

  nArg = node->op_lst[0]->op_lst.size();
  if (nArg<1 || nArg>3) {
    return 0;
  }
  /* replace function name with 'substring' */
  strcpy(node->name,"substring");
  switch (nArg) {
    /* function call has 2 parameters, 
     *  do nothing here */
    case 2: 
      return 1;
    /* add length control parameter if 
     *  only 1 parameter is provided */
    case 1:
      nd = create_node((char*)"1",m_endp,s_c_int);
      attach(node->op_lst[0],nd);
      return 1;
    /* exchange the pos and length parameter
     *  if 3 parameters are provided */
    case 3:
      nd = node->op_lst[0]->op_lst[2] ;
      node->op_lst[0]->op_lst.erase(
        node->op_lst[0]->op_lst.begin()+2);
      attach(node->op_lst[0],nd,1);
      return 1;
  }
  return 0;
}

int mysql_adpt::adpt_relike(stxNode *node,int p)
{
  stxNode *nd = 0, *tmp = 0;

  /* don't do adaption on non-function endpoints */
  if (node->type!=mktype(m_endp,s_func)) {
    return 1;
  }
  nd = node->op_lst[0] ;
  if (nd->op_lst.size()<2 || nd->op_lst.size()>3) {
    return -1;
  }
  /* change node type: m_endp:s_func -> m_expr:s_rexp */
  node->type = mktype(m_expr,s_rexp);
  /* move the original 'argument list' out */
  node->op_lst.erase(node->op_lst.begin());
  /* move the first 2 arguments to under 'node' */
  tmp = nd->op_lst[0] ;
  nd->op_lst.erase(nd->op_lst.begin());
  attach(node,tmp);
  tmp = nd->op_lst[0] ;
  nd->op_lst.erase(nd->op_lst.begin());
  attach(node,tmp);
  /* check the controling argument, if exists */
  if (nd->op_lst.size()>0) {
    /* 'i' : ignore case */
    if (strchr(nd->op_lst[0]->name,'i')) 
      sset(node->type,s_rexpb);
  }
  /* release the original 'argument list' */
  destroy_tree(nd);
  return 0;
}

int mysql_adpt::adpt_to_num(stxNode *node,int pos)
{
  int nArg = 0, val = 0;
  stxNode *nd = 0, *nd1 = 0, *tmp = 0;
  char *p = 0;

  if (!node->op_lst[0]) {
    return 0;
  }
  /* don't do adaption on non-function endpoints */
  if (node->type!=mktype(m_endp,s_func)) {
    return 1;
  }
  nArg = node->op_lst[0]->op_lst.size();
  if (nArg<1 || nArg>2) {
    return 0;
  }
  /* replace function name with 'cast' */
  strcpy(node->name,"cast");
  /* find terminating child leaft */
  for (nd = node->op_lst[0];
    nd->op_lst.size()>0;
    nd = nd->op_lst[0]) ;
  /* add tailing descriptors for argument 1 */
  tmp = create_node(0,m_endp,s_alias);
  strcpy(tmp->name, 
    (nd->type==mktype(m_endp,s_c_str)&&
    (char*)strchr(nd->name,'.')?
    " as decimal(20,6)":
    " as signed integer"));
  /* attach to 'cast' node */
  attach(node->op_lst[0],tmp) ;
  /* no more arguments */
  if (nArg==1) {
    return 1;
  }
  nd1= node->op_lst[0]->op_lst[1];
  /* process the format control argument 2 */
  if (nd->type==mktype(m_endp,s_c_str) && 
     nd1->type==mktype(m_endp,s_c_str) && 
     strcasestr(nd1->name,"x")) {
    /* translate from hex digit */
    sscanf(nd->name,"'%x'",&val);
    sprintf(nd->name,"'%d'",val);
  }
  /* yzhou added 2016.8.5: convert to double type */
  if (nd->type==mktype(m_endp,s_col) && 
     (p=strstr(nd1->name,"."))) {
    /* calculate float digit count */
    val = nd1->name+(strlen(nd1->name)-1)-(p+1) ;
    val = !val?1:val ;
    sprintf(node->op_lst[0]->op_lst[2]->name,
      "as decimal(20,%d)",val);
  }
  /* erase the format control argument */
  nd = node->op_lst[0] ;
  detach(nd->op_lst[1],1); 
  return 1;
}

int mysql_adpt::adpt_tochar(stxNode *node,int pos)
{
  int nArg = 0;
  stxNode *nd = 0;

  if (!node->op_lst[0]) {
    return 0;
  }
  /* don't do adaption on non-function endpoints */
  if (node->type!=mktype(m_endp,s_func)) {
    return 1;
  }
  /* XXX: replace function name 'to_char' with 'cast' when
   *  passing with only 1 single argument, otherwise replace
   *  with 'date_format' */
  nArg = node->op_lst[0]->op_lst.size();
  if (nArg==1) {
    strcpy(node->name,"cast");
    /* add format control argument if default 
     *  translation is used in 'trunc' */
    nd = create_node(0,m_endp,s_alias);
    strcpy(nd->name,"as char");
    attach(node->op_lst[0],nd);
    return 1;
  } else if (nArg==2) {
    strcpy(node->name,"date_format");
    /* do format translation on 
     *  exsiting arguments */
    __date_fmt_o2m(node->op_lst[0]
      ->op_lst[1]->name);
    return 1;
  }
  return 0;
}

int mysql_adpt::adpt_trunc(stxNode *node,int pos)
{
  int nArg = 0;
  stxNode *nd = 0;

  if (!node->op_lst[0]) {
    return 0;
  }
  /* don't do adaption on non-function endpoints */
  if (node->type!=mktype(m_endp,s_func)) {
    return 1;
  }
  /* XXX: replace function name 'trunc' with 'date_format',
   *  I assume the 1st argument to be date type, but
   *  I CANT judge argument's type if it's a variable */
  strcpy(node->name,"date_format");
  nArg = node->op_lst[0]->op_lst.size();
  if (nArg==1) {
    /* add format control argument if default 
     *  translation is used in 'trunc' */
    nd = create_node(0,m_endp,s_c_str);
    strcpy(nd->name,"'%y-%m-%d'");
    attach(node->op_lst[0],nd);
    return 1;
  } else if (nArg==2) {
    /* do format translation on 
     *  exsiting arguments */
    __date_fmt_o2m(node->op_lst[0]
      ->op_lst[1]->name);
    return 1;
  }
  return 0;
}

int mysql_adpt::adpt_sysdate(stxNode *node,int pos)
{
  stxNode *nd = 0;

  if (node->type!=mktype(m_endp,s_func)) {
    mset(node->type,m_endp);
    sset(node->type,s_func);
    nd = create_node(0,m_list,s_arg);
    attach(node,nd);
  }
  return 1;
}

int mysql_adpt::adpt_peql(stxNode *node,int pos)
{
  stxNode *n1 = 0, *nn = 0, *n0 = 0;

  /* dup the right operand of operator '+=' */
  n0 = /*dup_node*/dup_tree(node->op_lst[0]);
  /* detach left operand of operator '+=' */
  n1 = node->op_lst[1] ;
  node->op_lst.erase(node->op_lst.begin()+1);
  /* create a new '+' expr node */
  nn = create_node(0,m_expr,s_plus);
  attach(nn,n0);
  attach(nn,n1);
  /* attach to parent node */
  attach(node,nn);
  /* modify op type from '+=' to '=' */
  sset(node->type,s_eql);
  return 1;
}

int mysql_adpt::__add_rn_inc(stxNode *node,char *rn)
{
  stxNode *tmp = 0, *p = 0;

  /* change current node from 'rownum' to
   *  '@x:=@x+1' */
  node->type = mktype(m_expr,s_eql1);
  tmp = create_node(0,m_expr,s_plus);
  p = create_node(rn,m_endp,s_c_str);
  attach(tmp,p);
  p = create_node((char*)"1",m_endp,s_c_str);
  attach(tmp,p);
  attach(node,tmp);
  tmp = create_node(rn,m_endp,s_col);
  attach(node,tmp,0);
  return 1;
}

/* remove column prefix like 'schema' 'table' */
int mysql_adpt::__rm_col_prefix(stxNode *node)
{
  uint16_t i = 0, ps = 0;
  stxNode *p = 0, *prt = 0;

  if (!node) {
    return 0;
  }
  /* match node type with 'table' or 'schema' */
  if ((node->type==mktype(m_endp,s_tbl) ||
     node->type==mktype(m_endp,s_schema)) &&
     node->op_lst.size()==1) {
    p  = node->op_lst[0] ;
    /* detach the child node */
    node->op_lst.erase(node->op_lst.begin());
    /* remove current node item */
    ps = get_parent_pos(node);
    prt= node->parent ;
    detach(node,ps);
    /* attach the child above to parent */
    attach(prt,p,ps);
  }
  for (i=0;i<node->op_lst.size();i++) {
    __rm_col_prefix(node->op_lst[i]);
  }
  return 1;
}

int mysql_adpt::__add_col_def(stxNode *olst, stxNode *node)
{
  uint16_t i = 0;
  stxNode *p = 0;

  if (!node) {
    return 0;
  }
  if ((node->type==mktype(m_endp,s_tbl) ||
     node->type==mktype(m_endp,s_col) ||
     node->type==mktype(m_endp,s_schema)) &&
     node->op_lst.size()==1) {
    /* find column definitions */
    for (i=0;i<olst->op_lst.size();i++) {
      if (!cmp_tree(node,olst->op_lst[i]))
        break ;
    }
    /* if no definition found, then add 
     *  one to orig list */
    if (i>=olst->op_lst.size()) {
      p = dup_tree(node);
      /* add to original list */
      attach(olst,p);
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    __add_col_def(olst,node->op_lst[i]);
  }
  return 1;
}

int mysql_adpt::__mod_node_alias(stxNode *olst,
  stxNode *node)
{
  uint16_t i = 0, sp = 0;
  stxNode *pa = 0, *prt = 0;

  if (!node) {
    return 0;
  }
  /* try to replace specified nodes with alias */
  if (mget(node->type)==m_endp &&
     sget(node->type)>=s_tbl && 
     sget(node->type)<=s_col) {
    /* find column definitions and alias */
    for (i=1;i<olst->op_lst.size();i++) {
      if (!cmp_tree(node,olst->op_lst[i-1]) &&
         olst->op_lst[i]->type==mktype(
         m_endp,s_alias))
        break ;
    } 
    /* found definition and do replacement */
    if (i<olst->op_lst.size()) {
      prt= node->parent ;
      sp = get_parent_pos(node);
      /* remove this sub tree */
      detach(node,sp);
      /* if the node removed has no alias, 
       *  just add one */
      if (sp>=prt->op_lst.size() || prt->op_lst[sp]->type!=mktype(
         m_endp,s_alias)) {
        /* attach alias to original list */
        pa = dup_tree(olst->op_lst[i]);
        attach(prt,pa,sp);
      }
      /* make the alias node to be 'column' */
      prt->op_lst[sp]->type = 
        mktype(m_endp,s_col);
      /* end recursion */
      return 1;
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    __mod_node_alias(olst,node->op_lst[i]);
  }
  return 1;
}

/* remove duplicate columns in select list */
int mysql_adpt::__rm_dup_cols(stxNode *lst)
{
  uint16_t i = 0, n = 0;
  stxNode *nd = 0, *node = 0;

  for (i=0;i<lst->op_lst.size();i++) {
    nd = lst->op_lst[i] ;
    for (n=0;n<lst->op_lst.size();n++) {
      node = lst->op_lst[n] ;
      /* sub tree compare  */
      if (node!=nd && !cmp_tree(nd,node)) {
        detach(node,n);
        n-- ;
      }
    } /* end for (n=0) */
  }
  return 1;
}

int mysql_adpt::__process_list_items(stxNode *orig_lst,
  stxNode *lst)
{
  /* remove/replace list items with alias 
   *  if they have */
  __mod_node_alias(orig_lst,lst);
  /* insert column definition to orig list
   *  if not exists*/
  __add_col_def(orig_lst,lst);
  /* remove 'table'/'schema' type nodes of
   *   items */
  __rm_col_prefix(lst);
  /* remove duplicate columns from 
   *  select list */
  __rm_dup_cols(lst);
  return 1;
}

int mysql_adpt::__add_rn_wrapper(stxNode *pstmt,char *rn)
{
  uint16_t i = 0;
  stxNode *plist = 0, *tmp = 0, *prt = 0,
    *p = 0, *plist2 = 0, *prna = 0;

  /* search 'select' list in parent statement */
  for (i=0;i<pstmt->op_lst.size();i++) {
    plist = pstmt->op_lst[i];
    if (plist->type==mktype(m_list,s_sel)) 
      break ;
  }
  /* TODO: add assertion for case 
   *  without 'select' list  */
  /* backup the list */
  plist2 = dup_tree(plist);
  /* do serval processings on backup list */
  __process_list_items(plist,plist2);
  /* create '@x+1' node */
  p = __create_binary_nodes(
    mktype(m_expr,s_plus),
    create_node(rn,m_endp,s_c_str),
    create_node((char*)"1",m_endp,s_c_str)
    );
  /* create the '@x:=@x+1' node */
  p = __create_binary_nodes(
    mktype(m_expr,s_eql1),
    create_node(rn,m_endp,s_c_str), p
    );
  /* attach the node to 'select' list */
  attach(plist,p);
  /* change attribute of wildcast '*' to
   *  normal endpoint, for statement 
   *  output reasons */
  if (plist->op_lst[0]->type==
     mktype(m_keyw,s_wildcast)) 
  {
    strcpy(plist->op_lst[0]->name,"*");
    plist->op_lst[i]->type=mktype(m_endp,
      s_col);
  }
  /* detach the original 'select' statement 
   *  from its parent */
  prt = p = pstmt->parent ;
  i = get_parent_pos(pstmt);
  /* also delete related alias if exists */
  if (i<(p->op_lst.size()-1) && 
     p->op_lst[i+1]->type==mktype(m_endp,s_alias)) {
    /* backup the alias */
    prna = p->op_lst[i+1];
    /* remove it from parent */
    p->op_lst.erase(p->op_lst.begin()+i+1);
  }
  p->op_lst.erase(p->op_lst.begin()+i);
  /* create simple 'select * from xx' 
   *  wrapper statement */
  tmp = __create_empty_select_tree(false,true,false);
  attach(tmp->op_lst[0],plist2);
  /* attach this simple statement to parent */
  attach(prt,tmp);
  /* also attach alias for the new statement */
  attach(prt,prna);
  /* join the original statement above 
   *  to 'from' list of new statement */
  attach(tmp->op_lst[1],pstmt);
  /* also add alias */
  p = create_node(0,m_endp,s_alias);
  /* skip '@' */
  sprintf(p->name,"%s_wrapper",rn+1);
  attach(tmp->op_lst[1],p);
  return 1;
}

int mysql_adpt::__add_rn_init(stxNode *pstmt, char *rn)
{
  uint16_t i = 0;
  stxNode *plist = 0, *tmp = 0, *p;

  /* search 'from' list in parent statement */
  for (i=0;i<pstmt->op_lst.size();i++) {
    plist = pstmt->op_lst[i];
    if (plist->type==mktype(m_list,s_from)) 
      break ;
  }
  /* XXX: error */
  if (!plist) {
    printd("fatal: no from list found\n");
    return -1;
  }
  /* create the '@x:=-1' node */
  tmp = create_node(0,m_expr,s_eql1);
  p = create_node(rn,m_endp,s_c_str);
  attach(tmp,p);
  /* yzhou modified: 2015.12.25 */
  //p = create_node((char*)"-1",m_endp,s_c_str);
  p = create_node((char*)"0",m_endp,s_c_str);
  attach(tmp,p);
  p = tmp ;
  /* create the 'select @x:=-1' node */
  tmp = create_node(0,m_list,s_sel);
  attach(tmp,p);
  p = tmp ;
  tmp = create_node(0,m_stmt,s_select);
  attach(tmp,p);
  p = tmp ;
  /* add a '()' pair for the new statement */
  tmp = create_node(0,m_list,s_norm);
  attach(tmp,p);
  /* attach to 'from' list */
  attach(plist,tmp);
  /* add an alias for it */
  tmp = create_node(0,m_endp,s_alias);
  /* skip '@' */
  sprintf(tmp->name,"%s_init",rn+1);
  attach(plist,tmp);
  return 1;
}

inline int mysql_adpt::__add_rn_limit(
  stxNode *expr,
  stxNode *node,
  char *rn)
{
  uint16_t i = 0;
  stxNode *p = expr->parent, *p_op = 0, *pstmt= 0 ;

  if (!p) {
    return 0;
  }
  /* find parent statement */
  for (;p && mget(p->type)!=m_stmt;
    p=p->parent) ;
  if (!p) {
    printd("fatal: parent statement not found\n");
    return -1;
  }
  pstmt = p ;
  /* find the other operand node in 
   *  binary expression */
  for (i=0;i<expr->op_lst.size();i++) {
    p_op = expr->op_lst[i] ;
    if (p_op!=node)
      break ;
  }
  if (i>=expr->op_lst.size()) {
    printd("fatal: operand node not found\n");
    return -1;
  }
  /* move operand node out */
  expr->op_lst.erase(expr->op_lst.begin()+i);
  /* modify the expr node to '1=1' node */
  expr->type = mktype(m_expr,s_eql);
  strcpy(node->name,"1");
  p = create_node((char*)"1",m_endp,s_c_str);
  attach(expr,p);
  /* add a 'limit' node with the move-outed
   *  operand at statement ends */
  p = create_node(0,m_list,s_limit);
  attach(p,p_op);
  attach(pstmt,p);
  return 1;
}

int mysql_adpt::__process_normal_rn(stxNode *node, 
  char *rn)
{
  uint16_t i = 0;
  stxNode *nd = 0;

  if (!node) {
    return 0;
  }
  /* don't process any other scopes that dose 
   *  not belong to current statement */
  if (mget(node->type)==m_stmt) {
    return 1;
  }
  if (!strcasecmp(node->name,"rownum")) {
    /* test if this 'rownum' node locates
     *  under a '<=' or '<' expression */
    for (nd=node->parent;nd &&
      (nd->type==mktype(m_list,s_norm) ||
      mget(nd->type)==m_endp);
      nd=nd->parent);
    /* it's the case that 'rownum<xx' */
    if (nd && mget(nd->type)==m_expr &&
       (sget(nd->type)==s_lt ||
       sget(nd->type)==s_le)) {
      /* modify 'rownum<xx' to '1=1' and add
       *  limit condition */
      __add_rn_limit(nd,node,rn);
    } else {
      /* replace 'rownum' with @xx */
      strcpy(node->name,rn);
    }
  }
  /* search into children list */
  for (i=0;i<node->op_lst.size();i++) {
    __process_normal_rn(node->op_lst[i],rn);
  }
  return 1;
}

int mysql_adpt::adpt_rownum(stxNode *node,int pos)
{
  uint16_t i = 0;
  char rn_str[TKNLEN] ;
  stxNode *pstmt = 0, *p = 0;
  bool bSel = false ;

  /* get parent statement that contains this
   *  'rownum' node */
  for (pstmt=node->parent;pstmt;pstmt=pstmt->parent) {
    if (mget(pstmt->type)==m_stmt) 
      break ;
  }
  /* try to find out if current 'rownum' item
   *  locates within the 'select' list or not */
  for (p=node->parent;p && 
     mget(p->type)!=m_stmt;p=p->parent) {
    if (p->type==mktype(m_list,s_sel)) {
      bSel = true ;
      break ;
    }
  }
  /* generate the 'rn' name with random digits,
   *  for distinguishing with other 'rownum' 
   *  in other statement scopes */
  sprintf(rn_str,"@rn_%lld",((long long)node)&0xfff);
  /* the first 'rownum' node is under the 
   *  'select' list, so add a 'counter inc'*/
  if (bSel) {
    /* modify the rownum node to a 
     *  counter increment statement
     */
    __add_rn_inc(node,rn_str);
  } else {
    /* there's no rownum within 'select' 
     *  list, add the 'select' wrapper
     *  upon current parent statement */
    __add_rn_wrapper(pstmt,rn_str);
  }
  /* add a counter init statement in 
   *  'from' list */
  __add_rn_init(pstmt,rn_str);
  /* do adaptions on other 'rownum' s within 
   *  current statement scope */
  for (i=0;i<pstmt->op_lst.size();i++)
    __process_normal_rn(pstmt->op_lst[i],rn_str);
  return /*1*/REDO_HIERARCH;
}

int mysql_adpt::adpt_add_quote(stxNode *node,int pos)
{
  char buf[TKNLEN]="";

  /* add quote pair onto to mysql keyword */ 
  strcpy(buf,node->name);
  sprintf(node->name,"`%s`",buf);
  return 1;
}

int mysql_adpt::adpt_nvl(stxNode *node,int pos)
{
  /* it shold be a function call to 
   *  oracle 'nvl' function */
  if (node->type!=mktype(m_endp,s_func)) {
    return 0;
  }
  /* replace function name 'nvl' with
   *  mysql-style 'ifnull' */ 
  strcpy(node->name,"ifnull");
  return 1;
}

int mysql_adpt::adpt_currval(stxNode *node,int pos)
{
  stxNode *prt = node->parent ;

  if (!prt) {
    return 0;
  }
  /* attach '_currval' string to parent's name */
  strcat(prt->name,"_currval");
  /* modify parent type */
  prt->type = mktype(m_endp,s_func);
  /* modify current node type */
  node->type= mktype(m_list,s_arg);
  return 1;
}

int mysql_adpt::adpt_nextval(stxNode *node,int pos)
{
  stxNode *prt = node->parent ;

  if (!prt) {
    return 0;
  }
  /* attach '_nextval' string to parent's name */
  strcat(prt->name,"_nextval");
  /* modify parent type */
  prt->type = mktype(m_endp,s_func);
  /* modify current node type */
  node->type= mktype(m_list,s_arg);
  return 1;
}

int mysql_adpt::adpt_expr_in(stxNode *node, int pos)
{
  stxNode *nd = 0, *tmp = 0;

  /* check if there's '()' pair
   *  at 2nd operand of 'in' operator */
  if (node->op_lst[1]->type==mktype(m_list,s_norm)) {
    return 1;
  }
  /* detach the 1st operand */
  tmp = node->op_lst[1] ;
  node->op_lst.erase(node->op_lst.begin()+1);
  /* insert a 'normal list' node */
  nd = create_node(0,m_list,s_norm);
  attach(node,nd);
  attach(nd,tmp);
  return 1;
}

int mysql_adpt::adpt_insert(stxNode *node, int pos)
{
  /* remove alias name of 'insert' target
   *  if exists */
  if (node->op_lst[1]->type!=
     mktype(m_endp,s_alias)) {
    return 1;
  }
  destroy_node(node->op_lst[1]);
  node->op_lst.erase(node->op_lst.begin()+1);
  return 1;
}

inline int mysql_adpt::__add_subqry_wrapper(
  stxNode *node,
  std::vector<arg_pair> &lst,
  int pos)
{
  stxNode *prt = 0, *tmp = 0, *nd = 0;
  uint16_t i=0;

  /* try if sub query statement contains
   *  table names in lst, if so, add subquery
   *  onto this sub query */
  if (node->type==mktype(m_stmt,s_select)) {
    prt = node->parent ;
    for (i=0;i<lst.size();i++) {
      /* found 'from' list of the sub query 
       *  using the table names */
      if (find_in_tree(node->op_lst[1],
         (char*)lst[i].first.c_str())) 
        break ;
    }
    /* add a 'select from' statement 
     *  onto sub query node */
    if (i<lst.size()) {
      prt->op_lst.erase(prt->op_lst.begin()+pos);
      /* create an 'empty' select statement */
      nd = __create_empty_select_tree(true,true,false);
      attach(prt,nd,pos);
      tmp = create_node(0,m_list,s_norm);
      attach(nd->op_lst[1],tmp);
      attach(tmp,node);
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    __add_subqry_wrapper(node->op_lst[i],lst,i);
  }
  return 1;
}

inline int mysql_adpt::__replace_alias(stxNode *node,
  std::vector<arg_pair> &lst)
{
  uint16_t i=0;

  if (mget(node->type)==m_endp) {
    for (i=0;i<lst.size();i++) {
      if (lst[i].second==node->name) 
        strcpy(node->name,lst[i].first.c_str());
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    __replace_alias(node->op_lst[i],lst);
  }
  return 1;
}

int mysql_adpt::adpt_delete(stxNode *node, int pos)
{
  std::vector<arg_pair> n_list ;
  uint16_t i=0, np = 0 ;
  stxNode *nd = 0, *nw = 0, *ptr = 0;

  /* find 'from' sub node */
  for (;i<node->op_lst.size();i++) {
    if (node->op_lst[i]->type==
       mktype(m_list,s_from)) {
      nd = node->op_lst[i] ;
      np = i ;
      break ;
    }
  }
  if (!nd) {
    return 1;
  }
  /* remove alias name of 'from' target
   *  if exists */
  for (i=0;i<nd->op_lst.size();i++) {
    if (nd->op_lst[i]->type==
       mktype(m_endp,s_alias)) {
      /* get terminate leaf of node */
      for (ptr=nd->op_lst[i-1];
        ptr->op_lst.size()>0;
        ptr=ptr->op_lst[0]);
      /* save node and alias name */
      n_list.push_back(arg_pair(ptr->name,
        nd->op_lst[i]->name));
      /* remove the alias node */
      detach(nd->op_lst[i],i);
    }
  }
  /* add exclusive 'using' list if multiple 
   *  targets are deleted */
  if (nd->op_lst.size()>1 && 
     !find_in_tree(node,mktype(m_list,s_using))) {
    nd = dup_tree(nd);
    sset(nd->type,s_using);
    attach(node,nd,np+1);
  }
  /* find 'where' node */
  for (i=0;i<node->op_lst.size();i++) {
    if (node->op_lst[i]->type==
       mktype(m_list,s_where)) {
      nw = node->op_lst[i] ;
      break ;
    }
  }
  if (!nw) {
    return 1;
  }
  /* replace deleted alias name 
   *  with node name */
  __replace_alias(nw,n_list);
  /* add wrapper onto sub queries in 'where' node */
  __add_subqry_wrapper(nw,n_list);
  return 1;
}

int mysql_adpt::adpt_update(stxNode *node, int pos)
{
  std::vector<arg_pair> n_list ;
  uint16_t i=0 ;
  stxNode *nd = 0, *nw = 0, *ptr = 0;

  /* find 'from' sub node */
  for (;i<node->op_lst.size();i++) {
    if (node->op_lst[i]->type==
       mktype(m_list,s_upd)) {
      nd = node->op_lst[i] ;
      break ;
    }
  }
  if (!nd) {
    return 1;
  }
  for (i=0;i<nd->op_lst.size();i++) {
    /* get terminate leaf of node */
    for (ptr=nd->op_lst[i];ptr->op_lst.size()>0;
      ptr=ptr->op_lst[0]);
    /* save node name(table name) */
    n_list.push_back(arg_pair(ptr->name,
      ""));
  }
  /* find 'where' node */
  for (i=0;i<node->op_lst.size();i++) {
    if (node->op_lst[i]->type==
       mktype(m_list,s_where)) {
      nw = node->op_lst[i] ;
      break ;
    }
  }
  if (!nw) {
    return 1;
  }
  /* add wrapper onto sub queries in 'where' node */
  __add_subqry_wrapper(nw,n_list);
  return 1;
}

inline int mysql_adpt::__add_alias(stxNode *node)
{
  uint16_t i = 0;
  stxNode *ptr = 0, *nd = 0, *tmp = 0;
  
  /* check if sub query should be 
   *  added alias */
  for (nd=node,ptr=node->parent;ptr;
     nd=ptr,ptr=ptr->parent) {
    /* parent node should be of 'list' type 
     *  but not '()' */
    if (mget(ptr->type)==m_list && 
       sget(ptr->type)!=s_norm &&
       /* nd is of type '()' or is 'node' itself  */
       (nd->type==mktype(m_list,s_norm) ||
        nd==node)) 
      break ;
  }
  if (!ptr) {
    return 1;
  }
  /* get position of targeting node 
   *  in parent's list */
  if ((i=get_parent_pos(nd))<0) {
    return 0;
  }
  /* find and try adding alias if not exist */
  if (i==(ptr->op_lst.size()-1) ||
     ptr->op_lst[i+1]->type!=mktype(m_endp,s_alias)) {
    tmp = create_node(0,m_endp,s_alias);
    /* get memory address of variable 'i'
     *  as random number */
    sprintf(tmp->name,"t%u",
      (*(uint32_t*)&i)&0xffffff);
    attach(ptr,tmp,i+1);
  }
  return 1;
}

int mysql_adpt::__add_norm_list(stxNode *node)
{
  uint16_t i = 0;
  stxNode *ptr = 0, *tmp = 0;

  if (!node->parent) {
    return 0;
  }
  if (node->parent->type==mktype(m_list,s_norm)) {
    return 1;
  }
  /* check if statement is the toppest, if so,
   *  no need to add '()' pair */
  for (ptr=node->parent;
    ptr&&mget(ptr->type)!=m_stmt&&
    ptr->type!=mktype(m_expr,s_union)&&
    ptr->type!=mktype(m_expr,s_uniona);
    ptr=ptr->parent) ;
  if (!ptr) {
    return 1;
  }
  /* get position of targeting node 
   *  in parent's list */
  if ((i=get_parent_pos(node))<0) {
    return 0;
  }
  /* don't add '()' when current 'select' 
   *  statement is the 1st operand of 'union*'*/
  ptr = node->parent ;
  if ((ptr->type==mktype(m_expr,s_union) ||
     ptr->type==mktype(m_expr,s_uniona)) && !i) {
    return 1;
  }
  /* remove node at parent's list */
  ptr = node->parent ;
  ptr->op_lst.erase(ptr->op_lst.begin()+i);
  /* create '()' node */
  tmp = create_node(0,m_list,s_norm);
  attach(ptr,tmp,i);
  attach(tmp,node);
  return 1;
}

int mysql_adpt::__spread_wa_item(stxNode *node, stxNode *item)
{
  uint16_t i = 0;
  stxNode *nd = 0;

  if (!node) {
    return 1;
  }
  if (node->type==mktype(m_list,s_from) ||
     node->type==mktype(m_expr,s_ljoin) ||
     node->type==mktype(m_expr,s_rjoin) ||
     node->type==mktype(m_expr,s_ijoin) ||
     node->type==mktype(m_expr,s_cjoin) ||
     node->type==mktype(m_expr,s_join)) {
    for (i=0;i<node->op_lst.size();i++) {
      if (!strcasecmp(node->op_lst[i]->name,
         item->op_lst[0]->name)) 
        break ;
    }
    if (i<node->op_lst.size()) {
      /* if the target node has alias
       *  already, then remove the node */
      if (i<node->op_lst.size()-1 && 
         node->op_lst[i+1]->type==mktype(m_endp,s_alias)) {
        detach(node->op_lst[i],i);
      } else {
        /* modify type of matching item to alias
         *  in 'from' list */
        node->op_lst[i]->type = mktype(m_endp,s_alias);
      }
      /* duplicate the node */
      nd = dup_tree(item->op_lst[1]);
      /* insert into from list */
      attach(node,nd,i);
    }
  }
  for (i=0;i<node->op_lst.size();i++) {
    __spread_wa_item(node->op_lst[i],item);
  }
  return 1;
}

int mysql_adpt::adpt_withas(stxNode *node, int pos)
{
  uint16_t i = 0, n = 0;
  stxNode *nd = 0, *prt = 0;
  char *wname ;
  bool bSpread = false ;

  /* find 'with as' items that dose not
   *  include other 'with as' items */
  for (i=0;i<node->op_lst.size();i++) {
    nd = node->op_lst[i] ;
    if (nd->type!=mktype(m_waa,s_waa_item)) {
      continue ;
    }
    /* find if current item refers other
     *  item's name */
    for (n=0,bSpread=true;
       n<node->op_lst.size();n++) {
      if (nd==node->op_lst[n]) {
        continue ;
      }
      if (node->op_lst[n]->type!=
         mktype(m_waa,s_waa_item)) {
        continue ;
      }
      wname = node->op_lst[n]->op_lst[0]->name ;
      if (find_in_tree(nd,wname)) {
        bSpread = false ;
        break ;
      }
    }
    /* the 'with as' item dose not refer
     *  anyother items */
    if (bSpread) {
      /* try to spread the statement */
      //node->op_lst.erase(node->op_lst.begin()+i);
      __spread_wa_item(root,nd);
      detach(nd,i);
      /* force to rescan the 'with as' node */
      i=-1 ;
    }
  }
  /* find and save the spreaded statement */
  for (i=0;i<node->op_lst.size();i++) {
    nd = node->op_lst[i] ;
    if (nd->type!=mktype(m_waa,s_waa_item))
      break ;
  }
  /* XXX: error */
  if (i>=node->op_lst.size()) {
    return 0;
  }
  node->op_lst.erase(node->op_lst.begin()+i);
  /* remove the 'with as' statement header */
  prt = node->parent ;
  detach(node,pos);
  /* attach to parent of 'with as' statement */
  attach(prt,nd);
  return REDO_HIERARCH;
}

int mysql_adpt::adpt_select(stxNode *node, int pos)
{
  if (!__add_norm_list(node)||
     !__add_alias(node)) {
    return 0;
  }
  return 1;
}


#if TREE_NON_RECURSION==0
int mysql_adpt::do_mysqlization(
  stxNode *node, 
  int pos, 
  tAdptAct *act)
{
  int ret = 0;
  uint16_t i=0;
  adpt_cb f = act->func;

  if (!node) {
    return 0;
  }
  /* do adaption by action type */
  switch (act->sel) {
  case t_type:
    {
      if (act->u.t==node->type) {
        //printd("t: %s\n", sub_type_str(act->u.t));
        ret = (this->*f)(node,pos);
      }
    }
    break ;
  case t_key:
    {
      if (!strcasecmp(act->u.k,node->name))  {
        //printd("t: %s\n", act->u.k);
        ret = (this->*f)(node,pos);
      }
    }
    break ;
  }
  /* iterate children list */
  for (i=0; i<node->op_lst.size();i++) {
    if (do_mysqlization(node->op_lst[i],i,act)
       ==REDO_HIERARCH) {
      /* tell parent to re search the tree */
      ret = REDO_HIERARCH;
      i-- ;
    }
  }
  return ret;
}
#else
int mysql_adpt::do_mysqlization(
  stxNode *node, 
  int pos, 
  tAdptAct *act)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *ptr = node ;
  adpt_cb f = act->func;
  std::stack<int> stk;
  int _np = -1;

  for (;ptr&&(dir||_np>=0);) {
    /*  find place holder from tree */
    if (dir) {
      /* do adaption by action type */
      switch (act->sel) {
      case t_type:
        {
          if (act->u.t==ptr->type) {
            //printd("t: %s\n", sub_type_str(act->u.t));
            if ((this->*f)(ptr,get_parent_pos(ptr))
               ==REDO_HIERARCH)
              return REDO_HIERARCH ;
          }
        }
        break ;
      case t_key:
        {
          if (!strcasecmp(act->u.k,ptr->name))  {
            //printd("t: %s\n", act->u.k);
            if ((this->*f)(ptr,get_parent_pos(ptr))
               ==REDO_HIERARCH)
              return REDO_HIERARCH;
          }
        }
        break ;
      }
    }
    /* try next node */
    _np = _np<((int)ptr->op_lst.size()-1)?
      (_np+1):-1;
    if (_np<0) {
      if (!stk.empty()) {
        _np = stk.top();
        stk.pop();
      }
      ptr = ptr->parent ;
      dir = 0;
    } else {
      ptr = ptr->op_lst[_np];
      stk.push(_np);
      _np = -1;
      dir = 1;
    }
  } /* end for */
  return /*ret*/0;
}
#endif

/* traverse syntax tree , and convert tree 
 *  to mysql-style syntax */
int mysql_adpt::mysqlize_tree(stxNode *node, 
  int pos)
{
  uint16_t i=0;
  tAdptAct *act = 0;

  if (!node) {
    return 1;
  }
  /* do mysqlization */
  for (i=0; i<a_list.size(); i++) {
    act = (a_list[i]).get() ;
    if (!act || !act->func) {
      continue ;
    }
    /* adapts place hold later */
    if (act->u.t==mktype(m_endp,s_ph)) {
      continue ;
    }
    /* do mysqlization */
    if (do_mysqlization(/*root*/node,0,act)==REDO_HIERARCH) {
      i-- ;
    }
  }
  return 1;
}

