
#include "connstr.h"
#include "dbug.h"


namespace LEX_GLOBAL {

  uint16_t 
  next_token(std::string &str,uint16_t pos, 
    char *buf, int length) 
  {
    uint16_t sp = pos, np = 0;
    char *pstr; 
    int ln = 0;

    /* eliminate possible spaces */
    while (sp<str.size()&& isspace(str[sp])) sp++ ;
    np = sp ;
    /* find ending of type token */ 
    while (np<str.size() &&
      str[np]!='[' && str[np]!=']' && 
      str[np]!='<' && str[np]!='>' && 
      str[np]!='(' && str[np]!=')' && 
      str[np]!=',' && str[np]!=':' &&
      str[np]!='.' && str[np]!='=' &&
      str[np]!='+' && str[np]!='-' && 
      str[np]!='\''&& str[np]!='*' &&  
      str[np]!='/' && str[np]!='\"' &&
      str[np]!='!' && 
      /* add 2016.8.28: support math opt '%' */
      str[np]!='%' &&
      /* add 20160413: support ';' */
      str[np]!=';' &&
      /* add '|' '&' '~' '^' support */
      str[np]!='|' && str[np]!='&' &&
      str[np]!='^' && str[np]!='~' &&
      !isspace(str[np])) 
      np ++ ; 
    /* 20170320: compatible with following operators:
     *  '&' '|' '<<' '>>' '~' '^'
     */
    /* 20160301: compatible with these operators: 
     *  '<=' '>=' '<>' '!=' '+=' ':='
     */
    if ((uint16_t)(sp+1)<str.size()) {
      if ((str[sp]=='<'&& str[sp+1]=='>')  ||
         (str[sp]=='<' && str[sp+1]=='=')  ||
         (str[sp]=='>' && str[sp+1]=='=')  ||
         (str[sp]=='+' && str[sp+1]=='=')  ||
         (str[sp]==':' && str[sp+1]=='=')  ||
         (str[sp]=='|' && str[sp+1]=='|')  ||
         (str[sp]=='<' && str[sp+1]=='<')  ||
         (str[sp]=='>' && str[sp+1]=='>')  ||
         (str[sp]=='!' && str[sp+1]=='=')) {
        np+=2 ;
      }
    }
    /* 20160301: compatible with '(+)' */
    if ((uint16_t)(sp+2)<str.size() && str[sp]=='('&&
       str[sp+1]=='+' &&str[sp+2]==')') {
      np+=3 ;
    }
    if (sp>=str.size()) {
      sp = str.size();
    }
    /* copy the token */
    std::string tmp = str.substr(sp,!(np-sp)?
      1:(np-sp)) ;
    pstr = (char*)tmp.c_str();
    ln   = tmp.size();
    strncpy(buf,pstr,ln>length?length:ln);
    buf[ln] = '\0';
    return sp ;
  }
} ;

using namespace LEX_GLOBAL ;

/* 
 * class tns_parser
 */
tns_parser::tns_parser(void)
{
}

void tns_parser::init_tns_parse(void)
{
  int i = 0;

  /* user defined: tns file entry */
  strcpy(t_tbl[tEntry].key,"@__entry");
  strcpy(t_tbl[tDesc].key,"DESCRIPTION");
  strcpy(t_tbl[tAddr].key,"ADDRESS");
  strcpy(t_tbl[tAddrLst].key,"ADDRESS_LIST");
  strcpy(t_tbl[tHost].key,"HOST");
  strcpy(t_tbl[tProto].key,"PROTOCOL");
  strcpy(t_tbl[tPort].key,"PORT");
  strcpy(t_tbl[tCnnDat].key,"CONNECT_DATA");
  strcpy(t_tbl[tSvc].key,"SERVICE_NAME");
  strcpy(t_tbl[tSrv].key,"SERVER");
  strcpy(t_tbl[tUsr].key,"@__usr");
  strcpy(t_tbl[tPwd].key,"@__pwd");
  for (i=0; i<tMax; i++) {
    t_tbl[i].bEval = !(i>=tEntry&&i<=tAddr) ;
    t_tbl[i].val[0]= '\0';
  }
}

