
#ifndef __CACHE_WRAPPER_H__
#define __CACHE_WRAPPER_H__

#include "dbmgr.h"
#include "resizable_container.h"
#include "myproxy_trx.h"

class xa_item ;
class myproxy_backend ;
class cache_wrapper
{
  dbmgr m_cachePool ;

  myproxy_backend &m_parent ;

public:
  //cache_wrapper() = default;
  cache_wrapper(myproxy_backend&);

public:
  bool is_needed(xa_item *xai);

  int acquire_cache(xa_item *xai);

  void return_cache(xa_item *xai);

  int cache_row(xa_item *xai, tContainer & odrCol, char *res, size_t sz);

  int extract_cached_rows(safeClientStmtInfoList&,xa_item*);

  int tx_pending_res(xa_item *xai, int cfd);

  int do_cache_res(xa_item *xai, char *res, size_t sz);

  size_t get_free_size(xa_item *xai);
} ;

#endif /* __CACHE_WRAPPER_H__*/
