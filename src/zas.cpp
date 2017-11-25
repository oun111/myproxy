
#include <limits.h>
#include <string.h>
#include "errmsg.h"

#include <ctype.h>
#include <fcntl.h>
#include <vector>
#include <stack>
#include <string>
#include "zas.h"
#include "connstr.h"
#include "attrib_list.h"
#include "sql_exec.h"
#include "adaptors.h"
#include "dbug.h"
#include "dal.h"

#ifdef DBG_LOG
#include "dbg_log.h"
/* declares the debug log */
DECLARE_DBG_LOG(zas);
#endif


using namespace LEX_GLOBAL;

/* 
 * class zas_connect
 */
zas_connect::zas_connect(char *tns_path, const int nDal):
  m_dal(nDal),
  m_tnsPath(tns_path)
{
  /* do driver specific initialization */
  m_bd = create_basic_dal(m_dal);
}

zas_connect::~zas_connect(void)
{
  /* free everything */
  m_bd->end(m_bd);
  release_basic_dal(m_bd);
  m_bd = 0;
}

int zas_connect::parse_conn_str(char *str)
{
  tns_parser tp ;
  my_dsn_parser dp ;

  *conn_str.chDb       = 0;
  *conn_str.chUnixSock = 0;
  /* try oracle format first */
  if (tp.parse_conn_str(str,m_tnsPath)) {
    /* get results: */
    strcpy(conn_str.chUser, tp.get_tns(tns_parser::tUsr));
    strcpy(conn_str.chPwd,  tp.get_tns(tns_parser::tPwd));
    strcpy(conn_str.chHost, tp.get_tns(tns_parser::tHost));
    strcpy(conn_str.chDb,   tp.get_tns(tns_parser::tSvc));
    conn_str.port = atoi(tp.get_tns(tns_parser::tPort));
    //printd("parse TNS config '%s' OK.\n", str);
    return 1;
  }
  /* try mysql dsn format */
  if (dp.parse_conn_str(str)) {
    /* get results: */
    strcpy(conn_str.chUser, dp.user());
    strcpy(conn_str.chPwd,  dp.pwd());
    strcpy(conn_str.chHost, dp.host());
    strcpy(conn_str.chDb,   dp.db());
    strcpy(conn_str.chUnixSock,dp.sock());
    conn_str.port = dp.port();
    //printd("parse DSN config '%s' OK.\n", str);
    return 1;
  }
  /* TODO: try other formats here */
  {
    printd("unknown conn string '%s'.\n", str);
  }
  /* fail */
  return 0;
}

int zas_connect::rlogon(const char *cstr, const int autoComm) 
{
  return this->rlogon(const_cast<char*>(cstr),autoComm);
}

int zas_connect::rlogon(char *str, const int autoComm)
{
  if (m_bd->init) {
    m_bd->init(m_bd);
  }

  /* extract connection elements from conn_str,
   *   in plain text */
  if (!parse_conn_str(str)) {
    /* throw an exception */
    exceptions::__throw(0x99999,/*""*/str,
      "fatal: parsing connection string","");
    /* reset the mysql handle */
    m_bd->end(m_bd);
    return 0;
  }
  /* try connect server */
  if (!m_bd->login(m_bd,conn_str.chHost, conn_str.chUser, 
    conn_str.chPwd, *conn_str.chDb?conn_str.chDb:NULL, 
    conn_str.port, *conn_str.chUnixSock?conn_str.chUnixSock:
    "/tmp/mysql.sock"))
  {
    printd("connect to database server '%s' fail\n", 
      conn_str.chHost);
    /* throw an exception */
    exceptions::__throw(m_bd->last_errno(m_bd),"",
      m_bd->last_error(m_bd),"");
    /* reset the mysql handle */
    m_bd->end(m_bd);
    return 0;
  }

  m_bd->set_autocommit(m_bd,autoComm);

#ifdef DBG_LOG
  log_init();
#endif
  return 1;
}

int zas_connect::logoff(void)
{
  if (m_bd->end) {
    m_bd->end(m_bd);
    return 1;
  }
  return 0;
}

zas_connect& zas_connect::operator = (const zas_connect &c)
{
  return *this ;
}

int zas_connect::rollback(void)
{
  return m_bd->rollback(m_bd);
}

int zas_connect::commit(void)
{
  return m_bd->commit(m_bd);
}

