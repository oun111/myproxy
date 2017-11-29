
#include "connstr.h"
#include "sql_tree.h"
#include "dbug.h"
#include <assert.h>


using namespace stree_types;
using namespace LEX_GLOBAL;
/*
 * class sql_tree
 */
sql_tree::sql_tree(void)
{
  /* initialize the priority 
   *  definition list of expression */
  init_prio_group();
  /* reset related variables */
  reset();
}

sql_tree::~sql_tree(void)
{
}

void sql_tree::reset(void)
{
  /* init the option flags */
  fclr(of_ci);
  //fclr(of_nt);
}

void sql_tree::destroy_node(stxNode *node)
{
  if (node) {
    delete node ;
  }
}

int sql_tree::get_parent_pos(stxNode *node)
{
  uint16_t i=0;
  stxNode *prt = 0;

  if (!node || !node->parent) {
    return -1;
  }
  /* find position of node at its upper 
   *  parent list */
  prt = node->parent ;
  for (;i<prt->op_lst.size();i++)  {
    if (prt->op_lst[i]==node) 
      return i ;
  }
  return -1 ;
}

bool sql_tree::is_type_equals(uint32_t target,
  uint32_t mtype, uint32_t stype)
{
  return mget(target)==mtype &&
    sget(target)==stype ;
}

stxNode* sql_tree::create_node(char *name, int m, int s)
{
  stxNode *p = new stxNode;

  /* TODO: throw an exeption of memory
   *  failure */
  if (!p) {
    printd("error allocate tree node memory\n");
    return NULL ;
  }
#if 0
  mset(p->type,m);
  sset(p->type,s);
#else
  p->type=mktype(m,s);
#endif
  p->name[0] = '\0';
  if (name) strcpy(p->name,name);
  p->parent  = NULL;
  return p;
}

#if TREE_NON_RECURSION==0
 stxNode* sql_tree::find_in_tree(stxNode *node,int type)
{
  uint16_t i=0;
  stxNode *nd = 0;

  if (node->type==type) {
    return node;
  }
  for (i=0;i<node->op_lst.size();i++) {
    if ((nd=find_in_tree(node->op_lst[i],type)))
      return nd ;
  }
  return NULL;
}
#else
 stxNode* sql_tree::find_in_tree(stxNode *node,int type)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *ptr = node ;
  std::stack<int> stk ;
  int _np = -1;

  for (;ptr&&(dir||_np>=0);) {
    /*  find place holder from tree */
    if (dir && ptr->type==type) {
      return ptr ;
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
  return NULL;
}
#endif

#if TREE_NON_RECURSION==0
/*bool*/stxNode* sql_tree::find_in_tree(stxNode *node,
  char *str)
{
  uint16_t i=0;
  stxNode *nd = 0;

  if (!strcasecmp(node->name,str)) {
    return /*true*/node;
  }
  for (i=0;i<node->op_lst.size();i++) {
    if ((nd=find_in_tree(node->op_lst[i],str)))
      return /*true*/nd ;
  }
  return /*false*/NULL;
}
#else
/*bool*/stxNode* sql_tree::find_in_tree(stxNode *node,
  char *str)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *ptr = node ;
  std::stack<int> stk ;
  int _np = -1;

  for (;ptr&&(dir||_np>=0);) {
    /*  find place holder from tree */
    if (!strcasecmp(ptr->name,str)) {
      return /*true*/ptr ;
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
  return /*false*/NULL ;
}
#endif

#if TREE_NON_RECURSION==0
int sql_tree::cmp_tree(stxNode *t0, stxNode *t1)
{
  uint16_t i = 0;

  if (!t0 && !t1) {
    return 0;
  }
  if (!t0 || !t1 ||
     strcmp(t0->name,t1->name) ||
     t0->type!=t1->type ||
     t0->op_lst.size()!=t1->op_lst.size()) {
    return -1;
  }
  for (i=0;i<t0->op_lst.size();i++) {
    if (cmp_tree(t0->op_lst[i],t1->op_lst[i])) 
      return -1;
  }
  return 0;
}
#else
int sql_tree::cmp_tree(stxNode *t0, stxNode *t1)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *ptr = t0, *ptr1 = t1 ;
  std::stack<int> stk0, stk1 ;
  int _np0 = -1, _np1 = -1;

  if (!ptr && !ptr1) {
    return 0;
  }
  for (;ptr&&(dir||_np0>=0) &&
    ptr1&&(dir||_np1>=0);) {
    /* compare the nodes */
    if (dir) {
      if (!ptr || !ptr1 ||
         strcmp(ptr->name,ptr1->name) ||
         ptr->type!=ptr1->type ||
         ptr->op_lst.size()!=ptr1->op_lst.size()) {
        return -1 ;
      }
    }
    /* try next node for tree 0*/
    _np0 = _np0<((int)ptr->op_lst.size()-1)?
      (_np0+1):-1;
    if (_np0<0) {
      if (!stk0.empty()) {
        _np0 = stk0.top();
        stk0.pop();
      }
      ptr = ptr->parent ;
      dir = 0;
    } else {
      ptr = ptr->op_lst[_np0];
      stk0.push(_np0);
      _np0 = -1;
      dir = 1;
    }
    /* try next node for tree 1 */
    _np1 = _np1<((int)ptr1->op_lst.size()-1)?
      (_np1+1):-1;
    if (_np1<0) {
      if (!stk1.empty()) {
        _np1 = stk1.top();
        stk1.pop();
      }
      ptr1 = ptr1->parent ;
      if (dir) 
        return -1 ;
    } else {
      ptr1 = ptr1->op_lst[_np1];
      stk1.push(_np1);
      _np1 = -1;
      if (!dir) 
        return -1 ;
    }
  } /* end for */
  return 0;
}
#endif

#if TREE_NON_RECURSION==0
stxNode* sql_tree::dup_tree(stxNode *node)
{
  uint16_t i = 0;
  stxNode *nd = 0, *cnd = 0;
  
  if (!node) {
    return 0;
  }
  nd = create_node(node->name,0,0);
  nd->type=node->type ;
  for (i=0;i<node->op_lst.size();i++) {
    cnd = dup_tree(node->op_lst[i]);
    if (cnd) 
      attach(nd,cnd);
  }
  return nd ;
}
#else
stxNode* sql_tree::dup_tree(stxNode *node)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  stxNode *nd = node, *prt = 0, 
    *tmp = 0, *top = 0 ;
  int _np = -1;
  std::stack<int> stk ;

  for (;nd&&(dir||_np>=0);) {
    /* dup tree node when down-wards */
    if (dir) {
      tmp = create_node(nd->name,0,0);
      tmp->type=nd->type ;
      /* it's the tree root */
      if (!top) {
        top = prt = tmp ;
        tmp = 0;
      } else {
        /* attach new node with parent */
        attach(prt,tmp);
        prt = tmp ;
      } 
    }
    /* try next node */
    _np = _np<((int)nd->op_lst.size()-1)?
      (_np+1):-1;
    if (_np<0) {
      if (!stk.empty()) {
        _np = stk.top();
        stk.pop();
      }
      nd = nd->parent ;
      dir = 0;
      /* fall back to upper node */
      prt = prt->parent ;
    } else {
      nd = nd->op_lst[_np];
      stk.push(_np);
      _np = -1;
      dir = 1;
    }
  } /* end for */
  return top ;
}
#endif

stxNode* sql_tree::dup_node(stxNode *node)
{
  stxNode *dnode = create_node(0,0,0);

  strcpy(dnode->name,node->name);
  dnode->type  = node->type;
  dnode->parent= node->parent ;
  return dnode ;
}

void sql_tree::dump_node(stxNode *n) 
{
  if (!(n)) {
    printd("###node is null###\n");
  } else {
    printd("###[dump node]: value: '%s', "
      "type: '%s|%s'###\n", 
      n->name,
      main_type_str(n->type),
      sub_type_str(n->type)
      );
  }
}

void sql_tree::detach(stxNode *node, uint16_t pos)
{
  stxNode *prt = node->parent ;

  if (!prt) {
    return ;
  }
  if (pos>=prt->op_lst.size()) {
    return ;
  }
  prt->op_lst.erase(prt->op_lst.begin()+pos);
  destroy_tree(node);
}

void sql_tree::attach(
  stxNode *parent, 
  stxNode *child,
  int pos)
{
  if (!parent || !child) {
    return ;
  }
  if (pos==-1) {
    parent->op_lst.push_back(child);
  } else {
    pos = (uint16_t)pos>parent->op_lst.size()?
      parent->op_lst.size():pos;
    parent->op_lst.insert(
      parent->op_lst.begin()+pos,
      child);
  }
  child->parent = parent;
}

int sql_tree::parse_where_list(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;
  int pos = p ;

  pos = next_token(s,p,buf);
  if (strcasecmp(buf,"where")) {
    printd("no 'where' clouse found\n");
    return 0;
  }
  mov(p=pos,buf);
  return parse_list(parent,s_where,p);
}

int sql_tree::parse_from_list(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  /* test ending of simple-select-list case: 
   *  .select (select @var:=0) <alias> from <tbl> 
   *  .select xxx */
  if (*buf==')' || (std::string::size_type)p>=s.size()) {
    printd("simple 'select' stmt parsed\n");
    return 1;
  }
  if (!strcasecmp(buf,"from")) {
    mov(p,buf);
  }
  return parse_list(parent,s_from,p);
}

/* parse 'group by' list */
int sql_tree::parse_gb_list(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;
  int pos = p/*, tp = 0*/;

  pos = next_token(s,p,buf);
  /* check for 'group by' keywords */
  if (strcasecmp(buf,"group")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"by")) {
    printd("incorrect 'group by' keyword\n");
    return 0;
  }
  mov(p=pos,buf);
  /* parse 'order by' list contents */
  //fset(of_nt,s_gb);
  parse_list(parent,s_gb,p);
  //fclr(of_nt);
  return 1 ;
}

int sql_tree::parse_ob_list(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;
  int pos = p/*, tp = 0*/;

  pos = next_token(s,p,buf);
  /* check for 'order by' keywords */
  if (strcasecmp(buf,"order")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"by")) {
    printd("incorrect 'order by' keyword\n");
    return 0;
  }
  mov(p=pos,buf);
  /* parse 'order by' list contents */
  //fset(of_nt,s_ob);
  parse_list(parent,s_ob,p);
  //fclr(of_nt);
  return 1 ;
}

/* test ending of an end point */
bool sql_tree::is_endp_ends(char *b) 
{
  return
    /* yzhou added statement 
     *   ending checking */
    *b==';' || *b=='%'       || 
    *b==',' || *b=='+'       || 
    *b=='-' || *b=='*'       || 
    *b=='/' || *b==')'       || 
    *b==':' || *b=='='       || 
    !strcasecmp(b,"from")    || 
    !strcasecmp(b,"where")   || 
    !strcasecmp(b,"cross")   || 
    !strcasecmp(b,"join")    || 
    !strcasecmp(b,"left")    || 
    !strcasecmp(b,"inner")   || 
    !strcasecmp(b,"right")   || 
    !strcasecmp(b,"order")   || 
    !strcasecmp(b,"group")   || 
    !strcasecmp(b,"limit")   || 
    !strcasecmp(b,"union")   || 
    !strcasecmp(b,"for")     || 
    !strcasecmp(b,"start")   || 
    !strcasecmp(b,"connect") || 
    !strcasecmp(b,"values")  || 
    !strcasecmp(b,"select")  || 
    !strcasecmp(b,"set")     || 
    !strcasecmp(b,"when")    || 
    !strcasecmp(b,"having")  || 
    !strcasecmp(b,"using")   || 
    !strcasecmp(b,"on");
}

