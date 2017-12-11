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

int myproxy_frontend::deal_command(int fd, char *req, size_t sz) 
{
  int cmd=0;
  size_t sz_in = sz;
  char *pb = 0, *inb = req;

  if (!mysqls_is_packet_valid(inb,sz_in)) {
    log_print("invalid mysql packet received\n");
    return /*1*/MP_ERR;
  }
  /* set packet ending */
  inb[sz_in] = '\0';
  /* 
   * parse command code 
   */
  pb  = mysqls_get_body(inb);
  cmd = pb[0]&0xff ;
  /* 
   * try login first
   */
  if (do_com_login(fd,inb,sz_in)==MP_OK) {
    return /*0*/MP_OK;
  } 
  /* 
   * try commands in list 
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
    log_print("invalid connection status "
      "of id %d\n", connid);
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
  /* TODO: do cleanup here */
  return MP_OK;
}

/* process the 'field list' command */
int myproxy_frontend::do_com_field_list(int connid,char *inb,
  size_t sz)
{
  int sn = 0;
  char outb[MAX_PAYLOAD];
  size_t sz_out=0;

  /* extract serial number */
  sn = mysqls_extract_sn(inb);

  /* TODO: parse field structure of target table */
  /* allocate response buffer */
  /* send a dummy reply */
  if ((sz_out=mysqls_gen_dummy_qry_field_resp(
     outb,m_svrStat,m_charSet,sn+1))<0) {
    return MP_ERR ;
  }

  m_trx.tx(connid,outb,sz_out);
  return MP_OK;
}

size_t myproxy_frontend::calc_desc_tbl_resp_size(tTblDetails *td)
{
  tColDetails *cd = 0;
  size_t total = 0;
  uint16_t i=0;

  /* response column size */
  total = 512 ;
  /* 
   * calculate resp size 
   */
  for (i=0,cd=td->columns;cd&&i<td->num_cols;
     i++,cd=cd->next) {
    /* XXX: column name: strings a 'length-encoded' */
    total += strlen(cd->col.name)+1 +4;
    /* column type */
    total += strlen(mysqlc_type2str(cd->col.type))+1+10;
    /* null-able */
    total += (cd->ext.null_able?3:2)+1;
    /* FIXME: key type */
    total += 4+3;
    /* default */
    total += strlen(cd->ext.default_val)+1+4;
    /* extra */
    total += strlen(cd->ext.extra)+1+4;
  }
  /* the 'EOF' packet */
  total += 10;
  return total ;
}