int zas_connect::auto_commit_on(void)
{
  return m_bd->set_autocommit(m_bd, true);
}

/*
 * if invoking zas_connect::auto_commit_off(), means any
 *  changes in this transaction will not be reflected 
 *  in permanent storage, and none of changes in permanent
 *  storage will be 'seen' in this transaction, one should
 *  invoke zas_connect::commit() MANUALLY to save changes
 *  permanently. Refer to documentation 
 *  "START TRANSACTION, COMMIT, and ROLLBACK Syntax" at
 *  "http://dev.mysql.com/doc/refman/5.6/en/commit.html"
 */
int zas_connect::auto_commit_off(void)
{
  return m_bd->set_autocommit(m_bd, false);
}

long zas_connect::direct_exec(const std::string &stmt,
  int bException)
{
  return this->direct_exec(stmt.c_str());
}

long zas_connect::direct_exec(const char *stmt, int bException)
{
  return stmt && !m_bd->query(m_bd, stmt);
}

void zas_connect::change_db(const char *db)
{
  m_bd->select_db(m_bd,db);
}

size_t zas_connect::num_columns(void)
{
  return m_bd->num_columns(m_bd) ;
}

int zas_connect::get_dal_type(void)
{
  return m_dal ;
}

tBasicDal* zas_connect::get_dal(void)
{
  return m_bd ;
}

/* 
 * class zas_stream
 */
zas_stream::zas_stream(zas_connect &conn,const bool bMPrep):
  m_conn(conn),
  arg_ptr(INIT_ARG_PTR),
  col_ptr(INIT_COL_PTR),
  row_cnt(0),
  a_list(make_shared<attribute_list>()),
  m_exec
  (
    bMPrep?std::shared_ptr<exec_base>(
      new mysql_prep(m_conn.get_dal(),m_conn.get_dal_type())):
    std::shared_ptr<exec_base>(
      new mysql_qry(m_conn.get_dal()))
  ),
  bPrepMethod(bMPrep),
  m_adpt(std::shared_ptr<adaptor>(new mysql_adpt))
{
}

zas_stream::~zas_stream(void)
{
  /* release the argument attribute list if exists */
  clean();
}

int64_t zas_stream::get_rpc(void)
{
  return /*row_cnt*/(*m_exec).get_rpc() ;
}

void zas_stream::flush(void)
{
  /* reset argument pointer */
  //arg_ptr = INIT_ARG_PTR;
} 

void zas_stream::clean(const int flag)
{
  /* reset tmp statement buffer */
  sql_out = "" ;
  /* reset pointers */
  reset();
  /* free result set */
  if (m_exec.use_count()>0) {
    (*m_exec).release();
  }
  /* release the argument attribute list */
  //(*a_list).release();
}

void zas_stream::close(void)
{
  /* close sql statement streaming process */
  sql_stmt = "";
  this->clean();
}

void zas_stream::set_flush(const bool ff=true)
{
  flush_flag = ff ;
}

void zas_stream::set_commit(int cflag)
{
  if (cflag)
    m_conn.auto_commit_on() ;
  else 
    m_conn.auto_commit_off() ;
}

void zas_stream::reset(void)
{
  arg_ptr = INIT_ARG_PTR ;
  col_ptr = INIT_COL_PTR;
  row_cnt = 0 ;
}

void zas_stream::open(int size, const char *sqlstm)
{
  if (!sqlstm) {
    return ;
  }
#ifdef DBG_LOG
  log_id_set(this);
  printd("preparing statement: %s\n",sqlstm);
#endif
  //stmt_sz  = size ;
  /* store the statement */
  sql_stmt = sqlstm ;
  orig_stmt= sqlstm ;
  sql_out  = "" ;
  /* reset all pointers */
  reset();
  /* adapt oracle's syntax style to mysql's */ 
  if (!(*m_adpt).do_adaption(sql_stmt,&(*a_list),!!bPrepMethod)) {
    printd("sql adaption failed\n");
    exceptions::__throw(0x99999,
      orig_stmt.c_str(),"sql adaption failed","") ;
    return ;
  }
  /* prepare sql statement on mysql native api */
  if (!(*m_exec).prepare((*a_list),sql_stmt)) {
    printd("sql preparation failed\n");
    exceptions::__throw(0x99999,
      orig_stmt.c_str(),"sql preparation failed","") ;
    return ;
  }
  /* when '__LAZY_EXEC__' is defined, statement without 
   *   place-holders will be executed automatically,
   *   otherwise, rewind() will do this
   */
#ifdef __LAZY_EXEC__
  /* if no argument, execute the statement directly */
  if (!(*a_list).count()) {
    sql_out = sql_stmt;
    end_tx();
  }
#endif
}

