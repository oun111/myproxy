
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "mysqlc.h"
#include "mysqlc_priv.h"
#include "mysqlc_dbg.h"

#define OUTBAND_SVR  1

int main(void)
{
  int ret = 0;
  MYSQL *mysql = mysql_init(0);
  const char strStmt[] = 
#if OUTBAND_SVR == 1
  "select * from  sd_market_param_item  "
    //"where trademode= /*:f1<int>*/? and tradedate=/*:f2<bigint>*/?"
#else
  "select * from test_db.test_tbl "
  //"  where id>?"
#endif
  ;

  ret = mysql_real_connect(mysql,
#if OUTBAND_SVR ==1 
    "10.200.1.46",
    "nmts", "abc123",
    "whui",
    9007,
#else
    //"192.168.1.100",
    "127.0.0.1",
    "root", "123",
    "test_db",
    3306,
#endif
    "",0);
  if (!ret) {
    printd("error %d: %s, %s\n", mysql->last_errno,
      mysql->last_error, mysql->sql_state);
    return -1;
  }

  /* test 0 */
  {
    int val = 0;
    uint16_t i=0;
    MYSQL_BIND bnd[10];
    MYSQL_FIELD *fld = 0 ;
    MYSQL_RES *res = 0;
    char data[100][128];

    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    mysql_stmt_prepare(stmt, strStmt, strlen(strStmt));
    /*
     * execute phase #1
     */
    printd("prepare mode test 0\n");
    val = 1 ;
    *(int*)data[0] = val ;
    bnd[0].buffer = data[0] ;
    bnd[0].buffer_type = 3 ;
    val = 100 ;
    *(int*)data[1] = val ;
    bnd[1].buffer = data[1] ;
    bnd[1].buffer_type = 8 ;
    mysql_stmt_bind_param(stmt,bnd);
    //mysql_stmt_send_long_data(stmt,1,data,4);
    mysql_stmt_execute(stmt);
    /* copy result set info */
    res = mysql_stmt_result_metadata(stmt);
    fld = mysql_fetch_fields(res);
    for (i=0;i<mysql_num_fields(res);i++) {
      bnd[i].buffer_type = fld[i].type;
      bnd[i].buffer_length = fld[i].len;
      bnd[i].is_null = 0;
      bnd[i].length = 0;
      bnd[i].buffer = data[i] ;
    }
    /* bind result set, prepare to get rows */
    mysql_stmt_bind_result(stmt,bnd);
    mysql_stmt_store_result(stmt);
    dbug_output_results_bf(stmt);
    /*
     * execute phase #2
     */
    printd("prepare mode test 1\n");
    val = 4 ;
    *(int*)data[0] = val ;
    bnd[0].buffer = data[0] ;
    bnd[0].buffer_type = 3 ;
    val = 100 ;
    *(int*)data[1] = val ;
    bnd[1].buffer = data[1] ;
    bnd[1].buffer_type = 8 ;
    mysql_stmt_bind_param(stmt,bnd);
    //mysql_stmt_send_long_data(stmt,1,data,4);
    mysql_stmt_execute(stmt);
    /* copy result set info */
    res = mysql_stmt_result_metadata(stmt);
    fld = mysql_fetch_fields(res);
    for (i=0;i<mysql_num_fields(res);i++) {
      bnd[i].buffer_type = fld[i].type;
      bnd[i].buffer_length = fld[i].len;
      bnd[i].is_null = 0;
      bnd[i].length = 0;
      bnd[i].buffer = data[i] ;
    }
    /* bind result set, prepare to get rows */
    mysql_stmt_bind_result(stmt,bnd);
    mysql_stmt_store_result(stmt);
    dbug_output_results_bf(stmt);
    printd("affected rows: %llu\n", mysql_affected_rows(mysql));
    printd("total rows: %llu\n", mysql_num_rows(res));
    mysql_stmt_close(stmt);
  }
  /* test 1 */
  {
    printd("query mode test 2\n");
    printd("%s \n",strStmt);
    mysql_query(mysql,strStmt);
    dbug_output_results_tf(mysql);
    mysql_query(mysql,strStmt);
    dbug_output_results_tf(mysql);
  }
  mysql_close(mysql);
  return 0;
}
