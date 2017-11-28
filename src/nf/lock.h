
#ifndef __LOCK_H__
#define __LOCK_H__

/* normal mutex */
class nativeMutex {
protected:
  pthread_mutex_t &m_ ;
public:
  nativeMutex(pthread_mutex_t &m):m_(m)  {  do_lock(); }
  ~nativeMutex(void)      { do_unlock(); }
public:
  void do_lock(void)      { pthread_mutex_lock(&m_); }
  void do_unlock(void)    { pthread_mutex_unlock(&m_); }
  void init_lock(void)    { pthread_mutex_init(&m_,0); }
  void destroy_lock(void) { pthread_mutex_destroy(&m_);}
} ;

/* read lock */
class nativeRDLock {
protected:
  pthread_rwlock_t &m_;
public:
  nativeRDLock(pthread_rwlock_t &m):m_(m) { do_lock(); }
  ~nativeRDLock(void)     { do_unlock(); }
public:
  void do_lock(void)      { pthread_rwlock_rdlock(&m_); }
  void do_unlock(void)    { pthread_rwlock_unlock(&m_); }
  void init_lock(void)    { pthread_rwlock_init(&m_,0); }
  void destroy_lock(void) { pthread_rwlock_destroy(&m_); }
} ;

/* write lock */
class nativeWRLock {
protected:
  pthread_rwlock_t &m_;
public:
  nativeWRLock(pthread_rwlock_t &m):m_(m) { do_lock(); }
  ~nativeWRLock(void)     { do_unlock(); }
public:
  void do_lock(void)      { pthread_rwlock_wrlock(&m_); }
  void do_unlock(void)    { pthread_rwlock_unlock(&m_); }
  void init_lock(void)    { pthread_rwlock_init(&m_,0); }
  void destroy_lock(void) { pthread_rwlock_destroy(&m_); }
} ;


#endif /* __LOCK_H__*/
