
#ifndef __MYPROXY_BACKEND_H__
#define __MYPROXY_BACKEND_H__

#include <stdlib.h>
#include "myproxy_config.h"
#include "thread_helper.h"
#include "xamgr.h"
#include "cache_wrapper.h"
#include "hook.h"
#include "busi_base.h"


class myproxy_backend : public business_base  {
protected:

  /* the xa manager */
  xamgr m_xa ;

  safeScheQueue m_pendingQ ;

  safeClientStmtInfoList m_stmts ;

  cache_wrapper m_caches ;

  safeLoginSessions &m_lss;

  /* tx/rx methods */
  myproxy_epoll_trx m_trx ;

protected:

  /* deal with responses */
  int deal_query_res(xa_item*, int, char *req, size_t sz);

  int deal_stmt_prepare_res(xa_item*, int, char *req, size_t sz);

  int deal_stmt_execute_res(xa_item*, int cid, char *req, size_t sz);

  int try_do_pending(void);

  int new_xa(tSessionDetails*,sock_toolkit*,int,int,
    char*,size_t,int&,int=st_na);

  int get_route(int xaid, int id, tSqlParseItem &sp,
    bool fullroute, bool needcache,
    std::set<uint8_t> &rlist);

  int test_prepared(int cid, const char *req, size_t sz, int xaid);

  int save_sv_by_placeholders(tSqlParseItem *sp, 
    int total_phs, char *pReq,size_t sz);

  int try_pending_exec(int cfd, int xaid, sock_toolkit *st);

  int prep_done(xa_item *xai, int cfd);

  int get_ordering_col(char *cols, int num_cols, char **name);

  int do_cache_row(xa_item *xai, int myfd, char *res, size_t sz);

  int save_col_def_by_dn(xa_item *xai, int cfd, int myfd, 
    char *res, size_t sz);

  safeDnStmtMapList* get_stmt_id_map(int cid, char* req, size_t sz) ;

  int deal_parser_item(int cid, char *req, size_t sz, tSqlParseItem* &sp);

  int get_last_sn(xa_item *xai);

  int do_add_mapping(int myfd, int cfd, int stmtid, 
    int lstmtid, xa_item *xai);

protected:
  /* implemented from interface 'business_base' */
  int rx(sock_toolkit*,epoll_priv_data*,int) ;
  int tx(sock_toolkit*,epoll_priv_data*,int) ;
  int on_error(sock_toolkit*,int) { return 0; }

public:
  myproxy_backend(safeLoginSessions&) ;
  ~myproxy_backend() ;

public:
  int end_xa(int);
  int end_xa(xa_item*);

  /* do request in query mode */
  int do_query(sock_toolkit*, int cid, char *req, size_t sz);

  /* do the stmt_prepare request */
  int do_stmt_prepare(sock_toolkit*, int cid, char *req, size_t sz, 
    int=st_prep_trans);

  /* do the stmt_execute request */
  int do_stmt_execute(sock_toolkit*, int cid, char *req, size_t sz);

  /* do the send_long_data request */
  int do_send_blob(int cid, char *req, size_t sz);

  int xa_rx(xa_item*,int,char*,size_t);

  myproxy_epoll_trx& get_trx(void);

  /* release a client's resource */
  int close(int);

} ;

#endif /* __MYPROXY_BACKEND_H__*/