stxNode* sql_tree::parse_alias(int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p ;

  pos = next_token(s,pos,buf);
  if (is_endp_ends(buf)) {
    return NULL ;
  }
  p = pos ;
  /* there's an 'as' token, should 
   *  be an alias */
  if (!strcasecmp(buf,"as")) {
    mov(p,buf);
    p = next_token(s,p,buf);
  }
  /* constant string as alias */
  if (*buf=='\''||*buf=='\"') {
    *buf = '\0';
    if (copy_const_str(buf,p)) {
      /* error case, end parsing */
      p = s.size();
      return NULL ;
    }
    /* skip the rest "\'" */
    p++ ;
  } else {
    mov(p,buf);
  }
  /* get alias name behind */
  return strlen(buf)<=0?NULL:
    create_node(buf,m_endp,s_alias);
}

bool sql_tree::is_numeric_str(char *str)
{
  char *p ;

  /* support the hex digit format */
  if (!strncasecmp(str,"0x",2))
    return true ;
  for (p=str;p&&isdigit(*p);p++) ;
  return p!=str&&*p=='\0' ;
}

int sql_tree::suppress_column_type_decl(
  int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p;

  /* test if it's the column type declarator
   *  in this form: col :name<type> */
  pos = next_token(s,pos,buf);
  if (*buf!=':') {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  /* skip 'name' domain */
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (*buf!='<') {
    return 0;
  }
  mov(pos,buf);
  /* suppress 'type' */
  pos = next_token(s,pos,buf);
  mov(pos,buf);
  /* suppress '>' */
  pos = next_token(s,pos,buf);
  if (*buf!='>') {
    return 0;
  }
  mov(pos,buf);
  p = pos ;
  /* for debug */
  printd("skipped colmun type declaration\n");
  return 1;
}

#if 0
int sql_tree::copy_const_str(char *inbuf, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;
  char flag = 0; /* 0: \', 1: \" */

  p = next_token(s,p,buf);
  flag = *buf=='\"' ;
  do {
    /* protect the 'inbuf' boundary */
    if ((strlen(inbuf)+strlen(buf)+1)<TKNLEN)
      strcat(inbuf,buf);
    mov(p,buf);
    p = next_token(s,p,buf) ;
    /* statement ends */
    if ((std::string::size_type)p>=s.size()) 
      break ;
  } while ((!flag&&*buf!='\'') || 
    (flag&&*buf!='\"'));
  if (*buf!='\"' && *buf!='\'') {
    printd("unterminated string\n");
    /* mark as 'unterminated error' */
    strcpy(inbuf,"\'");
    return -1;
  }
  strcat(inbuf,buf);
  //mov(p,buf);
  return 0;
}
#else
int sql_tree::copy_const_str(char *inbuf, int &p)
{
  char qmark[3] = "";
  std::string &s = sql_stmt ;
  std::string::size_type np = s.find(s[p],p+1);
  size_t ln = 0;

  /* check if string is terminated */
  if (np==std::string::npos) {
    printd("unterminated string\n");
    /* mark as 'unterminated error' */
    p = s.size()-1;
    return -1;
  }
  ln = np-p-1;
  /* check capability */
  if (ln>=TKNLEN) {
    printd("constant string too long\n");
    p = np+1;
    return -1;
  }
  /* the quotaion mark */
  qmark[0] = s[p] ;
  qmark[1] = '\0';
  /* create the constant string */
  strcat(inbuf,qmark);
  strcat(inbuf,s.substr(p+1,ln).c_str());
  strcat(inbuf,qmark);
  p = np ;
  return 0;
}
#endif

int sql_tree::parse_index_items(stxNode *parent, const char *keyStr, int &pos)
{
  int t = 0;
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "";
  
  if (!strcasecmp(keyStr,"key")) t = s_key;
  else if (!strcasecmp(keyStr,"index"))t = s_idx ;
  else if (!strcasecmp(keyStr,"primary")) {
    t = s_primary_key ;
    /* skip 'key' */
    pos = next_token(s,pos,buf);
    assert(!strcasecmp(buf,"key"));
    mov(pos,buf);
  } else if (!strcasecmp(keyStr,"unique")) {
    t = s_unique_index ;
    pos = next_token(s,pos,buf);
    if (!strcasecmp(buf,"index") || !strcasecmp(buf,"key")) {
      mov(pos,buf);
      pos = next_token(s,pos,buf);
    }
  } else if (!strcasecmp(keyStr,"foreign")) {
    t = s_foreign_key ;
    pos = next_token(s,pos,buf);
    assert (!strcasecmp(buf,"key")) ;
    mov(pos,buf);
  } else if (!strcasecmp(keyStr,"constraint")) {
    t = s_foreign_key ;
    pos = s.find("key",pos);
    assert((size_t)pos!=std::string::npos);
    pos = next_token(s,pos,buf);
    mov(pos,buf);
  }

  stxNode *p = create_node(0,m_keyw,t);
  attach(parent,p);

  pos = next_token(s,pos,buf);
  /* for 'primary key' only */
  if (t==s_primary_key) {
    /* it's: 
     *  xxx primary key, 
     *                 ^
     * means current column def item is ended, break the main loop
     */
    if (*buf==',' || *buf==')') 
      return 1; 
    /* it's:
     * primary key auto_increment 
     *             ^^^^^^^^^^^^^^
     */
    if (*buf!='(') {
      return 0 ;
    }
  }

  if (*buf!='(') {
    strcpy(p->name,buf);
    mov(pos,buf);
    pos = next_token(s,pos,buf);
  }

  /* skip '(' symbol */
  mov(pos,buf);
  /* parse the argument list */
  fset(of_ci,ci_all);
  parse_list(p,s_index,pos);
  fset(of_ci,ci_ct);
  /* skip the ')' */
  mov(pos,buf);

  /* for the 'foreign key xxx references yyy(key)' */
  if (t==s_foreign_key) {
    stxNode *tmp = 0, *t1 = 0 ; 
    
    pos = next_token(s,pos,buf);
    assert(!strcasecmp(buf,"references"));
    mov(pos,buf);

    t1 = create_node(0,m_list,s_ref_lst);
    attach(p,t1);

    fset(of_ci,ci_all);
    tmp = parse_endpoint_item(pos);
    fset(of_ci,ci_ct);
    attach(t1,tmp);
  }

  return 0;
}

int 
sql_tree::parse_normal_ct_item(stxNode *parent, const char *strCol, int &pos)
{
  char buf[TKNLEN] = "";
  stxNode *p = 0;
  std::string &s = sql_stmt;
  size_t np = 0, ln=0;

#if 0
  p = create_node((char*)strCol,m_cdt,cda_col);
#else
  p = parse_endpoint_item(pos);
#endif
  attach(parent,p);

  /* parse column type */
  pos = next_token(s,pos,buf);
  np = pos;
  mov(np,buf);
  /* construct the column type string */
  if (!strcasecmp(buf,"varchar") || !strcasecmp(buf,"decimal")) {
    np = s.find(")",np);
    assert(np!=std::string::npos);
  }
  p = create_node(0,m_cdt,cda_col_type);
  attach(parent,p);

  /* fill in type string */
  ln = np-pos+1;
  strncpy(p->name,s.substr(pos,ln).c_str(),TKNLEN);
  p->name[ln] = '\0';

  pos = np+1;
  return 0;
}

stxNode* sql_tree::parse_ct_item(int &pos)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  stxNode *parent = create_node(0,m_list,s_cd_item), *p = 0;
  int prev = 0;

  while(1) {
    pos = next_token(s,pos,buf);
    prev= pos ;
    mov(pos,buf);

    /* 
     * process create definition item attributes, grammar:
     *
     *   col_name type [NOT NULL | NULL] [DEFAULT default_value] [AUTO_INCREMENT]
     *   [PRIMARY KEY] [reference_definition]
     *   or PRIMARY KEY (index_col_name,...)
     *   or KEY [index_name] (index_col_name,...)
     *   or INDEX [index_name] (index_col_name,...)
     *   or UNIQUE [INDEX] [index_name] (index_col_name,...)
     *   or [CONSTRAINT symbol] FOREIGN KEY index_name (index_col_name,...)
     *   [reference_definition]
     *   or CHECK (expr)
     *
     */
    if (!strcasecmp(buf,"null")) {
      p = create_node(0,m_keyw,s_null);
      attach(parent,p);
    }
    else if (!strcasecmp(buf,"not")) {
      p = create_node(0,m_keyw,s_not_null);
      attach(parent,p);
      /* next should be the 'null' symbol */
      pos = next_token(s,pos,buf);
      mov(pos,buf);
      assert(!strcasecmp(buf,"null"));
    }
    else if (!strcasecmp(buf,"default")) {
      p = create_node(0,m_keyw,s_default_val);
      attach(parent,p);

      stxNode *tp = parse_endpoint_item(pos);
      attach(p,tp);
    }
    else if (!strcasecmp(buf,"auto_increment")) {
      p = create_node(0,m_keyw,s_auto_inc);
      attach(parent,p);
    }
    else if (!strcasecmp(buf,"key") || !strcasecmp(buf,"index") || 
       !strcasecmp(buf,"primary") || !strcasecmp(buf,"unique") ||
       !strcasecmp(buf,"foreign") || !strcasecmp(buf,"constraint")) {

      if (parse_index_items(parent,buf,pos)==1) {
        break ;
      }

    } 
    else if (!strcasecmp(buf,"comment")) {

      stxNode *p = parse_endpoint_item(pos);

      p->type = mktype(m_keyw,s_comment);
      attach(parent,p);
    }
    /* TODO: */
#if 0
    else if (!strcasecmp(buf,"check")) {}
#endif
    /* normal column definitions */
    else {
      parse_normal_ct_item(parent,buf,prev);
      pos = prev ;
    }

    /* try next */
    pos = next_token(s,pos,buf);

    /* end of 'create list item list' */
    if (*buf==',' || *buf==')') break ; 
  } 

  return parent;
}

stxNode* sql_tree::parse_endpoint_item(
  int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  stxNode *node, *top, *tmp ;
  bool bSet ;
  int flag = ci_all ;

  /*
   * select list normal item types:
   *  {*  |
   *  query_name.* |
   *  [schema.]{table|view|materialized_view}.* |
   *  expr [[as] c_alias]}
   */
  p = next_token(s,p,buf);
  /* check statement ending */
  if (*buf==';') {
    return NULL;
  }
  node= create_node(NULL,m_endp,0);
  top = node ;
  /* signed constant */
  if (*buf=='-' || *buf=='+') {
    strcat(node->name,buf);
    p = next_token(s,p+1,buf) ;
  }
  /* test string for numeric digit */
  if (is_numeric_str(buf)) {
    strcat(node->name,buf);
    sset(node->type,s_c_int);
    mov(p,buf);
    /* test for floating point digit */
    p = next_token(s,p,buf);
    if (*buf=='.') {
      p = next_token(s,p+1,buf);
      mov(p,buf);
      strcat(node->name,".");
      strcat(node->name,buf);
      sset(node->type,s_c_float);
    }
  } else if (*buf=='*') {
    /* the wild cast symbol */
    strcpy(node->name,"*");
    mset(node->type,m_keyw);
    sset(node->type,s_wildcast);
    mov(p,buf);
  } else if (*buf=='\'' || *buf=='\"') {
    /* get string constant */
    if (copy_const_str(node->name,p)) {
#if 0
      return NULL;
#else
      /* error case, end the parsing */
      p = s.size();
      sset(node->type,s_c_str);
      return node;
#endif
    }
    sset(node->type,s_c_str);
    mov(p,buf);
  } else {
    /* other end point types */
    strcat(node->name,buf);
    sset(node->type,s_col);
    mov(p,buf);
  }
  /* test the next token */
  p = next_token(s,p,buf);
  /* ### case 2 */
  if (*buf=='.') {
    //sset(node->type,s_tbl);
    node->type = mktype(m_endp,s_tbl);
    /* get targeting column */
    p   = next_token(s,p+1,buf);
    node= create_node(buf,m_endp,s_col);
    attach(top,node);
    /* pre-get next token */
    mov(p,buf);
    p = next_token(s,p,buf);
    /* ### case 3 */
    if (*buf=='.') {
      p   = next_token(s,p+1,buf);
      tmp = create_node(buf,m_endp,s_col);
      attach(node,tmp);
      /* update sub type of parent nodes */
      sset(node->type,s_tbl);
      sset(node->parent->type,s_schema);
      /* pre-get next token */
      mov(p,buf);
      p = next_token(s,p,buf);
    }
  } 
  if (*buf==':') {
    /* try if it's the column type declarator
     *  in this form: col :#<type> */
    suppress_column_type_decl(p);
  }
  /* test if endpoint is to be 
   *  parsed as function */
  fget(of_ci,flag,bSet);
  /* parse as a function-type call */
  if ((flag&ci_func) && *buf=='(' && !is_ora_join(p)) {
    printf("parsing function\n");
    /* skip '(' symbol */
    mov(p,buf);
    /* change node type to function call */
    sset(node->type,s_func);
    /* parse the argument list */
    //fset(of_nt,s_arg);
    parse_list(node,s_arg,p);
    //fclr(of_nt);
    /* skip the ')' */
    mov(p,buf);
  }
  return top;
}