int myproxy_frontend::do_desc_table(int connid,int sn,
  char *tbl)
{
  tSessionDetails *pss = m_lss.get_session(connid);
  tTblDetails *td = 0;
  tColDetails *cd = 0;
  size_t total = 0, sz_out = 0, **sz_rows=0;
  uint16_t i=0;
  char *outb=0, **rows = 0, *rbuf = 0, *ptr=0, 
    *begin=0;
  tContainer con ;

  if (!pss) {
    /* FATAL: */
    log_print("connection region %d NOT found\n",connid);
    return MP_ERR;
  }
  /* trim spaces to get table name */
  for (;isspace(*tbl);tbl++) ;
  /* get table structure */
  td = m_tables.get(m_tables.gen_key
    ((char*)pss->db.c_str(),tbl));
  if (!td) {
    char errbuf[MAX_PAYLOAD];

    sz_out = do_err_response(sn,errbuf,ER_NO_SUCH_TABLE,
      ER_NO_SUCH_TABLE,(char*)pss->db.c_str(),tbl,0);

    m_trx.tx(connid,errbuf,sz_out);
    /* FATAL: */
    log_print("%s.%s NOT found\n",pss->db.c_str(),tbl);
    return MP_ERR;
  }
  /* calculate size */
  total = calc_desc_tbl_resp_size(td) ;
  con.tc_resize(total);
  /* allocate row buffer: (max-row-len+size_t-size) × column-count */
  rbuf = new char[total + sizeof(size_t)*td->num_cols];
  /* row size pointers */
  sz_rows = new size_t*[td->num_cols];
  /* row pointers */
  rows = new char*[td->num_cols];
  /* iterate each columns */
  ptr = rbuf ;
  for (i=0,cd=td->columns;cd&&i<td->num_cols;
     i++,cd=cd->next) {
    begin = ptr ;
    /* column name */
    ptr += lenenc_str_set(ptr,cd->col.name);
    /* column type */
    //ptr += lenenc_str_set(ptr,(char*)mysqlc_type2str(cd->col.type));
    ptr += lenenc_str_set(ptr,cd->ext.display_type);
    /* null-able */
    ptr += lenenc_str_set(ptr,(char*)(cd->ext.null_able?"YES":"NO"));
    /* key type */
    ptr += lenenc_str_set(
      ptr,(char*)(cd->ext.key_type==0?"PRI":
      cd->ext.key_type==1?"MUL":
      cd->ext.key_type==2?"UNI":"")
      );
    /* default */
    ptr += lenenc_str_set(ptr,cd->ext.default_val);
    //log_print("default: %s\n",cd->ext.default_val);
    /* extra */
    ptr += lenenc_str_set(ptr,cd->ext.extra);
    /* row content */
    rows[i] = begin ;
    /* row len */
    sz_rows[i]= (size_t*)ptr;
    *sz_rows[i] = ptr-begin ;
    /* next row */
    ptr+=sizeof(size_t) ;
  }
  /* generate the response */
  outb = con.tc_data();
  sz_out = mysqls_gen_desc_tbl_resp(outb,m_svrStat,
    m_charSet, sn+1,rows,sz_rows,td->num_cols);
  /*printf("%s: szout: %d, total: %d\n",
   * _func__,sz_out,total);*/

  delete [] rbuf ;
  delete [] sz_rows ;
  delete [] rows ;

  m_trx.tx(connid,outb,sz_out);

  return MP_OK;
}

int myproxy_frontend::do_sel_cur_db(int connid,int sn)
{
  /* get connection region */
  tSessionDetails *pss = m_lss.get_session(connid);
  char *rows[1], outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;

  if (!pss) {
    log_print("FATAL: connetion id %d not found\n",
      connid);
    return MP_ERR;
  }
  rows[0] = (char*)pss->db.c_str();
  sz_out  = mysqls_gen_simple_qry_resp(outb,
    m_svrStat,m_charSet,sn+1,
    (char*)"DATABASE()",rows,1);

  m_trx.tx(connid,outb,sz_out);
  return MP_OK;
}

int myproxy_frontend::do_set_autocommit(int connid,int sn)
{
  char outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;

  /* FIXME: deal 'autocommit' setting with trasactions */
  sz_out  = do_ok_response(sn,m_svrStat,outb);

  m_trx.tx(connid,outb,sz_out);
  return MP_OK;
}

int myproxy_frontend::do_sel_ver_comment(int connid,int sn)
{
  char *rows[1], outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;

  rows[0] = (char*)"myproxy source";
  sz_out  = mysqls_gen_simple_qry_resp(outb,m_svrStat,
    m_charSet,sn+1,(char*)"@@version_comment",rows,1);

  m_trx.tx(connid,outb,sz_out);
  return MP_OK;
}

int myproxy_frontend::do_show_dbs(int connid,int sn)
{
  uint16_t i=0;
  char /***rows=0,*/ outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;
  const size_t ndbs = m_conf.get_num_schemas();
  char *rows[ndbs];

  //rows = new char* [ndbs];
  for (i=0;i<m_conf.get_num_schemas();i++) {
    rows[i] = (char*)m_conf.get_schema(i)->name.c_str();
  }
  sz_out = mysqls_gen_simple_qry_resp(outb,m_svrStat,
    m_charSet,sn+1, (char*)"DataBase",rows,ndbs);
  //delete []rows ;

  m_trx.tx(connid,outb,sz_out);
  return MP_OK;
}

