
#ifndef __ADAPTORS_H__
#define __ADAPTORS_H__

#include "sql_tree.h"

class attribute_list;


using namespace std;

class adaptor {
public:
  adaptor() {}
  virtual ~adaptor() {}

public:
  /* adapting olracle specified language to mysql's */
  virtual int do_adaption(std::string&, 
    class attribute_list*l=NULL, 
    const bool=true) = 0;
} ;

class mysql_adpt : protected tree_serializer, public adaptor
{
/* tell the caller to re-scan the tree */
const int  REDO_HIERARCH =  2;
private:
  using arg_pair = std::pair<std::string,std::string> ;
  using adpt_cb =  int(mysql_adpt::*)(stxNode*,int);
  using tAdptAct = struct adpt_act_t {
    public:
    /* choose type/key value to
     *  do matching */
    uint8_t sel ;
    /* key or type values to be matched */
    union {
      int32_t t;
      char k[TKNLEN] ;
    } u ;
    /* callback function to process the 
     *  interest key or type value */
    adpt_cb func ;
  } ;

  /* root of the syntax tree */
  stxNode *root ;
  /* selector type */
  enum st {
    t_type,
    t_key
  } ;
  /* the adaption action list */
  std::vector<shared_ptr<tAdptAct> > a_list ;
  /* especially for 'char-type-placeholder' adaptions */
  tAdptAct* ac_ph ;

public:
  /* translate the tree to mysql-style syntax */
  //int mysqlize_tree(stxNode*,int,bool=false);
  int mysqlize_tree(stxNode*,int);

  void register_adpt_actions(void);
  void unregister_adpt_actions(void);

  /* gather place holder informations from tree */
  int get_placeholder_info(stxNode*,attribute_list*);

  int do_placeholder_adpt(stxNode*);

protected:
  int do_mysqlization(stxNode*,int,tAdptAct*);
  /* adaption with 'nextval' */
  int adpt_nextval(stxNode*,int);
  /* adaption with 'currval' */
  int adpt_currval(stxNode*,int);
  /* adaption with 'nvl' */
  int adpt_nvl(stxNode*,int);
  /* add quote onto mysql keywords */
  int adpt_add_quote(stxNode*,int);
  /* adaption with 'rownum' */
  int adpt_rownum(stxNode*,int);
  /* adpation with oracle '+=' operator */
  int adpt_peql(stxNode*,int);
  /* adpation with oracle 'sysdate' */
  int adpt_sysdate(stxNode*,int);
  /* translate oracle date format to mysql's */
  int __date_fmt_o2m(char*);
  /* adpation with oracle 'to_char' */
  int adpt_tochar(stxNode*,int);
  /* adpation with oracle 'trunc' */
  int adpt_trunc(stxNode*,int);
  /* adpation with oracle 'to_number' */
  int adpt_to_num(stxNode*,int);
  /* adpation with oracle 'substr' */
  int adpt_substr(stxNode*,int);
  /* adpation with oracle 'regexp_like' */
  int adpt_relike(stxNode*,int);
  /* adpation with oracle object name 'dbms_lob' */
  int adpt_dbms_lob(stxNode*,int);
  /* adpation with oracle 'dummy' */
  int adpt_dummy(stxNode*,int);
  /* adpation with oracle 'for update wait/nowait/of' */
  int adpt_4update_attr(stxNode*,int);
  /* adpation with oracle 'limit' */
  int adpt_limit(stxNode*,int);
  /* adaption with 'insert' */
  int adpt_insert(stxNode*,int);
  /* adaption with expression 'in' */
  int adpt_expr_in(stxNode*,int);
  /* adaption with 'delete' */
  int adpt_delete(stxNode*,int);
  /* adaption with 'update' */
  int adpt_update(stxNode*,int);
  /* adaption with 'select' */
  int adpt_select(stxNode*,int);
  /* adaption with 'with...as' */
  int adpt_withas(stxNode*,int);
  /* adaption with 'union/union all' */
  int adpt_union(stxNode*,int);
  /* adaption with oracle hierarchal machanism */
  int adpt_hierarchal(stxNode*,int);
  /* adaption with 'place holders' */
  int adpt_place_holder(stxNode*,int);
  /* adaption with 'merge into' */
  int adpt_merge_into(stxNode*, int);
  /* adaption with 'normal list' */
  int adpt_normal_list(stxNode*, int);
  /* adaption with 'oracle (+)' */
  int adpt_ora_joins(stxNode*,int);
  int __find_in_from_list(stxNode*, char*);
  int __find_in_join_expr(stxNode*, char*);
  /* process the oracle 'join' operation */
  int __ojoin2join(stxNode*,stxNode*,int,bool=false);
  int __process_ora_join(stxNode*,stxNode*);
  int __join_proc_method1(stxNode*,stxNode*,int,int);
  int __join_proc_method2(stxNode*,stxNode*,uint32_t);
  //int __join_proc_case2(stxNode*,stxNode*,int,int,bool);
  /* split 'start with' sub tree */
  int __split_sw_tree(stxNode*,stxNode**,stxNode**) ;
  /* fill 'where' tree with sub trees */
  int __join_where_clouse(stxNode*,stxNode*,
    stxNode*,stxNode*);
  /* generate target related stuffs */
  stxNode* __gen_where_stuff(stxNode*,char*,bool);
  stxNode* __create_inherit_nodes(char*,char*);
  stxNode* __create_empty_select_tree(bool,bool,bool);
  stxNode* __create_binary_nodes(int,stxNode*,stxNode*);
  stxNode* __create_dummy_start_with(stxNode*,char*);
  inline int __add_rn_wrapper(stxNode*,char*);
  inline int __add_rn_inc(stxNode*,char*);
  inline int __add_rn_init(stxNode*,char*);
  inline int __process_normal_rn(stxNode*,char* /*,char**/);
  inline int __add_rn_limit(stxNode*,stxNode*,char*);
  inline int __add_col_def(stxNode*,stxNode*);
  inline int __rm_col_prefix(stxNode*);
  inline int __rm_dup_cols(stxNode*);
  inline int __mod_node_alias(stxNode*,stxNode*);
  inline int __process_list_items(stxNode*,stxNode*);
  inline int __remove_prefix(stxNode*,int);
  /* replace alias with node name in tree */
  inline int __replace_alias(stxNode*,
    std::vector<arg_pair>&);
  /* add a wrapper onto sub query */
  inline int __add_subqry_wrapper(stxNode*,
    std::vector<arg_pair>&,int=0);
  /* add '()' pair onto sub queries */
  inline int __add_norm_list(stxNode*);
  /* add alias name sub queries */
  inline int __add_alias(stxNode*);
  /* expand the 'with as' item in statement */
  inline int __spread_wa_item(stxNode*, stxNode*);
  /* register adaption actions */
  int do_action_register(char*,adpt_cb);
  int do_action_register(uint32_t,adpt_cb);
  void reset(void);

public:
  mysql_adpt();
  ~mysql_adpt();

public:
  /* adapting olracle specified language to mysql's */
  virtual int do_adaption(std::string&, 
    class attribute_list*l=NULL, 
    const bool=true);
} ;


#endif /* __ADAPTORS_H__*/
