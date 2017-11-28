
#ifndef __MYPROXY_BUSI_H__
#define __MYPROXY_BUSI_H__

#include "busi_base.h"
#include "myproxy_config.h"
#include "sock_toolkit.h"
#include "myproxy_trx.h"
#include "myproxy_backend.h"
#include <memory>

#ifdef __VER_STR__
#undef __VER_STR__
#endif
#define __VER_STR__  "myproxy-0.01.13 alpha"

/* connection status */
enum eConnStats
{
  cs_init,
  cs_login_ok,
} ;

/* 
 * the front-end of myproxy 
 */
class myproxy_frontend : public business_base {

protected:
  /* default character set */
  int m_charSet ;

  /* server status */
  volatile uint16_t m_svrStat ;

  /* the formated proxy configs */
  //myproxy_config m_conf ;

  /* tx/rx methods */
  myproxy_epoll_trx m_trx ;

  /* command handlers */
  using cmdHandler = int(myproxy_frontend::*)(int,char*, size_t);
  using cmdNode_t  = struct tCommandNode {
    myproxy_frontend::cmdHandler ha ;
  } ;

  cmdNode_t *m_handlers;

  /* client variables */
  safeLoginSessions m_lss ;

  /* the table detail list */
  //safeTableDetailList m_tables;

  /* the executor */
  //myproxy_backend *m_exec ;
  std::shared_ptr<myproxy_backend> m_exec ;

  /* the TLS object */
  pthread_key_t m_tkey ;

protected:

  void unregister_cmd_handlers(void);

  void register_cmd_handlers(void);

  int deal_command(int,char*,size_t) ;

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
  /* 'set autocommit=x' response */
  int do_set_autocommit(int,int);

  /* process 'desc table' response */
  size_t calc_desc_tbl_resp_size(tTblDetails*);
  int do_desc_table(int,int,char*);

  /* 'select DATABASE()' reponse */
  int do_sel_cur_db(int,int);

  /* 'select @@version_comment' response */
  int do_sel_ver_comment(int,int);

  /* 'show databases' response */
  int do_show_dbs(int,int);

  /* 'show tables' response */
  int do_show_tbls(int,int);

  /* send greeting to client when login */
  int do_server_greeting(int);

public:
  myproxy_frontend(char*);
  virtual ~myproxy_frontend(void);

public:
  int rx(sock_toolkit*,epoll_priv_data*,int) ;

  int tx(sock_toolkit*,epoll_priv_data*,int) ;

  int on_error(sock_toolkit*,int) ;

} ;

#endif /* __MYPROXY_BUSI_H__*/

