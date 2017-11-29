#ifndef __DAL_H__
#define __DAL_H__

#include "mysqlc.h"
#ifdef _WIN32
#include "win/win.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum dal_items {
  dal_mysql,
  dal_max,
} ;

#define DAL_EXPORT 
#define DAL_IMPLICIT  static

/*
 * the basic DAL 
 */
typedef struct basic_dal_t {
  void *handle ;
  void *res ;
  int (*init)(void*);
  int (*end)(void*);
  int (*login)(void*,const char *host, 
   const char *user, const char *passwd, const char *db, 
   uint16_t port, const char *unix_socket);
  int (*last_errno)(void*);
  const char* (*last_error)(void*);
  int (*set_autocommit)(void*,bool);
  int (*commit)(void*);
  int (*rollback)(void*);
  int (*query)(void*,const char*);
  int (*select_db)(void*,const char*);
  ssize_t (*num_columns)(void*);
  size_t (*num_fields)(void*);
  size_t (*get_affected_rows)(void*);
  int (*store_result)(void*);
  void* (*get_column)(void*);
  bool (*test_disconnect)(void*);
  char** (*fetch_row)(void*);
  unsigned long long (*num_rows)(void*);
  void (*free_result)(void*);
} tBasicDal ;

/* initialize new basic DAL item */
DAL_EXPORT tBasicDal* create_basic_dal(int nDal) ;
/* free the basic DAL item */
DAL_EXPORT void release_basic_dal(void *pb);



/*
 * the statement based DAL 
 */
typedef struct statement_dal_t {
  void *handle ;
  void *res;
  int (*init)(void*,void*);
  int (*end)(void*);
  int (*prepare)(void *pStmt, 
    const char *query, uint32_t length);
  size_t (*get_param_count)(void*);
  int (*bind_param)(void*,void*);
  ssize_t (*num_columns)(void*);
  void* (*get_column)(void*);
  int (*get_result_metadata)(void*);
  int (*store_result)(void*,void*);
  int (*execute)(void*);
  int (*send_long_data)(void* pStmt, 
    uint16_t param_num, const char *data, 
    uint32_t length);
  int (*fetch)(void*);
  unsigned long long (*num_rows)(void*);
  int (*last_errno)(void*);
  const char* (*last_error)(void*);
  bool (*test_disconnect)(void*);
  void (*free_result)(void*);
  void*(*fetch_fields)(void*);
  size_t (*num_fields)(void*);
} tStmtDal ;

DAL_EXPORT tStmtDal* create_stmt_dal(int nDal);
DAL_EXPORT void release_stmt_dal(tStmtDal *pStmt);


#ifdef __cplusplus
}
#endif

#endif /* __DAL_H__*/

