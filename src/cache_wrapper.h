
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

  int save_err_res(xa_item *xai, char *res, size_t sz);

  bool is_err_pending(xa_item *xai) const;

  int move_err_buff(xa_item *xai, tContainer &con);

  int tx_pending_err(xa_item *xai,int cfd);

  int move_buff(xa_item *xai,tContainer &con);
} ;

#endif /* __CACHE_WRAPPER_H__*/
