
#ifndef __SQL_TREE_H__
#define __SQL_TREE_H__

#include <vector>
#include <stack>
#include <string>

struct SyntaxTreeNode {
  char name[TKNLEN];
  int32_t type ;
  std::vector<struct SyntaxTreeNode*> op_lst ;
  struct SyntaxTreeNode *parent ;
} ;
using stxNode = struct SyntaxTreeNode ;
using p_stxNode = struct SyntaxTreeNode** ;


namespace stree_types {
  /* type main/sub bits read/write operations */
#define mget(t)     (((uint32_t)t>>10)&0x3ff)
#define sget(t)     (((uint32_t)t)&0x3ff)
#define mset(t,v)   (t=(((uint32_t)t&~(0x3ff<<10))|((v&0x3ff)<<10)))
#define sset(t,v)   (t=(((uint32_t)t&~0x3ff)|(v&0x3ff)))
#define mktype(m,s) (((m&0x3ff)<<10)|(s&0x3ff))

  /* main syntax node types */
  enum mtype { 
    m_root, m_stmt, m_expr, 
    m_uexpr, /* unary expression */
    m_endp, m_list, 
    m_pha, /* place holder attribute */
    m_fua, /* 'for update' attribute */
    m_waa, /* 'with as' attribute */
    m_cwa, /* 'case when' attribute */
    m_mia, /* 'merge into' attribute */
    m_keyw,/* key words */
    m_cdt, /* create table: column definition types */
  };
  /* sub syntax node types */
  enum pha_type { 
    s_pha_name,/* place holder attribute: name */
    s_pha_t,   /* place holder attribute: type */
    s_pha_sz,  /* place holder attribute: size, if needed */
    s_pha_dir, /* place holder attribute: direction, if needed */
  };
  enum fua_type {
    s_fua_nowait, /* 'for update' attribute: nowait */
    s_fua_wait,   /* 'for update' attribute: wait */
    s_fua_oc,     /* 'for update' attribute: of columns */
  } ;
  enum waa_type {
    s_waa_item,   /* 'with as' attribute: wa item */
  } ;
  enum cwa_type {
    s_cwa_item,   /* 'case when' attribute: 'when' item */
    s_cwa_eitem,  /* 'case when' attribute: 'else' item */
    s_cwa_end,    /* 'case when' attribute: 'end' item */
  } ;
  enum mia_type {
    s_mia_dst,    /* the destination */
    s_mia_src,    /* the source */
    s_mia_match,  /* the action when matched */
    s_mia_unmatch,/* the action when unmatched */
  } ;
  /*  create table: column definition attributes */
  enum cda_type {
    cda_col,
    cda_col_type,
    //cda_primary_key_name, 
    //cda_key_name,
    //cda_index_name,
    /* TODO: check() */
  } ;
  enum stmt_type { s_select, s_insert, s_delete, s_update, 
    s_withas, /* statement type 'with as' */
    s_casew,  /* statement type 'case when' */
    s_merge,  /* statement type 'merge into' */
    s_alter,  /* statement type 'alter' */
    s_setparam, /* statement type 'set' */
    s_callproc,   /* statement type 'procedure' */
    s_truncate, /* statement type 'truncate' */
    s_show, /* statement type 'show' */
    s_desc, /* statement type 'desc' */
    s_commit,/* transaction control */
    s_rollback,
    s_cTbl, /* create table */
    s_cTbl_cond, /* create table if not exists */
    s_dTbl, /* drop table */
    s_dTbl_cond, /* drop table if exists */
  } ;
  /* unary expressions */
  enum uexpr_type {
    s_exts,   /* expr 'exists' */
    s_not_ext,/* expr 'not exists' */
    s_pir,    /* expr 'prior' in 'connect by' */
  } ;
  /* binary expressions */
  enum expr_type {  
    s_not,    /* flag 'not' */
    s_union,  /* union */
    s_uniona, /* union all */
    s_colon,  /* synbol ':' */
    s_ex_mark,/* symbol '!' */
    s_lbkt,   /* the left bracket '(' */
    s_btw,    /* operator 'between' */
    s_ljoin,  /* join operator 'left join' */
    s_join,   /* join operator 'join' */
    s_ijoin,  /* join operator 'inner join' */
    s_cjoin,  /* join operator 'cross join' */
    s_rjoin,  /* join operator 'right join' */
    s_bit_and,/* bit operator '&' */
    s_bit_or, /* bit operator '|' */
    s_bit_lshift, /* bit operator '<<' */
    s_bit_rshift, /* bit operator '>>' */
    s_bit_xor,    /* bit operator '^' */
    s_bit_reverse,/* bit operator '~' */
    s_and,    /* logic operator 'and' */
    s_or,     /* logic operator 'or' */
    s_bt,     /* logic operator '>' */
    s_be,     /* logic operator '>=' */
    s_lt,     /* logic operator '<' */
    s_le,     /* logic operator '<=' */
    s_ne1,    /* logic operator '!=' */
    s_ne,     /* logic operator '<>' */
    s_olj,    /* oracle left join (+) */
    s_orj,    /* oracle right join (+) */
    s_eql,    /* operator '=' */
    s_eql1,   /* operator ':=' */
    s_peql,   /* operator '+=' */
    s_in,     /* operator 'in' */
    s_not_in, /* operator 'not in' */
    s_like,   /* operator 'like' */
    s_escape, /* operator 'like' */
    s_rexp,   /* operator 'regexp' */
    s_rexpb,  /* operator 'regexp binary' */
    s_is,     /* operator 'is' */
    s_cat,    /* operator '||' */
    s_not_is, /* operator 'is not' */
    s_plus, s_reduce, s_multiply, s_mod, s_devide, /* mathematic opt */
    expr_max,
  };
  enum keyw_type {
    /* keywords in 'select' list */
    s_hint, s_all, s_distinct, s_unique, s_wildcast, 
    s_asc, /* keyword 'asc' in 'order by' list */
    s_dsc, /* keyword 'desc' in 'order by' list */
    s_fu,  /* keyword 'for update' */
    s_auto_inc,  /* keyword 'auto_increment' */
    s_null, /* keyword 'null' */
    s_not_null, /* keyword 'not null' */
    s_default_val, 
    s_primary_key, /* keyword 'primary key' */
    s_key,/* keyword 'key' */
    s_idx,/* keyword 'index' */
    s_unique_index,/* keyword 'unique index' */
    s_foreign_key,/* keyword 'foreign key' */
    s_comment, /* keyword 'comment' */
  } ;
  enum endp_type { 
    s_tbl, s_schema, s_func, 
    s_c_str,  /* string constant */
    s_c_int,  /* integer constant */
    s_c_float,/* floating point constant */
    s_col, s_alias, 
    s_ph,  /* place holder */
  } ;
  enum list_type { s_sel, s_from, s_where, 
    s_using,/* 'using' list in 'delete' statement */
    s_upd,  /* update list in 'update' statement */
    s_set,  /* set list in 'update' statement */
    s_fmt,  /* format list in 'insert' */
    s_val,  /* value list in 'insert' */
    s_arg,  /* function argument list */
    s_on,   /* left/right join ... on ... */
    s_ob,   /* order by */
    s_gb,   /* group by */
    s_hav,  /* having  */
    s_fuoc, /* for update of <columns> */ 
    s_limit,/* limit x[,y] */
    s_sw,   /* start with */
    s_cb,   /* connect by */
    s_odku, /* on duplicate key update */
    s_norm, /* normal list except above types */
    s_trunc,/* truncate list */
    s_show_lst,  /* the show list */
    s_desc_lst,  /* the desc list */
    s_cd_lst,  /* create definition list */
    s_cd_item, /* create definition item */
    s_index,  /* the key/index list */
    s_ref_lst, /* reference list in 'foreign key' creation */
    s_dTbl_lst, /* drop table list */
  };
  /* sub node type 'none' */
#define s_none /*(1<<30)*/1024
  /* dump main node type in text */
#define main_type_str(t)  (               \
  mget(t)==m_root?"root":                 \
  mget(t)==m_stmt?"statement":            \
  mget(t)==m_expr?"expression":           \
  mget(t)==m_uexpr?"unary expression":    \
  mget(t)==m_endp?"endpoint":             \
  mget(t)==m_list?"list":                 \
  mget(t)==m_pha?"placeholder attribute": \
  mget(t)==m_fua?"for update attribute":  \
  mget(t)==m_waa?"with as attribute":     \
  mget(t)==m_cwa?"case when attribute":   \
  mget(t)==m_mia?"merge into attribute":  \
  mget(t)==m_keyw?"keyword":              \
  mget(t)==m_cdt?"column definition":     \
  "main type n/a"                         \
)
  /* dump the sub node type in text */