void zas_stream::rewind(void)
{
#ifndef __LAZY_EXEC__
  /* if no argument, execute the statement directly */
  if (!(*a_list).count()) {
    sql_out = sql_stmt;
    end_tx();
  }
#endif
}

size_t zas_stream::num_rows(void) 
{ 
  return (*m_exec).num_rows(); 
}

int zas_stream::eof(void)
{
  return row_cnt>=(*m_exec).num_rows();
}

const char* zas_stream::get_stm_text(void)
{
  return orig_stmt.c_str();
}

void zas_stream::get_stm_text(char *s_orig, char *s_adpt)
{
  if (s_orig) {
    strcpy(s_orig,orig_stmt.c_str());
  }
  if (s_adpt) {
    strcpy(s_adpt,sql_stmt.c_str());
  }
}

void zas_stream::debug(void)
{
#if 1
  printd("original+++: %s\n",  orig_stmt.c_str());
  printd("formated---: %s\n",  sql_stmt.c_str());
#else
  (*m_exec).debug();
#endif
  /*printd("argp %d, count %d---------------\n", 
    arg_ptr, (*a_list).count());*/
}

zas_stream& zas_stream::operator = (const zas_stream &str)
{
  return *this ;
}

void zas_stream::begin_tx(void) 
{
  if (arg_ptr==INIT_ARG_PTR) {
    /* prepare the statement in tmp buffer if 
     *   to pass the 1st argument */ 
    sql_out = "" ;
  } 
}

bool zas_stream::is_prep_method(void)
{
  return bPrepMethod ;
}

/* argument value processing using query method */
int zas_stream::argv_proc_m_qry(int i, int ap)
{
  char str[TKNLEN]="";

  switch((*a_list).get_type(ap)) {
  /* attach to output buffer */
  case attribute_list::tInt:
    sprintf(str,"%d",*(int*)(*a_list).get_priv(ap));
    attach(i,str);
    break ;
  case attribute_list::tLong:
  case attribute_list::tBigint:
    sprintf(str,"%lld",*(long long*)(*a_list).get_priv(ap));
    attach(i,str);
    break ;
  case attribute_list::tLdouble:
    sprintf(str,"%Lf",*(long double*)(*a_list).get_priv(ap));
    attach(i,str);
    break ;
  case attribute_list::tDouble:
    sprintf(str,"%f",*(double*)(*a_list).get_priv(ap));
    attach(i,str);
    break ;
  case attribute_list::tChar:
    attach(i,(char*)(*a_list).get_priv(ap));
    break ;
  /* added 2016.12.1 */
  case attribute_list::tDatetime:
    {
      MYSQL_TIME *dt=(MYSQL_TIME*)(*a_list).get_priv(ap) ;

      sprintf(str,"%d-%d-%d %d:%d:%d",dt->year,dt->month,
        dt->day,dt->hour,dt->minute,dt->second);
      attach(i,str);
    }
    break ;
  default:
    printd("unknown argument value type %d\n", 
      (*a_list).get_type(ap));
    return 0;
  }
  return 1;
}

/* argument value processing using prepare method */
int zas_stream::argv_proc_m_prep(int i, int ap)
{
  switch((*a_list).get_type(ap)) {
  case attribute_list::tInt:
    (*m_exec).insert(i,*(int*)(*a_list).get_priv(ap));
    break ;
  case attribute_list::tLong:
  case attribute_list::tBigint:
    (*m_exec).insert(i,*(long long*)(*a_list).get_priv(ap));
    break ;
  case attribute_list::tLdouble:
    (*m_exec).insert(i,*(long double*)(*a_list).get_priv(ap));
    break ;
  case attribute_list::tDouble:
    (*m_exec).insert(i,*(double*)(*a_list).get_priv(ap));
    break ;
  case attribute_list::tChar:
    (*m_exec).insert(i,(char*)(*a_list).get_priv(ap));
    break ;
  /* added 2016.12.1 */
  case attribute_list::tDatetime:
    (*m_exec).insert(i,(MYSQL_TIME*)(*a_list).get_priv(ap));
    break ;
  default:
    printd("unknown argument value type %d\n", 
      (*a_list).get_type(ap));
    return 0;
  }
  return 1;
}

