
#ifndef __MP_FRONTEND_H__
#define __MP_FRONTEND_H__

#include "busi_base.h"
#include "mp_cfg.h"
#include "sock_toolkit.h"
#include "mp_trx.h"
#include "mp_backend.h"
#include <memory>

#ifdef __VER_STR__
#undef __VER_STR__
#endif
#define __VER_STR__  "myproxy-0.01.14 alpha"

/* 
 * the front-end of myproxy 
 */
class mp_frontend : public business_base {

protected:
  /* default character set */
  int m_charSet ;

  /* server status */
  volatile uint16_t m_svrStat ;

  /* the formated proxy configs */
  //mp_cfg m_conf ;

  /* tx/rx methods */
  mp_trx m_trx ;

  /* command handlers */
  using cmdHandler = int(mp_frontend::*)(int,char*, size_t);
  using cmdNode_t  = struct tCommandNode {
    mp_frontend::cmdHandler ha ;
  } ;

  cmdNode_t *m_handlers;

  /* client variables */
  safeLoginSessions m_lss ;

  /* the table detail list */
  //safeTableDetailList m_tables;

  /* the executor */
  //mp_backend *m_exec ;
  std::shared_ptr<mp_backend> m_exec ;

  /* the TLS object */
  pthread_key_t m_tkey ;

protected:

  void unregister_cmd_handlers(void);

  void register_cmd_handlers(void);

  int deal_pkt(int,char*,size_t,void*) ;

  /* the login process */
  int do_com_login(int,char*,size_t);

  /* process quit command */
  int do_com_quit(int,char*,size_t);

  /* process query command */
  int do_com_query(int,char*,size_t);

  /* process init_db command */
  int do_com_init_db(int,char*,size_t);

  /* process field list command */
  int do_com_field_list(int,char*,size_t);

  /* the default command handler */
  int default_com_handler(int,char*,size_t);

  /* process stmt_prepare command */
  int do_com_stmt_prepare(int,char*,size_t);

  /* process stmt_close command */
  int do_com_stmt_close(int,char*,size_t);

  /* process stmt_execute command */
  int do_com_stmt_execute(int,char*,size_t);

  /* process stmt_send_long_data command */
  int do_com_stmt_send_long_data(int,char*,size_t);
  /*
   * sub commands handling
   */

  /* 'select DATABASE()' reponse */
  int do_sel_cur_db(int,int);

  /* 'select @@version_comment' response */
  int do_sel_ver_comment(int,int);

  /* 'show databases' response */
  int do_show_dbs(int,int);

  /* 'show tables' response */
  int do_show_tbls(int,int);

  /* 'show processlist' response */
  int do_show_proclst(int,int);

  /* send greeting to client when login */
  int do_server_greeting(int);

public:
  mp_frontend(char*);
  virtual ~mp_frontend(void);

public:
  int rx(sock_toolkit*,epoll_priv_data*,int) ;

  int tx(sock_toolkit*,epoll_priv_data*,int) ;

  int on_error(sock_toolkit*,int) ;

} ;

#endif /* __MP_FRONTEND_H__*/

