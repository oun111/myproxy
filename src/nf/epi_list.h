
#ifndef __EPI_LIST_H__
#define __EPI_LIST_H__

#include <map>
#include <vector>
#include "lock.h"

using sock_toolkit = struct tSockToolkit;

using VP_TYPE = std::pair<sock_toolkit*,int> ;

using ITRM = std::map<sock_toolkit*,int>::iterator;

using ITRV = std::vector<VP_TYPE>::iterator;

/* epoll item list */
class epi_list {
protected:
  pthread_rwlock_t m_lk ;
#define try_read_lock()    nativeRDLock __rlock(m_lk) 
#define try_write_lock()   nativeWRLock __wlock(m_lk) 

protected:

  std::map<sock_toolkit*,int> m_list;
  std::vector<VP_TYPE> m_vec ;

public:
  epi_list() ;
  ~epi_list() ;

protected:
  int get(sock_toolkit *k, ITRM &itr) ;

  int insert(sock_toolkit *k, int v) ;

  int pop(sock_toolkit *&k,int &v);

  void clear(void);

public:

  int new_ep(sock_toolkit*);

  int get_idle(sock_toolkit* &k);

  int return_back(sock_toolkit* &k);

  void debug(void);

} ;

#endif /* __EPI_LIST_H__*/