/* process argument value assignment */
int zas_stream::argv_process(void)
{
  int i = 0, pi = 0, ap = 0;

  if (!(*a_list).count())
    return 1;
  /* check if the argument's insertions are 
   *  completed  */
  if (((*a_list).sph_count()+arg_ptr)<(*a_list).count())
    return 1;
  /* process all normal/shadow arguments' inputing */
  for (i=0; i<(*a_list).count(); i++) {
    pi = (*a_list).get_pi(i) ;
    ap = pi<0?i:pi;
    /* ## use prepare method */
    if (bPrepMethod) {
      argv_proc_m_prep(i,ap);
    } else {
      /* ## use query method */
      argv_proc_m_qry(i,ap);
    }
  } /* end for() */
  /* set the argument pointer to "ready execution"
   *   state */
  arg_ptr = (*a_list).count();
  return 1;
}

zas_stream& zas_stream::end_tx(void) 
{
  /* process argument value inputing
   *   automatically */
  arg_ptr++;
  argv_process();
  /* test if more args is needed */
  if (arg_ptr<(*a_list).count()) 
    return *this ;
  /* if no more arguments, then reset pointers */ 
  reset() ;
  if (!(*m_exec).direct_exec(sql_out)) {
    //printd("exec failed\n");
    exceptions::__throw((*m_exec).last_errno(),
      orig_stmt.c_str(),(*m_exec).last_error(),"") ;
    return *this;
  }
  (*m_exec).store_result();
  return *this; 
}

zas_stream& zas_stream::end_tx(double val) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

zas_stream& zas_stream::end_tx(long double val) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

zas_stream& zas_stream::end_tx(int val) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
zas_stream& zas_stream::end_tx(int64_t val) 
#else
zas_stream& zas_stream::end_tx(long long val) 
#endif
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

zas_stream& zas_stream::end_tx(long val) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

