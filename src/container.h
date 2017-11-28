
#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <vector>
#include <queue>
#include <unordered_map>
#include <map>
#include "lock.h"

template <typename __key_t, typename __val_t>
class safe_container_base
{
protected:
  pthread_rwlock_t m_lk ;
  #define try_read_lock()   nativeRDLock __rlock(m_lk) 
  #define try_write_lock()  nativeWRLock __wlock(m_lk) 
  //#define unlock()

public:
  //using ITR_TYPE = typename boost::unordered_map<__key_t,__val_t>::iterator ;
  using ITR_TYPE = typename std::unordered_map<__key_t,__val_t>::iterator ;

protected:
  //boost::unordered_map<__key_t,__val_t> m_list ;
  std::unordered_map<__key_t,__val_t> m_list ;

protected:
  void lock_init(void)   { 
    pthread_rwlock_init(&m_lk,0); 
  }
  void lock_release(void) {
    pthread_rwlock_destroy(&m_lk);
  }

protected:
  int insert(__key_t k,__val_t v) 
  {
    m_list[k] = v;
    return 0;
  }
  int drop(__key_t k)
  {
    m_list.erase(k);
    return 0;
  }

#define list_foreach_container_item    \
  for (auto i: m_list)

public:
  __val_t find(__key_t k)
  {
    ITR_TYPE i;

    i = m_list.find(k);
    return (i==m_list.end())?0:i->second;
  }
  __val_t next(ITR_TYPE &i, bool bStart=false)
  {
    try_read_lock();
    if (bStart) i = m_list.begin();
    else i++ ;
    return i!=m_list.end()?i->second:NULL ;
  }
  __val_t next(ITR_TYPE &i, __key_t &k, bool bStart=false)
  {
    {
      try_read_lock();
      if (bStart) i = m_list.begin();
      else i++ ;
    }
    try_read_lock();
    k = i==m_list.end()?0:i->first ;
    return i!=m_list.end()?i->second:NULL ;
  }
  size_t size(void) {
    return m_list.size();
  }
} ;

template <typename __val_t>
class safe_vector_base
{
protected:
  pthread_rwlock_t m_lk ;
  #define try_read_lock()   nativeRDLock __rlock(m_lk) 
  #define try_write_lock()  nativeWRLock __wlock(m_lk) 
  //#define unlock()

public:
  using ITR_TYPE = typename std::vector<__val_t>::iterator ;

protected:
  std::vector<__val_t> m_list ;

protected:
  void lock_init(void)   { 
    pthread_rwlock_init(&m_lk,0); 
  }
  void lock_release(void) {
    pthread_rwlock_destroy(&m_lk);
  }

protected:
  int insert(__val_t v) 
  {
    m_list.push_back(v);
    return 0;
  }
  int drop(int i)
  {
    m_list.erase(m_list.begin()+i);
    return 0;
  }
#define list_foreach_vector_item    \
  for (uint16_t i=0; i<m_list.size();i++)  

public:
  __val_t find(size_t i)
  {
    return (i>=m_list.size())?0:m_list[i];
  }
  int next(ITR_TYPE &i, bool bStart=false) 
  {
    //i = bStart?m_list.begin():++i ;
    if (bStart) i = m_list.begin();
    else ++i ;
    if (i==m_list.end())
      return -1;
    return 0;
  }
  size_t size(void)
  {
    return m_list.size();
  }
} ;


template <typename __val_t>
class safe_queue_base
{
protected:
  pthread_rwlock_t m_lk ;
  #define try_read_lock()   nativeRDLock __rlock(m_lk) 
  #define try_write_lock()  nativeWRLock __wlock(m_lk) 
  //#define unlock()

protected:
  std::queue<__val_t> m_queue ;

protected:
  void lock_init(void)   { 
    pthread_rwlock_init(&m_lk,0); 
  }
  void lock_release(void) {
    pthread_rwlock_destroy(&m_lk);
  }
  void push(__val_t v) 
  {
    m_queue.push(v);
  }
  void pop(void)
  {
    m_queue.pop();
  }
  __val_t& top(void)
  {
    return m_queue.front();
  }
} ;


#endif /* __CONTAINER_H__*/
