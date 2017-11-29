/*
 *  Module Name:   zas
 *
 *  Description:   encapsulates a set of streaming operations
 *                   on databases
 *
 *  Author:        yzhou
 *
 *  Last modified: Nov 25, 2017
 *  Created:       May 17, 2015
 *
 *  History:       refer to 'ChangeLog'
 *
 */ 
#ifndef __ZAS_H__
#define __ZAS_H__

#include <stdio.h>
#include <stdint.h>
#include <memory>
#include <string>
#include "mysqlc.h"

/* version number string of this module */
#define __MOD_VER__  "0.0.4 alpha"

using namespace std;

using tBasicDal = struct basic_dal_t ;

class zas_connect {
protected:
  tBasicDal *m_bd ;
  const int m_dal;
protected:
  /* connection string info */
  using connStr = struct tConnString {
    char chUser[64];
    char chPwd[256];
    char chTns[32];
    char chHost[32] ;
    char chProto[16] ;
    uint32_t port;
    char chDb[32];
    char chUnixSock[256];
  } ;
  /* connection string info */
  connStr conn_str;

  std::string m_tnsPath ;

protected:
  /* parse db connection string */
  int parse_conn_str(char *str);

public:
  /* yzhou modified 20150720 */
  static void initialize(const int=0) {}
  /* constructor */
  zas_connect(char* tns_path=NULL,const int nDal=-1);
  /* destructor */
  ~zas_connect(void);
  /* remote logon to server */
  int rlogon(char*,const int=0) ;
  int rlogon(const char*,const int=0) ;
  /* logoff from remove server */
  int logoff(void);
  /* object evaluation by reference */
  zas_connect& operator = (const zas_connect&) ;

  int rollback(void) ;

  int commit(void);

  int auto_commit_on(void);

  int auto_commit_off(void);
  /* execute sql statement directly */
  long direct_exec(const char*,const int=1);
  long direct_exec(const std::string&,const int=1);
  /* for compatible with otlv4 */
  void set_timeout(const int /*atimeout*/=0){} 
  /* use to change current database to 'db' */
  void change_db(const char*);
  /* get count of columns in result set */
  size_t num_columns(void);
  /* DAL usages */
  int get_dal_type(void);
  tBasicDal* get_dal(void);
} ;

class zas_datetime ;
class exec_base ;
class attribute_list ;
class adaptor;
class zas_stream {
//#define INIT_ARG_PTR  0
const int INIT_ARG_PTR =  0;
//#define INIT_COL_PTR  0
const int INIT_COL_PTR =  0;

protected:
  /* the connection object */
  zas_connect &m_conn ;
  bool flush_flag ;
  int  commit_flag;
  /* buffer for the sql statement */
  std::string sql_stmt ;
  /* the original sql statement */
  std::string orig_stmt ;
  /* output buffer for the current statement */
  std::string sql_out;
  /* argument pointer */
  int arg_ptr ;
  /* column pointer in a row */
  int col_ptr ;
  /* processed row count to current result set */
  int row_cnt ;
  /* current row set */
  char **cur_row ;
  /* the attribute list */
  shared_ptr<attribute_list> a_list;
  /* the mysql execution object */
  shared_ptr<exec_base> m_exec ;
  /* use prepare method or not */
  bool bPrepMethod ;
  /* the adaption object */
  shared_ptr<adaptor> m_adpt ;

protected:
  /* add integer type value to statement */
  zas_stream& do_int_tx(int);
  /* add double type value to statement */
  zas_stream& do_double_tx(double);
  /* add string value to statement */
  zas_stream& do_string_tx(char*);
  zas_stream& do_string_rx (char*);
  /* remove the non-sinificant digit '0' */
  void del_ns_digits(char*);
  /* reset all stream pointers */
  void reset(void);
  /* attach argument to output sql buffer, 
   *  for query mode only */
  int attach(int,char*);
  int find_next_placeholder(int);
  /* do works before writing argument in << operators */
  void begin_tx(void) ;
  /* do works after writing argument in << operators */
  zas_stream& end_tx(void) ;
  zas_stream& end_tx(char*) ;
  zas_stream& end_tx(int) ;
  zas_stream& end_tx(long) ;
#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
  zas_stream& end_tx(int64_t) ;
#else
  zas_stream& end_tx(long long) ;
#endif
  zas_stream& end_tx(double) ;
  zas_stream& end_tx(long double) ;
  zas_stream& end_tx(MYSQL_TIME&) ;
  /* do works before reading argument in >> operators */
  int begin_rx(void) ;
  /* do works after reading argument in >> operators */
  zas_stream& end_rx(void) ;
  /* process the shadow arguments */
  int argv_process(void);
  int argv_proc_m_qry(int, int);
  int argv_proc_m_prep(int, int);

public:
  zas_stream(zas_connect&,const bool=true);
  ~zas_stream(void);