#define sub_type_str(t) (             \
  /* statement types */               \
  mget(t)==m_stmt?                    \
   (sget(t)==s_select?"select":       \
   sget(t)==s_insert?"insert into":   \
   sget(t)==s_delete?"delete":        \
   sget(t)==s_update?"update":        \
   sget(t)==s_withas?"with":          \
   sget(t)==s_casew?"case":           \
   sget(t)==s_merge?"merge into":     \
   sget(t)==s_alter?"alter table":    \
   sget(t)==s_setparam?"set":         \
   sget(t)==s_callproc?"call":        \
   sget(t)==s_truncate?"truncate":    \
   sget(t)==s_show?"show":            \
   sget(t)==s_desc?"desc":            \
   sget(t)==s_commit?"commit":        \
   sget(t)==s_rollback?"rollback":    \
   sget(t)==s_cTbl?"create table":    \
   sget(t)==s_cTbl_cond?"create table if not exists":\
   sget(t)==s_dTbl?"drop table":    \
   sget(t)==s_dTbl_cond?"drop table if exists":\
   "stmt type n/a"):                  \
  mget(t)==m_expr?                    \
   /* expression types */             \
   (sget(t)==s_plus?"+":              \
   sget(t)==s_reduce?"-":             \
   sget(t)==s_multiply?"*":           \
   sget(t)==s_devide?"/":             \
   sget(t)==s_mod?"%":                \
   sget(t)==s_eql?"=":                \
   sget(t)==s_eql1?":=":              \
   sget(t)==s_peql?"+=":              \
   sget(t)==s_in?"in":                \
   sget(t)==s_not_in?"not in":        \
   sget(t)==s_bit_and?"&":            \
   sget(t)==s_bit_or?"|":             \
   sget(t)==s_bit_lshift?"<<":        \
   sget(t)==s_bit_rshift?">>":        \
   sget(t)==s_bit_xor?"^":            \
   sget(t)==s_and?"and":              \
   sget(t)==s_or?"or":                \
   sget(t)==s_bt?">":                 \
   sget(t)==s_lt?"<":                 \
   sget(t)==s_be?">=":                \
   sget(t)==s_le?"<=":                \
   sget(t)==s_ne1?"!=":               \
   sget(t)==s_ne?"<>":                \
   sget(t)==s_ljoin?"left join":      \
   sget(t)==s_join?"join":            \
   sget(t)==s_ijoin?"inner join":     \
   sget(t)==s_cjoin?"cross join":     \
   sget(t)==s_rjoin?"right join":     \
   sget(t)==s_olj?"ora left join":    \
   sget(t)==s_orj?"ora right join":   \
   sget(t)==s_like?"like":            \
   sget(t)==s_escape?"escape":        \
   sget(t)==s_rexp?"regexp":          \
   sget(t)==s_rexpb?"regexp binary":  \
   sget(t)==s_is?"is":                \
   sget(t)==s_cat?"||":               \
   sget(t)==s_btw?"between":          \
   sget(t)==s_not_is?"is not":        \
   sget(t)==s_not?"not":              \
   sget(t)==s_union?"union":          \
   sget(t)==s_uniona?"union all":     \
   "expr type n/a"):                  \
  /* unary expression types */        \
  mget(t)==m_uexpr?                   \
   (sget(t)==s_exts?"exists":         \
   sget(t)==s_not_ext?"not exists":   \
   sget(t)==s_pir?"prior":            \
   sget(t)==s_bit_reverse?"~":        \
   "unary expr type n/a"):            \
  /* end point types */               \
  mget(t)==m_endp?                    \
   (sget(t)==s_c_str?"string constant":         \
   sget(t)==s_c_int?"integer constant":         \
   sget(t)==s_c_float?"floating point constant":\
   sget(t)==s_col?"column":           \
   sget(t)==s_alias?"alias":          \
   sget(t)==s_tbl?"table":            \
   sget(t)==s_schema?"schema":        \
   sget(t)==s_func?"function":        \
   sget(t)==s_ph?"place holder":      \
   "endpoint type n/a"):              \
  /* list types */                    \
  mget(t)==m_list?                    \
   (sget(t)==s_sel?"select":          \
   sget(t)==s_from?"from":            \
   sget(t)==s_using?"using":          \
   sget(t)==s_where?"where":          \
   sget(t)==s_upd?"update list":      \
   sget(t)==s_set?"set":              \
   sget(t)==s_fmt?"format list":      \
   sget(t)==s_val?"values":           \
   sget(t)==s_arg?"argument list":    \
   sget(t)==s_on?"on":                \
   sget(t)==s_ob?"order by":          \
   sget(t)==s_gb?"group by":          \
   sget(t)==s_hav?"having":           \
   sget(t)==s_fuoc?"of":              \
   sget(t)==s_limit?"limit":          \
   sget(t)==s_sw?"start with":        \
   sget(t)==s_cb?"connect by":        \
   sget(t)==s_odku?"on duplicate key update":\
   sget(t)==s_norm?"normal list":     \
   sget(t)==s_trunc?"truncate list":  \
   sget(t)==s_show_lst?"show list":   \
   sget(t)==s_desc_lst?"desc list":   \
   sget(t)==s_cd_lst?"column def list":  \
   sget(t)==s_cd_item?"column def item":  \
   sget(t)==s_index?"index list":     \
   sget(t)==s_ref_lst?"references":   \
   sget(t)==s_dTbl_lst?"drop table list":\
   "list type n/a"):                  \
  mget(t)==m_pha?                     \
   (sget(t)==s_pha_name?"name":       \
   sget(t)==s_pha_t?"type":           \
   sget(t)==s_pha_sz?"size":          \
   sget(t)==s_pha_dir?"direction":    \
   "attr type n/a"):                  \
  mget(t)==m_fua?                     \
   (sget(t)==s_fua_wait?"wait":       \
   sget(t)==s_fua_nowait?"nowait":    \
   sget(t)==s_fua_oc?"column list":   \
   "attr type n/a"):                  \
  mget(t)==m_waa?                     \
   (sget(t)==s_waa_item?"wa item":    \
   "attr type n/a"):                  \
  mget(t)==m_cwa?                     \
   (sget(t)==s_cwa_item?"when":       \
   sget(t)==s_cwa_eitem?"else":       \
   sget(t)==s_cwa_end?"end":          \
   "attr type n/a"):                  \
  mget(t)==m_mia?                     \
   (sget(t)==s_mia_dst?"dest item":   \
   sget(t)==s_mia_src?"source item":  \
   sget(t)==s_mia_match?"when matched then update":      \
   sget(t)==s_mia_unmatch?"when not matched then insert":\
   "attr type n/a"):                  \
  mget(t)==m_keyw?                    \
   (sget(t)==s_hint?"hint":           \
   sget(t)==s_all?"all":              \
   sget(t)==s_asc?"asc":              \
   sget(t)==s_dsc?"desc":             \
   sget(t)==s_distinct?"distinct":    \
   sget(t)==s_unique?"unique":        \
   sget(t)==s_wildcast?"*":           \
   sget(t)==s_fu?"for update":        \
   sget(t)==s_auto_inc?"auto_increment": \
   sget(t)==s_null?"null":            \
   sget(t)==s_not_null?"not null":    \
   sget(t)==s_default_val?"default":  \
   sget(t)==s_primary_key?"primary key": \
   sget(t)==s_key?"key":              \
   sget(t)==s_idx?"index":            \
   sget(t)==s_unique_index?"unique":  \
   sget(t)==s_foreign_key?"foreign key":\
   sget(t)==s_comment?"comment":      \
   "attr type n/a"):                  \
  mget(t)==m_cdt?                     \
   (sget(t)==cda_col?"col name":      \
   sget(t)==cda_col_type?"col type":  \
   "attr type n/a"):                  \
  /* unknown types */                 \
  main_type_str(t)                    \
)
} ;

