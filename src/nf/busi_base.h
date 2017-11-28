
#ifndef __BUSI_BASE_H__
#define __BUSI_BASE_H__

#include "framework.h"

class business_base {
public:
  std::string m_desc ;

public:
  business_base(void) {
  }
  /* get module descriptions */
  char* get_desc(void) { return (char*)m_desc.c_str(); }

public:
  virtual int rx(sock_toolkit* st,epoll_priv_data* priv,int fd) = 0;

  virtual int tx(sock_toolkit* st,epoll_priv_data* priv,int fd) = 0;

  virtual int on_error(sock_toolkit* st,int fd) = 0;
} ;

#endif /* __BUSI_BASE_H__ */

