
#ifndef __DUMMY_BUSINESS_H__
#define __DUMMY_BUSINESS_H__

#include "busi_base.h"
#include "dummy_proto.h"


class dummy_busi : public business_base
{
public:
  dummy_busi(char*desc) { m_desc = desc; }
public:
  int rx(sock_toolkit *st,epoll_priv_data* priv, int fd) {
    printf("rx invoke with %d\n",fd);
    return -1;
  }
  int tx(sock_toolkit *st,epoll_priv_data* priv, int fd) {
    printf("tx invoke with %d\n",fd);
    return 0;
  }
  int on_error(sock_toolkit *st,int fd) {
    return 0;
  }
} ;

#endif /* __DUMMY_BUSINESS_H__ */

