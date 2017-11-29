
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <linux/tcp.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include "mysqlc.h"
#include "mysqlc_priv.h"
#include "mysqlc_dbg.h"
#include "errmsg.h"
#include "simple_types.h"


size_t timestore(MYSQL_TIME *tm, char *out)
{
  uint8_t ln = 0;

  /* out[0]: length field */
  out[1] = !!tm->neg;
  ul4store(tm->day,&out[2]);
  out[6] = (char)tm->hour ;
  out[7] = (char)tm->minute ;
  out[8] = (char)tm->second ;
  ul4store(tm->second_part,&out[9]);
  ln = (tm->hour||tm->minute||tm->second||tm->day)?8:
    tm->second_part?12:0;
  out[0] = ln ;
  return ln+1 ;
}

size_t timeget(MYSQL_TIME *tm, char *in)
{
  uint8_t ln = *(uint8_t*)in ;

  if (ln<=0) {
    bzero(tm, mysql_timestamp_time);
    return 1;
  }
  in++ ;
  tm->neg = *in ;
  in++ ;
  tm->day = byte4_2_ul(in);
  in+= 4;
  tm->hour= *in;
  in++ ;
  tm->minute= *in;
  in++ ;
  tm->second= *in;
  in++ ;
  tm->second_part = ln>8?byte4_2_ul(in):0;
  tm->year= tm->month= 0; 
  if (tm->day) {    
    /* Convert days to hours at once */
    tm->hour+= tm->day*24;
    tm->day= 0;
  }
  tm->time_type= mysql_timestamp_time;
  return 1 + ln ;
}

size_t datetimestore(MYSQL_TIME *tm, char *out)
{
  uint8_t ln = 0;
  
  /* out[0]: length field */
  ul2store(tm->year,&out[1]);
  out[3] = (char)tm->month ;
  out[4] = (char)tm->day ;
  out[5] = (char)tm->hour ;
  out[6] = (char)tm->minute ;
  out[7] = (char)tm->second ;
  ul4store(tm->second_part,&out[8]);
  ln = (tm->hour||tm->minute||tm->second)?7:
    (tm->year||tm->month||tm->day)?4:
    tm->second_part?11:0;
  out[0] = ln ;
  return ln + 1;
}

size_t dateget(MYSQL_TIME *tm, char *in)
{
  uint8_t ln = *(uint8_t*)in ;

  if (ln<=0) {
    bzero(tm, mysql_timestamp_date);
    return 1;
  }
  in++ ;
  tm->year= byte2_2_ul(in);
  in+= 2 ;
  tm->month = *in ;
  in++ ;
  tm->day = *in ;
  //in++ ;
  tm->hour= tm->minute= tm->second= 0;
  tm->second_part= 0;
  tm->neg= 0;
  tm->time_type= mysql_timestamp_date;
  return 1 + ln ;
}

size_t datetimeget(MYSQL_TIME *tm, char *in)
{
  uint8_t ln = *(uint8_t*)in ;

  if (ln<=0) {
    bzero(tm, mysql_timestamp_datetime);
    return 1;
  }
  in++ ;
  tm->neg = 0;
  tm->year= byte2_2_ul(in);
  in+=2;
  tm->month = *in;
  in++ ;
  tm->day   = *in;
  in++ ;
  if (ln>4) {
    tm->hour = *in;
    in++ ;
    tm->minute = *in;
    in++ ;
    tm->second = *in;
    in++ ;
  } else {
    tm->hour = tm->minute = tm->second = 0;
  }
  tm->second_part = ln>7?byte4_2_ul(in):0;
  tm->time_type= mysql_timestamp_datetime;
  return 1 + ln ;
}