bool sql_tree::is_join_item(int p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;

  /* a sub query, skip the colon pairs */
  if (str_test_subquery(p,p)) {
    /* a normal endpoint, skip the 
     *  [chema.]table[.column] string */
    while (1) {
      p = next_token(s,p,buf);
      mov(p,buf);

      p = next_token(s,p,buf);
      if (*buf=='.') p++ ;

      else break ;
    } 
  }
  /* test for keyword 'left' or 'right' */
  p = next_token(s,p,buf);
  if (!strcasecmp(buf,"left") || 
     !strcasecmp(buf,"right") ||
     !strcasecmp(buf,"cross") ||
     !strcasecmp(buf,"inner") ||
     !strcasecmp(buf,"join")) {
    return true ;
  }
  /* skip the possible alias */
  mov(p,buf);
  p = next_token(s,p,buf);
  if (!strcasecmp(buf,"left") ||
     !strcasecmp(buf,"right") ||
     !strcasecmp(buf,"cross") ||
     !strcasecmp(buf,"inner") ||
     !strcasecmp(buf,"join")) {
    return true ;
  }
  return false ;
}

stxNode* sql_tree::parse_using_list(int &p)
{
  stxNode *top=0, *node=0;

  fset(of_ci,(ci_all&~ci_join));
  node = parse_list_item(p);
  fclr(of_ci);
  if (node) {
    top = create_node(0,m_list,s_using); 
    attach(top,node);
  }
  return top ;
}

stxNode* sql_tree::parse_on_list(int &p)
{
  stxNode *top=0, *node=0;

  fset(of_ci,(ci_all&~ci_join));
  node = parse_list_item(p);
  fclr(of_ci);
  if (node) {
    top = create_node(0,m_list,s_on); 
    attach(top,node);
  }
  return top ;
}

/* p: begining of subquery, 
 * endp: end of subquery */
int sql_tree::str_test_subquery(int p, int &endp)
{
  int cnt = 0;
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  if ((size_t)p<s.size() && s[p]=='(') {
    /* a sub query, skip the colon pairs */
    for (++p,cnt=1;cnt>0&&
      (std::string::size_type)p<s.size();p++) 
    {
      cnt = s[p]=='('?(cnt+1):
        s[p]==')'?(cnt-1):cnt ;
    }
    if (cnt) {
      printd("unterminated bracket pairs\n");
      return -1;
    }
    endp = p ;
    return 0;
  }
  return -1;
}

bool sql_tree::has_join_in_subquery(int p)
{
  int pos = 0;
  char *psub = 0 ;
  std::string &s = sql_stmt ;

  /* find 'join' pattern in subquery */
  if (!str_test_subquery(p,pos)) {
    std::string tmp="" ;
    /* XXX: i don't know why 'psub' gets nothing
     *  sometimes in case 1, so i used case 2
     *  and it works always! */
#if 0
    psub = (char*)s.substr(p,pos-p).c_str();
#else
    tmp = s.substr(p,pos-p);
    psub= (char*)tmp.c_str();
#endif
    return !!strcasestr(psub,"join");
  }
  return false ;
}

/* skip '()' pair and find 'union' */
bool sql_tree::has_stmt_set(int p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;

  for (p++; *buf!=')'&&(uint16_t)p<s.size();) {
    str_test_subquery(p,p);
    p = next_token(s,p,buf);
    if (!strcasecmp(buf,"union"))
      return true ;
    else mov(p,buf);
  }
  return false ;
}

stxNode* sql_tree::parse_join_item(int &p)
{
  stxNode *node = 0, *top = 0;
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  int st = s_none;

  /* pre-create the main node */
  top = create_node(0,m_expr,s_none);
  for (;;st=s_none) {
    p = next_token(s,p,buf);
    /* test end of join list */
    if (is_on_list_ends(buf,p) ||
       *buf==',') {
      if (top) {
        node = top;
        top  = NULL ;
      }
      break ;
    } else if (!strcasecmp(buf,"left") 
       || !strcasecmp(buf,"right")
       || !strcasecmp(buf,"inner")
       || !strcasecmp(buf,"cross")
       || !strcasecmp(buf,"join")) {
      /* get sub type */
      st = !strcasecmp(buf,"left")?s_ljoin:
        !strcasecmp(buf,"right")?s_rjoin:
        !strcasecmp(buf,"inner")?s_ijoin:
        !strcasecmp(buf,"cross")?s_cjoin:
        s_join ;
      mov(p,buf);
      /* update 'join' type */
      if (!top) {
        top = create_node(0,m_expr,st);
      } else {
        sset(top->type,st);
      }
      if (node) {
        attach(top,node);
      }
      /* skip the 'join' token */
      if (st!=s_join) {
        p = next_token(s,p,buf);
        mov(p,buf);
      }
    } else if (!strcasecmp(buf,"on") ||
       !strcasecmp(buf,"using")) {
      mov(p,buf);
      /* parse the 'on' list */
      if (!strcasecmp(buf,"on")) {
        node = parse_on_list(p);
      } else {
        /* the 'using (...)' clouse */
        node = parse_using_list(p);
      }
      attach(top,node);
      /* end processing a whole 'join' node, 
       *  move the node pointer upwards */
      node = top;
      top  = NULL ;
    } else {
      /* 2016.3.18: yzhou added bug fix to support 
       *  'nesting join' statements */
      p = next_token(s,p,buf);
#if 0
      if (*buf=='(' && has_join_in_subquery(p)) {
        p++ ;
      } else if (top->op_lst.size()<=0) {
        fset(of_ci,(ci_all&~ci_join));
      }
      /* parse 'join' item sub tree */
      node = parse_complex_item(p);
      /* skip the ending ')' if exists */
      p = next_token(s,p,buf);
      if (*buf==')')   p++ ;
      /* cleanup control flag */
      fclr(of_ci);
#else
      if (*buf=='(' && has_stmt_set(p)) {
        /* parse 'join' item with statement set */
        p++;
        node = parse_stmt_set(s_norm,p);
      } else {
        /* test for nesting 'join' item */
        if (*buf=='(' && has_join_in_subquery(p)) 
          p++; 
        else if (top->op_lst.size()<=0) 
          fset(of_ci,(ci_all&~ci_join));
        /* parse normal 'join' item */
        node = parse_complex_item(p);
      }
      /* skip the ending ')' if exists */
      p = next_token(s,p,buf);
      if (*buf==')')   p++ ;
      /* cleanup control flag */
      fclr(of_ci);
#endif
      /* attach join item to main node */
      attach(top,node);
      /* test alias behind */
      p = next_token(s,p,buf);
      /* get possible alias */
      if (!is_endp_ends(buf) && 
         (node=parse_alias(p))) {
        /* attach alias node */
        attach(top,node);
      }
      node = NULL ;
    } /* end else */
  } /* end while(1) */
  return node ;
}

