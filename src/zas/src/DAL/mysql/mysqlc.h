/*
 *  Module Name:   mysqlc
 *
 *  Description:   implements a tiny mysql client driver
 *
 *  Author:        yzhou
 *
 *  Last modified: May 4, 2017
 *  Created:       Jun 13, 2016
 *
 *  History:       refer to 'ChangeLog'
 *
 */ 
#ifndef __MYSQLC_H__
#define __MYSQLC_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "list.h"
#include "mysqlc_priv.h"

#ifdef _WIN32
  #include "win/win.h"
#else
  #include <stdbool.h>
#endif

#ifdef __VER_STRING__
#undef __VER_STRING__
#endif
/* module version string */
#define __VER_STRING__  "v1.00.12 alpha"

#ifdef	__cplusplus
extern "C" {
#endif

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
enum enum_field_types { 
  MYSQL_TYPE_DECIMAL, MYSQL_TYPE_TINY,
  MYSQL_TYPE_SHORT,  MYSQL_TYPE_LONG,
  MYSQL_TYPE_FLOAT,  MYSQL_TYPE_DOUBLE,
  MYSQL_TYPE_NULL,   MYSQL_TYPE_TIMESTAMP,
  MYSQL_TYPE_LONGLONG,MYSQL_TYPE_INT24,
  MYSQL_TYPE_DATE,   MYSQL_TYPE_TIME,
  MYSQL_TYPE_DATETIME, MYSQL_TYPE_YEAR,
  MYSQL_TYPE_NEWDATE, MYSQL_TYPE_VARCHAR,
  MYSQL_TYPE_BIT,
  /*
    mysql-5.6 compatibility temporal types.
    they're only used internally for reading rbr
    mysql-5.6 binary log events and mysql-5.6 frm files.
    they're never sent to the client.
  */
  MYSQL_TYPE_TIMESTAMP2,
  MYSQL_TYPE_DATETIME2,
  MYSQL_TYPE_TIME2,
  MYSQL_TYPE_NEWDECIMAL=246,
  MYSQL_TYPE_ENUM=247,
  MYSQL_TYPE_SET=248,
  MYSQL_TYPE_TINY_BLOB=249,
  MYSQL_TYPE_MEDIUM_BLOB=250,
  MYSQL_TYPE_LONG_BLOB=251,
  MYSQL_TYPE_BLOB=252,
  MYSQL_TYPE_VAR_STRING=253,
  MYSQL_TYPE_STRING=254,
  MYSQL_TYPE_GEOMETRY=255
};

#define mysqlc_type2str(t) (\
  (t)==MYSQL_TYPE_DECIMAL?"decimal":\
  (t)==MYSQL_TYPE_TINY?"tiny":\
  (t)==MYSQL_TYPE_SHORT?"short":\
  (t)==MYSQL_TYPE_LONG?"long":\
  (t)==MYSQL_TYPE_FLOAT?"float":\
  (t)==MYSQL_TYPE_DOUBLE?"double":\
  (t)==MYSQL_TYPE_NULL?"null":\
  (t)==MYSQL_TYPE_TIMESTAMP?"timestamp":\
  (t)==MYSQL_TYPE_LONGLONG?"long long":\
  (t)==MYSQL_TYPE_INT24?"int24":\
  (t)==MYSQL_TYPE_DATE?"date":\
  (t)==MYSQL_TYPE_TIME?"time":\
  (t)==MYSQL_TYPE_DATETIME?"datetime":\
  (t)==MYSQL_TYPE_YEAR?"year":\
  (t)==MYSQL_TYPE_NEWDATE?"new date":\
  (t)==MYSQL_TYPE_VARCHAR?"varchar":\
  (t)==MYSQL_TYPE_BIT?"bit":\
  (t)==MYSQL_TYPE_TIMESTAMP2?"timestamp2":\
  (t)==MYSQL_TYPE_DATETIME2?"datetime2":\
  (t)==MYSQL_TYPE_TIME2?"time2":\
  (t)==MYSQL_TYPE_NEWDECIMAL?"new decimal":\
  (t)==MYSQL_TYPE_ENUM?"enum":\
  (t)==MYSQL_TYPE_SET?"set":\
  (t)==MYSQL_TYPE_TINY_BLOB?"tiny blob":\
  (t)==MYSQL_TYPE_MEDIUM_BLOB?"medium blob":\
  (t)==MYSQL_TYPE_LONG_BLOB?"long blob":\
  (t)==MYSQL_TYPE_BLOB?"blob":\
  (t)==MYSQL_TYPE_VAR_STRING?"var string":\
  (t)==MYSQL_TYPE_STRING?"long string":\
  (t)==MYSQL_TYPE_GEOMETRY?"geometry":\
  "n/a" \
)

