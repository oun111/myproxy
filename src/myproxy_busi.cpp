#include "myproxy_busi.h"
#include <signal.h>
#include "porting.h"
#include "simple_types.h"
#include "mysqld_ername.h"
#include "sql_parser.h"
#include "env.h"

using namespace GLOBAL_ENV;
using namespace global_parser_items;


/*
 * front-end of myproxy
 */
myproxy_frontend::myproxy_frontend(char*desc) :
  /* default char set: utf8 */
  m_charSet(0x21),
  m_svrStat(SERVER_STATUS_AUTOCOMMIT)
{
  m_desc = desc ;

  /* the mysql command handlers */
  register_cmd_handlers();

  /* init the executor */
  m_exec = std::shared_ptr<myproxy_backend>(new myproxy_backend(m_lss));

  /* acquire the TLS */
  pthread_key_create(&m_tkey,NULL);
}

myproxy_frontend::~myproxy_frontend(void)
{
  unregister_cmd_handlers();
  /* release the TLS */
  pthread_key_delete(m_tkey);
}

void myproxy_frontend::unregister_cmd_handlers(void)
{
  if (m_handlers)
    delete []m_handlers ;
}

void myproxy_frontend::register_cmd_handlers(void)
{
  int i=0;

  /* standard mysql commands */
  m_handlers = new cmdNode_t[com_end] ;
  for (;i<com_end;i++) {
    m_handlers[i].ha = &myproxy_frontend::default_com_handler;
  }
  /* command 'com_query' */
  m_handlers[com_query].ha = &myproxy_frontend::do_com_query;
  /* command 'com_init_db' */
  m_handlers[com_init_db].ha = &myproxy_frontend::do_com_init_db;
  /* command 'com_field_list' */
  m_handlers[com_field_list].ha = &myproxy_frontend::do_com_field_list;
  /* command 'com_quit' */
  m_handlers[com_quit].ha = &myproxy_frontend::do_com_quit;
  /* command 'com_stmt_prepare' */
  m_handlers[com_stmt_prepare].ha = &myproxy_frontend::do_com_stmt_prepare;
  /* command 'com_stmt_close' */
  m_handlers[com_stmt_close].ha = &myproxy_frontend::do_com_stmt_close;
  /* command 'com_stmt_execute' */
  m_handlers[com_stmt_execute].ha = &myproxy_frontend::do_com_stmt_execute;
  /* command 'com_stmt_send_long_data' */
  m_handlers[com_stmt_send_long_data].ha = &myproxy_frontend::do_com_stmt_send_long_data;
}

int myproxy_frontend::deal_pkt(int fd, char *req, size_t sz_in, void *arg) 
{
  int cmd=0, rc = 0;
  char *pb = 0, *inb = req;

  if (!mysqls_is_packet_valid(inb,sz_in)) {
    log_print("invalid mysql packet received\n");
    return /*1*/MP_ERR;
  }
  /* set packet ending */
  //inb[sz_in] = '\0';
  /* 
   * parse command code 
   */
  pb  = mysqls_get_body(inb);
  cmd = pb[0]&0xff ;
  /* 
   * try login first
   */
  /* login OK */
  if ((rc=do_com_login(fd,inb,sz_in))==MP_OK) return MP_OK;
  /* login fail */
  else if (rc==MP_ERR) return MP_ERR;
  /* 
   * not login request, try other commands below... 
   */
  if (cmd>=com_sleep && cmd<com_end) {
    log_print("cmd: %s chan %d\n",mysql_cmd2str[cmd],fd);
    /* invoke handlers */
    cmdHandler pf = m_handlers[cmd].ha ;
    return (this->*pf)(fd,inb,sz_in);
  }
  log_print("unknown command code %d\n",cmd);
  return /*-1*/MP_ERR;
}