int tns_parser::do_recurse_parse(FILE *file)
{
  int i=0, ret = 0;
  int ln = 0;
  char token[TKNLEN] = "";
  unsigned char stage = st_key, last_key = tMax ;
  bool bEndLoop =false;

  while (!bEndLoop) {
    ln = next_token(file,token,TKNLEN);
    if (!ln)
      return 0 ;
    switch (stage)
    {
      case st_key:
      {
        for (i=0; i<tMax; i++) {
          if (!test_token(token,t_tbl[i].key))
            break ;
        }
        if (i>=tMax) {
          /* syntax error */
          printd("error parsing token %s\n", token);
          return 0 ;
        }
        /* need evaluation */
        if (t_tbl[i].bEval) {
          last_key = i ;
        }
        //printd("i %d, last %d, key %s\n", i, last_key, t_tbl[i].key);
        /* to stage '=' */
        stage = st_eql;
      }
      break ;

      case st_eql:
      {
        if (test_token(token,"=")) {
          /* syntax error */
          printd("error parsing token %s\n", token);
          return 0 ;
        }
        /* to stage value or '(' */
        ret = test_token(file,"(",1);
        if (ret<0) {
          /* file ends */
          return 0;
        } else if (!ret) {
          /* not match '(' */
          /* read the token */
          ln = next_token(file,token,TKNLEN);
          if (!t_tbl[last_key].bEval) {
            /* syntax error */
            printd("error parsing token %s\n", token);
            return 0;
          }
          /* do evaluation */
          /* 2016.4.22: may lead to 'out of bound' error, fix it */
          ln = ln<TKNLEN?ln:(TKNLEN-1); 
          strncpy(t_tbl[last_key].val,token,ln);
          t_tbl[last_key].val[ln] = '\0';
          bEndLoop = true ;
          /* debug */
          /*printd("eval %d: %s=%s\n", last_key,t_tbl[last_key].key,
            token);*/
          /* reset the indicator */
          last_key = tMax ;
        } else {
          /* try '(' stage */
          stage = st_lBrack ;
        }
      }
      break ;

      case st_lBrack:
      {
        if (test_token(token,"(")) {
          /* syntax error */
          printd("error parsing token %s\n", token);
          return 0;
        }
        /* try recurse processing */
        if (!do_recurse_parse(file)) {
          return 0;
        }
        /* try ')' stage */
        stage = st_rBrack ;
      }
      break ;

      case st_rBrack:
      {
        if (test_token(token,")")) {
          /* syntax error */
          printd("error parsing token %s\n", token);
          return 0;
        }
        /* to stage value or '(' */
        ret = test_token(file,"(",1);
        if (ret<0) {
          /* file ends */
          return /*0*/1;
        } else if (!ret) {
          /* not match '(', may be upper entries' ')' */
          bEndLoop = true ;
        } else {
          /* try '(' stage */
          stage = st_lBrack ;
        }
      }
      break ;

      default: 
        return 0;
    } /* switch */
  } /* while loop */
  return 1;
}

#if 0
uint16_t tns_parser::prev_token(std::string &str,
  uint16_t pos, char *buf, int length) 
{
  int ln = 0;
  uint16_t sp = pos, np = 0, p;
  char *pstr; 

  /* eliminate possible spaces */
  while (sp>0 && isspace(str[sp])) sp-- ;
  np = sp ;
  /* find begining of type token */ 
  while (np>0 && str[np]!='[' && str[np]!=']' && 
    str[np]!='<' && str[np]!='>' && 
    str[np]!='(' && str[np]!=')' && 
    str[np]!=',' && str[np]!=':' &&
    str[np]!='.' && str[np]!='=' &&
    str[np]!='+' && str[np]!='-' && 
    str[np]!='\'' && str[np]!='*' &&  
    str[np]!='/' && 
    !isspace(str[np])) 
    np -- ; 
  /* copy the token */
  p    = !np?np:!(sp-np)?np:(np+1);
  ln   = !np?(sp+1):!(sp-np)?1:(sp-np);
  pstr = (char*)str.substr(p,ln).c_str() ;
  ln   = strlen(pstr);
  strncpy(buf,pstr,ln>length?length:ln);
  buf[ln]='\0';
  return p ;
}
#endif