/* 
 * copy from $(mysql source)/include/mysql.h 
 */
enum mysql_option 
{
  MYSQL_OPT_CONNECT_TIMEOUT, MYSQL_OPT_COMPRESS, MYSQL_OPT_NAMED_PIPE,
  MYSQL_INIT_COMMAND, MYSQL_READ_DEFAULT_FILE, MYSQL_READ_DEFAULT_GROUP,
  MYSQL_SET_CHARSET_DIR, MYSQL_SET_CHARSET_NAME, MYSQL_OPT_LOCAL_INFILE,
  MYSQL_OPT_PROTOCOL, MYSQL_SHARED_MEMORY_BASE_NAME, MYSQL_OPT_READ_TIMEOUT,
  MYSQL_OPT_WRITE_TIMEOUT, MYSQL_OPT_USE_RESULT,
  MYSQL_OPT_USE_REMOTE_CONNECTION, MYSQL_OPT_USE_EMBEDDED_CONNECTION,
  MYSQL_OPT_GUESS_CONNECTION, MYSQL_SET_CLIENT_IP, MYSQL_SECURE_AUTH,
  MYSQL_REPORT_DATA_TRUNCATION, MYSQL_OPT_RECONNECT,
  MYSQL_OPT_SSL_VERIFY_SERVER_CERT, MYSQL_PLUGIN_DIR, MYSQL_DEFAULT_AUTH,
  MYSQL_OPT_BIND,
  MYSQL_OPT_SSL_KEY, MYSQL_OPT_SSL_CERT, 
  MYSQL_OPT_SSL_CA, MYSQL_OPT_SSL_CAPATH, MYSQL_OPT_SSL_CIPHER,
  MYSQL_OPT_SSL_CRL, MYSQL_OPT_SSL_CRLPATH,
  MYSQL_OPT_CONNECT_ATTR_RESET, MYSQL_OPT_CONNECT_ATTR_ADD,
  MYSQL_OPT_CONNECT_ATTR_DELETE,
  MYSQL_SERVER_PUBLIC_KEY,
  MYSQL_ENABLE_CLEARTEXT_PLUGIN,
  MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS,

  /* MariaDB options */
  MYSQL_PROGRESS_CALLBACK=5999,
  MYSQL_OPT_NONBLOCK,
  MYSQL_OPT_USE_THREAD_SPECIFIC_MEMORY,
 
  /* 2017.2.4: switch to tune on/off fast receiving */
  MYSQL_OPT_FAST_RX = 6100,
};

/* 
 * copy from $(mysql source)/include/mysql.h 
 */
enum mysql_protocol_type
{
  MYSQL_PROTOCOL_DEFAULT, MYSQL_PROTOCOL_TCP, MYSQL_PROTOCOL_SOCKET,
  MYSQL_PROTOCOL_PIPE, MYSQL_PROTOCOL_MEMORY
};

typedef struct mysql_res_t {
  /* this's a reference to column set in 
   *  'MYSQL_STMT', so no need to free it */
  container *col_ref;
  /* raw data of result set, stored row by row */
  LIST *results ;
  /* row reading pointer */
  LIST *rp ;
  /* row count in result set */
  int rows ;
  /* row datas in text format, used in 
   *  query mode only */
  container row_data; 
  /* row pointer returned to client */
  char **data ;

  /* yzhou added 2017.2.1 begin */
  /* pointer to connection object */
  void *mysql_obj ;
  /* the latest packet number */
  int pkt_num ;
  /* more result-set flag */
  bool more ;
  /* yzhou added 2017.2.1 end */

  char reserve[1];
}__attribute__((packed)) MYSQL_RES ;


/* 
 * the mysql connection object 
 */