  void flush(void) ;

  void clean(const int=0) ;

  void set_flush(const bool);

  void set_commit(int);

  void open(int,const char*);

  void close(void);

  void debug(void);
  const char* get_stm_text(void);
  void get_stm_text(char*,char*);
  /* current processed row count */
  int64_t get_rpc();
  /* check if there's buffered data in result struct */
  int eof(void);
  /* no need to reset stream here */
  void rewind(void);
  /* get current execution method */
  bool is_prep_method(void);
  /* get row count of result set */
  size_t num_rows(void) ;
  MYSQL_FIELD* get_column_info(int);
  /* 
   * various streaming operators
   */
  zas_stream& operator = (const zas_stream&) ;

  zas_stream& operator <<(const char*);

  zas_stream& operator <<(unsigned char*);

  /* on 64bit platform, type 'long' has same size
   *   as type 'int64_t' */
#if !defined(_ARCH_PPC64) && !defined(__x86_64__) && !defined(__LP64__)
  zas_stream& operator << (int64_t);

  zas_stream& operator >>(int64_t&);
#else
  zas_stream& operator << (long long);

  zas_stream& operator >>(long long&);
#endif

  zas_stream& operator <<(long);

  zas_stream& operator <<(int);

  zas_stream& operator <<(unsigned);

  zas_stream& operator <<(short);

  zas_stream& operator <<(unsigned short);

  zas_stream& operator <<(double);

  zas_stream& operator <<(long double);

  zas_stream& operator <<(float);

  zas_stream& operator >>(char*);

  zas_stream& operator >>(unsigned char*);

  zas_stream& operator >>(long&);

  zas_stream& operator >>(int&);

  zas_stream& operator >>(unsigned&);

  zas_stream& operator >>(short&);

  zas_stream& operator >>(unsigned short&);

  zas_stream& operator >>(int8_t&);

  zas_stream& operator >>(double&);

  zas_stream& operator >>(long double&);

  zas_stream& operator >>(float&);

  zas_stream& operator << (zas_datetime&);

  zas_stream& operator >> (zas_datetime&);
} ;

class zas_exception {

public:
  enum{disabled=0,enabled=1};

public:
  std::string __stm_text, __msg, __var_info ;
  char *stm_text, *msg, *var_info ;
  /* error code */
  int code;

public:
  zas_exception(char*,char*,char*,int);
  ~zas_exception();
} ;

/* copy from otlv4.h source */
class zas_datetime {

public:
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  unsigned long fraction;
  int frac_precision;

public:
  zas_datetime() :
    year(1900),
    month(1),
    day(1),
    hour(0),
    minute(0),
    second(0),
    fraction(0),
    frac_precision(0) 
  {
  }

  zas_datetime(
    const int ayear,
    const int amonth,
    const int aday,
    const int ahour,
    const int aminute,
    const int asecond,
    const unsigned long afraction=0,
    const int afrac_precision=0) :
      year(ayear),
      month(amonth),
      day(aday),
      hour(ahour),
      minute(aminute),
      second(asecond),
      fraction(afraction),
      frac_precision(afrac_precision)
  {
  }

} ;

/* branch prediction */
#define my_likely(e)   __builtin_expect(!!(e),1)
#define my_unlikely(e) __builtin_expect(!!(e),0)

/* for eliminating the macro redefinitions */
#ifdef FIELD_TYPE_STRING 
  #undef FIELD_TYPE_STRING
#endif
#ifdef FIELD_TYPE_FLOAT 
  #undef FIELD_TYPE_FLOAT
#endif
#ifdef FIELD_TYPE_DOUBLE 
  #undef FIELD_TYPE_DOUBLE
#endif

#endif /* __ZAS_H__ */
