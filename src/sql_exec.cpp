
#include <vector>
#include "attrib_list.h"
#include "zas.h"
#include "sql_exec.h"
#include "dbug.h"
#include "dal.h"


/* 
 * class exec_base
 */
exec_base::exec_base(tBasicDal *pb):
  m_bd(pb)
{
}

int64_t exec_base::get_rpc(void)
{
  return m_bd->get_affected_rows(m_bd);
}

char* exec_base::last_error(void)
{
  return const_cast<char*>(m_bd->last_error(m_bd));
}

int exec_base::last_errno(void)
{
  return m_bd->last_errno(m_bd);
}

/* 
 * class mysql_qry
 */
mysql_qry::mysql_qry(tBasicDal *pb):
  exec_base(pb)
{
  //printd("using zas query method\n");
}

mysql_qry::~mysql_qry(void)
{
}

char** mysql_qry::row(void)
{
  return m_bd->fetch_row(m_bd);
}

long long int mysql_qry::num_rows(void)
{
  return m_bd->num_rows(m_bd);
}

int mysql_qry::num_fields(void)
{
  return m_bd->num_fields(m_bd);
}

int mysql_qry::prepare(attribute_list &l,std::string &s)
{
  return 1;
}

int mysql_qry::store_result(void)
{
  return m_bd->store_result(m_bd);
}

int mysql_qry::release(void)
{
  m_bd->free_result(m_bd);
  return 1;
}

/* do execute the statement */
int mysql_qry::direct_exec(const char *str)
{
  /* free previous result if exist */
  release();
  /* try reconnection if needed */
  reconnect();
  /* do zas_connect::execute */
  return m_bd->query(m_bd,str);
}

/* do execute the statement */
int mysql_qry::direct_exec(std::string &s)
{
  /* free previous result if exist */
  release();
  /* try reconnection if needed */
  reconnect();
  /* do zas_connect::execute */
  return m_bd->query(m_bd,s.c_str());
}

MYSQL_FIELD* mysql_qry::get_column_info(int nCol)
{
  if (nCol>=m_bd->num_columns(m_bd)) {
    return NULL;
  }
  return ((MYSQL_FIELD*)m_bd->get_column(m_bd))+nCol ;
}

void mysql_qry::reconnect(void)
{
  if (m_bd->test_disconnect(m_bd)) {
    printd("doing reconnection\n");
  }
}

/* 
 * class mysql_prep
 */
mysql_prep::mysql_prep(tBasicDal *pb, const int nDal) :
  exec_base(pb),
  m_sd(0),
  m_dal(nDal),
  pbArg(NULL),
  pbRes(NULL),
  pArgBuf(NULL),
  pResBuf(NULL),
  pResStr(NULL)
{
  //printd("using zas prepare method\n");
}

mysql_prep::~mysql_prep(void)
{
  clean();
}

int mysql_prep::num_fields(void)
{
  return m_sd->num_fields(m_sd);
}

int mysql_prep::prepare(attribute_list &list,
  std::string &stmt)
{
  int cnt = 0;
  /*std::string &*/sql_stmt = stmt ;
  
  /* do clean up resource if neccesary */
  clean();
  m_sd = create_stmt_dal(m_dal);
  /* prepare the statement */
  pAlist = &list;
  cnt    = pAlist->count();
  /* do statement prepare actions */
  if (!m_sd || m_sd->init(m_bd,m_sd)) {
    printd("statement init failed\n");
    return 0;
  }

  reconnect();

  if (!m_sd || m_sd->prepare(m_sd, sql_stmt.c_str(), 
      sql_stmt.size())) {
    printd("statement: %s\nprepare failed, %s\n", 
      sql_stmt.c_str(), last_error());
    /* throw an exception */
    exceptions::__throw(last_errno(),
      sql_stmt.c_str(),last_error(),"") ;
    /* release statement */
    m_sd->end(m_sd);
    return 0;
  }

  /* dont do the rest steps if 
   *   no arguments in statement */
  if (cnt<=0) {
    //printd("no argument in attribute list\n");
    return 1;
  }

  /* check parameter count */
  if (m_sd->get_param_count(m_sd)!=(size_t)cnt) {
    printd("statement param count %ld error, "
      "expect %d\n", m_sd->get_param_count(m_sd), 
      cnt);
    m_sd->end(m_sd);
    return 0;
  }

  /* alloc and fill arguments into argument struct */
  alloc_args(cnt);
  fill_args();
  m_sd->bind_param(m_sd,pbArg);
  return 1;
}

int mysql_prep::alloc_res(int cnt)
{
  pResStr = (char**)realloc(pResStr,
    sizeof(char*)*cnt);
  if (!pResStr) {
    return 0;
  }
  /* allocate data structs */
  pbRes  = (MYSQL_BIND*)realloc(pbRes,
    sizeof(MYSQL_BIND)*cnt);
  pResBuf= (tDbe*)realloc(pResBuf,
    sizeof(tDbe)*cnt);
  if (!pbRes || !pResBuf) {
    return 0;
  }
  memset(pbRes,  0,sizeof(*pbRes)*cnt);
  memset(pResBuf,0,sizeof(tDbe)*cnt);
  return 1;
}