/* 
 * sql language adaption using syntax 
 *  tree machanism 
 */
class sql_tree {

//protected:
public:
  /* option flag that controls which complex item 
   *  type is to be processed */
  enum ci_type {
    ci_nest = 1<<0, /* nesting type */
    ci_stmt = 1<<1, /* statement type */
    ci_join = 1<<2, /* 'join' type */
    ci_phd  = 1<<3, /* place holder type */
    ci_norm = 1<<4, /* normal type */
    ci_ury  = 1<<5, /* 'unary expr' type */
    ci_func = 1<<6, /* function type */
    ci_ct   = 1<<7, /* create table type */
    ci_all  = 0xfff,/* process all */
  } ;
  /* option flag types */
  enum of_type {
    of_ci,   /* complex item options */
    /*of_nt,*/   /* node type */
    of_max,
  } ;
  /* option flag structure definition */
  using tOptFlag = struct tOptionFlag {
    uint32_t f;
    uint32_t v[of_max];
  } ;
#define fset(_f,_v) do{             \
  fc_flag.f   |= (1<<(_f)) ;       \
  fc_flag.v[_f] = (uint32_t)(_v) ; \
} while(0)
#define fget(_f,_v,_s) do{          \
  _s = !!((fc_flag.f)&(1<<(_f)));  \
  _v = _s?fc_flag.v[_f]:_v;        \
} while(0)
#define fclr(_f) do{                \
  fc_flag.f &= ~(1<<(_f)) ;        \
} while(0)
  /* move pointer by length of token */