bool sql_tree::is_place_holder(int &p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  int pos = 0;

  /* test for place holders in the form 
   *  like: :name<type> */
  pos = next_token(s,p,buf);
  if (*buf!=':') {
    return false ;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  return *buf=='<' ;
}

stxNode* sql_tree::parse_place_holder(int &p)
{
  stxNode *node = 0, *tmp = 0;
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;

  /* construct 'place holder' node */
  node = create_node(0,m_endp,s_ph);
  /* skip the ':' symbol */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* get 'name' attribute */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* attach to main node */
  tmp = create_node(buf,m_pha,s_pha_name);
  attach(node,tmp);
  /* 
   * get contents within '< >' pair
   */
  /* skip '<' */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* get 'type' attribute */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* attach to main node */
  tmp= create_node(buf,m_pha,s_pha_t);
  attach(node,tmp);
  /* for the type 'unsigned xxx' */
  if (!strcasecmp(buf,"unsigned")) {
    p = next_token(s,p,buf);
    strcat(tmp->name," ");
    strcat(tmp->name,buf);
    mov(p,buf);
  }
  /* also get size attribute for 
   *  type 'char' */
  if (!strcasecmp(buf,"char")) {
    /* skip the '[' symbol */
    p = next_token(s,p,buf);
    if (*buf=='[') {
      mov(p,buf);
      p = next_token(s,p,buf);
      mov(p,buf);
      /* attach to main node */
      tmp= create_node(buf,m_pha,s_pha_sz);
      attach(node,tmp);
      /* skip the ']' symbol */
      p = next_token(s,p,buf);
      mov(p,buf);
    }
  }
  /* skip token behind 'type' attribute */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* test next attribute */
  if (*buf==',') {
    p = next_token(s,p,buf);
    mov(p,buf);
    /* parse direction attribute */
    if (!strcasecmp(buf,"in") ||
       !strcasecmp(buf,"out")) {
      /* attach to main node */
      tmp = create_node(buf,m_pha,s_pha_dir);
      attach(node,tmp);
    } else {
      printd("unknown attribute %s\n", buf);
    }
    /* skip '>' symbol */
    p = next_token(s,p,buf);
    mov(p,buf);
  }
  return node ;
}

bool sql_tree::is_unary_expr(int p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "";

  p = next_token(s,p,buf);
  /* test for 'prior' operator */
  if (!strcasecmp(buf,"prior")) {
    return true ;
  }
  /* test for 'exists' operator */
  if (!strcasecmp(buf,"exists")) {
    return true ;
  }
  /* test for 'not exists' operator */
  if (!strcasecmp(buf,"not")) {
    mov(p,buf);
    p = next_token(s,p,buf);
    if (!strcasecmp(buf,"exists"))
      return true; 
  }
  if (*buf=='~') {
    return true;
  }
  return false ;
}

stxNode* sql_tree::parse_unary_expr(int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "";
  stxNode *node = 0, *tmp = 0;
  int st = s_none ;

  /* parse and create unary expression trees 
   */
  p = next_token(s,p,buf);
  mov(p,buf);
  if (!strcasecmp(buf,"prior")) {
    st = s_pir ;
  } else if (!strcasecmp(buf,"exists")) {
    st = s_exts ;
  } else if (!strcasecmp(buf,"not")) {
    p = next_token(s,p,buf);
    mov(p,buf);
    if (!strcasecmp(buf,"exists"))
      st = s_not_ext ;
  } else if (*buf=='~') {
    st = s_bit_reverse;
  }
  if (st==s_none) {
    return NULL ;
  }
  /* create the expr node */
  tmp = parse_complex_item(p);
  if (tmp) {
    /* attach with 'unary expr' node */
    node = create_node(0,m_uexpr,st);
    attach(node,tmp);
  }
  return node ;
}

stxNode* sql_tree::parse_complex_item(int &p)
{
  bool bset= false ;
  int flag = ci_all;
  stxNode *node = 0;
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  /* get and reset 'complex item' controling flags */
  fget(of_ci,flag,bset);
  /* test and parse the 'left join'
   *  /'right join' clouse */
  if (flag&ci_join && is_join_item(p)) {
    return parse_join_item(p) ;
  }
  /* test and skip the possible '(' symbol */
  if (flag&ci_nest && *buf=='(') {
    p++ ;
    /* get the required list type,
     *  if needed */
#if 0
    fget(of_nt,(flag=s_norm),bset);
#else
    flag = s_norm ;
#endif
    /* process the inner statement, or
     *  a statement set such as 'union [all]' */
    node = parse_stmt_set(flag,p);
    p = next_token(s,p,buf);
    if (*buf==')') {
      p++ ;
    }
    return node ;
  }
  /* test the list item for a 
   *  normal statement */
  if (flag&ci_stmt && is_stmt(p)) {
    return parse_stmt(p);
  }
  /* test for place holders */
  if (flag&ci_phd && is_place_holder(p)) {
    return parse_place_holder(p);
  }
  /* test for unary expressions */
  if (flag&ci_ury && is_unary_expr(p)) {
    return parse_unary_expr(p);
  }
  /* normal endpoint */
  if (flag&ci_norm) {
    return parse_endpoint_item(p);
  }
  /* the 'create table list' item */
  if (flag&ci_ct) {
    return parse_ct_item(p);
  }
  return NULL ;
}

/* create low priority node structure */
stxNode* sql_tree::create_lp_node(
  stxNode *child, 
  std::stack<int> &so, 
  std::stack<stxNode*> &se)
{
  stxNode *tp = 0;

  if (so.empty()) {
    return /*NULL*/child ;
  }
  /* create the low priority tree node */
  tp = create_node(0,m_expr,so.top());
  attach(tp,se.top());
  attach(tp,child);
  so.pop();
  se.pop();
  return tp ;
}

bool sql_tree::is_ora_join(int p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  if (*buf!='(')
    return false ;
  p = next_token(s,p+1,buf);
  if (*buf!='+')
    return false ;
  next_token(s,p+1,buf);
  if (*buf!=')')
    return false ;
  return true;
}

int sql_tree::get_op_type(int &p)
{
  int op_type = s_none, pos = 0;
  char buf[TKNLEN] = "", tmp[TKNLEN] = "";
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  /* test for operator types */
  op_type = 
    !strcasecmp(buf,"!=")?s_ne1:
    !strcasecmp(buf,"<>")?s_ne:
    !strcasecmp(buf,"<=")?s_le:
    !strcasecmp(buf,">=")?s_be:
    !strcasecmp(buf,"+=")?s_peql:
    !strcasecmp(buf,":=")?s_eql1:
    !strcasecmp(buf,"(+)")?s_olj:
    /* bit operators support */
    !strcasecmp(buf,">>")?s_bit_rshift:
    !strcasecmp(buf,"<<")?s_bit_lshift:
    // '~' is treat as unary operator
    *buf=='^'?s_bit_xor:
    /* bit operators support end */
    *buf=='+'?s_plus:
    *buf=='-'?s_reduce:
    *buf=='*'?s_multiply:
    *buf=='/'?s_devide:
    *buf=='%'?s_mod:
    *buf=='='?s_eql:
    *buf=='>'?s_bt:
    *buf=='<'?s_lt:
    *buf=='('?s_lbkt:
    !strcasecmp(buf,"||")?s_cat:
    !strcasecmp(buf,"between")?s_btw:
    !strcasecmp(buf,"and")?s_and:
    !strcasecmp(buf,"or")?s_or:
    !strcasecmp(buf,"in")?s_in:
    !strcasecmp(buf,"like")?s_like:
    !strcasecmp(buf,"escape")?s_escape:
    !strcasecmp(buf,"regexp")?s_rexp:
    !strcasecmp(buf,"not")?s_not:
    !strcasecmp(buf,"is")?s_is:
    /* bit operators support begin */
    *buf=='|'?s_bit_or:
    *buf=='&'?s_bit_and:
    /* bit operators support end */
    s_none ;
  /* none of the op type is recognized, exit */
  if (op_type==s_none) {
    return s_none ;
  }
  /* process with myltiplex types */
  switch (op_type) {
    /* type starts with 'is' */
    case s_is:
      mov(pos=p,buf);
      /* get operator behind */
      pos = next_token(s,pos,tmp);
      op_type = !strcasecmp(tmp,"not")?s_not_is:
        op_type ;
      /* it's the 'is not' operator */
      if (op_type!=s_is) {
        p = next_token(s,p=pos,buf);
      }
      break ;
    /* type starts with 'not' */
    case s_not:
      mov(p,buf);
      /* get operator behind */
      p = next_token(s,p,buf);
      op_type = !strcasecmp(buf,"in")?s_not_in:
        op_type ;
      break ;
    /* type (+) */
    case s_olj:
      next_token(s,p+strlen(buf),tmp);
      op_type = (*tmp=='=')?s_orj:s_olj ;
      break ;
    /* type 'regexp xxx' */
    case s_rexp:
      pos = next_token(s,p+strlen(buf),tmp);
      /* 'regexp binary' */
      if (!strcasecmp(tmp,"binary")) {
        op_type = s_rexpb ;
        strcpy(buf,tmp);
        p = pos ;
      }
      break ;
    default: break ;
  }
  /* skip last symbol of the 
   *  operator */
  mov(p,buf);
  return op_type ;
}

/* initialize the operator priority 
 *  definition group */
void sql_tree::init_prio_group(void)
{
  int prio = 0, i = 0;

  /* priority group 0, lowest */
  for (i=s_not;i<=s_rjoin;i++)
    prio_lst[i] = prio ;
  /* priority group 1 */
  for (prio++,i=s_and;i<=s_or;i++)
    prio_lst[i] = prio ;
  /* priority group 2 */
  for (prio++,i=s_bt;i<=s_ne;i++)
    prio_lst[i] = prio ;
  /* priority group 3 */
  for (prio++,i=s_olj;i<=s_not_is;i++)
    prio_lst[i] = prio ;
  /* priority group 4 */
  for (prio++,i=s_plus;i<=s_reduce;i++)
    prio_lst[i] = prio ;
  /* priority group 5 */
  for (prio++,i=s_multiply;i<=s_devide;i++)
    prio_lst[i] = prio ;
}

/* test op1 is larger equal than op2 */
bool sql_tree::is_prio_le(uint32_t op1, 
  uint32_t op2)
{
  if (op1>=expr_max || op2>=expr_max) {
    return false ;
  }
  return prio_lst[op1]==prio_lst[op2]?true:
    op1>op2 ;
}

stxNode* sql_tree::parse_list_item(int &p)
{
#if 0
#define oj_proc(nd,jt) do{ \
  if (jt==s_olj || jt==s_orj) { \
    sset(nd->type,jt);          \
    jt = s_none ;}              \
} while(0)
#else
  auto oj_proc = [&](auto &nd, auto &jt)  {
    if (jt==s_olj || jt==s_orj) { 
      sset(nd->type,jt);
      jt = s_none ;
    }
  };
#endif
  stxNode *node = 0;
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  int op_type = 0, op_oj = s_none;
  std::stack<int>  s_op; /* saving operators */
  std::stack<stxNode*> s_endp ;/* saving end points */

  while (1) {
    /* may be a complex item, such as 
     *  expressions or sub query 
     *  statements or function call or
     *  normal end point  */
    node = parse_complex_item(p);
__try_more_ops:
    /* end of expression reached, exist loop */
    p = next_token(s,p,buf);
    if (*buf==';' ||*buf==',' || 
       !strcasecmp(buf,"from") ||
       !strcasecmp(buf,"left") ||
       !strcasecmp(buf,"right")||
       !strcasecmp(buf,"inner")||
       !strcasecmp(buf,"cross")||
       !strcasecmp(buf,"join")||
       !strcasecmp(buf,"where")) {
      break ;
    }
    /* none of the op type is recognized, 
     *  exit loop */
    if ((op_type=get_op_type(p))==s_none) {
      break ;
    }
    /* get next operator when op is 
     *  'oracle left/right join' */
    if (op_type==s_olj || op_type==s_orj) {
      /* save op */
      op_oj = op_type ;
      goto __try_more_ops ;
    }
    if (!s_op.empty() &&
       is_prio_le(s_op.top(),op_type)) {
      /* recreate the low-priority nodes */
      node = create_lp_node(node,s_op,s_endp);
      /* if the 'oracle join' flag is set then 
       *  modify the node type to 'join' */
      oj_proc(node,op_oj);
    }
    /* save the received node and op */
    s_op.push(op_type);
    s_endp.push(node);
  }
  /* join the left low-priority node, 
   *  if exists */
  while (!s_op.empty()) {
    node = create_lp_node(node,s_op,s_endp);
    oj_proc(node,op_oj);
  }
  return node;
}

/* parse keyword in 'order by' list */
stxNode* sql_tree::parse_ob_keys(
  char *tkn, int &p)
{
  int st = s_none; 

  if (!strcasecmp(tkn,"asc")) {
    st = s_asc ;
    mov(p,tkn);
  } else if (!strcasecmp(tkn,"desc")) {
    st = s_dsc;
    mov(p,tkn);
  } else {
    //printd("unknown keyword '%s' in 'order by' stmt\n", tkn);
    return NULL ;
  }
  return create_node(NULL,m_keyw,st);
}

/* parse keyword in 'select' list */
stxNode* sql_tree::parse_sl_keys(
  char *tkn, int &p)
{
  int st = s_none; 

  if (!strcasecmp(tkn,"hint")) {
    st = s_hint ;
    mov(p,tkn);
  } else if (!strcasecmp(tkn,"all")) {
    st = s_all;
    mov(p,tkn);
  } else if (!strcasecmp(tkn,"distinct")) {
    st = s_distinct;
    mov(p,tkn);
  } else if (!strcasecmp(tkn,"unique")) {
    st = s_unique;
    mov(p,tkn);
  } else {
    return NULL ;
  }
  return create_node(NULL,m_keyw,st);
}

/* test ending of a select list */
bool sql_tree::is_sel_list_ends(char *tkn, int &p) {
  /* check for statement ending by ';' */
  return !strcasecmp(tkn,"from")||*tkn==')' ||
    !strcasecmp(tkn,"union")    ||*tkn==';' ||
    (p>=(int)sql_stmt.size());
}

/* test ending of a from list */
bool sql_tree::is_frm_list_ends(char *tkn, int &p) {
  return *tkn==')'||*tkn==';' ||
    !strcasecmp(tkn,"where")  || 
    !strcasecmp(tkn,"union")  || 
    !strcasecmp(tkn,"order")  || 
    !strcasecmp(tkn,"group")  || 
    !strcasecmp(tkn,"limit")  || 
    !strcasecmp(tkn,"start")  || 
    !strcasecmp(tkn,"connect")|| 
    !strcasecmp(tkn,"for")    || 
    !strcasecmp(tkn,"having") || 
    !strcasecmp(tkn,"using")  || 
    (p>=(int)sql_stmt.size());
}

/* test ending of a where list */
bool sql_tree::is_where_list_ends(char *tkn, int &p) {
  return *tkn==')'||*tkn==';' ||
    !strcasecmp(tkn,"union")  || 
    !strcasecmp(tkn,"order")  || 
    !strcasecmp(tkn,"group")  || 
    !strcasecmp(tkn,"limit")  || 
    !strcasecmp(tkn,"start")  || 
    !strcasecmp(tkn,"connect")|| 
    !strcasecmp(tkn,"for")    || 
    !strcasecmp(tkn,"when")   || 
    !strcasecmp(tkn,"on")     || 
    !strcasecmp(tkn,"having") || 
    (p>=(int)sql_stmt.size());
}

