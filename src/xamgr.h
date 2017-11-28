
#ifndef __XA_MANAGER_H__
#define __XA_MANAGER_H__

#include <string>
#include <memory>
#include <stdint.h>
#include "myproxy_config.h"
#include "xa_item.h"
#include "sock_toolkit.h"
#include "dnmgr.h"

class xamgr {

protected:
  safeXAList m_xaList ;

  /* the datanode groups */
  dnmgr m_dnMgr ;

public:
  xamgr(void) ;
  virtual ~xamgr(void) ;

public:

  /* acquire xa channel, begins a new transaction */
  int begin(
    int cfd,  /* client fd */
    void *parent,
    int &xid, /* the unique xa id */
    int = 0
  );

  /* send requests to xa channel */
  int execute(sock_toolkit*,int,int,std::vector<uint8_t>&, 
    char *req, size_t szReq, void*, void*);

  /* call back on request's execution complete */
  int end_exec(xa_item*);

  /* release the xa channel */
  int release(int xid);

  /* get xa item */
  xa_item* get_xa(int xid);

  dnmgr& get_dnmgr(void);
} ;

#endif /* __XA_MANAGER_H__*/