int myproxy_frontend::do_com_login(int connid,
  char *inb,size_t sz)
{
  tSessionDetails *pss = 0;
  char usr[MAX_NAME_LEN]="",pwd_in[MAX_PWD_LEN]="",
    db[MAX_NAME_LEN]="", *pwd = 0;
  size_t sz_in = 0, sz_out=0;
  uint32_t sn = 0;
  char outb[MAX_PAYLOAD];
  int ret = MP_OK;

  /* get serial number */
  sn = mysqls_extract_sn(inb);
  /* starts from packet body */
  inb = mysqls_get_body(inb);
  /* check if it's login request */
  if (!mysqls_is_login_req(inb,sz)) {
    //log_print("not a login request\n");
    return MP_FURTHER;
  }
  if (!(pss=m_lss.get_session(connid))) {
    log_print("failed get connection info "
      "by id %d\n", connid);
    sz_out = do_err_response(sn,outb,
      ER_NET_READ_ERROR_FROM_PIPE, 
      ER_NET_READ_ERROR_FROM_PIPE);
    ret = MP_ERR ;
    goto __end_login ;
  }
  /* check if connection is in 'init' state, 
   *  if so, login request is expected */
  if (pss->status!= cs_init) {
    log_print("invalid connection status %d "
      "of id %d\n",pss->status,connid);
    sz_out = do_err_response(sn,outb,
      ER_NET_READ_ERROR_FROM_PIPE, 
      ER_NET_READ_ERROR_FROM_PIPE);
    ret = MP_ERR ;
    goto __end_login ;
  }
  if (mysqls_parse_login_req(inb,sz,usr,pwd_in,&sz_in,db)) {
    log_print("failed parsing login request "
      "by id %d\n", connid);
    sz_out = do_err_response(sn,outb,
      ER_NET_READ_ERROR_FROM_PIPE, 
      ER_NET_READ_ERROR_FROM_PIPE);
    ret = MP_ERR ;
    goto __end_login ;
  }
  if (*db!='\0' && !m_conf.is_db_exists(db)) {
    log_print("no database %s\n", db);
    sz_out = do_err_response(sn,outb,
      ER_BAD_DB_ERROR,
      ER_BAD_DB_ERROR,db);
    ret = MP_ERR ;
    goto __end_login ;
  }
  pwd = m_conf.get_pwd(db,usr);
  if (!pwd) {
    log_print("no auth entry found for "
      "'%s'@'%s'\n", usr,db);
    sz_out = do_err_response(sn,outb,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR, 
      usr, db);
    ret = MP_ERR ;
    goto __end_login ;
  }
  /* authenticate password */
  if (mysqls_auth_usr(usr,pwd,pwd_in,
     sz_in,pss->scramble,pss->sc_len)) {
    log_print("auth failed for user %s\n",usr);
    sz_out = do_err_response(sn,outb,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR, 
      usr, db);
    ret = MP_ERR ;
    goto __end_login ;
  }
  /* save user@db */
  pss->db = db ;
  pss->usr= usr;
  pss->pwd= pwd_in;
  {
    struct sockaddr_in sa ;
    socklen_t ln = sizeof(sa) ;
    char chPort[32];

    getpeername(connid,(struct sockaddr*)&sa,&ln);
    sprintf(chPort,":%d",sa.sin_port);
    pss->addr = inet_ntoa(sa.sin_addr);
    pss->addr += chPort;
  }
  /* move region to next status */
  pss->status = cs_login_ok;
  log_print("(%d) user %s@%s login ok\n",connid,usr,db);
  /* send an ok response */
  sz_out = do_ok_response(sn,m_svrStat,outb);
  ret = MP_OK ;

__end_login:
  /* send directly to client */
  m_trx.tx(connid,outb,sz_out);
  return ret;
}

/* process the 'quit' command */
int myproxy_frontend::do_com_quit(int connid,char *inb,
  size_t sz)
{
  sock_toolkit *st = (sock_toolkit*)pthread_getspecific(m_tkey);

  /* do cleanup here */
  m_exec.get()->close(connid);

  close1(st,connid);

  return MP_OK;
}

/* process the 'field list' command */
int myproxy_frontend::do_com_field_list(int connid,char *inb,
  size_t sz)
{
  int sn = 0;
  size_t sz_out=0;
  std::string tbl ;
  tSessionDetails *pss = m_lss.get_session(connid);

  if (!pss) {
    log_print("FATAL: connetion id %d not found\n",
      connid);
    return MP_ERR;
  }

  /* extract serial number */
  sn = mysqls_extract_sn(inb);

  /* target table name */
  tbl = inb+5 ;

  /* get table details */
  tTblDetails *td = m_tables.get(pss->db.c_str(),tbl.c_str());

  /* if table detail not available, send a dummy response */
  if (!td || !m_tables.is_valid(td)) {
    char outb[MAX_PAYLOAD];
    char *rows[] {(char*)"com_field_list failed"};

    sz_out  = mysqls_gen_qry_field_resp(outb,
      m_svrStat,m_charSet,sn+1,(char*)pss->db.c_str(),
      (char*)tbl.c_str(),rows,1);
    m_trx.tx(connid,outb,sz_out);
    return MP_OK ;
  }

  /* send actual table column details */
  {
    const size_t numRows = td->columns.size();
    char *rows[numRows];
    tColDetails *cd = 0;
    char *outb = 0;
    size_t total = 0;
    safeTableDetailList::col_itr itr ;
    bool bStart = true;

    /* calc the packet size and assign column names */
    for (uint16_t i=0;i<numRows;i++) {
      /* get each columns */
      cd = m_tables.next_col((char*)pss->db.c_str(),(char*)tbl.c_str(),itr,bStart);
      if (!cd)
        break ;

      total += strlen(cd->col.name)+pss->db.length()+tbl.length()+100 ;

      rows[i] = cd->col.name ;

      if (bStart) bStart=false;
    }

    outb = new char [total];

    sz_out = mysqls_gen_qry_field_resp(outb,
      m_svrStat,m_charSet,sn+1,(char*)pss->db.c_str(),
      (char*)tbl.c_str(),rows,numRows);
    m_trx.tx(connid,outb,sz_out);

    delete []outb ;
  }

  return MP_OK;
}