/* test ending of an 'order by' list */
bool sql_tree::is_ob_list_ends(char *tkn, int &p) {
  return is_frm_list_ends(tkn,p);
}

bool sql_tree::is_on_list_ends(char *tkn, int &p) {
  return is_ob_list_ends(tkn,p);
}

/* test ending of a 'format' list 
 *  in 'insert' statements */
bool sql_tree::is_fmt_list_ends(char *tkn, int &p) {
  return !strcasecmp(tkn,"values")||*tkn==')' ||
    !strcasecmp(tkn,"on")    ||
    !strcasecmp(tkn,"where") ||
    /* add 20160412: test ending by ';' */
    *tkn==';' ||
    (p>=(int)sql_stmt.size());
}

/* test ending of a 'update' list 
 *  in 'update' statements */
bool sql_tree::is_update_list_ends(char *tkn, int &p) {
  return !strcasecmp(tkn,"set") ||
    !strcasecmp(tkn,"where")    ||
    (p>=(int)sql_stmt.size());
}

/* test ending of 'create table' list */
bool sql_tree::is_ct_list_ends(char *tkn, int &p) {
  return *tkn==')';
}

int sql_tree::parse_list(
  stxNode *parent, 
  int st, 
  int &p,
  stxNode **child)
{
  /* test ending */
  using fLstEnd = bool(sql_tree::*)(char*,int&);
  /* parse keyword in list */
  using fParseKey = stxNode*(sql_tree::*)(char*,int&);
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  stxNode *node = 0, *pList = 0, *pExtra=0;
  fLstEnd is_end  = 0 ;
  fParseKey get_key =0 ;

  /* create the 'xxx list' node */
  pList = create_node(NULL,m_list,st);
  if (!pList) {
    return 0;
  }
  /* get necessary function pointers */
  is_end  = get_end_func(st);
  get_key = get_kwp_func(st);
  /* iterate the whole list */
  while (1) {
    p = next_token(s,p,buf);
    /* test ending of 'list' */
    if (is_end&&(this->*is_end)(buf,p)) {
      break ;
    }
    /* test and get for posible keywords 
     *  before 'list' */
    node = get_key?(this->*get_key)(buf,p):NULL;
    if (!node) {
      /* process the real select list */
      node = parse_list_item(p);
      /* parse the possible alias */
      p = next_token(s,p,buf);
      /* test and get token after list item, 
       *  may be alias or keyword */
      pExtra = get_key?(this->*get_key)(buf,p):NULL ;
      pExtra = !pExtra?parse_alias(p):pExtra ;
      if (pExtra) {
        /* test next token behind alias */
        p = next_token(s,p,buf);
      }
      if (*buf==',') {
        /* skip the ',' symbol */
        mov(p,buf);
      }
    }
    /* attach node and its alias to 'select list' set */
    if (node) {
      /* attach node item */
      attach(pList,node);
      /* attach extra item if exists */
      if (pExtra) 
        attach(pList,pExtra);
    }
  } /* end while(1) */
  /* attach 'select list' node to statement node */
  if (parent) {
    attach(parent,pList);
  } 
  if (child) {
    *child = pList ;
  }
  return 1;
}

int sql_tree::parse_for_update(stxNode *parent, int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p ;
  stxNode *node = 0, *tmp = 0;
  int st = 0;

  /* test for the 'for update' element */
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"for")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"update")) {
    printd("incorrect 'for update' keyword\n");
    return 0;
  }
  mov(p=pos,buf);
  /* create node of 'for update' */
  node = create_node(0,m_keyw,s_fu);
  attach(parent,node);
  /* test attribute of 'for update' */
  pos = next_token(s,p,buf);
  st  = !strcasecmp(buf,"nowait")?s_fua_nowait:
    !strcasecmp(buf,"wait")?s_fua_wait:
    !strcasecmp(buf,"of")?s_fua_oc:s_none ;
  if (st==s_none) {
    printd("unknown 'for update' attr %s\n", buf);
    return 0;
  }
  mov(p=pos,buf);
  /* get value of 'wait' and 'of columns' */
  if (st==s_fua_wait || st==s_fua_nowait) {
    /* create the attribute node */
    tmp = create_node(0,m_fua,st);
    attach(node,tmp);
    if (st==s_fua_wait) {
      /* process the 'wait' value */
      p = next_token(s,p,buf);
      strcpy(tmp->name,buf);
      mov(p,buf);
    }
  } else if (st==s_fua_oc) {
    /* process the 'of columns' list */
    //fset(of_nt,s_fuoc);
    parse_list(node,s_fuoc,p);
    //fclr(of_nt);
  }
  return 1;
}

/* parse the 'limit x[,y]' item */
int sql_tree::parse_limit_list(stxNode *parent, int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p/*, np = 0 */;

  /* test the 'limit' item */
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"limit")) {
    return 0;
  }
  mov(p=pos,buf);
  /* parse 'limit' list contents */
  //fset(of_nt,s_limit);
  parse_list(parent,s_limit,p);
  //fclr(of_nt);
  return 1;
}

int sql_tree::parse_connect_by(stxNode *parent, int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p ;

  /* test for the 'connect by' element */
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"connect")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"by")) {
    printd("incorrect 'connect by' keyword\n");
    return 0;
  }
  mov(p=pos,buf);
  return parse_list(parent,s_cb,p);
}

int sql_tree::parse_start_with(stxNode *parent, int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  int pos = p, ret = 0 ;
  stxNode *psw = 0, *nd = 0;
  uint16_t i = 0;

  /* test for the 'start with' element */
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"start")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"with")) {
    printd("incorrect 'start with' keyword\n");
    return 0;
  }
  mov(p=pos,buf);
  /* parse 'start with' list contents */
  ret = parse_list(0,s_sw,p,&psw);
  /* find if 'connect by' list exists
   *  in parent */
  for (i=0;i<parent->op_lst.size();i++) {
    nd = parent->op_lst[i] ;
    if (nd->type==mktype(m_list,s_cb)) 
      break ;
  }
  /* the 'start with' list should always be
   *  in front of 'connect by' */
  attach(parent,psw,i<parent->op_lst.size()?
    i:-1);
  return ret;
}

int sql_tree::parse_having_list(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt ;
  int pos = p ;

  pos = next_token(s,p,buf);
  if (strcasecmp(buf,"having")) {
    return 1;
  }
  mov(p=pos,buf);
  return parse_list(parent,s_hav,p);
}

int sql_tree::parse_flexible_items(stxNode *parent, int &p)
{
  using ef = int(sql_tree::*)(stxNode*,int&);
  ef f_list[]= {
    &sql_tree::parse_ob_list,
    &sql_tree::parse_gb_list,
    &sql_tree::parse_for_update,
    &sql_tree::parse_limit_list,
    &sql_tree::parse_start_with,
    &sql_tree::parse_connect_by,
    &sql_tree::parse_having_list,
  } ;
  uint16_t i = 0, n = 0 ;
  const uint16_t item_count = 
    sizeof(f_list)/sizeof(f_list[0]);
  
  /* try more extra items: ensure all extra
   *  items are tested and processed */
  for (; n<item_count; n++) {
    for (i=0; i<item_count; i++) 
      (this->*f_list[i])(parent,p);
  }
  return 1;
}

/* process 'on duplicate key update' of
 *  'insert' statement */
int sql_tree::parse_odku_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  int pos = p;

  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"on")) {
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"duplicate")) {
    printd("incorrect 'on duplicate key update' "
      "keyword\n");
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"key")) {
    printd("incorrect 'on duplicate key update' "
      "keyword\n");
    return 0;
  }
  mov(pos,buf);
  pos = next_token(s,pos,buf);
  if (strcasecmp(buf,"update")) {
    printd("incorrect 'on duplicate key update' "
      "keyword\n");
    return 0;
  }
  mov(pos,buf);
  /* parse the list */
  //fset(of_nt,s_odku);
  parse_list(parent,s_odku,pos);
  //fclr(of_nt);
  p = pos ;
  return 1;
}

int sql_tree::parse_insert_stmt(stxNode *parent, int &p)
{
/* remove 'values' keyword from 'value list' */
#if 0
#define RM_VAL(prt) do {\
  int n = prt->op_lst.size()-1;\
  stxNode *vn = prt->op_lst[n] ;\
  if (vn->type==mktype(m_list,s_val)) {\
    /* there're stuffs under 'values' */ \
    if (vn->op_lst.size()>0) {\
      stxNode *node = vn->op_lst[0];\
      if (mget(node->type) == m_stmt) { \
        vn->op_lst.erase(\
          vn->op_lst.begin());\
        detach(vn,n);\
        attach(prt,node); \
      }  \
    } else { \
      detach(vn,n);\
    } \
  }\
} while(0) 
#else
  auto RM_VAL = [&](auto &prt) {

    int n = prt->op_lst.size()-1;
    stxNode *vn = prt->op_lst[n] ;

    if (vn->type==mktype(m_list,s_val)) {
      /* there're stuffs under 'values' */ 
      if (vn->op_lst.size()>0) {
        stxNode *node = vn->op_lst[0];

        if (mget(node->type) == m_stmt) {
          vn->op_lst.erase(
            vn->op_lst.begin());
          detach(vn,n);
          attach(prt,node);
        }
      } else {
        detach(vn,n);
      }
    }
  } ;
#endif
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  stxNode *node = 0;
  int st = 0;

  /* skip the 'into' keyword */
  p = next_token(s,p,buf);
  /* XXX: this 'if' block is used to ignore the 'insert'
   *  target processing in 'insert' part of 'merge into' 
   *  statement, and to process those in normal 'insert' 
   *  statements */
  if (!strcasecmp(buf,"into")) {
    mov(p,buf);
    /* get 'insert' target */
    fset(of_ci,ci_all&~ci_func);
    node = parse_endpoint_item(p);
    fclr(of_ci);
    attach(parent,node);
    p = next_token(s,p,buf);
    /* test if there's alias behind target name */
    if (*buf!='(' && 
       strcasecmp(buf,"values") &&
       strcasecmp(buf,"select")) {
      /* get the alias if exists */
      node = parse_alias(p);
      attach(parent,node);
      p = next_token(s,p,buf);
    }
  }
  /* parse the first list, assume to be 
   *  the 'format' list */
  if (strcasecmp(buf,"values")) {
    //fset(of_nt,s_fmt);
    node = parse_complex_item(p);
    attach(parent,node);
    //fclr(of_nt);
    /* assume to be 'format' list if 
     *  '(' exists */
    if (*buf=='(')
      sset(node->type,s_fmt);
  }
  /* skip the 'values' keyword */
  p = next_token(s,p,buf);
  if (!strcasecmp(buf,"values")) {
    mov(p,buf);
  }
  /* test ending of statement, the statement's
   *   structure is:
   *  insert <tbl> <value list>*/
  if (p>=(int)s.size()) {
    st = parent->op_lst.size()-1;
    /* change 'format' list type to 'value' type */
    if (is_type_equals(parent->op_lst[st]->type,
       m_list,s_fmt)) {
      mset(parent->op_lst[st]->type,m_list);
      sset(parent->op_lst[st]->type,s_val);
    }
    /* remove 'values' before subqueries in
     *  'value list' */
    RM_VAL(parent);
    return 1;
  }
  /* try if it's the 'on duplicate key update' list */
  parse_odku_stmt(parent,p);
  /* otherwise, the structure is: 
   *  insert <tbl> <format list> <value list> 
   * then parse the 'value' list */
  //fset(of_nt,s_val);
  parse_list(parent,s_val,p);
  //fclr(of_nt);
  /* remove 'values' before subqueries in
   *  'value list' */
  RM_VAL(parent);
  /* XXX: parse the possible 'where' list, for
   *  'insert' part of 'merge into' statements only */
  p = next_token(s,p,buf);
  if (!strcasecmp(buf,"where")) {
    parse_where_list(parent,p);
    return 1;
  }
  /* try if it's the 'on duplicate key update' list */
  parse_odku_stmt(parent,p);
  return 1;
}