#define mov(_p_,_t_)  ((_p_)+=strlen((_t_)))

  /* get list-ending-testing function */
#define get_end_func(t) (              \
  (t)==s_sel||(t)==s_arg||(t)==s_norm||\
   (t)==s_dTbl_lst?                    \
   &sql_tree::is_sel_list_ends:        \
    (t)==s_from?                       \
   &sql_tree::is_frm_list_ends:        \
    (t)==s_show_lst||(t)==s_desc_lst|| \
    (t)==s_where || (t)==s_trunc?      \
   &sql_tree::is_where_list_ends:      \
    (t)==s_ob||(t)==s_fuoc||           \
    (t)==s_limit||(t)==s_sw||          \
    (t)==s_cb||(t)==s_gb||(t)==s_hav?  \
   &sql_tree::is_ob_list_ends:         \
    (t)==s_fmt||(t)==s_val||(t)==s_odku? \
   &sql_tree::is_fmt_list_ends:        \
    (t)==s_upd||(t)==s_set?            \
   &sql_tree::is_update_list_ends:     \
    (t)==s_cd_lst||(t)==s_index?       \
   &sql_tree::is_ct_list_ends:         \
   NULL                                \
)
  /* get list-keyword-parsing function */
#define get_kwp_func(t) (              \
  (t)==s_sel || (t)==s_arg?            \
   &sql_tree::parse_sl_keys:           \
    (t)==s_ob?                         \
   &sql_tree::parse_ob_keys:           \
   NULL                                \
)

