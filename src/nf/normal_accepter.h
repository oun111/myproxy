
#ifndef __NORMAL_ACCEPTER_H__
#define __NORMAL_ACCEPTER_H__

#include "busi_base.h"
#include "sock_toolkit.h"


class normal_accepter : public business_base
{
protected:
  /* client data */
  void *m_priv ;

  //epoll_priv_data *m_ep;

public:
  normal_accepter(void*) ;
  virtual ~normal_accepter(void) ;

public:
  int rx(sock_toolkit* st,epoll_priv_data* priv,int fd) ;
  int tx(sock_toolkit* st,epoll_priv_data* priv,int fd) ;
  int on_error(sock_toolkit* st,int fd) { return 0; }
} ;


#endif /* __NORMAL_ACCEPTER_H__*/