zas_stream& zas_stream::end_tx(char *str) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(str,strlen(str)+1,arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

zas_stream& zas_stream::end_tx(MYSQL_TIME &val) 
{
  /* save argument value and get suitable arg pointer*/
  int ret = (*a_list).argv_save(&val,sizeof(val),arg_ptr);
  /* if argument insertion fail, then decrement the 
   *  arg pointer */
  arg_ptr = !ret?(arg_ptr-1):arg_ptr ;
  /* call to do tx actually */
  return end_tx();
}

int zas_stream::begin_rx(void)
{
  /* fetch a new row if no row is fetched before, or   
   *   all columns of previous row are completely read */
  if (!row_cnt || (col_ptr>=(*m_exec).num_fields())) {  
    cur_row = (*m_exec).row();  
    if (!cur_row) { 
      /* report an error */  
      printd("no more rows\n");
      /* throw an exception */
      exceptions::__throw((*m_exec).last_errno(),
        orig_stmt.c_str(),(*m_exec).last_error(),"") ;
      return 0;
    } 
    row_cnt++; 
    col_ptr=INIT_COL_PTR; 
  }  
  return 1; 
}

zas_stream& zas_stream::end_rx(void) 
{
  /* process next column */ 
  col_ptr++ ;  
  /* free current result set if stream ends*/
  if (eof() && (col_ptr>=(*m_exec).num_fields())) { 
    /*printd("end stream\n");*/
    (*m_exec).release();
    cur_row = NULL ; 
  } 
  return *this; 
}

/* remove the non-sinificant digit '0' */
void zas_stream::del_ns_digits(char *chDigi)
{
  int n = strlen(chDigi)-1;

  while (chDigi[n]=='0') n-- ;
  chDigi[n+1] = 0;
}

int zas_stream::find_next_placeholder(int p)
{
  int pos = 0;
  char buf[TKNLEN];

  while (1) {
    /* starting pos of current place holder */
    pos = p = sql_stmt.find(":",p);
    if ((/*uint32_t*/string::size_type)p==string::npos) {
      //printd("next place holder not found\n");
      return -1;
    }
    p++ ;
    /* check if it's place holder in this form:
     *  :{name}<type> */
    p = next_token(sql_stmt,p,buf);
    p+= strlen(buf);
    p = next_token(sql_stmt,p,buf);
    if (*buf!='<') {
      continue ;
    }
    /* found one */
    break ;
  }
  return pos ;
}

int zas_stream::attach(int argp, char *str)
{
  int i = 0, pos = 0, sp = 0, np = 0;

  /* test and get begining position of 
   *  required place holder */
  for (i=-1;i<argp;i++,pos++) {
    /* starting pos of current place holder */
    pos = find_next_placeholder(pos) ;
    /* no more place holders */
    if (pos<0) {
      return -1;
    }
  }
  /* get ending position of place holder */
  np = sql_stmt.find(">",pos);
  /* get starting pos of next place holder */
  sp= find_next_placeholder(np);
  /*
   * output the place holder's value and 
   *  its related statements
   */
  /* output statement before the 1st 
   *  place holder */
  if (!argp) {
    sql_out += sql_stmt.substr(0,pos-1);
  }
  /* the place holder's value */
  sql_out += str ;
  /* output the rest part */
  sql_out += sql_stmt.substr(np+1,sp<0?
    sql_stmt.size()-np:sp-np-1);
  return 1;
}

zas_stream& zas_stream::do_string_tx(char *str)
{
  uint32_t ln = 0;

  if (!str) {
    return *this ;
  }
  /* begin operator process */
  begin_tx();
  /* char type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tChar)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tChar));
    //return *this ;
  }
  /* length checking */
  ln = strlen(str);
  if ((*a_list).get_size2(arg_ptr)<=ln) {
    printd("argument %d expects size less than '%d', "
      "but size of '%d' is provided\n", arg_ptr, 
      (*a_list).get_size2(arg_ptr), ln);
    //return *this ;
  }
  /* end operator process */
  return end_tx(str);
}

zas_stream& zas_stream::operator << (unsigned char *str)
{
  return do_string_tx(reinterpret_cast<char*>(str));
}

zas_stream& zas_stream::operator << (const char *str)
{
  return do_string_tx(const_cast<char*>(str));
}

zas_stream& zas_stream::operator << (long val)
{
  /* begin operator process */
  begin_tx();
  /* long type checking */

#ifdef _WIN32 
  if (my_unlikely((*a_list).get_type2(arg_ptr)
    !=attribute_list::tLong)) {
#else
  if (my_unlikely((*a_list).get_type2(arg_ptr)
    !=attribute_list::tLong
 #if defined(_ARCH_PPC64) || defined(__x86_64__) || defined(__LP64__)
    && (*a_list).get_type2(arg_ptr)
    !=attribute_list::tBigint
 #endif
    )) {
#endif /* ifndef _WIN32 */
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tLong));
    //return *this ;
  }
  /* end operator process */
  return end_tx(val);
}

#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
zas_stream& zas_stream::operator << (int64_t val)
#else
zas_stream& zas_stream::operator << (long long val)
#endif
{
  /* begin operator process */
  begin_tx();
  /* big int type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tBigint)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tBigint));
    //return *this ;
  }
  /* end operator process */
  return end_tx(val);
}

zas_stream& zas_stream::do_int_tx(int val)
{
  /* begin operator process */
  begin_tx();
  /* int type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tInt)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tInt));
    //return *this ;
  }
  /* end operator process */
  return end_tx(val);
}

zas_stream& zas_stream::operator << (int val)
{
  return do_int_tx(val);
}

zas_stream& zas_stream::operator << (short val)
{
  return do_int_tx(val);
}

zas_stream& zas_stream::operator << (unsigned val)
{
  return do_int_tx(val);
}

zas_stream& zas_stream::operator << (unsigned short val)
{
  return do_int_tx(val);
}

zas_stream& zas_stream::do_double_tx (double val)
{
  /* begin operator process */
  begin_tx();
  /* double type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tDouble)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tDouble));
    //return *this ;
  }
  /* end operator process */
  return end_tx(val);
}

zas_stream& zas_stream::operator << (long double val)
{
  /* begin operator process */
  begin_tx();
  /* double type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tLdouble)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tLdouble));
    //return *this ;
  }
  /* end operator process */
  return end_tx(val);
}

zas_stream& zas_stream::operator << (double val)
{
  return do_double_tx(val);
}

zas_stream& zas_stream::operator << (float val)
{
  return do_double_tx(val);
}

zas_stream& zas_stream::do_string_rx (char *str)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value, assume 'str' will be valid */
  if (cur_row && cur_row[col_ptr]) {
    strcpy(str,cur_row[col_ptr]);
  }
  return end_rx();
}