size_t getbin(MYSQL_BIND *bind, char *in)
{
  char *begin = in ;
  size_t len = lenenc_int_get(&in);
  size_t cp_len = len<(size_t)bind->buffer_length?
    len:bind->buffer_length ;

  memcpy(bind->buffer,in,cp_len);
  *bind->length = cp_len ;
  /* XXX: some string has type 'blob' 
   *  so add terminate character for it */
  ((char*)bind->buffer)[cp_len] = '\0';
  return (in-begin)+len ;
}

size_t getstr(MYSQL_BIND *bind, char *in)
{
  char *begin = in ;
  size_t len = lenenc_int_get(&in);
  size_t cp_len = len<(size_t)bind->buffer_length?
    len:(bind->buffer_length-1) ;

  memcpy(bind->buffer,in,cp_len);
  *bind->length = cp_len ;
  ((char*)bind->buffer)[cp_len] = '\0';
  return (in-begin)+len ;
}

/* do malloc with size alignment */
void* a_alloc(char *p,size_t s)
{
#define ALIGN_P  (sizeof(double)-1)
  size_t a_sz = (s+ALIGN_P)&(~ALIGN_P) ;

  return realloc(p,a_sz);
}

void container_free(container *pc)
{
  if (pc) {
    if (pc->c) free(pc->c);
    pc->total = pc->number = 0;
    pc->c = NULL ;
  }
}

char* str_store(char *ptr, const char *str)
{
  size_t ln = 0;
  
  if (!str || !*str) {
    return ptr ;
  }
  ln = (ln=strlen(str))>MAX_NAME_LEN?
    (MAX_NAME_LEN-1):ln ;
  ptr = stpncpy(ptr,(char*)str,ln);
  ptr[0] = '\0';
  return ptr+1 ;
}

int set_last_error(MYSQL *mysql, const char *msg, 
  int no, const char *sqlstat)
{
  int ln = strlen(msg);

  if (!mysql) {
    return -1;
  }
  /* error message */
  ln = ln>MAX_ERRMSG?(MAX_ERRMSG-1):ln ;
  memcpy(mysql->last_error,msg,ln);
  mysql->last_error[ln] = '\0';
  /* sql state */
  strncpy(mysql->sql_state,sqlstat,
    MAX_SQLSTATE-1);
  mysql->sql_state[MAX_SQLSTATE] = '\0';
  /* error number */
  mysql->last_errno = no ;
  return 0;
}

#ifdef _WIN32
int new_tcp_myclient(MYSQL *mysql, const char *host, 
  uint16_t port)
{
  WSADATA wsaData;
  SOCKADDR_IN sa;

  /* init winsock2 */
  WSAStartup(MAKEWORD(2,2), &wsaData);
  /* init socket */
  mysql->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (mysql->sock==INVALID_SOCKET) {
    set_last_error(mysql,"can't init winsock",
      CR_SOCKET_CREATE_ERROR, DEFAULT_SQL_STATE);
    return -1;
  }
  /* prepare connection informations */
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);   
  sa.sin_addr.s_addr = inet_addr(host);
  if (connect(mysql->sock, (SOCKADDR*)&sa, sizeof(SOCKADDR))==SOCKET_ERROR) {
    set_last_error(mysql,"can't connect to database",
      CR_SOCKET_CREATE_ERROR, DEFAULT_SQL_STATE);
    return -1;
  }
  return 0;
}
#else
int new_tcp_myclient(MYSQL *mysql, const char *host, 
  uint16_t port)
{
  int fd = 0, flag = 1;
  struct addrinfo *res, hints, *rp=0;
  char buff[64]="";

  host = (!host)||(!*host)?"localhost":host ;
  sprintf(buff,"%d",port);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = 0;
  hints.ai_protocol = 0;

  /* get host info */
  if (getaddrinfo(host,buff,&hints,&res)) {
    printd("get address info of %s:%d failed(%s)\n",
      host,port,strerror(errno));
  }
  for (rp = res;rp;rp = rp->ai_next) {
    fd = socket(rp->ai_family,rp->ai_socktype,
      rp->ai_protocol);
    if (fd<0) {
      /* try next socket */
      continue;
    }
    if (connect(fd,rp->ai_addr,rp->ai_addrlen)!=-1) {
      /* save host info */
      mysql->server.addr = htonl(
        ((struct sockaddr_in*)rp->ai_addr)->sin_addr.s_addr);
      mysql->server.port = htons(
        ((struct sockaddr_in*)rp->ai_addr)->sin_port);
      /* successful */
      break ;
    }
    close(fd);
    fd = 0;
  } /* for (rp=res...) */
  freeaddrinfo(res);
  if (fd<=0) {
    set_last_error(mysql,"can't connect to database",
      CR_SOCKET_CREATE_ERROR, DEFAULT_SQL_STATE);
    return -1;
  }
  if (mysql->sock) {
    close(mysql->sock);
  }
  mysql->sock = fd ;
  /* enable TCP_NODELAY when possible */
  flag = 1;
  setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,
    (char*)&flag,sizeof(int));
  /* enable SO_KEEPALIVE when possible */
  flag = 1;
  setsockopt(fd,IPPROTO_TCP,SO_KEEPALIVE,
    (char*)&flag,sizeof(int));
  return 0;
}
#endif