int sql_tree::parse_casewhen_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  stxNode *tNode = 0, *node = 0, *pItem =0;

  /* test if the targeting endpoint 
   *  locates before 'when' */
  p = next_token(s,p,buf);
  if (strcasecmp(buf,"when")) {
    /* get targeting endpoint */
    tNode = parse_complex_item(p);
    p = next_token(s,p,buf);
  }
  /* skip the 'when' token */
  mov(p,buf);
  /* parse 'case...when' list */
  while (1) {
    /*
     * create 'case when' item node
     */
    pItem = create_node(0,m_cwa,s_cwa_item);
    attach(parent,pItem);
    /* parse item after 'when' */
    if (tNode) {
      /* construct expr node if 'case when' statement is:
       *  case <ep> when <condN> then ... */
      node = create_node(0,m_expr,s_eql);
      /* should use 'dup_tree' here to duplicate 
       *  a whole sub tree */
      attach(node,/*dup_node*/dup_tree(tNode));
      attach(node,parse_list_item(p));
    } else {
      /* in this case, the statement is:
       *  case when <expr> then... */
      node = parse_list_item(p);
    }
    /* attach 'expr' node with 'case when' node */
    attach(pItem,node);
    /* skip 'then' token */
    p = next_token(s,p,buf);
    mov(p,buf);
    /* parse item after 'then' */
    attach(pItem,parse_list_item(p));
    /* check ending of list and create 
     *  'else item' node */
    p = next_token(s,p,buf);
    if (strcasecmp(buf,"when")) {
      /* meets the statement end or the 
       *  'else' item */
      if (!strcasecmp(buf,"else")) {
        mov(p,buf);
        /* create 'else item' node */
        pItem = create_node(0,m_cwa,s_cwa_eitem);
        attach(parent,pItem);
        /* parse item after 'else' */
        attach(pItem,parse_list_item(p));
      }
      /* skip the 'end' token */
      p = next_token(s,p,buf);
      if (!strcasecmp(buf,"end")) {
        /* create 'end item' node */
        pItem = create_node(0,m_cwa,s_cwa_end);
        attach(parent,pItem);
        mov(p,buf);
      }
      /* remove the pre-allocated target 
       *  endpoint node */
      /*destroy_node*/destroy_tree(tNode);
      break ;
    }
    /* skip another 'when' token */
    mov(p,buf);
  } /* while(1) */
  return 1;
}

int sql_tree::parse_show_stmt(stxNode *parent, int &p)
{
  /* parse 'show <target>' list */
  parse_list(parent,s_show_lst,p);
  return 1;
}

int sql_tree::parse_desc_stmt(stxNode *parent, int &p)
{
  /* parse 'desc <target>' list */
  parse_list(parent,s_desc_lst,p);
  return 1;
}

int sql_tree::parse_simple_transac_stmt(stxNode *parent, int &p)
{
  /* parse commit/rollback */
  return 1;
}

int sql_tree::parse_create_tbl_additions(stxNode *parent, int &pos)
{
  std::string &s = sql_stmt ;

  while (pos<(int)s.size()) {

    stxNode *p = parse_list_item(pos);

    attach(parent,p);
  }
  return 0;
}

int sql_tree::parse_create_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt;
  
  p = next_token(s,p,buf);
  mov(p,buf);
  /* 'create table' command  */
  if (!strcasecmp(buf,"table")) {
    /* set parent node type */
    parent->type = mktype(m_stmt,s_cTbl);

    /* it's 'if not exists' */
    p = next_token(s,p,buf);
    if (!strcasecmp(buf,"if")) {
      /* skip 'if' */
      mov(p,buf);
      /* skip 'not' */
      p = next_token(s,p,buf);
      assert(!strcasecmp(buf,"not"));
      mov(p,buf);
      /* skip 'exists' */
      p = next_token(s,p,buf);
      assert(!strcasecmp(buf,"exists"));
      mov(p,buf);

      /* change parent's type: 
       *  'create table' -> 'create table if not exists' 
       */
      sset(parent->type,s_cTbl_cond);
    }

    /* table name */
    p = next_token(s,p,parent->name);
    mov(p,parent->name);

    /* skip '(' */
    p = next_token(s,p,buf);
    if (*buf=='(') {
      mov(p,buf);
      /* force to deal with the 'create' list */
      fset(of_ci,ci_ct);
      parse_list(parent,s_cd_lst,p);
      fset(of_ci,ci_all);

      /* skip ')' */
      p = next_token(s,p,buf);
      mov(p,buf);

      /* parse addition items after create list, 
       *  like 'engine', character, etc */
      parse_create_tbl_additions(parent,p);

    } else if (!strcasecmp(buf,"select")) {
      /* TODO: create table with 'select' */
    }

  }
  return 1;
}

int sql_tree::parse_drop_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "";
  std::string &s = sql_stmt;
  
  p = next_token(s,p,buf);
  mov(p,buf);
  /* 'drop table' command  */
  if (!strcasecmp(buf,"table")) {
    parent->type = mktype(m_stmt,s_dTbl);

    /* it's 'if exists' */
    p = next_token(s,p,buf);
    if (!strcasecmp(buf,"if")) {
      /* skip 'if' */
      mov(p,buf);
      /* skip 'exists' */
      p = next_token(s,p,buf);
      assert(!strcasecmp(buf,"exists"));
      mov(p,buf);

      /* change parent's type: 
       *  'drop table' -> 'drop table if xists' 
       */
      sset(parent->type,s_dTbl_cond);
    }

    /* parse drop list */
    parse_list(parent,s_dTbl_lst,p);
  }
  return 0;
}

int sql_tree::parse_truncate_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN]="";
  std::string &s = sql_stmt ;

  p = next_token(s,p,buf);
  /* skip the optional 'table' keyword */
  if (!strcasecmp(buf,"table")) {
    mov(p,buf);
  }
  /* parse the table list */
  parse_list(parent,s_trunc,p);
  return 1;
}

int sql_tree::parse_call_proc_stmt(stxNode *parent, int &p)
{
  stxNode *node = 0;

  node = parse_list_item(p);
  attach(parent,node);
  return 1;
}

int sql_tree::parse_setparam_stmt(stxNode *parent, int &p)
{
  stxNode *node = 0;

  node = parse_list_item(p);
  attach(parent,node);
  return 1;
}

int sql_tree::parse_alter_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN];
  stxNode *nd = 0;
  std::string &s = sql_stmt ;

  /* skip the keyword 'table' */
  p = next_token(s,p,buf);
  mov(p,buf);
  /* parse the 'alter table' name */
  nd = parse_complex_item(p);
  attach(parent,nd);
  /* parse the 'alter' target */
  nd = parse_list_item(p);
  attach(parent,nd);
  return 1;
}

int sql_tree::parse_mergeinto_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  stxNode *node = 0, *nd = 0;
  int st = s_none ;

  /* process 'source/dest' items */
  do {
    p = next_token(s,p,buf);
    mov(p,buf);
    st = !strcasecmp(buf,"into")?s_mia_dst:
      s_mia_src;
    /* create the 'src/dst attribute' node */
    nd = create_node(0,m_mia,st);
    attach(parent,nd);
    /* parse 'src/dest' target */
    node = parse_complex_item(p);
    attach(nd,node);
    /* process alias */
    p = next_token(s,p,buf);
    if (strcasecmp(buf,"using") && 
       strcasecmp(buf,"on")) {
      node = parse_alias(p);
      attach(nd,node);
      p = next_token(s,p,buf);
    }
  } while (strcasecmp(buf,"on")) ;
  /* skip 'on' token */
  mov(p,buf);
  /* process 'on' list */
  nd = parse_on_list(p);
  attach(parent,nd);
  /* 
   * parse 'when' items 
   */
  while (p<(int)s.size()) {
    /* skip 'when matched' tokens */
    p = next_token(s,p,buf);
    mov(p,buf);
    p = next_token(s,p,buf);
    mov(p,buf);
    if (!strcasecmp(buf,"matched")) {
      /* create the 'when matched' attribute node */
      nd = create_node(0,m_mia,s_mia_match);
      attach(parent,nd);
      /* skip 'then' token */
      p = next_token(s,p,buf);
      mov(p,buf);
      /* skip 'update' token */
      p = next_token(s,p,buf);
      mov(p,buf);
      /* parse the 'update set...' items behind as 
       *  'update' statement */
      parse_update_stmt(nd,p);
    } else {
      /* create the 'when not matched' attribute node */
      nd = create_node(0,m_mia,s_mia_unmatch);
      attach(parent,nd);
      /* skip 'matched then' tokens */
      p = next_token(s,p,buf);
      mov(p,buf);
      p = next_token(s,p,buf);
      mov(p,buf);
      /* skip 'insert' token */
      p = next_token(s,p,buf);
      mov(p,buf);
      /* parse the 'insert...' items behind as 
       *  'insert' statement */
      parse_insert_stmt(nd,p);
    }
  }
  return 1;
}

int sql_tree::parse_update_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] = "" ;
  std::string &s = sql_stmt ;
  //int st = 0;

  /* parse 'update' list */
  p = next_token(s,p,buf);
  /* XXX: this 'if' block is used to ignore the 'update'
   *  list processing in 'update' part of 'merge into' 
   *  statement, and to process those in normal 'update' 
   *  statements */
  if (strcasecmp(buf,"set")) {
    //fset(of_nt,s_upd);
    parse_list(parent,s_upd,p);
    //fclr(of_nt);
    p = next_token(s,p,buf);
  }
  /* skip the 'set' token */
  if (!strcasecmp(buf,"set")) {
    mov(p,buf);
  }
  /* parse 'set' list */
  //fset(of_nt,s_set);
  parse_list(parent,s_set,p);
  //fclr(of_nt);
  /* parse the 'where' list */
  if (!parse_where_list(parent,p)) {
    return 0;
  }
  return 1;
}

int sql_tree::parse_delete_stmt(stxNode *parent, int &p)
{
  char buf[TKNLEN] ;

  /* parse the 'from' list */
  if (!parse_from_list(parent,p)) {
    return 0;
  }
  /* test for 'using' list */
  p = next_token(sql_stmt,p,buf);
  if (!strcasecmp(buf,"using")) {
    mov(p,buf);
    attach(parent,parse_using_list(p));
  }
  /* parse the 'where' list */
  if (!parse_where_list(parent,p)) {
    return 0;
  }
  return 1;
}

int sql_tree::parse_select_stmt(stxNode *parent, int &p)
{
  /* the 'select' list */
  if (!parse_list(parent,s_sel,p)) {
    return 0;
  }
  /* the 'from' list */
  if (!parse_from_list(parent,p)) {
    return 0;
  }
  /* try extra elements such as 'order by',
   *  'for update', etc */
  parse_flexible_items(parent,p);
  /* test if the statement has a 'where' clouse 
   *  or not, process the 'where' list if it has */
  if (!parse_where_list(parent,p)) {
    return 0;
  }
  /* try extra elements such as 'order by',
   *  'for update', etc */
  parse_flexible_items(parent,p);
  return 1;
}

int sql_tree::parse_withas_stmt(stxNode *parent, int &p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;
  stxNode *node = 0, *tmp = 0;
  
  /* parse the 'wa item' list */
  while (1) {
    /* create the attribute node */
    node = create_node(0,m_waa,s_waa_item);
    /* parse the 'name' */
    tmp = parse_endpoint_item(p);
    attach(node,tmp);
    /* skip the 'as' token */
    p = next_token(s,p,buf);
    mov(p,buf);
    /* parse the 'item' */
    tmp = parse_complex_item(p);
    attach(node,tmp);
    /* attach attribute node to parent */
    attach(parent,node);
    /* check if 'wa item' list ends */
    p = next_token(s,p,buf);
    if (*buf!=',') {
      break ;
    }
    p++ ;
  }
  /* parse main statement */
  node = parse_complex_item(p);
  /* attach node to parent */
  attach(parent,node);
  return 1;
}