private:
  /* the target sql statement */
  std::string sql_stmt ;
  /* priority definition of operators */
  uint16_t prio_lst[stree_types::expr_max];
  /* the option flag that passed over
   *  function call chains */
  tOptFlag fc_flag ;

public:
  sql_tree(void);
  ~sql_tree(void);

public:
  /* construct the syntax tree by inputed 
   *   sql statement */
  stxNode* build_tree(std::string&);
  /* print a whole tree from inputed node */
  void print_tree(stxNode*,int);
  /* release a whole tree from inputed node */
  void destroy_tree(stxNode*,bool=true);
  /* attach child node to parent node */
  void attach(stxNode*,stxNode*,int=-1);
  /* detach child node from its upper parent */
  void detach(stxNode*,uint16_t);
  /* get parent position number at upper list */
  int get_parent_pos(stxNode*);
  /* construct a single syntax tree node */
  stxNode* create_node(char*,int,int);
  /* find a token within tree */
   stxNode* find_in_tree(stxNode*,int);
  /*bool*/stxNode* find_in_tree(stxNode*,char*);

protected:
  /* print the content of a tree node */
  void dump_node(stxNode*) ;
  /* duplicates a given node */
  stxNode* dup_node(stxNode*);
  /* duplicates a given sub tree */
  stxNode* dup_tree(stxNode*);
  /* compare 2 given trees */
  int cmp_tree(stxNode*,stxNode*);
  /* destroy a single node */
  void destroy_node(stxNode*);
  /* remove redundant list nodes in given type */
  void remove_redundant_nodes(stxNode**,int);
  /* test if 'target' type is equals to 
   *  the given type */
  bool is_type_equals(uint32_t,uint32_t,
    uint32_t) ;

