
#ifndef __MP_TRX_H__
#define __MP_TRX_H__

#include "ctnr_impl.h"
#include "dbg_log.h"
#include "sock_toolkit.h"



enum myproxy_state {
  MP_ERR = -1,
  MP_OK  =  0,
  MP_FURTHER = 1, /* needed further processing */
  MP_IDLE = 2, /* nothing to do */
} ;

constexpr int nMaxRecvBlk = 20000 ;

class mp_trx {

protected:
  int triger_tx(int fd);

public:
  int rx(sock_toolkit *st, epoll_priv_data* priv, int fd);

  int tx(int,char*,size_t);

  int flush_tx(int);
};

#endif /* __MP_TRX_H__*/