int tns_parser::next_token(FILE *file, char buf[], int len)
{
#define RETURN_IF_EOF(f)  do {\
  if (feof(f)) return 0; \
} while(0)
  char ch ;
  int i = 0;

  /* ignore leading spaces */
  while(isspace(ch=fgetc(file))) {
    RETURN_IF_EOF(file);
  }
  /* if commet is met, try read next line */
  while (ch=='#') {
    /* read whole line */
    while((ch=fgetc(file))!='\r' && ch!='\n') ;
    /* aproach to next line */
    while (isspace(ch)) ch=fgetc(file);
    RETURN_IF_EOF(file);
  }
  /* copy the token */
  do {
    buf[i++] = ch ;
    ch = fgetc(file);
    RETURN_IF_EOF(file);
    /* these items are considered to be tokens */
    if (buf[i-1]=='(' || buf[i-1]==')' ||
      buf[i-1]=='=') {
      break ;
    }
  } while (!isspace(ch) && ch!='(' 
    && ch!=')' && ch!='=' && i<len) ;
  /* string end */
  len = len<i?len:i ;
  buf[len] = '\0' ;
  //printd("%s\n", buf);
  /* return back stream */
  fseek(file,-1,SEEK_CUR);
  return len;
}

int tns_parser::test_token(char *tkn1, const char *tkn2)
{
  return strcasecmp(tkn1,tkn2);
}

int tns_parser::test_token(FILE *file, const char *tkn, 
  const int len)
{
  int ln;
  char buf[TKNLEN] ;

  if (!(ln = next_token(file,buf,TKNLEN)))
    return -1;
  /* return back to stream */
  fseek(file,ln*(-1),SEEK_CUR);
  /* compare */
  if (ln!=len || strncasecmp(buf,tkn,len))
    return 0;
  return 1;
}

int tns_parser::do_tns_parse(char *entry, std::string &tns_path)
{
  int ret = 0, ln = 0;
  FILE *file ;
  char buf[TKNLEN] ;
  const char *tnsFile = 0;

  if (tns_path.size()==0) {
    char tmp[PATH_MAX] = "";

    tnsFile = tmp;
    sprintf(tmp,"%s/network/admin/tnsnames.ora",
      getenv("MYSQL_HOME"));
  } else {
    tnsFile = const_cast<char*>(tns_path.c_str());
  }

  if (!entry) {
    return 0;
  }
  file = fopen(tnsFile,
  "r"
#ifdef _WIN32
  "b"
#endif
  );
  if (!file) {
    printd("cannot open %s\n", tnsFile);
    return 0;
  }
  printd("opening tns file %s\n", tnsFile);

  ln = strlen(entry);
  /* 2016.4.22: may lead to 'out of bound' error, fix it */
  ln = ln>=TKNLEN?(TKNLEN-1):ln ;
  strncpy(t_tbl[tEntry].key, entry, ln); 
  t_tbl[tEntry].key[ln] = '\0';
  /* search for entry */
  do {
    ret = test_token(file, entry, ln) ;
    /* file ends */
    if (ret<0) {
      printd("TNS entry '%s' not found\n", entry);
      fclose(file);
      return 0;
    }
    /* not the target, try next one */
    if (!ret)
      next_token(file,buf,TKNLEN);
    /* when token name matches entry name , test 
     *  if it's the begining of entry */ 
    if (ret) {
      int sp = ftell(file);

      next_token(file,buf,TKNLEN);
      if (test_token(file,"=",1)!=1) {
        ret = 0;
        continue ;
      } else {
        fseek(file,sp,SEEK_SET);
      }
    }
  } while (ret!=1);
  /* parse file */
  ret = do_recurse_parse(file);
  fclose(file);
  return ret;
}