int myproxy_frontend::do_sel_cur_db(int connid,int sn)
{
  /* get connection region */
  tSessionDetails *pss = m_lss.get_session(connid);
  char outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;

  if (!pss) {
    log_print("FATAL: connetion id %d not found\n",
      connid);
    return MP_ERR;
  }

  {
    char *cols[] = { (char*)"DATABASE()" };
    char *rows[1] = { (char*)pss->db.c_str(), };

    sz_out  = mysqls_gen_normal_resp(outb,
      m_svrStat,m_charSet,sn+1,(char*)"",(char*)"",
      cols,1,rows,1);
    m_trx.tx(connid,outb,sz_out);
  }

  return MP_OK;
}

int myproxy_frontend::do_sel_ver_comment(int connid,int sn)
{
  char outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;
  char *rows[1] { (char*)"myproxy source", } ;
  char *cols[] { (char*)"@@version_comment"}  ;

  sz_out = mysqls_gen_normal_resp(outb,m_svrStat,
    m_charSet,sn+1,(char*)"",(char*)"",cols,1,rows,1);
  m_trx.tx(connid,outb,sz_out);

  return MP_OK;
}

int myproxy_frontend::do_show_dbs(int connid,int sn)
{
  uint16_t i=0;
  size_t sz_out = 0, sz_total = 100;
  const size_t ndbs = m_conf.get_num_schemas();
  char *rows[ndbs];

  for (i=0;i<m_conf.get_num_schemas();i++) {
    rows[i] = (char*)m_conf.get_schema(i)->name.c_str();

    sz_total += m_conf.get_schema(i)->name.length()+10;
  }

  {
    /* allocates out buffer */
    char *outb = new char [sz_total];
    char *cols[] {(char*)"DataBase"} ;

    sz_out = mysqls_gen_normal_resp(outb,m_svrStat,
      m_charSet,sn+1,(char*)"",(char*)"",cols,1,rows,ndbs);
    m_trx.tx(connid,outb,sz_out);

    delete []outb ;
  }

  return MP_OK;
}

int myproxy_frontend::do_show_tbls(int connid,int sn)
{
  uint16_t i=0;
  size_t sz_out = 0, sz_total = 150;
  const size_t c_ntbls =m_tables.size();
  size_t valid_tbls = c_ntbls ;
  char *rows[c_ntbls];
  tTblDetails *td = 0;
  /* get connection region */
  tSessionDetails *pss = m_lss.get_session(connid);
  safe_container_base<uint64_t,tTblDetails*>::ITR_TYPE itr ;

  if (!pss) {
    log_print("FATAL: connetion id %d not found\n",connid);
    return MP_ERR;
  }

  /* encoding the table name list */
  for (i=0,td=m_tables.next(itr,true);td&&i<c_ntbls;
     td=m_tables.next(itr),i++) {

    /* only show valid tables in current db */
    if (td->schema!=pss->db || !m_tables.is_valid(td)) {
      valid_tbls--, i-- ;
      continue ;
    }

    rows[i]  = (char*)td->table.c_str();
    sz_total+= td->table.length() + 5;
  }

  {
    /* allocate output buffer */
    char *outb = new char [sz_total] ;
    char *cols[] {(char*)"Table"} ;

    sz_out = mysqls_gen_normal_resp(outb,m_svrStat,
      m_charSet,sn+1,(char*)"",(char*)"",cols,1,rows,valid_tbls);
    m_trx.tx(connid,outb,sz_out);

    delete []outb ;
  }

  return MP_OK;
}

