
#include "dal.h"
#include "mysqlc.h"
#include "dbug.h"

DAL_IMPLICIT int mysql_stmt_dal_init(void *pDal, void *pStmt)
{
  tBasicDal *pd = (tBasicDal*)pDal ;
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  ps->handle = mysql_stmt_init(pd->handle);
  if (!ps->handle) {
    return -1;
  }
  return 0;
}

DAL_IMPLICIT int mysql_stmt_dal_close(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  mysql_stmt_close(ps->handle);
  return 0;
}

DAL_IMPLICIT int mysql_stmt_dal_prepare(void *pStmt, 
  const char *stmt, uint32_t length)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;
  int ret = 0;

  ret = mysql_stmt_prepare(ps->handle, stmt, length);
  return ret;
}

DAL_IMPLICIT size_t mysql_stmt_dal_get_param_count(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_param_count(ps->handle);
}

DAL_IMPLICIT int mysql_stmt_dal_bind_param(void *pStmt, void *params)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_bind_param(ps->handle,params);
}

DAL_IMPLICIT ssize_t mysql_stmt_dal_num_columns(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return ((MYSQL_STMT*)ps->handle)->columns.number;
}

DAL_IMPLICIT void* mysql_stmt_dal_get_column(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return ((MYSQL_STMT*)ps->handle)->columns.c;
}

DAL_IMPLICIT int mysql_stmt_dal_get_result_metadata(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  ps->res = mysql_stmt_result_metadata(ps->handle);
  return !ps->res?-1:0;
}

DAL_IMPLICIT int mysql_stmt_dal_store_result(void *pStmt, 
  void *pRes)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  if (mysql_stmt_bind_result(ps->handle,pRes) ||
      mysql_stmt_store_result(ps->handle)) {
    return -1;
  }
  return 0;
}

DAL_IMPLICIT int mysql_stmt_dal_execute(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_execute(ps->handle);
}

DAL_IMPLICIT int mysql_stmt_dal_send_long_data(void *pStmt, 
  uint16_t param_num, const char *data, 
  uint32_t length)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_send_long_data(ps->handle,param_num,
    data, length);
}

DAL_IMPLICIT int mysql_stmt_dal_fetch(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_fetch(ps->handle);
}

DAL_IMPLICIT unsigned long long mysql_stmt_dal_num_rows(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_num_rows(ps->handle);
}

DAL_IMPLICIT int mysql_stmt_dal_last_errno(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_errno(ps->handle);
}

DAL_IMPLICIT const char* mysql_stmt_dal_last_error(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_error(ps->handle);
}

DAL_IMPLICIT bool mysql_stmt_dal_test_disconnect(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_stmt_is_disconnect(ps->handle);
}

DAL_IMPLICIT void mysql_stmt_dal_free_result(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  if (ps->res) {
    mysql_free_result(ps->res);
    ps->res = NULL;
  }
}

DAL_IMPLICIT void* mysql_stmt_dal_fetch_fields(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_fetch_fields(ps->res);
}

DAL_IMPLICIT size_t mysql_stmt_dal_num_fields(void *pStmt)
{
  tStmtDal *ps  = (tStmtDal*)pStmt ;

  return mysql_num_fields(ps->res);
}



DAL_EXPORT tStmtDal* create_stmt_dal(int nDal)
{
  tStmtDal *ps  = 0 ;

  if (nDal==dal_mysql) {

    ps = (tStmtDal*)malloc(sizeof(tStmtDal));

    ps->handle = NULL;
    ps->res    = NULL;
    ps->init   = mysql_stmt_dal_init;
    ps->end    = mysql_stmt_dal_close;
    ps->prepare= mysql_stmt_dal_prepare;
    ps->bind_param  = mysql_stmt_dal_bind_param;
    ps->num_columns = mysql_stmt_dal_num_columns;
    ps->get_column  = mysql_stmt_dal_get_column;
    ps->store_result= mysql_stmt_dal_store_result;
    ps->test_disconnect = mysql_stmt_dal_test_disconnect;
    ps->get_param_count = mysql_stmt_dal_get_param_count;
    ps->send_long_data  = mysql_stmt_dal_send_long_data;
    ps->execute  = mysql_stmt_dal_execute;
    ps->fetch    = mysql_stmt_dal_fetch;
    ps->num_rows = mysql_stmt_dal_num_rows;
    ps->last_error   = mysql_stmt_dal_last_error;
    ps->last_errno   = mysql_stmt_dal_last_errno;
    ps->free_result  = mysql_stmt_dal_free_result;
    ps->fetch_fields = mysql_stmt_dal_fetch_fields;
    ps->get_result_metadata= mysql_stmt_dal_get_result_metadata;
    ps->num_fields= mysql_stmt_dal_num_fields;

  } else {
    printd("warning: unknown statement DAL type %d\n", nDal);
  }

  return ps;
}

DAL_EXPORT void release_stmt_dal(tStmtDal *pStmt)
{
  if (pStmt) {
    free(pStmt);
  }
}

