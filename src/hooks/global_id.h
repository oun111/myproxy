
#ifndef __GLOBAL_ID_H__
#define __GLOBAL_ID_H__

#include "hook.h"
#include "container_impl.h"
#include "dbwrapper.h"


class id_cache {

private:
  /*unqlite_db*/ /*b_db*/ leveldb_db m_db ;

  std::string m_cacheDesc ;

  pthread_mutex_t m_lk ;
  #define try_lock()   nativeMutex __rlock(m_lk) 

private:
  void lock_init(void);
  void lock_release(void) ;

public:
  id_cache(const char*) ;
  ~id_cache(void) ;

public:
  /* increment/decrement for int val */
  int int_fetch_and_add(char *key, int &val);
} ;

/* hook global id without placeholder in statement */
class sql_tree;
class global_id_hook : public hook_object {

private:
  id_cache m_cache ;

  safeGlobalIdColumnList &m_gidConf ;

public:
  global_id_hook(void); 
  ~global_id_hook(void) ;

protected:
  int get_table_name(stxNode*,char*&,char*&);

  int get_target_col_in_tree(sql_tree&,stxNode*, char*);

  int deal_target_col(stxNode *pv, int pos, int gid);

  int add_target_col(sql_tree&,stxNode *pf, stxNode *pv, char *col, int gid);

  int eliminate_norm_list(sql_tree &,stxNode *node);

  int update(sql_tree&,stxNode*,stxNode*,const char*,const char*);

public:
  int run_hook(sql_tree&,stxNode *root,void *param) ;
} ;


#endif /* __GLOBAL_ID_H__*/