int myproxy_frontend::do_show_proclst(int connid,int sn)
{
  char *outb = 0 ;
  size_t total = 400, sz_out = 0;
  const size_t nCols = 9;
  const size_t nConn = m_lss.size();
  tSessionDetails* ps = 0;
  safeLoginSessions::ITR_TYPE itr ;
  char *rows[nConn*nCols] ;
  uint16_t i=0;

  /* FIXME: NOT thread-safe */
  for (i=0,ps=m_lss.next(itr,true);ps;ps=m_lss.next(itr),i++) {

    total += ps->db.length()+ ps->usr.length() + 50 ;

    /* session id */
    rows[i*nCols+0] = (char*)ps->id.c_str();
    /* user */
    rows[i*nCols+1] = (char*)ps->usr.c_str();
    /* host */
    rows[i*nCols+2] = (char*)ps->addr.c_str();
    /* db */
    rows[i*nCols+3] = (char*)ps->db.c_str();
    /* command */
    rows[i*nCols+4] = (char*)ps->cmd.c_str();
    /* time */
    char tmp[64] = "";
    sprintf(tmp,"%lld",time(NULL)-ps->times);
    ps->s_times = tmp ;
    rows[i*nCols+5] = (char*)ps->s_times.c_str();
    /* state */
    rows[i*nCols+6] = (char*)ps->stat.c_str();
    /* info */
    rows[i*nCols+7] = (char*)ps->info.c_str();
    /* progress */
    rows[i*nCols+8] = (char*)" ";
  }

  {
    const char *cols[] {"Id", "User", "Host", "Db",
      "Command", "Time", "State", "Info", "Progress"
    } ;

    /* allocates out buffer */
    outb = new char [total];

    sz_out = mysqls_gen_normal_resp(outb,m_svrStat,
      m_charSet,sn+1,(char*)"",(char*)"",(char**)cols,nCols,
      rows,nConn);
    m_trx.tx(connid,outb,sz_out);

    delete [] outb ;
  }

  return MP_OK;
}

/* process the 'query' command */
int myproxy_frontend::do_com_query(int connid,
  char *inb,size_t sz)
{
  /* extract serial number */
  int sn = mysqls_extract_sn(inb), ret = 0;
  char *pStmt = inb+5;
  int cmd = s_na ;

  /* do a simple parse on incoming statement */
  ret = do_simple_explain(pStmt,sz-5,cmd);

  /* mark down the activities */
  if (ret==MP_OK) {
    m_lss.set_cmd(connid,st_qry,pStmt);
  }

  switch (cmd) {
    /* make this com_query request be sharding
     *  in backends */
    case s_sharding:
      {
        sock_toolkit *st = (sock_toolkit*)
          pthread_getspecific(m_tkey);

        ret = m_exec.get()->do_query(st,connid,inb,sz);
      }
      break ;
    /* 'select DATABASE()' */
    case s_sel_cur_db:
      {
        ret = do_sel_cur_db(connid,sn);
      }
      break ;
    /* 'select @@version_comment' */
    case s_sel_ver_comment:
      {
        ret = do_sel_ver_comment(connid,sn);
      }
      break ;
    /* 'show databases' */
    case s_show_dbs:
      {
        ret = do_show_dbs(connid,sn);
      }
      break ;
    /* 'show tables' */
    case s_show_tbls:
      {
        ret = do_show_tbls(connid,sn);
      }
      break ;
    /* 'show tables' */
    case s_show_proclst:
      {
        ret = do_show_proclst(connid,sn);
      }
      break ;

    /* un-support commands */
    default:
      {
        size_t sz_out = 0;
        char outb[MAX_PAYLOAD] ;

        log_print("un-support query command : %s\n", pStmt);

        /* allocate response buffer */
        sz_out = do_err_response(sn,outb,ER_INTERNAL_UNSUPPORT_SQL,
          ER_INTERNAL_UNSUPPORT_SQL, pStmt);
        (void)g_mysqldError ;

        m_trx.tx(connid,outb,sz_out);
        ret = MP_ERR;
      }
      break ;
  }

  /* for any errors, mark down it */
  if (ret==MP_ERR) {
    m_lss.set_cmd(connid,st_error,pStmt);
  }

  return ret;
}