private:
  /* reset related variables */
  void reset(void);
  /* process 'where' list in all statements */
  int parse_where_list(stxNode*,int&);
  /* process 'from' list in all statements */
  int parse_from_list(stxNode*,int&);
  /* process 'having' list in all statements */
  int parse_having_list(stxNode*,int&);
  /* process 'order by' list in all statements */
  int parse_ob_list(stxNode*,int&);
  /* process 'group by' list in all statements */
  int parse_gb_list(stxNode*,int&);
  /* process the 'for update' element  */
  int parse_for_update(stxNode*,int&);
  /* process the 'limit x[,y]' element  */
  int parse_limit_list(stxNode*,int&);
  /* process items locates flexibly in statements, 
   *  such as 'order by', 'group by', etc */
  int parse_flexible_items(stxNode*,int&);
  /* process the 'start with' clouse */
  int parse_start_with(stxNode*,int&);
  /* process the 'connect by' clouse */
  int parse_connect_by(stxNode*,int&);
  /* test for statement */
  bool is_stmt(int);
  stxNode* create_lp_node(stxNode*, 
    std::stack<int>&, std::stack<stxNode*>&);
  /* initialize the priority group */
  void init_prio_group(void);
  /* test string for numerics */
  bool is_numeric_str(char*);
  /* check if the 1st's priority is 
   *  larger equal to 2nd one */
  bool is_prio_le(uint32_t,uint32_t);
  /* check statement for oracle 'join' operation */
  bool is_ora_join(int);
  /* parse the normal 'left/right join' items */
  bool is_join_item(int);
  /* check for join items in nesting subqueries */
  int str_test_subquery(int,int&);
  bool has_join_in_subquery(int);
  bool has_stmt_set(int);
  stxNode* parse_join_item(int&);
  stxNode* parse_on_list(int&);
  stxNode* parse_using_list(int&);
  /* parse the place holders */
  bool is_place_holder(int&);
  stxNode* parse_place_holder(int&);
  /* process complex select list item, such as
   *  expressions or sub statements */
  stxNode* parse_complex_item(int&);
  /* parse the key/index item */
  int parse_index_items(stxNode*,const char*,int&);
  /* parse normal col def items, such as a normal column definition */
  int parse_normal_ct_item(stxNode*,const char*,int&);
  /* parse 'create table' item */
  stxNode* parse_ct_item(int&);
  /* copy string constant */
  int copy_const_str(char*,int&);
  /* process the basic item in 'select list' */
  stxNode* parse_endpoint_item(int&/*,bool=true*/);
  /* skip the column type declaration */
  int suppress_column_type_decl(int&);
  /* parse the unary expressions */
  bool is_unary_expr(int);
  stxNode* parse_unary_expr(int&);
  /* get operator type at current position */
  int get_op_type(int&);
  /* process a normal select list item */
  stxNode* parse_list_item(int&);
  /* process an alias of anything */
  stxNode* parse_alias(int&);
  /* test ending of an end point */
  bool is_endp_ends(char*) ;
  /* test ending of a select list */
  bool is_sel_list_ends(char*,int&) ;
  /* test ending of a from list */
  bool is_frm_list_ends(char*,int&) ;
  /* test ending of a where list */
  bool is_where_list_ends(char*,int&) ;
  /* test ending of a 'order by' list */
  bool is_ob_list_ends(char*,int&) ;
  /* test ending of a 'xx join on' list */
  bool is_on_list_ends(char*,int&) ;
  /* test ending of a 'format' list of
   *  'insert' statement */
  bool is_fmt_list_ends(char*,int&) ;
  /* test ending of a 'update' list of
   *  'update' statement */
  bool is_update_list_ends(char*,int&) ;
  /* test ending of a 'create table' list */
  bool is_ct_list_ends(char*,int&) ;
  /* parse keywrods in select list */
  stxNode* parse_sl_keys(char*,int&);
  /* parse keywrods in 'order by' list */
  stxNode* parse_ob_keys(char*,int&);
  /* process the complete select list */
  int parse_list(stxNode*,int,int&,p_stxNode=/*NULL*/0);
  /* syntax processing on 'select' statement */
  int parse_select_stmt(stxNode*,int&);
  /* syntax processing on 'with as' statement */
  int parse_withas_stmt(stxNode*,int&);
  /* syntax processing on 'insert' statement */
  int parse_insert_stmt(stxNode*,int&);
  /* process 'on duplicate key update' of
   *  'insert' statement */
  int parse_odku_stmt(stxNode*,int&);
  /* syntax processing on 'delete' statement */
  int parse_delete_stmt(stxNode*,int&);
  /* syntax processing on 'update' statement */
  int parse_update_stmt(stxNode*,int&);
  /* syntax processing on 'case when' statement */
  int parse_casewhen_stmt(stxNode*,int&);
  /* syntax processing on 'merge into' statement */
  int parse_mergeinto_stmt(stxNode*,int&);
  /* syntax processing on 'alter' statement */
  int parse_alter_stmt(stxNode*,int&);
  /* syntax processing on 'set xx=yy' statement */
  int parse_setparam_stmt(stxNode*,int&);
  /* syntax processing on 'call procedure()' statement */
  int parse_call_proc_stmt(stxNode*,int&);
  /* syntax processing on 'truncate' statement */
  int parse_truncate_stmt(stxNode*,int&);
  /* syntax processing on 'show' statement */
  int parse_show_stmt(stxNode*,int&);
  /* syntax processing on 'desc' statement */
  int parse_desc_stmt(stxNode*,int&);
  /* syntax processing on simple transaction control statement */
  int parse_simple_transac_stmt(stxNode*,int&);
  /* syntax processing on a 'single' statement */
  stxNode* parse_stmt(int&);
  /* process statement set such as 'union [all]' */
  stxNode* parse_stmt_set(int,int&);
  /* syntax processing on 'create xxx' statement */
  int parse_create_tbl_additions(stxNode*,int&);
  int parse_create_stmt(stxNode*,int &);
  /* syntax processing on 'drop xxx' statement */
  int parse_drop_stmt(stxNode*,int&);
} ;

class tree_serializer : public sql_tree {

private:
  /* the output statement */
  std::string out_stmt ;

protected:
  /* indicates the sql prepare mode or not */
  bool bPrep ;

public:
  int do_serialize(stxNode*,std::string&);
  int eliminate_comments(std::string&);

private:
  void reset(void);
  /* convert syntax tree to string */
  int serialize_tree(stxNode*);
  /* serialize the place holder */
  int serialize_ph(stxNode*);
  /* add commas in suitable place of list */
  int add_comma(stxNode*,uint16_t);
  /* add 'as' token for 'with as' attribute list */
  int add_wa(stxNode*,uint16_t);
  /* add 'using' token for 'merge into' statement */
  int add_using(stxNode*,uint16_t);
  /* add binary operator at expression */
  int add_op(stxNode*,uint16_t);
  /* add 'then' token for 'case when' attribute 
   *  of statement */
  int add_cw(stxNode*,uint16_t);
} ;


#endif /* __SQL_TREE_H__*/