typedef struct mysql_conn_t {
  bool reconnect;
  /* connection info to server */
#ifdef _WIN32
  SOCKET sock ;
#else
  int sock ;
#endif
  struct __host_addr {
    uint32_t addr ;
    uint16_t port ;
  }  server ;
  /* packet number */
  uint32_t nr ;
  /* version */
  uint8_t proto_version;
  char svr_version[MAX_BUFF];
  /* connection id */
  uint32_t conn_id ;
  /* capability flag */
  uint32_t svr_cap ;
  /* character set */
  uint8_t charset ;
  /* server status */
  uint16_t svr_stat;
  /* auth data plugin management */
  char ap_data[MAX_BUFF];
  uint8_t ap_len ;
  char ap_name[64];
  /* client capability flags */
  uint32_t client_cap ;
  /* error management */
  int num_warning;
  uint64_t affected_rows ;

  /* yzhou added */
  uint64_t last_inserted_id ;

  /* the command buffer */
  container cmd_buff ;
  /* columns info set */
  container columns;
  /* raw result set */
  MYSQL_RES *res ;

  /* error management */
#define MAX_ERRMSG  512
#define MAX_SQLSTATE  6
  char last_error[MAX_ERRMSG+1];
  uint16_t last_errno ;
  char sql_state[MAX_SQLSTATE+1];

  /* connection info */
  struct connInfo_t {
    char host[MAX_NAME_LEN]; 
    uint16_t port;
    char user[MAX_NAME_LEN]; 
    char passwd[MAX_NAME_LEN]; 
    char db[MAX_NAME_LEN]; 
  } m_ci ;

  /* 
   * the uncompressed data buffer 
   */
  container ucomp_buff,
    /* the uncompressed tmp data buffer */
    ucomp_tmp;

  /* time stamp when connected OK */
#if 0
  uint64_t ts_ok ;
#else
  struct timestamp_t ts_ok ;
#endif

  /* compress test */
  bool compress ;

  /* fast rx mode */
  bool fast_rx ;

  /* reserve bytes */
  char reserve[1];
} __attribute__((packed)) MYSQL ;

  /*
   * the column's definition 
 */
typedef struct mysql_column_t {
  char schema[MAX_NAME_LEN];
  /* the owner table of this column */
  char tbl[MAX_NAME_LEN];
  /* alias of owner table */
  char tbl_alias[MAX_NAME_LEN];
  /* column name */
  char name[MAX_NAME_LEN];
  /* alias of column */
  char alias[MAX_NAME_LEN];
  /* character set of this column */
  uint16_t charset ;
  /* maximum length of this column */
  uint32_t len;
  /* column type */
  uint8_t type ;
  /* column flags */
  uint16_t flags ;
  /* FIXME: the max shown digit, what's this? */
  uint8_t decimal;
}__attribute__((packed)) MYSQL_COLUMN, MYSQL_FIELD;

/*
 * the place-holder binding info
 */
typedef struct mysql_bind_t {
  /* order number */
  uint16_t num ;
  /* long data flag */
  bool long_data_used ;
  /* buffer type */
  int buffer_type ;
  /* buffer length */
  int buffer_length ;
  /* the data buffer */
  void *buffer;
  /* various pointers */
  unsigned long *length;
  char *is_null;
  char *error;
  /* default buffers to pointers */
  unsigned long length_value ;
  char is_null_value;
  char error_value ;
}__attribute__((packed)) MYSQL_BIND ;

typedef struct mysql_result_t {
  /* the row raw data */
  container r ;
  /* the link list node */
  LIST node ;
}__attribute__((packed)) MYSQL_RESULT ;

/*
 * mysql statement object
 */
typedef struct mysql_stmt_t {
  /* the connection object */
  MYSQL *mysql;

  /* statement id */
  uint32_t stmt_id ;
  /* cursor type */
  int cursor_type ;

  /* containers  */
#define COLUMN(o) ((MYSQL_COLUMN*)((o).c))
#define PARAM(o)  ((MYSQL_BIND*)((o).c))
#define BIND(o)  ((MYSQL_BIND*)((o).c))
  /* place-holder & signal-row result */
  container params, binds;

  /* server-returned place-holder infos */
  container pholders;

  /* error references */
  char *last_error ;
  uint16_t *last_errno ;
  char *sql_state ;

  /* time stamp when prepared OK */
#if 0
  uint64_t ts_ok ;
#else
  struct timestamp_t ts_ok ;
#endif

  /* columns info set */
  container columns;
  /* raw result set */
  MYSQL_RES *res ;

  /* reserve bytes */
  char reserve[1];
}__attribute__((packed)) MYSQL_STMT;

