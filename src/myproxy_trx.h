
#ifndef __MYPROXY_TRX_H__
#define __MYPROXY_TRX_H__

#include "container_impl.h"
#include "dbg_log.h"
#include "sock_toolkit.h"



enum myproxy_state {
  MP_ERR = -1,
  MP_OK  =  0,
  MP_FURTHER = 1, /* needed further processing */
  MP_IDLE = 2, /* nothing to do */
} ;

const int nMaxRecvBlk = 10000 ;

class myproxy_epoll_trx {

public:
  int rx(int,epoll_priv_data*,char*&,size_t&);

  int rx_blk(int,epoll_priv_data*,char*,ssize_t&,const size_t=nMaxRecvBlk);

  void end_rx(char*&);

  int tx(int,char*,size_t);
};

#endif /* __MYPROXY_TRX_H__*/