char* preserve_send_buff(MYSQL *mysql, 
  size_t size)
{
  container *pc = &mysql->cmd_buff ;

  pc->number = size ;
  /* also preserve header len (4 bytes) */
  size += 4 ;
  if (size>(size_t)pc->total) {
    pc->c= (char*)a_alloc(pc->c,size);
    /* update cmd buffer capacity */
    pc->total = size ;
  }
  return ((char*)pc->c) + 4 ;
}

void reassign_send_buff_size(MYSQL *mysql, 
  const size_t size)
{
  container *pc = &mysql->cmd_buff ;
  size_t sz = pc->total -4;

  pc->number = size>sz?sz:size ;
}

int send_by_len(MYSQL *mysql, char *buff,
  const int len)
{
  int total = 0, ret = 0, remain = len;

  while (total<len) {
    ret = send(mysql->sock,buff+total,remain,0);
    if (ret<=0) {
      set_last_error(mysql,"lost connection to server",
       CR_SERVER_LOST, DEFAULT_SQL_STATE);
      return -1;
    }
    total += ret ;
    remain-= ret ;
  }
  return total ;
}

int recv_by_len(MYSQL *mysql, char *buff, 
  size_t const total)
{
  size_t remain = 0, len = 0;
  int ret = 0 ;

  if (!total) {
    return 0;
  }
  if ((ret=recv(mysql->sock,buff,total,0))<=0) {
    set_last_error(mysql,"lost connection to server",
     CR_SERVER_LOST, DEFAULT_SQL_STATE);
    return -1;
  }
  remain = total - ret ;
  len = ret ;
  while (remain>0) {
    ret = recv(mysql->sock,buff+len,remain,0);
    len += ret;
    remain-= ret ;
  }
  return len;
}

bool ts_compare(struct timestamp_t *t0, 
  struct timestamp_t *t1)
{
  return t1->sec>0 && ((t0->sec>t1->sec) || 
    ((t0->sec==t1->sec) && (t0->usec>t1->usec))) ;
}

#ifdef _WIN32
void ts_update(struct timestamp_t *t)
{
  LARGE_INTEGER lUSec, lFeq;

  QueryPerformanceFrequency(&lFeq) ;
  QueryPerformanceCounter(&lUSec);
  t->usec = lUSec.QuadPart*1000000/lFeq.QuadPart ;
  /* always set to 1 second */
  t->sec  = 1;
}
#else
void ts_update(struct timestamp_t *t)
{
  struct timeval tv = {0,0} ;

  gettimeofday(&tv,0);
  t->sec = tv.tv_sec ;
  t->usec= tv.tv_usec ;
}
#endif