/*
 * mysql date time object
 */
typedef struct mysql_time_t {
  uint16_t  year, month, day, hour, minute, second;
  uint32_t second_part;
  bool neg;
  int time_type;
} MYSQL_TIME;


extern int mysql_real_connect(MYSQL *mysql, const char *host, 
 const char *user, const char *passwd, const char *db, 
 uint16_t port, const char *unix_socket,uint32_t client_flag);

extern MYSQL* mysql_init(MYSQL *mysql);

extern void mysql_close(MYSQL *mysql);

extern int mysql_select_db(MYSQL *mysql, const char *schema);

extern bool mysql_autocommit(MYSQL *mysql, bool auto_mode);

extern MYSQL_STMT* mysql_stmt_init(MYSQL *mysql);

extern bool mysql_stmt_close(MYSQL_STMT *stmt);

extern int mysql_stmt_prepare(MYSQL_STMT *stmt, 
  const char *query, uint32_t length);

extern unsigned long mysql_stmt_param_count(MYSQL_STMT *stmt);

extern bool mysql_stmt_bind_param(MYSQL_STMT *stmt, MYSQL_BIND *bnd) ;

extern bool mysql_stmt_send_long_data(MYSQL_STMT *stmt, 
  uint16_t param_num, const char *data, uint32_t length);

extern bool mysql_stmt_send_long_data_direct(MYSQL_STMT*,
  const char*,size_t);

extern bool mysql_stmt_execute(MYSQL_STMT *stmt);

extern int mysql_stmt_execute_direct(MYSQL_STMT*,char*,size_t);

extern void mysql_free_result(MYSQL_RES *result) ;

extern MYSQL_RES* mysql_stmt_result_metadata(MYSQL_STMT *stmt);

extern unsigned int mysql_num_fields(MYSQL_RES *res) ;

extern MYSQL_FIELD* mysql_fetch_fields(MYSQL_RES *res);

extern bool mysql_stmt_bind_result(MYSQL_STMT *stmt, MYSQL_BIND *my_bind);

extern int mysql_stmt_store_result(MYSQL_STMT *stmt) ;

extern int mysql_stmt_fetch(MYSQL_STMT *stmt);

extern void dbug_output_results_bf(MYSQL_STMT *stmt);

extern unsigned long long mysql_affected_rows(MYSQL *mysql);

extern unsigned long long mysql_num_rows(MYSQL_RES *res);

extern int mysql_query(MYSQL *mysql, const char *query);

extern MYSQL_RES * mysql_store_result(MYSQL *mysql) ;

extern char** mysql_fetch_row(MYSQL_RES *res) ;

extern void dbug_output_results_tf(MYSQL *mysql);

extern bool my_init(void);

extern void my_end(int infoflag);

extern unsigned long long mysql_stmt_num_rows(MYSQL_STMT *stmt) ;

extern int mysql_options(MYSQL *mysql,enum mysql_option option, 
  const void *arg) ;

extern bool mysql_commit(MYSQL *mysql);

extern bool mysql_rollback(MYSQL *mysql);

extern const char *mysql_stmt_error(MYSQL_STMT *stmt);

extern uint16_t mysql_stmt_errno(MYSQL_STMT *stmt);

extern const char *mysql_error(MYSQL *mysql);

extern uint16_t mysql_errno(MYSQL *mysql);

extern bool mysql_is_disconnect(MYSQL *mysql);

extern bool mysql_stmt_is_disconnect(MYSQL_STMT *stmt);

extern bool mysql_query_is_disconnect(MYSQL*);

//extern void mysql_result_begin(MYSQL *mysql);

extern int mysql_result_next(void*,bool,char**,size_t*,bool);

extern int mysql_pickup_text_col_val(void*,int,
  char*,size_t,const char*, uint8_t*,size_t*,size_t*);

extern int mysql_pickup_bin_col_val(void*,int,
  char*,size_t,const char*, uint8_t*,size_t*,size_t*);

extern int mysql_calc_blob_count(char*,int,int*);

extern int mysql_stmt_exec_get_ph(char*,int,int,
  int*,/*void***/char*);

extern size_t mysql_stmt_exec_get_ph_subsequent(
  char*,bool,int,int,char*);

#ifdef	__cplusplus
}
#endif

#endif /*  __MYSQLC_H__ */

