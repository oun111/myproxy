
#ifndef __ID_CACHE_H__
#define __ID_CACHE_H__

#include "dbwrapper.h"
#include "lock.h"
#include "id_cache.h"

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

protected:
  int insert_new_unsafe(char *key,int val);

public:
  /* increment/decrement for int val */
  int int_fetch_and_add(char *key, int &val);
  int int_fetch_and_sub(char *key, int &val);
} ;

#endif /* __ID_CACHE_H__*/

