#ifndef __SQL_EXEC_H__
#define __SQL_EXEC_H__

#include "mysqlc.h"

/*
 * definitions of private structs and values 
 */
class attribute_list;
class zas_connect;
using tBasicDal = struct basic_dal_t;
using tStmtDal  = struct statement_dal_t;


namespace exceptions {
  /* throw a zas related exception */
  void __throw(int code, const char *stmt, 
    const char *errmsg, const char *var) ;
};

class exec_base {
protected:
#if 0
  zas_connect *m_conn ;
  /* mysql object pointer */
  MYSQL *pMysql ;
  /* mysql result set pointer */
  MYSQL_RES *pMyRes ;
#else
  tBasicDal *m_bd ;
#endif
public:
  exec_base(tBasicDal*);
  virtual ~exec_base(void){};
  /* get last error info */
  char* last_error();
  int last_errno();
  /* get affected rows */
  int64_t get_rpc(void);
  /* get field count of current result set */
  virtual int num_fields(void) = 0 ;
  /* get row count of current result set */
  virtual long long int num_rows(void)=0 ;
  /* free structure resource */
  virtual int release(void)=0;
  /* prepare for the sql statement */
  virtual int prepare(attribute_list&,
    std::string&)=0;
  /* insert argument value into statement */
  virtual int insert(int,char*)=0;
  virtual int insert(int,int)=0;
  virtual int insert(int,long)=0;
#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
  virtual int insert(int,int64_t)=0;
#else
  virtual int insert(int,long long)=0;
#endif
  virtual int insert(int,double)=0;
  virtual int insert(int,long double)=0;
  virtual int insert(int,float)=0;
  virtual int insert(int,MYSQL_TIME*)=0;
  /* do execute the prepared statement */
  virtual int direct_exec(std::string&)=0;
  virtual int direct_exec(const char*)=0;
  /* store the result */
  virtual int store_result(void)=0 ;
  /* get a row from result */
  virtual char** row(void)=0 ;
  /* do reconnection */
  virtual void reconnect(void)=0;

  virtual MYSQL_FIELD* get_column_info(int)=0;
} ;

/*
 * brief: encapsulates the mysql-query mechanisms
 */
class mysql_qry : public exec_base {

protected:
  /* do reconnection */
  void reconnect(void) ;

public:
  mysql_qry(tBasicDal*);
  ~mysql_qry(void);
  /* get field count of current result set */
  int num_fields(void) ;
  /* get a row from result */
  char** row(void) ;
  /* get row count of current result set */
  long long int num_rows(void) ;
  /* prepare for the sql statement */
  int prepare(attribute_list&,std::string&);
  /* store the result */
  int store_result(void) ;
  /* free structure resource */
  int release(void);
  /* do execute the prepared statement */
  int direct_exec(std::string&);
  int direct_exec(const char*);
  /* XXX: these inherited functions are no-sense */
  int insert(int,char*)  { return 1; };
  int insert(int,int)    { return 1; };
  int insert(int,long)   { return 1; };
#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
  int insert(int,int64_t){ return 1; };
#else
  int insert(int,long long){ return 1; };
#endif
  int insert(int,double) { return 1; };
  int insert(int,long double) { return 1; };
  int insert(int,float)  { return 1; };
  int insert(int,MYSQL_TIME*) { return 1; }
  MYSQL_FIELD* get_column_info(int);
} ;

/*
 * brief: encapsulates the mysql-prepare mechanisms
 */
class mysql_prep : public exec_base {

private:
  /* inputed by mysql_stream, no need to manage */
  attribute_list *pAlist ;
#if 0
  MYSQL_STMT *pMyStmt ;
#else
  tStmtDal *m_sd ;
  const int m_dal ;
#endif
  MYSQL_BIND *pbArg, *pbRes;
  using tDbe = struct tDBElement {
    long unsigned length ;
    char error;
    char is_null ;
    union {
      float  val_f;
      double val_d;
      uint8_t val8;
      uint16_t val16 ;
      uint32_t val32;
      uint64_t val64 ;
      char buf[LONG_DATABUF_LEN] ;
    } u;
  } ;
  tDbe *pArgBuf, *pResBuf ;
  char **pResStr;
  std::string sql_stmt;

protected:
  int fill_args(void);
  int alloc_args(int);
  void free_args(void);
  int alloc_res(int);
  void free_res(void);
  int do_insert(int, void*, int);
  /* do reconnection */
  void reconnect(void) ;
  int do_exec(void);
public:
  mysql_prep(tBasicDal*,const int);
  ~mysql_prep(void);
  /* get field count of current result set */
  int num_fields(void) ;
  /* get a row from result */
  char** row(void) ;
  /* get row count of current result set */
  long long int num_rows(void) ;
  /* cleanup all structures */
  void clean(void);
  /* free result structure */
  int release(void);
  /* prepare for the sql statement */
  int prepare(attribute_list&,std::string&);
  /* store the result */
  int store_result(void) ;
  /* do execute the prepared statement */
  int direct_exec(std::string&);
  int direct_exec(const char*);
  /* insert argument value into statement */
  int insert(int,char*);
  int insert(int,int);
  int insert(int,long);
#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && \
  !defined(__LP64__)
  int insert(int,int64_t);
#else
  int insert(int,long long);
#endif
  int insert(int,double);
  int insert(int,long double);
  int insert(int,float);
  int insert(int,MYSQL_TIME*) ;
  //template <class Type> int do_insert(int,Type);
  /* XXX: for test only */
  void debug(void);
  /* get last error info */
  char* last_error();
  int last_errno();
  MYSQL_FIELD* get_column_info(int);
} ;

#endif /* __SQL_EXEC_H__*/