bool sql_tree::is_stmt(int p)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;

  next_token(s,p,buf);
  return 
    !strcasecmp(buf,"select") ||
    /* statement 'with as' */
    !strcasecmp(buf,"with")   ||
    !strcasecmp(buf,"insert") ||
    !strcasecmp(buf,"delete") ||
    !strcasecmp(buf,"case")   ||
    !strcasecmp(buf,"merge")  ||
    !strcasecmp(buf,"alter")  ||
    !strcasecmp(buf,"set")    ||
    !strcasecmp(buf,"update") ||
    !strcasecmp(buf,"truncate")||
    !strcasecmp(buf,"show")   ||
    !strcasecmp(buf,"desc")   ||
    !strcasecmp(buf,"rollback")||
    !strcasecmp(buf,"commit") ||
    !strcasecmp(buf,"create") ||
    !strcasecmp(buf,"drop")   ||
    !strcasecmp(buf,"call") ;
}

stxNode* sql_tree::parse_stmt(int &pos)
{
  stxNode *node = 0;
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "" ;

  if (!is_stmt(pos)) {
    printd("fatal: expects a statement\n");
    return NULL ;
  }
  pos = next_token(s,pos,buf);
  mov(pos,buf);
  /* process statement by types */
  if (!strcasecmp(buf,"select")) {
    /* 'select' statement */
    node = create_node(0,m_stmt,s_select);
    parse_select_stmt(node,pos);
  } else if (!strcasecmp(buf,"with")) {
    /* 'with as' statement */
    node = create_node(0,m_stmt,s_withas);
    parse_withas_stmt(node,pos);
  } else if (!strcasecmp(buf,"insert")) {
    /* 'insert' statement */
    node = create_node(0,m_stmt,s_insert);
    parse_insert_stmt(node,pos);
  } else if (!strcasecmp(buf,"delete")) {
    /* 'delete' statement */
    node = create_node(0,m_stmt,s_delete);
    parse_delete_stmt(node,pos);
  } else if (!strcasecmp(buf,"update")) {
    /* 'update' statement */
    node = create_node(0,m_stmt,s_update);
    parse_update_stmt(node,pos);
  } else if (!strcasecmp(buf,"case")) {
    /* 'case when' statement */
    node = create_node(0,m_stmt,s_casew);
    parse_casewhen_stmt(node,pos);
  } else if (!strcasecmp(buf,"merge")) {
    /* 'merge into' statement */
    node = create_node(0,m_stmt,s_merge);
    parse_mergeinto_stmt(node,pos);
  } else if (!strcasecmp(buf,"alter")) {
    /* 'alter' statement */
    node = create_node(0,m_stmt,s_alter);
    parse_alter_stmt(node,pos);
  } else if (!strcasecmp(buf,"set")) {
    /* 'set xx=yy' statement */
    node = create_node(0,m_stmt,s_setparam);
    parse_setparam_stmt(node,pos);
  } else if (!strcasecmp(buf,"call")) {
    /* 'call procedure()' statement */
    node = create_node(0,m_stmt,s_callproc);
    parse_call_proc_stmt(node,pos);
  } else if (!strcasecmp(buf,"truncate")) {
    /* 'truncate' statement */
    node = create_node(0,m_stmt,s_truncate);
    parse_truncate_stmt(node,pos);
  } else if (!strcasecmp(buf,"show")) {
    /* 'show' statement */
    node = create_node(0,m_stmt,s_show);
    parse_show_stmt(node,pos);
  } else if (!strcasecmp(buf,"desc")) {
    /* 'desc' statement */
    node = create_node(0,m_stmt,s_desc);
    parse_desc_stmt(node,pos);
  } else if (!strcasecmp(buf,"commit")) {
    /* 'commit' statement */
    node = create_node(0,m_stmt,s_commit);
    parse_simple_transac_stmt(node,pos);
  } else if (!strcasecmp(buf,"rollback")) {
    /* 'rollback' statement */
    node = create_node(0,m_stmt,s_rollback);
    parse_simple_transac_stmt(node,pos);
  } else if (!strcasecmp(buf,"create")) {
    /* 'create xxx' statement */
    node = create_node(0,m_stmt,0);
    parse_create_stmt(node,pos);
  } else if (!strcasecmp(buf,"drop")) {
    /* 'drop xxx' statement */
    node = create_node(0,m_stmt,0);
    parse_drop_stmt(node,pos);
  } else {
    printd("unknown statement type '%s'\n", buf);
  }
  return node;
}

stxNode* sql_tree::parse_stmt_set(int lt,int &pos)
{
  std::string &s = sql_stmt ;
  char buf[TKNLEN] = "";
  stxNode *node = 0, *top = 0 ;
  int s_type = s_none;

  do {
    /* process one single sub statement */
    parse_list(NULL,lt,pos,&node);
    if (top) {
      attach(top,node);
      node= top ;
      top = NULL;
    }
    /* test for a 'parallel' statement set */
    pos = next_token(s,pos,buf);
    if (strcasecmp(buf,"union")) {
      break ;
    }
    s_type = s_union ;
    mov(pos,buf);
    /* test for 'union all' */
    pos = next_token(s,
      pos,buf);
    if (!strcasecmp(buf,"all")) {
      s_type = s_uniona ;
      mov(pos,buf);
    }
    /* attach sub statement to statement set */
    top = create_node(0,m_expr,s_type) ;
    attach(top,node);
  } while (1) ;
  return node ;
}

stxNode* sql_tree::build_tree(std::string &stmt)
{
  int pos = 0;
  stxNode *node = 0, *t_root = 0 ;

  reset();
  /* get target statement */
  sql_stmt = stmt ;
  /* create tree root */
  t_root = create_node(0,m_root,0);
  if (!t_root) {
    return NULL;
  }
  /* try the statement set */
  node = parse_stmt_set(s_norm,pos);
  /* attach statements to root */
  attach(t_root,node);
  return t_root;
}

#if TREE_NON_RECURSION==0
void sql_tree::destroy_tree(
  stxNode *ptr, 
  bool bSelf /* also remove the node itselft */
  )
{
  uint16_t i=0;

  if (!ptr) {
    return ;
  }
  for (i=0; i<ptr->op_lst.size(); i++) {
    destroy_tree(ptr->op_lst[i]) ;
  }
  ptr->op_lst.clear();
  /* also destroy the node itself */
  if (bSelf) {
    destroy_node(ptr);
  }
}
#else
void sql_tree::destroy_tree(
  stxNode *ptr,
  bool bSelf /* also remove the node itselft */
  )
{
#if 0
#define RM_LEAF(p,r) do {\
  uint8_t i = 0; \
  stxNode *tn = p->parent ;\
  if (tn&&!r) { \
    i = get_parent_pos(p); \
    tn->op_lst.erase(tn->op_lst.begin()+i); \
    if (!stk.empty()) {_np = stk.top()-1;stk.pop();} \
    destroy_node(p); \
    p= tn ; } \
  else { destroy_node(p);p=0;}\
} while(0)
#else 
  auto RM_LEAF = [&](auto &p, auto &r) {
    uint8_t i = 0;
    stxNode *tn = p->parent ;

    if (tn&&!r) {
      i = get_parent_pos(p);
      tn->op_lst.erase(tn->op_lst.begin()+i);
      if (!stk.empty()) {
        _np = stk.top()-1;stk.pop();
      }
      destroy_node(p);
      p= tn ; 
    }
    else destroy_node(p);p=0;
  } ;
#endif
  stxNode *nd = ptr;
  int _np = -1;
  std::stack<int> stk ;

  for (;;) {
    /* try next child */
    _np = _np<((int)ptr->op_lst.size()-1)?
      (_np+1):-1;
    if (_np<0) {
      if (!stk.empty()) {
        _np = stk.top(); 
        stk.pop(); 
      }
      /* stop here if reaches back to root
       *  and no chilren should be processed */
      if (ptr==nd)  break ;
    } else {
      ptr = ptr->op_lst[_np];
      stk.push(_np); 
      _np = -1; 
    }
    /* remove leavies */
    if (ptr&&ptr->op_lst.size()<=0 &&
       (ptr!=nd || bSelf)) {
      RM_LEAF(ptr,(ptr==nd));
    }
  } /* end for */
  /* remove the root if possible */
  if (bSelf && ptr==nd) {
    RM_LEAF(nd,true);
  }
}
#endif

#if 1
void sql_tree::print_tree(stxNode *ptr,int level)
{
  uint16_t i=0,l=0;

  if (!ptr) {
    return ;
  }
  /* dump the tree node */
  for (l=0,printf("%d ", level);l<level;
    l++,printf("-")) ;
  printf(" (name: %s, type<%s|%s>, child(%d)) [%p, %p]\n",
    ptr->name, 
    main_type_str(ptr->type),
    sub_type_str(ptr->type),
    (int)ptr->op_lst.size(),
    ptr,ptr->parent);
  for (i=0; i<ptr->op_lst.size(); i++) {
    print_tree(ptr->op_lst[i],level+1) ;
  }
}
#else
void sql_tree::print_tree(stxNode *ptr, int level)
{
  uint8_t dir = 1; /* tree traverse directtion: 0 up, 1 down */
  uint16_t l=0;
  int _np = -1;
  std::stack<int> stk ;

  for (;ptr&&(dir||_np>=0);) {
    /* dump the tree node */
    if (dir) {
      for (l=0,printf("%d ", level);l<level;
        l++,printf("-")) ;
      printf(" (name: %s, type<%s|%s>, child(%d)) [%p, %p]\n",
        ptr->name, 
        main_type_str(ptr->type),
        sub_type_str(ptr->type),
        (int)ptr->op_lst.size(),
        ptr,ptr->parent);
    }
    /* try next node */
    _np = _np<((int)ptr->op_lst.size()-1)?(_np+1):-1;
    if (_np<0) {
      if (!stk.empty()) {
        _np = stk.top(); 
        stk.pop(); 
      }
      ptr = ptr->parent ;
      dir = 0;
      level--;
    } else {
      ptr = ptr->op_lst[_np]; 
      stk.push(_np); 
      _np = -1; 
      dir = 1;
      level++;
    }
  } /* end for */
}
#endif


/*
 * class tree_serializer
 */
int tree_serializer::eliminate_comments(std::string &stmt)
{
  int depth = 0;
  std::string &s = stmt ;
  std::string::size_type sp = 0, pos = 0;
  char buf[TKNLEN] = "";

  /* eliminate nesting comments */
  for(;pos<s.size();) {
    pos = next_token(s,static_cast<uint16_t>(pos),buf);
    /* get beginning */
    if (*buf=='/'&& ((pos+1)<s.size()) && s[pos+1]=='*') {
      if (!depth++) sp = pos ;
      pos ++;
    } else if (*buf=='*'&& ((pos+1)<s.size()) && s[pos+1]=='/') {
      /* get ending */
      if (!(--depth)) {
        stmt.erase(sp,pos-sp+2);
        pos = sp-2;
      }
      pos ++;
    }
    mov(pos,buf);
  }
  /* error */
  if (depth) {
    printd("incorrect multi-line comment\n");
    return 0;
  }
  return 1;
}