zas_stream& zas_stream::operator >> (char *str)
{
  return do_string_rx(str);
}

zas_stream& zas_stream::operator >> (unsigned char *str)
{
  return do_string_rx((char*)str);
}

zas_stream& zas_stream::operator >> (long &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0L;
  if (cur_row && cur_row[col_ptr]) {
    val = atol(cur_row[col_ptr]);
  } 
  return end_rx();
}

#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
zas_stream& zas_stream::operator >> (int64_t &val)
#else
zas_stream& zas_stream::operator >> (long long &val)
#endif
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0LL;
  if (cur_row && cur_row[col_ptr]) {
    val = atoll(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (int &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0;
  if (cur_row && cur_row[col_ptr]) {
    val = atoi(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (unsigned short &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0;
  if (cur_row && cur_row[col_ptr]) {
    val = atoi(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (short &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0;
  if (cur_row && cur_row[col_ptr]) {
    val = atoi(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (int8_t &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0;
  if (cur_row && cur_row[col_ptr]) {
    sscanf(cur_row[col_ptr],"%hhd",&val);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (unsigned &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0;
  if (cur_row && cur_row[col_ptr]) {
    val = atoi(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (double &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0.0;
  if (cur_row && cur_row[col_ptr]) {
    /* FIXME: is the precision enough for use? */
    val = atof(cur_row[col_ptr]);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (long double &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0.0;
  if (cur_row && cur_row[col_ptr]) {
    /* FIXME: the precision here may not be enough */
    //val = atof(cur_row[col_ptr]);
    sscanf(cur_row[col_ptr],"%Lf",&val);
  } 
  return end_rx();
}

zas_stream& zas_stream::operator >> (float &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  val = 0.0;
  if (cur_row && cur_row[col_ptr]) {
    /* FIXME: is the precision enough for use? */
    val = atof(cur_row[col_ptr]);
  } 
  return end_rx();
}

/* yzhou added 2016.12.1 */
zas_stream& zas_stream::operator >> (zas_datetime &val)
{
  if (!begin_rx())
    return *this ;
  /* copy the column value */
  bzero(&val, sizeof(val));
  if (cur_row && cur_row[col_ptr]) {
    MYSQL_TIME *tm = (MYSQL_TIME*)cur_row[col_ptr];

    val.year  = tm->year;
    val.month = tm->month ;
    val.day   = tm->day ;
    val.hour  = tm->hour ;
    val.minute= tm->minute ;
    val.second= tm->second ;
  } 
  return end_rx();
}

zas_stream& zas_stream::operator << (zas_datetime &val)
{
  MYSQL_TIME val_tm ;

  /* begin operator process */
  begin_tx();
  /* double type checking */
  if (my_unlikely((*a_list).get_type2(arg_ptr)
     !=attribute_list::tDatetime)) {
    printd("argument %d expects type '%s', but "
      "'%s' type is provided\n", arg_ptr, 
      type_str((*a_list).get_type2(arg_ptr)),
      type_str(attribute_list::tDatetime));
    //return *this ;
  }
  val_tm.year  = val.year;
  val_tm.month = val.month;
  val_tm.day   = val.day;
  val_tm.hour  = val.hour;
  val_tm.minute= val.minute;
  val_tm.second  = val.second;
  /* end operator process */
  return end_tx(val_tm);
}

MYSQL_FIELD* zas_stream::get_column_info(int nCol)
{
  //return (*m_exec)?(*m_exec).get_column_info(nCol):NULL;
  return (*m_exec).get_column_info(nCol);
}

/* 
 * class zas_exception
 */
zas_exception::zas_exception(char *stmt, char *m,
  char *var, int c)
{
  /* copy exception info */
  __stm_text = stmt ;
  __var_info = var ;
  __msg      = m ;
  stm_text   = (char*)__stm_text.data();
  var_info   = (char*)__var_info.data();
  msg        = (char*)__msg.data();
  /* translate NETWORK error code to oracle defined,
   *   refer to mysql_src_tree/include/errmsg.h */
  code = (c>=CR_SOCKET_CREATE_ERROR && c<=CR_SERVER_GONE_ERROR)
      || (c>=CR_WRONG_HOST_INFO && c<=CR_SERVER_LOST)
      || (c==CR_SSL_CONNECTION_ERROR)
      /* TODO: add more mysql network errors */
    ?12150:c;
}

zas_exception::~zas_exception(void)
{
}

