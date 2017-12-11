/*
 *  Module Name:   framework
 *
 *  Description:   encapsulates a set of distributed 
 *                   network server implementations
 *
 *  Author:        yzhou
 *
 *  Last modified: Sep 28, 2017
 *  Created:       Apr 7, 2016
 *
 *  History:       refer to 'ChangeLog'
 *
 */ 
#ifndef __FRAMEWORK_H__
#define __FRAMEWORK_H__

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "thread_helper.h"
#include "lock.h"
//#include "distrib.h"
#include "dbg_log.h"
#include "sock_toolkit.h"
//#include "container.h"
//#include "container_implements.h"
#include "toolset.h"
#include "resizable_container.h"

/* version number string */
#define __VER_STR__  "v0.0.1 alpha"


/* default business port to normal clients */
constexpr int DEFAULT_BUSI_PORT=23300;

using dbg_info =  struct tDebugInfo {
  int n_reply2client ;
  int n_clientReqs ;
  int n_loginReqs ;
} ;

class epoll_impl 
{
protected:
  /* address/port of normal business */
  addr_t m_busiAddr ;
  /* thread pool */
  thread_helper<void(epoll_impl::*)(int),epoll_impl> *m_threads;
  size_t num_thread ;
#if 0
  /* memory pool */
  struct pool_tag{} ;
  typedef boost::singleton_pool<pool_tag,MAX_PACKETSZ> m_pool ;
#endif
  //typedef memory_man m_pool ;
  /* the server socket handle to normal clients */
  int m_busi_fd;
  /* the epoll control handle */
  //int m_efd ;
  /* business handler */
  void *m_busi ;
  /* connection handler */
  void *m_accepter ;
  /* debug info */
  dbg_info m_dbg ;
  /* flag to indicate working or not */
  bool m_run ;
  /* the socket toolkits */
  //sock_toolkit m_stk ;
#if 0
  /* the temporary buffer when rx 
   *  from socket interface  */
  pthread_key_t m_tKey ;
#endif
  /* socket info table */
  //safeSockTrxTbl m_sockTbl ;
  /* the per-thread details */
  //threadDispatchList m_thdDetails ;
  /* XXX: the per-thread index number(TLS) */
  //__thread int m_threadIdx ;

public:
  epoll_impl(void*pb=0,char*disp=0,
    int busi_port=DEFAULT_BUSI_PORT,size_t szThdPool=10) ;
  ~epoll_impl(void);

public:
  void start(void);
  void stop(void);

public:
  /* XXX: test: the accept task */
  void accept_task(int);
  /* the 'epoll' processing loop */
  void event_task(int) ;
  /* the 'maintainance' loop */
  void mt_task(int);

protected:
  //int init_accepter(void);
  /* calculates the thread pool size */
  void calc_thd_pool_sz(void);
  /* for debug only */
  void init_dbug(void);
  void output_dbug_info(void);
#if 0
  int __connect_in_notice(int);
  int __sync_priv(int);
  int do_req_split(int,disp_pkt_hdr*);
  int do_res_merge(char*);
  int send_merged_res(int,disp_pkt_hdr*);
  int do_fast_reply(int,disp_pkt_hdr*);
#endif
} ;

#endif  /* __FRAMEWORK_H__ */