int tns_parser::parse_conn_str(char *str, std::string &tns_path)
{
  int ln = 0;
  char *p;
  char tns_name[TKNLEN];
  
  /* initialize the tns info table */
  init_tns_parse();
  /* check if it's oracle format like: user/pwd@tns */
  if (!strchr(str,'/') || !strchr(str,'@')) {
    printd("not oracle format\n");
    return 0;
  }
  /* get username */
  while (str && isspace(*str)) str++ ;
  for (p=str;p&& !isspace(*p) && *p!='/'; p++);
  if (*p!='/' && !isspace(*p)) {
    printd("syntax error while parsing user "
      "name, '%c' expected\n", '/');
    return 0;
  }
  ln = (p-str)>TKNLEN?TKNLEN:(p-str) ;
  strncpy(t_tbl[tUsr].val,str,ln);
  t_tbl[tUsr].val[/*p-str*/ln] = '\0';
  p = strchr(str,'/')+1;
  /* get password */
  for (str=p; str && isspace(*str); str++) ;
  for (p=str; p&&!isspace(*p)&&*p!='@'; p++);
  if (*p!='@' && !isspace(*p)) {
    printd("syntax error while parsing user "
      "name, '%c' expected\n", '@');
    return 0;
  }
  ln = (p-str)>TKNLEN?TKNLEN:(p-str) ;
  strncpy(t_tbl[tPwd].val,str,ln);
  t_tbl[tPwd].val[/*p-str*/ln] = '\0';
  p = strchr(str,'@')+1;
  /* get tns name */
  for (str=p; str && isspace(*str); str++) ;
  while (p && *p) p++;
  ln = (p-str)>TKNLEN?TKNLEN:(p-str) ;
  strncpy(tns_name,str,ln);
  tns_name[p-str] = '\0';
  /* parse tns file */
  return do_tns_parse(tns_name,tns_path);
}

char* tns_parser::get_tns(uint32_t idx)
{
  return (idx<tMax)?t_tbl[idx].val:NULL;
}

/* 
 * class my_conn_parser
 */

my_dsn_parser::my_dsn_parser(void)
{
}

int my_dsn_parser::parse_dsn_element(const char *str, const char *name, 
  char *buf, int szBuf)
{
  int i=0;
  char *p ;
  char *pName = const_cast<char*>(name),
       *pStr  = const_cast<char*>(str);
  size_t szName = strlen(name);

  if (!(p=strcasestr(pStr,pName))) {
    printd("dsn element '%s' not "
      "found\n", name);
    return 0;
  }
  /*printd("dsn element '%s' found\n", 
     pName);*/
  p += szName ;
  /* parse the following '=' */
  while (p && isspace(*p)) p++ ;
  if (*p !='=') {
    printd("dsn syntax error "
      "within element '%s', '%c' expected\n", 
       name, '=');
    return 0;
  }
  p++ ;
  /* get value */
  while (p && isspace(*p)) p++ ;
  for (i=0;p&& i<szBuf && !isspace(*p) && *p!=';' &&*p!='=';
    buf[i++] = *p++) ;
  buf[i] = '\0' ;
  /* check the ';' */
  while (p && isspace(*p)) p++ ;
  if (*p !=';') {
    printd("dsn syntax error "
      "within element '%s', '%c' expected\n", 
        name, ';');
    return 0;
  }
  return 1;
}

int my_dsn_parser::parse_conn_str(char *str)
{
  char buf[TKNLEN] ;

  /* process dsn format in odbc form like this: 
   *   'dsn=d1;server=s1;database=db1;uid=u1;pwd=p1;' 
   * also process with alias names
   */
  /* server */
  if (!parse_dsn_element(str,DSN_HOST,d_info.chHost,TKNLEN)) {
    /* report an error */
    return 0;
  }
  /* port */
  if (!parse_dsn_element(str,DSN_PORT,buf,TKNLEN)) {
    /* report an error */
    return 0;
  } else {
    d_info.port = atoi(buf);
  }
  /* username */
  if (!parse_dsn_element(str,DSN_USR,d_info.chUser,TKNLEN)) {
    /* report an error */
    return 0;
  }
  /* password */
  if (!parse_dsn_element(str,DSN_PWD,d_info.chPwd,PWDLEN)) {
    /* report an error */
    return 0;
  }
  /* database */
  d_info.chDb[0] = '\0';
  if (!parse_dsn_element(str,DSN_DB,d_info.chDb,TKNLEN)) {
    /* report an error */
    return 0;
  }
  /* unix socket */
  d_info.chSock[0] = '\0';
  if (!parse_dsn_element(str,DSN_SOCK,d_info.chSock,PATH_MAX)) {
    /* report an error */
    return 0;
  }
  return 1;
}

/* functions for getting dsn values */
const char* my_dsn_parser::host(void)
{
  return d_info.chHost ;
}

const char* my_dsn_parser::user(void)
{
  return d_info.chUser ;
}

const char* my_dsn_parser::pwd(void)
{
  return d_info.chPwd ;
}

const char* my_dsn_parser::sock(void)
{
  return d_info.chSock ;
}

const char* my_dsn_parser::db(void)
{
  return d_info.chDb ;
}

const int my_dsn_parser::port(void)
{
  return d_info.port ;
}