/* process the 'init_db' command */
int myproxy_frontend::do_com_init_db(int connid,
  char *inb,size_t sz)
{
  uint32_t sn = 0;
  char db[128], *pwd = 0;
  tSessionDetails *pss = 0;
  size_t sz_out = 0;
  char outb[MAX_PAYLOAD];
  int ret = MP_OK ;

  /* get serial number */
  sn = mysqls_extract_sn(inb);
  inb += 5 ;
  /* get db */
  memcpy(db,inb,sz-5);
  db[sz-5] = '\0';
  /* check new db by config */
  if (!m_conf.is_db_exists(db)) {
    log_print("no database %s\n", db);
    sz_out = do_err_response(sn,outb,ER_BAD_DB_ERROR,
      ER_BAD_DB_ERROR,db);
    ret = MP_ERR;
    goto __end_init_db;
  }
  /* get connection region */
  pss  = m_lss.get_session(connid);
  if (!pss) {
    log_print("connetion id %d not found\n",connid);
    sz_out = do_err_response(sn, outb,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR, 
      "", db);
    ret = MP_ERR;
    goto __end_init_db;
  }
  /* get configure password */
  pwd = m_conf.get_pwd(db,(char*)pss->usr.c_str());

  /* authenticate password */
  if (!pwd || mysqls_auth_usr((char*)pss->usr.c_str(),
     pwd,(char*)pss->pwd.c_str(),pss->pwd.size(),
     pss->scramble,pss->sc_len)) {
    log_print("auth failed for user %s\n",pss->usr.c_str());
    sz_out = do_err_response(sn,outb,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR,
      ER_ACCESS_DENIED_NO_PASSWORD_ERROR, 
      pss->usr.c_str(), db);
    ret = MP_ERR;
    goto __end_init_db;
  }
  /* record current db */
  pss->db= db;
  sz_out = do_ok_response(sn,m_svrStat,outb);
  ret    = MP_OK ;

__end_init_db:
  m_trx.tx(connid,outb,sz_out);
  return ret;
}

int myproxy_frontend::default_com_handler(int connid,
  char *inb,size_t sz)
{
  char *cmd = 0;

  cmd = mysqls_get_body(inb);
  log_print("******* command %d\n",cmd[0]);
  return MP_OK;
}

int myproxy_frontend::do_com_stmt_prepare(int connid,
  char *inb,size_t sz)
{
  sock_toolkit *st = (sock_toolkit*)pthread_getspecific(m_tkey);

  std::string sql(inb+5,sz-5);
  log_print("prepare sql: %s\n",sql.c_str());

  /* return back the logical statement id */
  m_exec.get()->do_stmt_prepare(st,connid,inb,sz);
  return MP_OK;
}

int myproxy_frontend::do_com_stmt_close(int connid,
  char *inb,size_t sz)
{
  int stmtid = 0;

  mysqls_get_stmt_prep_stmt_id(inb,sz,&stmtid);
  log_print("closing logical statement by id %d\n", stmtid);

  m_exec.get()->do_stmt_close(connid,stmtid);

  return MP_OK;
}

int myproxy_frontend::do_com_stmt_execute(int connid,
  char *inb,size_t sz)
{
  int stmtid = 0;
  sock_toolkit *st = (sock_toolkit*)pthread_getspecific(m_tkey);

  mysqls_get_stmt_prep_stmt_id(inb,sz,&stmtid);
  log_print("trying execute stmt by logical id %d\n", stmtid);

  m_exec.get()->do_stmt_execute(st,connid,inb,sz);

  return MP_OK;
}

int myproxy_frontend::do_com_stmt_send_long_data(int connid,
  char *inb,size_t sz)
{
  int stmtid = 0;

  mysqls_get_stmt_prep_stmt_id(inb,sz,&stmtid);
  log_print("trying send blob by logical id %d\n", stmtid);

  m_exec.get()->do_send_blob(connid,inb,sz);

  return MP_OK;
}

/* this function is called at server side when a 
 *  new client is just connected */
int myproxy_frontend::do_server_greeting(int cid)
{
  //tSessionDetails cs ;
  tSessionDetails *cs = 0 ;
  size_t sz = 0;
  char outb[MAX_PAYLOAD];
  
  //log_print("cid %d\n",cid);
  /* 
   * create new connection region 
   */
  cs = m_lss.add_session(cid);
  /* 
   * generate greeting packet 
   */
  sz = mysqls_gen_greeting(cid,m_charSet,
    m_svrStat,cs->scramble,(char*)__VER_STR__,
    outb,MAX_PAYLOAD);

  m_trx.tx(cid,outb,sz);
  return MP_OK;
}

int 
myproxy_frontend::rx(sock_toolkit* st,epoll_priv_data *priv,int fd) 
{
  /* store the sock_toolkit */
  pthread_setspecific(m_tkey,(void*)st);

  int ret = m_trx.rx(st,priv,fd) ;

  /* client closed */
  if (ret==MP_ERR) {
    m_exec.get()->close(fd);
  }

  return ret ;
}

int 
myproxy_frontend::tx(sock_toolkit* st,epoll_priv_data *priv,int fd) 
{
  return do_server_greeting(fd);
}

int myproxy_frontend::on_error(sock_toolkit *st, int fd) 
{
  return m_exec?m_exec->close(fd):-1;
}