void mysql_prep::free_res(void)
{
  if (pResStr) {
    free(pResStr) ;
    pResStr = NULL;
  }
  if (pResBuf) {
    free(pResBuf) ;
    pResBuf = NULL;
  }
  if (pbRes) {
    free(pbRes) ;
    pbRes = NULL;
  }
}

int mysql_prep::alloc_args(int cnt)
{
  /* allocate data structs */
  pbArg  = (MYSQL_BIND*)realloc(pbArg,
    sizeof(MYSQL_BIND)*cnt);
  pArgBuf= (tDbe*)realloc(pArgBuf,
    sizeof(tDbe)*cnt);
  if (!pbArg || !pArgBuf) {
    return 0;
  }
  memset(pbArg,  0,sizeof(*pbArg)*cnt);
  memset(pArgBuf,0,sizeof(tDbe)*cnt);
  return 1;
}

void mysql_prep::debug(void)
{
  printd("1: %p, %p, %p\n", 
    pArgBuf, pbArg, pResStr);
}

void mysql_prep::free_args(void)
{
  if (pArgBuf) {
    free(pArgBuf) ;
    pArgBuf = NULL;
  }
  if (pbArg) {
    free(pbArg) ;
    pbArg = NULL;
  }
}

int mysql_prep::fill_args(void)
{
  int i, type;
  
  for (i=0; i<pAlist->count(); i++) {
    type                = (*pAlist).get_type(i);
    pbArg[i].error      = &pArgBuf[i].error ;
    pbArg[i].is_null    = &pArgBuf[i].is_null ;
    pbArg[i].length     = &pArgBuf[i].length ;
    pArgBuf[i].is_null  = false ;
    /* assign argument type and buffer */
    switch(type) {
      case attribute_list::tInt:
        /* to compatible with 64/32bit platforms */
        pbArg[i].buffer      = &pArgBuf[i].u.val32;
        pbArg[i].buffer_type = MYSQL_TYPE_LONG;
        break ;
      case attribute_list::tLong:
        /* yzhou: we require more precisions for 'long' type */
#if 0
        pbArg[i].buffer      = &pArgBuf[i].u.val32;
        pbArg[i].buffer_type = MYSQL_TYPE_LONG ;
        break ;
#endif
      case attribute_list::tBigint:
        pbArg[i].buffer      = &pArgBuf[i].u.val64;
        pbArg[i].buffer_type = MYSQL_TYPE_LONGLONG ;
        break ;
      case attribute_list::tChar:
        pbArg[i].buffer      = &pArgBuf[i].u.buf;
        pbArg[i].buffer_type  = MYSQL_TYPE_STRING ;
        pbArg[i].buffer_length= LONG_DATABUF_LEN ;
        break ;
      case attribute_list::tLdouble:
      case attribute_list::tDouble:
        pbArg[i].buffer      = &pArgBuf[i].u.val_d;
        pbArg[i].buffer_type = MYSQL_TYPE_DOUBLE ;
        break ;
      /* added 2016.12.1 */
      case attribute_list::tDatetime:
        pbArg[i].buffer      = &pArgBuf[i].u.buf;
        pbArg[i].buffer_type = MYSQL_TYPE_DATETIME ;
        break ;
      default: 
        printd("unhandled attribute type %d\n", type);
        break ;
    } /* end switch() */
  }
  return 1;
}

MYSQL_FIELD* mysql_prep::get_column_info(int nCol)
{
  if (nCol>=m_sd->num_columns(m_sd)) {
    return NULL;
  }
  return ((MYSQL_FIELD*)m_sd->get_column(m_sd))+nCol ;
}

void mysql_prep::clean(void)
{
  free_res();
  free_args();

  if (m_sd) {

    m_sd->free_result(m_sd);

    m_sd->end(m_sd);

    release_stmt_dal(m_sd);
    m_sd = 0;
  }
}

int mysql_prep::release(void)
{
  if (m_sd) {
    m_sd->free_result(m_sd);
  }
  return 1;
}

int mysql_prep::store_result(void)
{
  int cnt = 0;
  uint8_t i = 0;
  MYSQL_FIELD *f_list ;

  release();

  /* get result meta data */
  if (m_sd->get_result_metadata(m_sd)) {
    return 0;
  }
  if ((cnt=num_fields())<=0) {
    return 0;
  }
  /* allocate result buffers */
  if (!alloc_res(cnt)) {
    return 0;
  }
  /* get fields attributes */
  f_list = static_cast<MYSQL_FIELD*>(m_sd->fetch_fields(m_sd));
  for (i=0; i<cnt; i++) {
    pbRes[i].buffer       = pResBuf[i].u.buf ;
    pbRes[i].error        = &pResBuf[i].error ;
    pbRes[i].is_null      = &pResBuf[i].is_null ;
    pbRes[i].length       = &pResBuf[i].length ;
    pbRes[i].buffer_type  = f_list[i].type ;
    pbRes[i].buffer_length= LONG_DATABUF_LEN ;
  }
  /* load result from remote */
  if (m_sd->store_result(m_sd,pbRes)) {
    return 0;
  }
  return 1;
}

