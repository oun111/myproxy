
#ifndef __CONNSTR_H__
#define __CONNSTR_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <string>
#ifdef _WIN32
  #include "win/win.h"
#endif

/* max length of a token */
const int TKNLEN = /*64*/68;
const int PWDLEN = 256;
const int LONG_DATABUF_LEN =/*128*/4096;



namespace LEX_GLOBAL {

  extern uint16_t next_token(std::string &str,uint16_t pos, 
    char *buf, int length=TKNLEN) ;
} ;

class tns_parser {
public:
  /* elements in tns file, except tUsr&tPwd */
  enum tnsIdx { tEntry, tDesc, tAddr, tAddrLst, tHost, tProto,
    tPort, tCnnDat, tSvc, tSrv, tUsr, tPwd, tMax, } ;
  /* tns parsing stages */
  enum eStages { st_key, st_eql, st_lBrack, st_rBrack,} ;

protected:
  using tns_info =  struct tTnsInfo {
    char key[TKNLEN] ;
    bool bEval ;
    char val[TKNLEN];
  } ;
  /* the tns string info */
  tns_info t_tbl[tMax] ;

protected:
  /* get next token from stream */
  int next_token(FILE*, char*,int);
  /* test if next token in stream matches with input one */
  int test_token(FILE*, const char*,const int);
  /* test 2 inputed tokens */
  int test_token(char*, const char*);
  /* parse the tns statement */
  int do_recurse_parse(FILE*);
  /* initialize the tns info table */
  void init_tns_parse(void);
  /* parse oracle tns file */
  int do_tns_parse(char*,std::string&);

public:
  tns_parser(void);
  /* parse connection string in oracle format */
  int parse_conn_str(char*,std::string&);
  /* get parsed tns info by item index */
  char* get_tns(uint32_t);
  /* get next token from string buffer */
  //static uint16_t prev_token(std::string&,uint16_t,char*,int=TKNLEN);
} ;

class my_dsn_parser {
/* keyword name definition in a dsn string */
const char* DSN_HOST = "server";
const char* DSN_PORT = "port";
const char* DSN_USR  = "usr";
const char* DSN_PWD  = "pwd";
const char* DSN_DB   = "dbname";
const char* DSN_SOCK = "unix_socket";
protected:
  using dsn_info = struct tDsnInfo {
    char chUser[TKNLEN];
    char chPwd[PWDLEN];
    char chHost[TKNLEN] ;
    uint32_t port;
    char chDb[TKNLEN];
    char chSock[PATH_MAX];
  } ;
  dsn_info d_info ;

protected:
  /* get dsn section values by name */
  int parse_dsn_element(const char *str, 
    const char *name, char *buf, int sz); 

public:
  my_dsn_parser(void);
  /* parse connection string/dsn in mysql format */
  int parse_conn_str(char*);
  /* get dsn values */
  const char* host(void);
  const int   port(void);
  const char* user(void);
  const char* pwd(void);
  const char* db(void);
  const char* sock(void);
} ;


#endif /* __CONNSTR_H__*/