int myproxy_frontend::do_show_tbls(int connid,int sn)
{
  uint16_t i=0;
  char **rows=0, outb[MAX_PAYLOAD] ;
  size_t sz_out = 0;
  size_t ntbls =m_tables.size();
  tTblDetails *td = 0;
  /* get connection region */
  tSessionDetails *pss = m_lss.get_session(connid);
  safe_container_base<uint64_t,tTblDetails*>::ITR_TYPE itr ;

  if (!pss) {
    log_print("FATAL: connetion id %d not found\n",connid);
    return MP_ERR;
  }
  rows  = new char* [ntbls];
  /* encoding the table name list */
  for (i=0,td=m_tables.next(itr,true);td&&i<ntbls;
     td=m_tables.next(itr),i++) {
    /* only show tables in current db */
    if (td->schema!=pss->db) {
      ntbls--, i-- ;
      continue ;
    }
    rows[i] = (char*)td->table.c_str();
  }
  sz_out = mysqls_gen_simple_qry_resp(outb,m_svrStat,
    m_charSet,sn+1, (char*)"Table",rows,ntbls);
  delete []rows ;

  m_trx.tx(connid,outb,sz_out);
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
  if (ret==-1) {
    log_print("command error: %s\n",pStmt);
    return MP_ERR;
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
    /* 'desc table' */
    case s_desc_tbl:
      {
        ret = do_desc_table(connid,sn,pStmt+4);
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
    /* 'set autocommit' */
    case s_set_autocommit:
      {
        ret = do_set_autocommit(connid,sn);
      }
      break ;
    /* un-support commands */
    default:
      {
        size_t sz_out = 0;
        char outb[MAX_PAYLOAD] ;

        /* allocate response buffer */
        sz_out = do_err_response(sn,outb,ER_PARSE_ERROR,
          ER_PARSE_ERROR,find_mysqld_error(ER_SYNTAX_ERROR),
          pStmt,0);

        m_trx.tx(connid,outb,sz_out);
        ret = MP_ERR;
      }
      break ;
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
  char *pbody=0;
  sock_toolkit *st = (sock_toolkit*)pthread_getspecific(m_tkey);

  /* get the sql to be prepared */
  pbody = mysqls_get_body(inb)+1;
  log_print("prepare sql: %s\n",pbody);
  /* return back the logical statement id */
  m_exec.get()->do_stmt_prepare(st,connid,inb,sz);
  return MP_OK;
}

int myproxy_frontend::do_com_stmt_close(int connid,
  char *inb,size_t sz)
{
  int stmtid = 0;

#if 0
  tSqlSplitPkt *ps = 0;
  /* FIXME: put a valid 'sn' value here */
  int sn = 0;

  ps = m_reqs.find_by_sn(connid,sn);
  if (!ps) {
    log_print("invalid cid %d or sn %d\n",
      connid,sn);
    return 0;
  }
#endif
  mysqls_get_stmt_prep_stmt_id(inb,sz,&stmtid);
  /* FIXME: this droping will lead to a crash after
   *  inserting a new statement id */
  //m_stmts.drop(connid,stmtid);
  log_print("closing logical statement by id %d\n", stmtid);
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
  
  log_print("cid %d\n",cid);
  /* 
   * create new connection region 
   */
#if 0
  mysqls_gen_rand_string(cs.scramble,AP_LENGTH-1);
  cs.scramble[AP_LENGTH] = 0;
  cs.sc_len = AP_LENGTH-1;
  cs.status = cs_init ;
  m_lss.add_session(cid,&cs);
#else
  cs = m_lss.add_session(cid);
#endif
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
  int ret = 0;
  char *req =0;
  size_t sz = 0;

  /* store the sock_toolkit */
  pthread_setspecific(m_tkey,(void*)st);

  /* process requests */
  while (
    (ret=m_trx.rx(fd,priv,req,sz))==MP_OK && 
    (/*ret=*/deal_command(fd,req,sz))==MP_OK
  ) ;

  /* end up processing */
  if (ret!=MP_FURTHER) {
    m_trx.end_rx(req);
  }

  /* client closed */
  if (ret==MP_ERR) {
    m_exec.get()->close(fd);
  }
  return ret;
}

int 
myproxy_frontend::tx(sock_toolkit* st,epoll_priv_data *priv,int fd) 
{
  //printf("tx invoke %d\n",fd);
  return do_server_greeting(fd);
}

int myproxy_frontend::on_error(sock_toolkit *st, int fd) 
{
  return m_exec?m_exec->close(fd):-1;
}