int tree_serializer::add_op(stxNode *node,uint16_t pos)
{
  /* add operator for binary expression */
  int cnt = node->op_lst.size();
  std::string &s = out_stmt ;
  stxNode *nxt = 0;

  if (pos>=cnt) {
    return 0;
  }
  /* process the 'oracle X join' operator */
  if (is_type_equals(node->type,m_expr,s_olj) ||
     is_type_equals(node->type,m_expr,s_orj)) {
    /* left join */
    if (sget(node->type)==s_olj) {
      s = !pos?(s+"(+)="):s;
    } else {
      /* right join */
      s += !pos?"=":"(+) " ;
    }
    return 1;
  } 
  /* don't add operator for the 
   *  last operand */
  if (pos==(cnt-1)) {
    return 0;
  }
  nxt = node->op_lst[pos+1];
  /* sql left/right/inner/cross join expression */
  if (is_type_equals(node->type,m_expr,s_ljoin)||
     is_type_equals(node->type,m_expr,s_rjoin) ||
     is_type_equals(node->type,m_expr,s_ijoin) ||
     is_type_equals(node->type,m_expr,s_cjoin) ||
     is_type_equals(node->type,m_expr,s_join)) {
    /* add operator behind operand that stays before
     *  items which shouldn't be alias. For the 'Xjoin' 
     *  operators, the next operand should not 
     *  be 'on' or 'using' list */
    if (!is_type_equals(nxt->type,m_endp,s_alias) &&
       !is_type_equals(nxt->type,m_list,s_on) && 
       !is_type_equals(nxt->type,m_list,s_using)) {
      s+= sub_type_str(node->type);
      s+= " ";
    }
  } else {
    /* normal binary expression, add operator 
     *  before 1st operand */
    if (!pos) {
      s+= " ";
      s+= sub_type_str(node->type);
      s+= " ";
    }
  }
  return 1;
}

int tree_serializer::add_comma(
  stxNode *node, /* list node object */
  uint16_t pos   /* position of current list item */
  )
{
  stxNode *p = 0, *nxt = 0;
  std::string &s = out_stmt ;
  int cnt = node->op_lst.size();

  if (pos>=cnt) {
    return 0;
  }
  /* it's senseless to add comman to list
   *  that has less items than 2 */
  if (cnt<2) {
    return 0;
  }
  /* no need to add comma at the end of 
   *  list */
  if (pos==(cnt-1)) {
    return 0;
  }
  /* get the current/next list item */
  p  = node->op_lst[pos];
  nxt= node->op_lst[pos+1];
  /* 'order by' list */
  if (is_type_equals(node->type,m_list,s_ob)) {
    /* for order by list, add commas before 
     *  items that are NOT keywords */
    if (mget(nxt->type)!=m_keyw) 
      s += ",";
  } 
  /* 'with as' statement list */
  else if (is_type_equals(node->type,m_stmt,s_withas)) {
    /* add comma behind each 'with as'
     *  attribute items */
    if (is_type_equals(nxt->type,m_waa,s_waa_item))
      s += ",";
  } else {
    /* other lists */
    if (!is_type_equals(nxt->type,m_endp,s_alias) &&
       mget(p->type)!=m_keyw) 
      /* add comma behind those not 'keywords' and
       *  next item of which in list is not alias */
      s += "," ;
  }
  return 1;
}

int tree_serializer::serialize_ph(stxNode *node)
{
  bool bColon = false ;
  stxNode *p = 0;
  std::string &s = out_stmt ;
  int i = 0, cnt = node->op_lst.size();
  int pos = 0;

  s += ":" ;
  pos= s.size()-1;
  /* get name attribute */
  for (;i<cnt;i++) {
    p = node->op_lst[i] ;
    if (is_type_equals(p->type,m_pha,s_pha_name)) {
      s += p->name ;
      break ;
    }
  }
  s += "<" ;
  /* get type attribute */
  for (i=0;i<cnt;i++) {
    p = node->op_lst[i] ;
    if (is_type_equals(p->type,m_pha,s_pha_t)) {
      s += p->name ;
      break ;
    }
  }
  /* process 'char' type especially */
  if (!strcasecmp(p->name,"char")) {
    /* for query mode, add '' pair onto char-type
     *  place holders */
    if (!bPrep) {
      s.insert(pos,"\'");
      bColon = true ;
    }
    /* get size attribute */
    for (i=0;i<cnt;i++) {
      p = node->op_lst[i] ;
      if (is_type_equals(p->type,m_pha,s_pha_sz)) {
        break ;
      }
    }
    if (i<cnt) {
      s += "[" ;
      s += p->name ;
      s += "]" ;
    }
  }
  /* get direction attribute */
  for (i=0;i<cnt;i++) {
    p = node->op_lst[i] ;
    if (is_type_equals(p->type,m_pha,s_pha_dir)) 
      break ;
  }
  /* 'direction' is optional */
  if (i<cnt) {
    s += ",";
    s += p->name;
  }
  s += ">";
  /* for query mode, add '' pair onto char-type
   *  place holders */
  if (bColon) {
    s += "\'";
  }
  s += " ";
  return 1;
}

/* add 'then' in 'case when' items of 
 *  statements */
int tree_serializer::add_cw(
  stxNode *node, /* list node object */
  uint16_t pos   /* position of current list item */
  )
{
  std::string &s = out_stmt ;

  /* add 'then' just behind the
   *  'case when' condition */
  if (!pos) {
    s += " then ";
  }
  return 1;
}

/* add 'as' in 'with as' statements */
int tree_serializer::add_wa(
  stxNode *node, /* list node object */
  uint16_t pos   /* position of current list item */
  )
{
  std::string &s = out_stmt ;

  /* add 'as' just behind the
   *  'with as' name */
  if (!pos) {
    s += " as ";
  }
  return 1;
}

/* add 'using' in 'merge into' statements */
int tree_serializer::add_using(
  stxNode *node, /* list node object */
  uint16_t pos   /* position of current list item */
  )
{
  std::string &s = out_stmt ;

  /* do 'add 'using'' operation behind the
   *  'dst' item of 'merge into' statement */
  if (1==pos) {
    s += " using ";
  }
  return 1;
}

int tree_serializer::serialize_tree(stxNode *node)
{
  /* latency processing to list items */
  using fLProc = int(tree_serializer::*)(stxNode*,uint16_t);
  uint16_t i = 0;
  std::string &s = out_stmt ;
  int mt = s_none, st = s_none ;
  bool bBckPair = false ;/* add '()' pair */
  fLProc lp = 0;

  if (!node) {
    return 0;
  }
  switch (mt=mget(node->type)) {
    /* 'for udpate' attributes */
    case m_fua:
      switch(st=sget(node->type)) {
        case s_fua_wait:
        case s_fua_nowait:
          s += sub_type_str(node->type);
          s += " ";
          if (st==s_fua_wait)
            s += node->name;
          s += " ";
          break ;
        case s_fua_oc:
          break ;
      }
      break ;
    /* 'merge into' attributes */
    case m_mia:
      /* 'matched/unmatched' attributes */
      if (sget(node->type)==s_mia_match ||
         sget(node->type)==s_mia_unmatch) {
        s += sub_type_str(node->type);
        s += " ";
      } else if (sget(node->type)==s_mia_dst) {
        /* do 'add 'using'' operation behind the
         *  'dst' item of 'merge into' statement */
        lp = &tree_serializer::add_using;
      }
      break ;
    /* 'case when' attributes */
    case m_cwa:
      s += sub_type_str(node->type);
      s += " ";
      if (sget(node->type)==s_cwa_item) {
        lp = &tree_serializer::add_cw ;
      }
      break ;
    /* 'with as' attributes */
    case m_waa:
      /* add 'as' for each 'with as' attribute 
       *  items */
      if (sget(node->type)==s_waa_item) {
        lp = &tree_serializer::add_wa;
      }
      break ;
    /* create table column definition type */
    case m_cdt:
      {
        int t = sget(node->type);

        if (t==cda_col || t==cda_col_type) {
          s += node->name ;
          s += " ";
        }
      }
      break ;
    /* statement type */
    case m_stmt:
      /* do 'add comma' operation for each 
       *  attribute items of 'with as' statements */
      if (sget(node->type)==s_withas) {
        lp = &tree_serializer::add_comma;
      }
    /* uanry expression type */
    case m_uexpr:
    /* keywords */
    case m_keyw:
      {
        s += sub_type_str(node->type);
        s += " ";

        /* special for 'create table' */
        if (node->type==mktype(m_stmt,s_cTbl) || 
           node->type==mktype(m_stmt,s_cTbl_cond) ||
           node->type==mktype(m_keyw,s_default_val) ||
           node->type==mktype(m_keyw,s_key) ||
           node->type==mktype(m_keyw,s_idx) ||
           node->type==mktype(m_keyw,s_comment) ||
           node->type==mktype(m_keyw,s_primary_key) 
           ) {
          s += node->name;
          s += " ";
        }
      }
      break ;
    /* endpoint type */
    case m_endp:
      /* for place holders */
      if (sget(node->type)==s_ph) {
        serialize_ph(node);
        break ;
      } 
      /* for other endpoint types */
      s += node->name;
      /* add dot symbol for 'table/schema' */
      st = sget(node->type) ;
      s += (st==s_tbl||st==s_schema)?".":
        st==s_func?"":" ";
      break ;
    /* list type */
    case m_list:
      switch (sget(node->type)) {
        /* index list */
        case s_index:
        /* create table: column definition list */
        case s_cd_lst:
        /* 'format' list of insert */
        case s_fmt:
        /* add '()' pair for 'arg list' */
        case s_arg:
          /* do commas adding */
          lp = &tree_serializer::add_comma;
          bBckPair = true ;
          break ;
        /* 'from' list */
        case s_norm:
          bBckPair = true ;
          /* add commas for normal list of 
           *  'in' expression */
          if (node->parent && 
             (node->parent->type==mktype(m_expr,s_in) ||
             node->parent->type==mktype(m_expr,s_not_in))) {
            lp = &tree_serializer::add_comma;
          }
          break ;
        /* reference list in 'foreign key' creation */
        case s_ref_lst:
        /* having list */
        case s_hav:
        /* the start with & connect by list */
        case s_sw:
        case s_cb:
        /* the 'on' list */
        case s_on:
        /* the 'where list' */
        case s_where:
          s += sub_type_str(node->type);
          s += " ";
          break ;
        /* 'using' list in 'delete' statement */
        case s_using:
          bBckPair = true ;
        /* 'on duplicate key update' of insert statement */
        case s_odku:
        /* limit list */
        case s_limit:
        /* 'order by' list */
        case s_ob:
        /* 'set' list in 'update' statement */
        case s_set:
        /* add 'from' token before 'from list' */
        case s_from:
          s += sub_type_str(node->type);
          s += " ";
        /* drop table list */
        case s_dTbl_lst:
        /* select list */
        case s_sel:
        /* update list */
        case s_upd:
          /* do commas adding */
          lp = &tree_serializer::add_comma;
          break ;
        /* 'value' list of insert */
        case s_val:
        /* 'for update of' list */
        case s_fuoc:
          s += sub_type_str(node->type);
          s += " ";
          /* add '()' pair */
          bBckPair = true ;
          /* do commas adding */
          lp = &tree_serializer::add_comma;
          break ;
        /* 'group by' list */
        case s_gb:
          s += sub_type_str(node->type);
          s += " ";
          /* do commas adding */
          lp = &tree_serializer::add_comma;
          break ;
      }
      break ;
    case m_expr:
      /* add operators for 'binary expressions' */
      lp = &tree_serializer::add_op ;
      break ;
  }
  /* 
   * iterates children 
   */
  s += bBckPair?"(":"" ;
  for (i=0;i<node->op_lst.size();i++) {
    /* process one child */
    serialize_tree(node->op_lst[i]);
    /* do latency processing for 
     *  list items */
    if (lp) {
      (this->*lp)(node,i);
    }
  }
  s += bBckPair?") ":"" ;
  return 1;
}

int tree_serializer::do_serialize(stxNode *t, 
  std::string &sout)
{
  int ret = 0;

  reset();
  ret = serialize_tree(t);
  sout= out_stmt ;
  return ret ;
}

void tree_serializer::reset(void)
{
  out_stmt = "";
}