int mysql_prep::do_exec(void)
{
  /* maintain connection to database if needed */
  reconnect();
  /* try execution */
  return !m_sd->execute(m_sd);
}

/* do execute the prepared statement */
int mysql_prep::direct_exec(std::string &unused)
{
  return do_exec();
}

int mysql_prep::direct_exec(const char *unused)
{
  return do_exec();
}

int mysql_prep::do_insert(int n, void *val, int sz)
{
  if (n>=pAlist->count()) {
    return 0;
  }
  if (!pbArg) {
    printd("fatal: argument buffer not initialize\n");
    return 0;
  }
  memcpy(pbArg[n].buffer,val,sz);
  return 1;
}

/* insert argument values into argument buffer */
int mysql_prep::insert(int n, float val)
{
  return do_insert(n,&val,sizeof(val));
}

int mysql_prep::insert(int n, long double val)
{
  return do_insert(n,&val,sizeof(val));
}

int mysql_prep::insert(int n, double val)
{
  return do_insert(n,&val,sizeof(val));
}

#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
int mysql_prep::insert(int n, int64_t val)
#else
int mysql_prep::insert(int n, long long val)
#endif
{
  return do_insert(n,&val,sizeof(val));
}

int mysql_prep::insert(int n, long val)
{
  return do_insert(n,&val,sizeof(val));
}

int mysql_prep::insert(int n, int val)
{
  return do_insert(n,&val,sizeof(val));
}

/* added 2016.12.1 */
int mysql_prep::insert(int n, MYSQL_TIME *val)
{
  return do_insert(n,val,sizeof(*val));
}

int mysql_prep::insert(int n, char *buf)
{
  if (!do_insert(n,buf,strlen(buf)+1))
    return 0;
  reconnect();
  /* note: REMEMBER to send string arguments with 
   *   this function! */
  m_sd->send_long_data(m_sd,n,buf,strlen(buf));
  return 1;
}

/* get a row from result set */
char** mysql_prep::row(void)
{
  int i=0;

  /* no more rows */
  if (m_sd->fetch(m_sd)) {
    return NULL;
  }
  /* assign values by type into result 
   *   buffer itself */
  for (i=0; i<num_fields(); i++) {
    switch(pbRes[i].buffer_type) {
      /* yzhou added 20161201: leave MYSQL_TIME in its
       *  original format */
      case MYSQL_TYPE_DATETIME:
      /* type "new decimal" should be 
       *   deal as string */
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
        /*sprintf((char*)pbRes[i].buffer,"%s",
          (char*)pbRes[i].buffer);*/
        break ;
      case MYSQL_TYPE_LONG:
        /* to compatible with 64/32bit platforms */
        /*sprintf((char*)pbRes[i].buffer,"%ld",
          *(long*)pbRes[i].buffer);*/
        sprintf((char*)pbRes[i].buffer,"%d",
          *(int*)pbRes[i].buffer);
        break ;
      case MYSQL_TYPE_LONGLONG:
        sprintf((char*)pbRes[i].buffer,"%lld",
          *(long long*)pbRes[i].buffer);
        break ;
      case MYSQL_TYPE_DOUBLE:
        sprintf((char*)pbRes[i].buffer,"%f",
          *(double*)pbRes[i].buffer);
        break ;
      case MYSQL_TYPE_FLOAT:
        sprintf((char*)pbRes[i].buffer,"%f",
          *(float*)pbRes[i].buffer);
        break ;
      default: 
        printd("unhandled buffer type %d\n", 
          pbRes[i].buffer_type);
        break ;
    }
    /* XXX: yzhou added: just zero the field if 
     *   it's 'null' */
    if (*pbRes[i].is_null) {
      *(long double*)pbRes[i].buffer=0;
    }
    pResStr[i] = (char*)pbRes[i].buffer ;
  }
  return (char**)pResStr ;
}

/* get row count of current result set */
long long int mysql_prep::num_rows(void)
{
  return m_sd->num_rows(m_sd);
}

int mysql_prep::last_errno(void)
{
  return m_sd->last_errno(m_sd);
}

char* mysql_prep::last_error(void)
{
  return const_cast<char*>(m_sd->last_error(m_sd));
}

void mysql_prep::reconnect(void)
{
#ifdef DB_RECONN
  if (m_sd->test_disconnect(m_sd)) {
    /* disable autocommit when reconnected */
    m_bd->set_autocommit(m_bd,false);
    /* re-prepare the statement */
    m_sd->prepare(m_sd, sql_stmt.c_str(),
      sql_stmt.size());
    printd("doing reconnection\n");
  }
#endif /* DB_RECONN==1 */
}


/*
 * exception
 */
namespace exceptions {

  /* throw a zas related exception */
  void __throw(int code, const char *stmt, 
    const char *errmsg, const char *var) 
  {
    if (code>0) {
      throw zas_exception( 
        (char*)stmt, 
        (char*)errmsg, 
        (char*)var, 
        code  
        );
    } 
  } 
};

