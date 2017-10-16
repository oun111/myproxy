
#include "dal.h"
#include "mysqlc.h"
#include "dbug.h"

/*
 * mysql DAL implementations
 */
DAL_IMPLICIT int mysql_dal_init(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;
  int val = 0;

  my_init();

  /* create and initialize DAL object */
  if (!(pd->handle=mysql_init(NULL))) {
    /* TODO: err message here */
    return -1;
  }

  /* set connection options */
  mysql_options(pd->handle, MYSQL_OPT_LOCAL_INFILE, &val);
  val = MYSQL_PROTOCOL_DEFAULT ;
  mysql_options(pd->handle, MYSQL_OPT_PROTOCOL, &val);
  mysql_options(pd->handle, MYSQL_SET_CHARSET_NAME, "utf8");
  val = 0;
  mysql_options(pd->handle, MYSQL_OPT_FAST_RX, &val);
  //mysql_options(pd->handle, MYSQL_OPT_COMPRESS, 0);
#ifdef DB_RECONN
  /* enable auto reconnection machanism 
   *   in mysql-client lib */
  ((MYSQL*)pd->handle)->reconnect = true;
#endif

  return 0;
}

DAL_IMPLICIT int mysql_dal_close(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  if (pd->handle) {
    mysql_close(pd->handle);
    pd->handle = 0;
    my_end(0);
  }
  return 0;
}

DAL_IMPLICIT int mysql_dal_login(void *pDal, const char *host, 
  const char *user, const char *passwd, const char *db, 
  uint16_t port, const char *unix_socket)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_real_connect(pd->handle, 
    host, user, passwd, db, port, unix_socket, 
    0/*CLIENT_MULTI_STATEMENTS*/) ;
}

DAL_IMPLICIT int mysql_dal_last_errno(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_errno(pd->handle);
}

DAL_IMPLICIT const char* mysql_dal_last_error(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_error(pd->handle);
}

DAL_IMPLICIT int mysql_dal_set_autocommit(void *pDal, bool ac)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_autocommit(pd->handle,ac);
}

DAL_IMPLICIT int mysql_dal_commit(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_commit(pd->handle);
}

DAL_IMPLICIT int mysql_dal_rollback(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_rollback(pd->handle);
}

DAL_IMPLICIT int mysql_dal_query(void *pDal, const char *stmt)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_query(pd->handle,stmt);
}

DAL_IMPLICIT int mysql_dal_sel_db(void *pDal, const char *db)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_select_db(pd->handle,db);
}

DAL_IMPLICIT ssize_t mysql_dal_num_columns(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return ((MYSQL*)pd->handle)->columns.number;
}

DAL_IMPLICIT size_t mysql_dal_num_fields(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_num_fields(pd->res);
}

DAL_IMPLICIT size_t mysql_dal_affected_rows(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_affected_rows(pd->handle);
}

DAL_IMPLICIT int mysql_dal_store_result(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return !!(pd->res=mysql_store_result(pd->handle));
}

DAL_IMPLICIT void* mysql_dal_get_column(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return ((MYSQL*)pd->handle)->columns.c;
}

DAL_IMPLICIT bool mysql_dal_test_disconnect(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_query_is_disconnect(pd->handle);
}

DAL_IMPLICIT char** mysql_dal_fetch_row(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_fetch_row(pd->res);
}

DAL_IMPLICIT void mysql_dal_free_result(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  if (pd->res) {
    mysql_free_result(pd->res);
    pd->res = 0;
  }
}

DAL_IMPLICIT unsigned long long mysql_dal_num_rows(void *pDal)
{
  tBasicDal *pd = (tBasicDal*)pDal ;

  return mysql_num_rows(pd->res);
}



/* initialize new basic DAL item */
DAL_EXPORT tBasicDal* create_basic_dal(int nDal) 
{
  tBasicDal *pb = 0;

  if (nDal==dal_mysql) {

    pb = (tBasicDal*)malloc(sizeof(tBasicDal));

    pb->handle = NULL;
    pb->res  = NULL;
    pb->init = mysql_dal_init;
    pb->end  = mysql_dal_close;
    pb->login= mysql_dal_login;
    pb->last_error = mysql_dal_last_error;
    pb->last_errno = mysql_dal_last_errno;
    pb->set_autocommit= mysql_dal_set_autocommit;
    pb->commit  = mysql_dal_commit;
    pb->rollback= mysql_dal_rollback;
    pb->query   = mysql_dal_query;
    pb->num_rows= mysql_dal_num_rows;
    pb->select_db  = mysql_dal_sel_db;
    pb->num_columns= mysql_dal_num_columns;
    pb->num_fields = mysql_dal_num_fields;
    pb->get_column = mysql_dal_get_column;
    pb->fetch_row  = mysql_dal_fetch_row;
    pb->get_affected_rows = mysql_dal_affected_rows;
    pb->test_disconnect   = mysql_dal_test_disconnect;
    pb->store_result = mysql_dal_store_result;
    pb->free_result  = mysql_dal_free_result;

  } else {
    printd("warning: unknown DAL type %d\n", nDal);
  }

  return pb;
} 

DAL_EXPORT void release_basic_dal(void *pb)
{
  if (pb) {
    free(pb);
  }
}

