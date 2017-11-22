
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <zlib.h>
#ifndef _WIN32
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif
#include "mysqlc.h"
#include "errmsg.h"
#include "misc.h"
#include "mysqlc_dbg.h"
#include "simple_types.h"
#include "crypto.h"
#include "list.h"


#define RETURN(c) do {\
  if (buff) free(buff); \
  return c ;\
} while (0)

/* standard protocol version */
const int proto_ver = 0x0A ;
/* max allow packet size */
const uint32_t sz_max_allow_packet = 1024L*1024L*1024L ;
 
/* server capabilyties */
enum svr_caps {
  /* Client supports plugin authentication */
  sc_client_auth_plugin = (1UL << 19),
} ;

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
enum clt_caps {
  client_long_password = 1, /* new more secure passwords */
  client_found_rows = 2,    /* found instead of affected rows */
  client_long_flag = 4,  /* get all column flags */
  client_connect_with_db = 8, /* one can specify db on connect */
  client_no_schema = 16,  /* don't allow database.table.column */
  client_compress = 32,  /* can use compression protocol */
  client_odbc = 64,   /* odbc client */
  client_local_files = 128, /* can use load data local */
  client_ignore_space = 256, /* ignore spaces before '(' */
  client_protocol_41 = 512,  /* new 4.1 protocol */
  client_interactive = 1024,  /* this is an interactive client */
  client_ssl = 2048,  /* switch to ssl after handshake */
  client_ignore_sigpipe = 4096,  /* ignore sigpipes */
  client_transactions = 8192,  /* client knows about transactions */
  client_reserved = 16384,  /* old flag for 4.1 protocol  */
  client_secure_connection = 32768,  /* new 4.1 authentication */
  client_multi_statements = (1ul << 16), /* enable/disable multi-stmt support */
  client_multi_results = (1ul << 17), /* enable/disable multi-results */
  client_ps_multi_results = (1ul << 18), /* multi-results in ps-protocol */
  client_plugin_auth = (1ul << 19), /* client supports plugin authentication */
  client_connect_attrs = (1ul << 20), /* client supports connection attributes */
  /* enable authentication response packet to be larger than 255 bytes. */
  client_plugin_auth_lenenc_client_data = (1ul << 21),
  /* don't close the connection for a connection with expired password. */
  client_can_handle_expired_passwords = (1ul << 22),
  client_progress = (1ul << 29),   /* client support progress indicator */
  client_ssl_verify_server_cert = (1ul << 30),
  client_remember_options = (1ul << 31),
} ;

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
enum enum_server_command
{
  com_sleep, com_quit, com_init_db, com_query, com_field_list,
  com_create_db, com_drop_db, com_refresh, com_shutdown, com_statistics,
  com_process_info, com_connect, com_process_kill, com_debug, com_ping,
  com_time, com_delayed_insert, com_change_user, com_binlog_dump,
  com_table_dump, com_connect_out, com_register_slave,
  com_stmt_prepare, com_stmt_execute, com_stmt_send_long_data, com_stmt_close,
  com_stmt_reset, com_set_option, com_stmt_fetch, com_daemon,
  /* don't forget to update const char *command_name[] in sql_parse.cc */

  /* must be last */
  com_end
};

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
enum enum_cursor_type
{
  cursor_type_no_cursor= 0,
  cursor_type_read_only= 1,
  cursor_type_for_update= 2,
  cursor_type_scrollable= 4
};

/*
 * character set definition
 */
const struct charset_info_t {
  int cs_num ;
  char cs_name[MAX_NAME_LEN];
} 
g_cs_info[] = 
{
  /* 
   * #0 utf8 general
   */
  { 33, "utf8" },
  /* 
   * #1 latin1 general
   */
  { 8, "latin1" },
  /* 
   * #2 big5 general
   */
  { 1, "big5" },
  /* 
   * #3 unicode general
   */
  { 128, "unicode" },
  /* TODO: more charsets here */
} ;
#define charset_number  (sizeof(g_cs_info)/sizeof(g_cs_info[0]))

#ifdef _WIN32
int get_cs_by_name(char *s) {
  uint16_t i=0; 
  int cs = -1; 
  for (i=0;i<charset_number;i++) 
    if (!strcmp(g_cs_info[i].cs_name,s)){ 
      cs = g_cs_info[i].cs_num ; 
      break ;
    } 
  return cs ;
}
#else
#define get_cs_by_name(s)  ({\
  uint16_t i=0; \
  int cs = -1; \
  for (i=0;i<charset_number;i++) \
    if (!strcmp(g_cs_info[i].cs_name,s)){ \
      cs = g_cs_info[i].cs_num ; \
      break ;\
    } \
  cs; \
})
#endif

/* default is utf8 */
#define default_charset  get_cs_by_name("utf8")

/* 
 * copy from $(mysql source)/libmysql/libmysql.c 
 */

/* 1 (length) + 2 (year) + 1 (month) + 1 (day) */
#define MAX_DATE_REP_LENGTH 5

/*
  1 (length) + 1 (is negative) + 4 (day count) + 1 (hour)
  + 1 (minute) + 1 (seconds) + 4 (microseconds)
*/
#define MAX_TIME_REP_LENGTH 13

/*
  1 (length) + 2 (year) + 1 (month) + 1 (day) +
  1 (hour) + 1 (minute) + 1 (second) + 4 (microseconds)
*/
#define MAX_DATETIME_REP_LENGTH 12

int store_ucomp_frame(MYSQL *mysql)
{
  int clen = 0, len = 0 ;
  char *buf = 0, hdr[16];
  container *pc = &mysql->ucomp_buff,
    *pc_tmp = &mysql->ucomp_tmp ;

  /* skip uncompressed len + compressed 
   *  serial number + compressed len */
  if (recv_by_len(mysql,hdr,7)<=0) {
    printd("error get packet compressed length "
      "from server\n");
    return -1;
  }
  /* get compressed len */
  printd("hdr: %x %x %x %x %x %x %x\n",
    hdr[0],hdr[1],hdr[2],hdr[3],
    hdr[4],hdr[5],hdr[6]);
  len = byte3_2_ul(&hdr[4]) ;
  /* the payload was not compressed, just store them
   *  in buffer directly */
  if (len<=0) {
    /* get real uncompress payload len */
    len = byte3_2_ul(hdr);
    /* renew an uncompressed buffer */
    if (pc->total<len) {
      pc->c = (char*)a_alloc(pc->c,len);
    }
    buf = mysql->ucomp_buff.c ;
    if (recv_by_len(mysql,buf,len)<=0) {
      printd("error get packet compressed length "
        "from server\n");
      return -1;
    }
    pc->number = 0;
    pc->total  = len;
    /* XXX: test */
#if 0
    {
      printf("rx compressed payload: \n");
      hex_dump(buf,len);
    }
#endif
    return 0;
  }
  /* 
   * the payload was compressed, uncompress it 
   */
  /* get real compressed len */
  clen = byte3_2_ul(hdr);
  /* renew an uncompressed buffer */
  if (pc->total<len) {
    pc->c = (char*)a_alloc(pc->c,len);
  }
  /* update tmp uncompressed buffer */
  if (pc_tmp->total<len) {
    pc_tmp->c = (char*)a_alloc(pc_tmp->c,len);
    pc_tmp->total = len ;
  }
  buf = pc_tmp->c ;
  /* get compressed header+payload */
  if (recv_by_len(mysql,buf,clen)<=0) {
    printd("error get compressed packet "
      "from server\n");
    return -1;
  }
  /* XXX: test */
#if 0
  {
    printf("rx compressed payload: \n");
    hex_dump(buf,clen);
  }
#endif
  /* load uncompress data in */
  if (do_uncompress((uint8_t*)buf,clen,
     (uint8_t*)pc->c,(int*)&len)) {
    printd("uncompress packet failed\n");
    return -1;
  }
  /* reset read pointer */
  pc->number = 0;
  pc->total  = len;
#if 1
  printd("%s: try to uncompress the payload, "
    "clen %d, len %d\n",__func__, clen, len);
  printd("%s: uc rp: %d, rc total: %d, total2: %d\n",
    __func__, pc->number, pc->total, len);
#endif
  return 0;
}

bool is_ucomp_buff_empty(MYSQL *mysql)
{
  /* compre read pointer with whole data size */
  return mysql->ucomp_buff.number >= 
    mysql->ucomp_buff.total ;
}

/* 
 * get current uncompressed read pointer
 */
char* current_ucomp_rp(MYSQL *mysql)
{
  return (char*)mysql->ucomp_buff.c+
    mysql->ucomp_buff.number ;
}

int get_ucomp_header(MYSQL *mysql, int *total,
  uint8_t *sn)
{
  char hdr[6], *rp = current_ucomp_rp(mysql);
  int ln = 0, rest = 0;

  ln = mysql->ucomp_buff.total-
    mysql->ucomp_buff.number;
  /*
   * header's not complete
   */
  if (ln<4) {
    /* calculate part length of header from current 
     *  compressed frame */
    rest = 4 - ln ;
    /* get 1st header part */
    memcpy(hdr,rp,ln);
    printd("warning: packet header insufficient, "
      "len: %d, rest: %d\n", ln, rest);
    /* get rest part of header from 
     *  next frame */
    if (store_ucomp_frame(mysql)<0) {
      return -1;
    }
    /* get 2nd part of header */
    memcpy(hdr+ln,mysql->ucomp_buff.c,rest);
    /* get packet size */
    *total = byte3_2_ul(hdr);
    /* get packet sn */
    *sn = hdr[3] ;
    /* update read pointer */
    mysql->ucomp_buff.number = rest;
  } else {
    /* 
     * this's a complete header
     */
    /* get packet size */
    *total = byte3_2_ul(rp);
    /* get packet sn */
    *sn = rp[3] ;
    /* update read pointer upwards by skipping the
     *  packet header */
    mysql->ucomp_buff.number += 4 ;
  }
  return 0;
}

int get_ucomp_payload(MYSQL *mysql, char **buff, 
  int total)
{
  int rest = 0, ln = 0, len = total;
  char *rp = current_ucomp_rp(mysql);

  /* 
   * this's an incomplete payload
   */
  ln = mysql->ucomp_buff.total-
    mysql->ucomp_buff.number ;
  if (ln<total) {
    /* get rest payload len in current 
     *  uncompressed buffer */
    rest = total - ln ;
    len = ln ;
    printd("warning: insufficient packet payload, "
      "rest: %d, current: %d\n",rest,len);
  }
  *buff = (char*)a_alloc(*buff,total+2);
  /* get packet body, may be the 1st part
   *  of payload */
  memcpy(*buff,rp,len);
  /* update uncompress buffer read pointer */
  mysql->ucomp_buff.number += len ;
  /*
   * printf("%s: uc rp: %d, rc sz: %d, len: %d\n",
    __func__, mysql->ucomp_buff.number, 
    mysql->ucomp_buff.total,len);
   */
  /* 
   * process rest data in next compressed frame 
   */
  if (rest>0) {
    /* get next compressed frame */
    int ret = store_ucomp_frame(mysql);

    /* error */
    if (ret<0) {
      printd("fatal get rest frame\n");
      return -1;
    }
    /* get uncompressed payload data directly */
    if (ret>0) {
      if (recv_by_len(mysql,*buff+len,rest)<=0) {
        printd("fail get payload directly\n");
        return -1;
      }
    } else {
      /* get rest data from uncompressed buffer */
      memcpy(*buff+len,mysql->ucomp_buff.c,rest);
      mysql->ucomp_buff.number = rest;
    }
    /*printf("%s: orig len: %d, rest len: %d\n", __func__,
      len, rest);*/
    len += rest;
  }
  return len ;
}

int read_ucomp_pkt(MYSQL *mysql, char **buff,
  int pkt_num)
{
  int total = 0;
  uint8_t sn = 0;

  /* 
   * get packet header 
   */
  if (get_ucomp_header(mysql,&total,&sn)) {
    printd("get uncompressed header fail\n");
    return -1;
  }
  printd("%s: len: %d, rp : %d\n", __func__, 
    total, mysql->ucomp_buff.number);
  /* 
   * check serial number 
   */
  if (sn!=(mysql->nr+pkt_num)) {
    printd("packet sequence %d not matches "
      " with packet number %d, len: %d\n",
      sn, mysql->nr+pkt_num, total);
    //return 1;
  }
  /* get payload */
  return get_ucomp_payload(mysql,buff,total) ;
}

static int do_send(MYSQL *mysql)
{
#define MIN_PKT_LEN  50
  container *pc = &mysql->cmd_buff ;
  char *buff = pc->c ;
  int ret = 0;
  char *tmp = 0;
  int ln = 0;

  /* fill header */
  ul3store(pc->number,buff);
  buff[3] = mysql->nr ;
  ln = pc->number + 4;
  /* 
   * send packet without compression 
   */
  if (!mysql->compress) 
    return send_by_len(mysql,buff,ln);
  /* 
   * try compress if possible 
   */
  ln += (pc->number>=MIN_PKT_LEN)?128:7 ;
  tmp = a_alloc(0,ln);
  if (pc->number>=MIN_PKT_LEN) {
    if (do_compress((uint8_t*)(buff),
       pc->number+4,(uint8_t*)(tmp+7),&ln)) {
      printd("compress packet failed\n");
      free(tmp);
      return -1;
    }
    buff = tmp ;
    /* fill compressed len into header */
    ul3store(ln,buff);
    ul3store(pc->number+4,buff+4);
    ln += 7;
  } else {
    /* fill compressed len into header */
    memcpy(tmp+7,buff,pc->number+4);
    buff = tmp ;
    ul3store(0,buff+4);
    ul3store(pc->number+4,buff);
  }
  /* fill in compressed sn */
  buff[3] = /*mysql->nr*/0 ;
  /* send the whole packet */
  ret = send_by_len(mysql,buff,ln);
  free(tmp);
  return ret ;
}

static int do_recv(MYSQL *mysql, char **buff, 
  uint8_t pkt_num)
{
  char hdr[16] = "";
  int len = 0, ret=0;

  /* 
   * try the compressed process 
   */
  if (mysql->compress) {
    /* test if there're packets pending in 
     *  uncompressed buffer, read one if so */
    if (!is_ucomp_buff_empty(mysql))
      return read_ucomp_pkt(mysql,buff,pkt_num);
    /* recv and uncompress packets */
    ret = store_ucomp_frame(mysql) ;
    if (ret<0)
      return -1;
    /* read one packet from uncompressed buffer */
    if (!ret && !is_ucomp_buff_empty(mysql))
      return read_ucomp_pkt(mysql,buff,pkt_num);
  }
  /*
   * non-compressed process
   */
  /* get length + serial number of packet */
  if (recv_by_len(mysql,hdr,4)<=0) {
    printd("error get packet length "
      "from server\n");
    return -1;
  }
  /* get packet size */
  len = byte3_2_ul(hdr);
  /* check packet number */
  if ((uint8_t)hdr[3]!=(mysql->nr+pkt_num)) {
    printd("packet sequence %d not matches "
      " with packet number %d, len: %d\n",
      hdr[3], mysql->nr+pkt_num, len);
    //return 1;
  }
  *buff = (char*)a_alloc(*buff,len+2);
  /* get packet body */
  if (recv_by_len(mysql,*buff,len)<0) {
    printd("error get packet body "
      "from server");
    return -1;
  }
  return len ;
}

static int get_error_info(MYSQL *mysql, 
  char *buff, size_t len)
{
  int ret = 0;
  char *end = 0;

  /* the OK status */
  if (!buff[0]) {
    /* skip the error state header */
    buff++ ;
#if 0
    mysql->affected_rows = 0 ;
    /* affected rows: 1 byte */
    if ((uint8_t)buff[0]<0xfb) {
      mysql->affected_rows = buff[0]&0xff ;
    } else if ((uint8_t)buff[0]==0xfc) {
      /* affected rows: 2 byte */
      mysql->affected_rows = byte2_2_ul(buff+1) ;
    } else if ((uint8_t)buff[0]==0xfd) {
      /* affected rows: 3 byte */
      mysql->affected_rows = byte3_2_ul(buff+1) ;
    } else if ((uint8_t)buff[0]==0xfe) {
      /* affected rows: 8 byte */
      mysql->affected_rows = byte8_2_ul(buff+1) ;
    }
#else
    mysql->affected_rows    = lenenc_int_get(&buff);
    mysql->res->rows        = mysql->affected_rows ;
    mysql->last_inserted_id = lenenc_int_get(&buff);
#endif
    return 0;
  }
  /* error status */
  if ((uint8_t)buff[0]==0xff) {
    end = buff + len ;
    /* skip error header */
    buff++ ;
    /* error number */
    ret = byte2_2_ul(buff);
    mysql->last_errno = ret ;
    buff+=2 ;
    /* skip the sql state marker '#' */
    buff++ ;
    /* get sql state */
    strncpy(mysql->sql_state,buff,5);
    mysql->sql_state[5] = '\0';
    buff+= 5;
    /* get error description */
    strncpy(mysql->last_error,buff,end-buff);
    mysql->last_error[end-buff] = '\0';
    if (ret) {
      printd("errno:%d, sqlstate:%s, error:%s\n",
        mysql->last_errno, mysql->sql_state,
        mysql->last_error);
    }
    return -1;
  }
  /* other packet type */
  return 1;
}

static int get_simple_reply(MYSQL *mysql)
{
  char *buff = 0;
  int pkt_num = 1;
  /*size_t*/int len = 0;

  /* only get the 1st mysql frame */
  if ((len=do_recv(mysql,&buff,pkt_num))<=0) {
    printd("%s: err: %s\n", __func__, strerror(errno));
    RETURN(-1);
  }
  /* get reply */
  if (buff && get_error_info(mysql,buff,len)<0)
    RETURN(-1);
  RETURN(0);
}

/* the unnamed arguments are 'string/size' pairs */
static int do_exec_cmd_simple(MYSQL *mysql, 
  int cmd, const int nArg, ...)
{
  int ret=0, n=0;
  char *buff = 0;
  const uint8_t *str = 0;
  uint32_t sz = 1, ln = 0;
  va_list args ;

  /* calculate argument buffer size */
  va_start(args,nArg);
  for (n=0;n<nArg;n++) {
    str = va_arg(args, const uint8_t*);
    sz += va_arg(args, uint32_t) ;
  }
  va_end(args);
  /* get begining of sending buffer */
  buff = preserve_send_buff(mysql,sz) ;
  /* XXX: reset the packet number */
  mysql->nr = 0;
  /* set command code */
  buff[0] = cmd;
  buff++ ;
  /* copy each 'string/size' pair into tx buffer */
  va_start(args,nArg);
  for (n=0;n<nArg;n++) {
    str= va_arg(args, const uint8_t*);
    ln = va_arg(args, uint32_t) ;
    memcpy(buff,str,ln);
    buff += ln ;
  }
  va_end(args);
  /* send the command */
  if (do_send(mysql)<=0) {
    printd("failed send command\n");
    ret = -1;
  }
  return ret;
}

static int mysql_reconnect(MYSQL *mysql)
{
  struct connInfo_t *pc = 0;

  if (!mysql) {
    return -1;
  }
  if (!mysql->reconnect) {
    return 0;
  }
  mysql->last_errno = 0;
  mysql->last_error[0] = '\0';
  /* try reconnect */
  printd("trying reconnect to server\n");
  pc = &mysql->m_ci ;
  return !mysql_real_connect(mysql,pc->host,
    pc->user,pc->passwd,pc->db,pc->port,NULL,0);
}

static int test_conn(MYSQL *mysql)
{
  if (!mysql) {
    return -1;
  }
  if (do_exec_cmd_simple(mysql,com_ping,0)) {
    printd("failed\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

static  int check_conn(MYSQL *mysql)
{
  if (!mysql) {
    set_last_error(mysql,"lost connection to server",
      CR_SERVER_LOST, DEFAULT_SQL_STATE);
    return -1;
  }
  /* check if lost connection to server, 
   *  do reconnect if possible  */
  if (mysql_is_disconnect(mysql)) {
    return -1;
  }
  return 0;
}

static int get_summary_info(MYSQL_STMT *stmt, char *buff)
{
  MYSQL *mysql = stmt->mysql;

  if (buff[0])
    return -1;
  if (check_conn(mysql)) {
    return -1;
  }
  buff++ ;
  /* the statement id */
  stmt->stmt_id = byte4_2_ul(buff);
  printd("stmt id: %d\n", stmt->stmt_id);
  buff+= 4;
  /* the column count */
  stmt->columns.number = byte2_2_ul(buff);
  printd("num col: %d\n", stmt->columns.number);
  buff+=2 ;
  /* the placeholder count */
  stmt->params.number = byte2_2_ul(buff);
  stmt->pholders.number = stmt->params.number;
  printd("num params: %d\n", stmt->params.number);
  buff+=2 ;
  /* skip filter byte */
  buff++ ;
  /* get warning count */
  mysql->num_warning = byte2_2_ul(buff);
  printd("num warnings: %d\n", mysql->num_warning);
  //buff++ ;
  return 0;
}

static int get_column_info(container *cols, char *buff,
  int nCol)
{
  container *c = cols;
  char *ptr = buff ;
  MYSQL_COLUMN *pc = 0, **pp;

  pp = (MYSQL_COLUMN**)&(c->c);
  /* if no columns, alloc one */
  if (!*pp || c->total<c->number) {
    /* realloc the columns buffer */
    c->total = c->number ;
    *pp = a_alloc((char*)*pp,sizeof(*pc)*c->number);
  }
  if (!*pp || nCol>=c->number) {
    return -1;
  }
  pc  = *pp + nCol ;
  /* skip catalog */
  ptr = lenenc_str_get(ptr,0,MAX_NAME_LEN);
  /* schema */
  ptr = lenenc_str_get(ptr,pc->schema,MAX_NAME_LEN);
  /* owner table alias */
  ptr = lenenc_str_get(ptr,pc->tbl_alias,MAX_NAME_LEN);
  /* owner table */
  ptr = lenenc_str_get(ptr,pc->tbl,MAX_NAME_LEN);
  /* column alias */
  ptr = lenenc_str_get(ptr,pc->alias,MAX_NAME_LEN);
  /* column name */
  ptr = lenenc_str_get(ptr,pc->name,MAX_NAME_LEN);
  /* skip the 'next length' 0xc */
  ptr++ ;
  /* column character set */
  pc->charset = byte2_2_ul(ptr);
  ptr+=2 ;
  /* max length of column */
  pc->len = byte4_2_ul(ptr);
  ptr+=4 ;
  /* XXX: get column type, this's important */
  pc->type= ptr[0] ;
  ptr++ ;
  /* column flags */
  pc->flags = byte2_2_ul(ptr);
  ptr+=2 ;
  /* max shown digit */
  pc->decimal = ptr[0] ;
  ptr++ ;
  /* skip filter bytes */
  ptr+=2;
#if 0
  /* XXX: debug */
  {
    printd("********column %d info********* \n", nCol);
    printd("schema: %s\n", pc->schema);
    printd("table: %s\n", pc->tbl);
    printd("table alias: %s\n", pc->tbl_alias);
    printd("column: %s\n", pc->name);
    printd("column alias: %s\n", pc->alias);
    printd("char set: %d\n", pc->charset);
    printd("max len: %d\n", pc->len);
    printd("column type: %u\n", pc->type);
    printd("column flags: 0x%x\n", pc->flags);
    printd("decimal: 0x%x\n", pc->decimal);
    printd("\n");
  }
#endif
  return 0;
}

#ifndef _WIN32
#define next_nr(nr) ({ \
  nr = (nr==0xff)?0:(nr+1);  \
})
#else
#define next_nr(nr) (\
  nr = (nr==0xff)?0:(nr+1)  \
)
#endif

static int get_stmt_prepare_reply(MYSQL_STMT *stmt)
{
  char *buff = 0;
  uint8_t pkt_num = 1, n = 0;
  int ln = 0;
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return -1;
  }
  /* 
   * try the first frame 
   */
  if ((ln=do_recv(mysql,&buff,pkt_num))<=0 ||
     /* test if error occors */
     get_error_info(mysql,buff,ln)<0 ||
     /* get prepare summary info */
     get_summary_info(stmt,buff)<0) { 
    /* error exit */
    RETURN(-1);
  }
  /* 
   * fetch all the place-holder rows 
   */
  for (n=0;
     stmt->pholders.number>0 &&
     (ln=do_recv(mysql,&buff,next_nr(pkt_num)))>0 &&
     /* calcute place holder's number */
     n<stmt->pholders.number && 
     /* check 'eof' frame */
     buff[0]!=0xfe; n++) {
    /* get each place-holder's info, the PROTOCOL format 
     *  of place-holders are same with columns' , so
     *  use get_column_info() to fetch  */
    get_column_info(&stmt->pholders,buff,n);
  }
  /*
   * get related table columns info
   */
  for (n=0;
    stmt->columns.number>0 &&
     (ln=do_recv(mysql,&buff,next_nr(pkt_num)))>0 &&
     /* calcute place holder's number */
     n<stmt->columns.number && 
     /* check 'eof' frame */
     buff[0]!=0xfe; n++) { 
    /* get each column's info */
    get_column_info(&stmt->columns,buff,n);
  }
  /* release buff */
  RETURN(0);
}

/*
 * brief: 
 *
 *  process initial handshake request from server, refer to:
 *
 *  1. $(mysql source)/sql-comm/client.c
 *  2. http://dev.mysql.com/doc/internals/en/connection-phase-packets.html#packet-Protocol::Handshake
 *
 */
static int parse_init_handshake(MYSQL *mysql)
{
  /*size_t*/int pkt_len = 0;
  char *buff = 0, *ptr=0, *end= 0, *tmp=0;
  /* auth plugin data */
  char *ap_data = 0;
  /* the length of 1st 'auth data' */
  const int AP_DATA1_LEN = 8,
    /* max length of 2nd 'auth data' */
    AP_DATA2_LEN = 12 ;
  const char DEFAULT_PLUGIN_NAME[] = 
    "mysql_native_password";

  /* get server handshake packet */
  pkt_len = do_recv(mysql,&buff,0);
  if (pkt_len<=0 || !buff) {
    printd("failed get handshake request(%s)\n", 
      strerror(errno));
    RETURN(-1);
  }
  ptr = buff ;
  end = buff + pkt_len ;
  /* protocol version */
  mysql->proto_version = ptr[0] ;
  if (mysql->proto_version != proto_ver) {
    printd("invalid protocol version 0x%x\n",
      mysql->proto_version);
    RETURN(-1);
  }
  ptr++ ;
  /* server version */
  tmp = stpcpy(mysql->svr_version,ptr);
  printd("server version: %s\n", 
    mysql->svr_version);
  ptr+= (tmp-mysql->svr_version+1) ;
  /* connection id */
  mysql->conn_id = byte4_2_ul(ptr);
  printd("connection id 0x%x\n",mysql->conn_id);
  /* the 'auth plugin data' 1st */
  ap_data = ptr + 4 ;
  ptr = ap_data + AP_DATA1_LEN + 1  ;
  memcpy(mysql->ap_data,ap_data,AP_DATA1_LEN);
  /* get 1st of capability flag */
  mysql->svr_cap = byte2_2_ul(ptr);
  printd("server capabilities 1st: 0x%x\n", 
    mysql->svr_cap);
  ptr += 2 ;
  /* test if there're more bytes */
  if (end>=(ptr+16)) {
    /* get character set */
    //mysql->charset = ptr[0] ;
    printd("server character set: %d\n", /*mysql->charset*/ptr[0]);
    ptr ++;
    /* get sever status */
    mysql->svr_stat= byte2_2_ul(ptr);
    printd("server status: 0x%x\n", mysql->svr_stat);
    ptr += 2 ;
    /* get 2nd of capability flag */
    mysql->svr_cap|= byte2_2_ul(ptr)<<16;
    printd("server capabilities: 0x%x\n", mysql->svr_cap);
    ptr += 2;
    /* get 'auth data len' from server */
    mysql->ap_len = ptr[0] - 1 ;
    if (mysql->ap_len<0) {
      /* XXX: error message here */
      RETURN(-1);
    }
    ptr ++ ;
    printd("auth data len: %d\n", mysql->ap_len);
    /* skip reserve bytes */
    ptr += 10;
  }
  /* get the reset 'auth plugin data' */
  if (end >= (ptr+AP_DATA2_LEN)) {
    /* rest length dosen't include the tailing '0' of
     *  authentication data */
    size_t rest_ln = mysql->ap_len 
      - AP_DATA1_LEN  ;

    //ap_data = ptr ;
    memcpy(mysql->ap_data+AP_DATA1_LEN,
      ptr,rest_ln);
    ptr[rest_ln] = 0;
    ptr += rest_ln + 1 ;
    printd("rest auth len: %zu\n", rest_ln);
    /* deal with 'auth plugin name', if exists */
    if (mysql->svr_cap & sc_client_auth_plugin) {
      strcpy(mysql->ap_name,ptr);
      printd("auth plugin name: %s\n", mysql->ap_name);
    } else {
      /* indicate an 'auth plugin name' here */
      strcpy(mysql->ap_name,DEFAULT_PLUGIN_NAME);
      printd("default auth plugin name: %s\n", mysql->ap_name);
    }
  }
  /* packet ends */
  RETURN(0);
}

static int do_login_process(MYSQL *mysql,
  const char *user,const char *passwd,
  const char *db, const uint32_t client_flag)
{
  char *buff = preserve_send_buff(
    mysql,MAX_BUFF), *end= buff;
  size_t sz = 0;

  /* add user defined capacities */
  mysql->client_cap |= client_flag;
  /* set client capability flags */
  mysql->client_cap  |= client_long_password | 
    client_long_flag |  client_transactions  |
    client_protocol_41 | client_secure_connection | 
    client_plugin_auth | /*client_connect_attrs | */
    /*client_connect_with_db |*/ client_plugin_auth_lenenc_client_data ;
  if (db) {
    mysql->client_cap |= client_connect_with_db ;
  } else {
    mysql->client_cap &= (~client_connect_with_db) ;
  }
  /* write client flags */
  ul4store(mysql->client_cap,end);
  printd("client flags: 0x%x\n", mysql->client_cap);
  /* max packet size */
  ul4store(sz_max_allow_packet,end+4);
  printd("max packet size: %d\n", sz_max_allow_packet);
  /* character set */
  end[8] = mysql->charset ;
  end+=9 ;
  /* the reserve bytes */
  bzero(end,23);
  end+=23 ;
  /* user name */
  end = str_store(end,user);
  /* wirte the auth response */
  if (mysql->ap_len<=0) {
    *end++ = 0;
  } else {
    /* there're auth response data */
    if (mysql->client_cap & client_plugin_auth_lenenc_client_data) {
#if 0
      if (mysql->ap_len<255) {
        *end++ = mysql->ap_len ;
      } else if (mysql->ap_len<65535) {
        ul2store(mysql->ap_len,end);
        end += 2 ;
      } else if (mysql->ap_len<16777216) {
        ul3store(mysql->ap_len,end);
        end += 3 ;
      } else {
        ul4store(mysql->ap_len,end);
        end += 4 ;
      }
#else
      end += lenenc_int_set(mysql->ap_len,end);
#endif
    } else {
      /* ap_len < 255 */
      *end++ = mysql->ap_len ;
    }
    /* generate and store 'auth response data' */
    if (gen_auth_data(mysql->ap_data,mysql->ap_len,
       passwd,(uint8_t*)end)) {
      printd("failed generate auth data\n");
      return -1;
    }
    end += mysql->ap_len ;
  }
  /* write db */
  if (mysql->client_cap & client_connect_with_db) {
    end = str_store(end,db);
  }
  /* the auth plugin name */
  if (mysql->client_cap & client_plugin_auth) {
    end = str_store(end,mysql->ap_name);
  }
  /* 
   * TODO: write the 'connect attribute' fields 
   */
  /* calculate packet size */
  sz = end - buff ;
  printd("packet size: %zu\n", sz);
  /* increase the packet number */
  //mysql->nr ++ ;
  /* always starts nr at '1' during 
   *  init hands hake  */
  mysql->nr = 1 ;
  /* update actual data size */
#if 0
  mysql->cmd_buff.number = sz;
#else
  reassign_send_buff_size(mysql,sz);
#endif
  /* send the packet */
  if (do_send(mysql)<=0) {
    printd("error send login request\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

static char* do_store_param(char *out, MYSQL_BIND *param)
{
  switch (param->buffer_type) {
    case MYSQL_TYPE_NULL:
      break;
    case MYSQL_TYPE_TINY:
      {
        *out = *(uint8_t*)param->buffer ;
        out++ ;
      }
      break;
    case MYSQL_TYPE_SHORT:
      {
        uint16_t val = *(uint16_t*)param->buffer ;
        ul2store(val,out);
        out+=2 ;
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        uint32_t val = *(uint32_t*)param->buffer ;
        ul4store(val,out);
        out+=4 ;
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        long long val = *(long long*)param->buffer;
        ul4store(val&0xffffffff,out);
        ul4store((val>>32)&0xffffffff,out+4);
        out += 8;
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        float val = *(float*)param->buffer;
        float4store(val,out);
        out += 4;
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        double val = *(double*)param->buffer;
        double8store_be(val,out);
        out += 8;
      }
      break;
    case MYSQL_TYPE_TIME:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)param->buffer;
        out += timestore(tm,out) ;
      }
      break;
    case MYSQL_TYPE_DATE:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)param->buffer;
        tm->hour= tm->minute= tm->second= tm->second_part= 0;
        out += datetimestore(tm,out);
      }
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)param->buffer;
        out += datetimestore(tm,out);
      }
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      {
        memcpy(out,param->buffer,
          param->buffer_length);
        out += param->buffer_length;
      }
      break;
    default:
      break ;
  }
  return out ;
}

static int read_binary_row(LIST **cur, 
  bool bNew, char *buff, size_t sz)
{
  /* 
   * binary row = header[00] + (column-count+9)/8 + binary-results 
   */
  const size_t offs = /*1*/0 ;
  size_t data_sz = sz - offs;
  MYSQL_RESULT *res = 0;

  /* initially, no result buffers */
  if (bNew) {
    res  = a_alloc(0,sizeof(MYSQL_RESULT));
    bzero(res,sizeof(*res));
    *cur = list_add(*cur,&(res->node));
  } else {
    res = container_of(*cur,MYSQL_RESULT,node);
  }
  res->r.number = data_sz ;
  /* allocate the binary result buffer */
  if (res->r.number>res->r.total) {
    res->r.total = res->r.number;
    res->r.c = a_alloc(res->r.c,data_sz);
  }
  /* 
   * save binary result, including null-bitmap
   */
  memcpy(res->r.c,buff+offs,data_sz);
  return 0;
}

static int release_raw_rows(MYSQL_RES *mr)
{
  LIST *p = 0, *next=0;
  MYSQL_RESULT *res = 0;

  if (!mr)
    return 0;
  for (p=mr->results;p;p=next) {
    next = list_delete(p,p) ;
    res = container_of(p,MYSQL_RESULT,node);
    if (res->r.c) {
#if 0
      free(res->r.c);
#else
      container_free(&res->r);
#endif
    }
    free(res);
  } /* end for() */
  mr->results = 0;
  return 0;
}

static int get_raw_rows(MYSQL_RES *mr,
  int rows_to_get)
{
  LIST *cur = 0;
  int ln = 0;
  char *buff =0;
  bool bNew = !mr->results ;
  int n = 0;
  MYSQL *mysql = 0;
  int *pkt_num = &mr->pkt_num;

  if (!mr->more) {
    printd("%s: no more rows\n",__func__);
    return 1;
  }
  mysql = mr->mysql_obj ;
  /* fetch & store all rows locally */
  for (cur=mr->results/*,mr->rows=0*/;
     (rows_to_get<0?1:(n<rows_to_get)) &&
     (ln=do_recv(mysql,&buff,next_nr(*pkt_num)))>0 &&
     /* check 'eof' frame */
     (uint8_t)buff[0]!=0xfe;
     mr->rows++,n++) {
    /* parse result rows */
    read_binary_row(&cur,bNew,buff,ln);
    /* assign result head if it's null */
    if (!mr->results) 
      mr->results = cur ;
    /* check if new result buffer should be 
     *  allocated */
    bNew= !(cur->next);
    cur = !bNew?cur->next:cur;
  }
  if ((uint8_t)buff[0]==0xfe) {
    mr->more = false;
  }
  /* reset row reading pointer */
  mr->rp = !mr->more&&mysql->fast_rx?0:mr->results;
  RETURN(0);
}

static int get_stmt_exec_reply(
  void *in_obj,     /* the statement or connection objct */
  const bool bStmt, /* indicates statement/connection object */
  const bool bGetResult /* gets binary results or not */
  )
{
  char *buff =0;
  int pkt_num = 1, n=0;
  int ln = 0;
  //LIST *cur = 0;
  MYSQL_RES *mr = 0;
  //bool bNew = 0;
  MYSQL *mysql = 0;
  MYSQL_STMT *stmt = 0;
  container *cols  = 0;

  /* if object type is statement, save columns& result sets
   *  into statement object, otherwise, using
   *  connection object */
  if (bStmt) {
    stmt = (MYSQL_STMT*)in_obj;
    mysql= stmt->mysql ;
    cols = &stmt->columns ;
  } else {
    mysql= (MYSQL*)in_obj;
    cols = &mysql->columns ;
  }
  if (check_conn(mysql)) {
    printd("the mysql connection object "
      "is invalid\n");
    RETURN(-1);
  }
  /* save result sets by object type */
  if (!(mr = bStmt?stmt->res:mysql->res)) {
    printd("the result meata data is "
      "not created\n");
    RETURN(-1);
  }
  /* add 2017.2.1 */
  mr->mysql_obj = mysql ;
  /* add 2017.2.1 end */
  //bNew = !mr->results ;
  mr->rows = 0;
  if ((ln=do_recv(mysql,&buff,pkt_num))<=0 ||
     /* test if error occors */
     get_error_info(mysql,buff,ln)<0) { 
    /* error exit */
    RETURN(-1);
  }
  /* get columns count */
  cols->number = (uint8_t)buff[0];
  /* 
   * get all columns' info 
   */
  for (n=0;
     cols->number>0 &&
     (ln=do_recv(mysql,&buff,next_nr(pkt_num)))>0 &&
     /* calcute place holder's number */
     n<cols->number && 
     /* check 'eof' frame */
     (uint8_t)buff[0]!=0xfe; n++) {
    get_column_info(cols,buff,n);
    printd("col: %s\n", COLUMN(*cols)[n].name);
  }
  /* 
   * parse the 'Binary Protocol Resultset Row' 
   *  according to cursor types
   */
  if (!bGetResult) {
    RETURN(0);
  }
  /* no result set returns, just go back */
  if (cols->number<=0) {
    RETURN(0);
  }
  /* XXX: test */
#if 0
  /* fetch & store all rows locally */
  for (cur=mr->results,mr->rows=0;
     (ln=do_recv(mysql,&buff,next_nr(pkt_num)))>0 &&
     /* check 'eof' frame */
     (uint8_t)buff[0]!=0xfe;
     mr->rows++) {
    /* parse result rows */
    read_binary_row(&cur,bNew,buff,ln);
    /* assign result head if it's null */
    if (!mr->results) 
      mr->results = cur ;
    /* check if new result buffer should be 
     *  allocated */
    bNew= !(cur->next);
    cur = !bNew?cur->next:cur;
  }
  /* reset row reading pointer */
  mr->rp = mr->results;
#else
  mr->pkt_num = pkt_num ;
  mr->more  = true;
  /* if fast receive mode is enabled, then rx one row,
   *  otherwise, receive whole result set */
  get_raw_rows(mr,mysql->fast_rx?1:-1);
#endif
#if 0
  /* XXX: test */
  {
    LIST *p = mr->results;
    uint16_t i=0, cnt=0;

    printd("rows detail(total: %d): ============\n",
      mr->rows);
    for (n=0;p&&n<mr->rows;p=list_rest(p),n++) {
      MYSQL_RESULT *res = container_of(p,MYSQL_RESULT,node);
      printd("row %d(data size: %zu): ", cnt++,res->r.number);
      for (i=0;i<res->r.number;i++) 
        printf(" 0x%x ", ((char*)(res->r.c))[i] & 0xff);
      printf("\n");
    }
  }
#endif
  RETURN(0);
}

static char* fetch_row_data(MYSQL_BIND *bind, char *data)
{
  //printd("filed type: %d\n", bind->buffer_type);
  switch (bind->buffer_type) {
    case MYSQL_TYPE_NULL: break;
    case MYSQL_TYPE_TINY:
      {
        uint8_t val = *(uint8_t*)data;
        *(uint8_t*)bind->buffer = val ;
        data++ ;
      }
      break;
    case MYSQL_TYPE_SHORT:
      {
        uint16_t val = byte2_2_ul(data);
        ul2store/*_be*/(val,bind->buffer);
        data += 2;
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        uint32_t val = byte4_2_ul(data);
        ul4store/*_be*/(val,bind->buffer);
        data += 4 ;
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        uint64_t val = byte8_2_ul(data);
        ul8store_be(val,bind->buffer);
        data += 8;
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        float val = byte4_2_float(data);
        float4store/*_be*/(val,bind->buffer);
        data += 4;
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        double val = byte8_2_double(data);
        double8store/*_be*/(val,bind->buffer);
        data += 8;
      }
      break;
    case MYSQL_TYPE_TIME:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)bind->buffer ;
        data += timeget(tm,data);
      }
      break;
    case MYSQL_TYPE_DATE:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)bind->buffer ;
        data += dateget(tm,data);
      }
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      {
        MYSQL_TIME *tm = (MYSQL_TIME*)bind->buffer ;
        data += datetimeget(tm,data);
      }
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_BIT:
      {
        data += getbin(bind,data);
      }
      break ;
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      {
        data += getstr(bind,data);
      }
      break;
    default:
      printd("unknown filed type %d\n", 
        bind->buffer_type);
      break ;
  }
  return data ;
}

/* output text-ed format results */
void dbug_output_results_tf(MYSQL *mysql)
{
  uint16_t i=0, n=0;
  MYSQL_RES *mr = 0;
  char **results = 0;
    
  if (check_conn(mysql)) {
    printd("invalid mysql connection object\n");
    return ;
  }
  mr = mysql->res ;
  printd("rows fields detail: ============\n");
  /* initiates the iteration */
  mysql_store_result(mysql);
  for (i=0;(results=mysql_fetch_row(mr));i++) {
    printd("row %d: ", i);
    for (n=0;n<mysql_num_fields(mr);n++) 
      printf("%s ", results[n][0]=='\0'?"null":
        results[n]);
    printf("\n");
  }
  printf("\n");
}

/* output binary format results */
void dbug_output_results_bf(MYSQL_STMT *stmt)
{
  uint16_t i=0;
  container *ct =0;
  MYSQL_BIND *bind, *end, **pp ;
  MYSQL_RES *mr = 0;
  MYSQL *mysql = stmt->mysql ;
    
  if (check_conn(mysql)) {
    printd("invalid mysql connection object\n");
    return ;
  }
  /* get result sets from statement object */
  if (!(mr=stmt->res)) {
    printd("no result set meta data created\n");
    return ;
  }
  printd("rows fields detail(total: %d): ============\n", 
    mr->rows);
  /* initiates the iterator */
  for (i=0;i<mr->rows;i++) {
    mysql_stmt_fetch(stmt);
    ct = &stmt->binds ;
    pp = (MYSQL_BIND**)&(ct->c) ;
    printd("row %d: ", i);
    for (bind=*pp, end=*pp+ct->number; bind<end; bind++) {
      if (*(bind->is_null)) {
        printf("%p ", NULL);
        continue ;
      }
      switch (bind->buffer_type)
      {
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
        printf("%s ", (char*)bind->buffer);
        break ;
      case MYSQL_TYPE_LONG:
        printf("%d ", *(int*)bind->buffer);
        break ;
      case MYSQL_TYPE_LONGLONG:
        printf("%lld ",*(long long*)bind->buffer);
        break ;
      case MYSQL_TYPE_DOUBLE:
        printf("%f ", *(double*)bind->buffer);
        break ;
      case MYSQL_TYPE_FLOAT:
        printf("%f ", *(float*)(bind->buffer));
        break ;
      default: break ;
      }
    }
    printf("\n");
  }
  printf("\n");
}

static int update_conn_info(MYSQL *mysql, const char *host, 
  const uint16_t port, const char *user, const char *passwd, 
  const char *db)
{
  struct connInfo_t *pi = 0;

  if (!mysql) {
    return -1;
  }
  pi = &mysql->m_ci ;
  memcpy(pi->user,user,MAX_NAME_LEN-1);
  memcpy(pi->passwd,passwd,MAX_NAME_LEN-1);
  if (db) memcpy(pi->db,db,MAX_NAME_LEN-1);
  memcpy(pi->host,host,MAX_NAME_LEN-1);
  pi->port = port;
  return 0;
}

/*
 * user interfaces
 */

/* XXX: libmysqlclient doesn't have this API */
bool mysql_is_disconnect(MYSQL *mysql)
{
  return mysql?(mysql->last_errno==CR_SERVER_LOST)
    :true;
}

/* test disconnection under query mode */
bool mysql_query_is_disconnect(MYSQL *mysql)
{
  /* connection test */
  if (!test_conn(mysql)) {
    return false ;
  }
  /* try reconnection */
  if (mysql_reconnect(mysql)) {
    return true ;
  }
  /* reconnection ok is treated as disconnected */
  return true;
}

/* test disconnection under prepare mode */
bool mysql_stmt_is_disconnect(MYSQL_STMT *stmt)
{
  MYSQL *mysql = 0 ;

  assert(stmt&&stmt->mysql);
  mysql = stmt->mysql ;
  /* connection test */
  if (!test_conn(mysql)) {
    goto __ret ;
  }
  /* try reconnection */
  if (mysql_reconnect(mysql)) {
    return true ;
  }
__ret:
#if 0
  return stmt->ts_ok>0 &&
    stmt->ts_ok<stmt->mysql->ts_ok;
#else
  return ts_compare(&mysql->ts_ok,
    &stmt->ts_ok);
#endif
}

int mysql_select_db(MYSQL *mysql, const char *schema)
{
  if (check_conn(mysql)) {
    return -1;
  }
  if (do_exec_cmd_simple(mysql,
     com_init_db,1,schema,strlen(schema))) {
    printd("failed\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

int mysql_real_connect(MYSQL *mysql, const char *host, 
 const char *user, const char *passwd, const char *db, 
 uint16_t port, const char *unix_socket,uint32_t client_flag)
{
  if (!mysql) {
    return 0;
  }
  /* reset the compress flag */
  mysql->compress = 0;
  /* connect to server by TCP */
  if (new_tcp_myclient(mysql,host,port)) {
    /* XXX: error message here */
    return 0;
  }
  /* check the initial handshake */
  if (parse_init_handshake(mysql)) {
    /* XXX: error message here */
    return 0;
  }
  /* do authentication process */
  if (do_login_process(mysql,user,passwd,
     db,client_flag)) {
    printd("login to database failed\n");
    return 0;
  }
  /* test compression */
  if (mysql->client_cap & client_compress) {
    mysql->compress = 1;
  }
  /* reset last error no code */
  mysql->last_errno = 0;
  /* try 'change db' command */
  if (db &&mysql_select_db(mysql,db)) {
    /* XXX: error message here */
    return 0;
  }
  /* update connection infomations */
  update_conn_info(mysql,host,port,
    user,passwd,db) ;
  /* update connection timestamp */
  ts_update(&mysql->ts_ok);
  /* 
   * TODO: also consider the following optional fetures:
   *
   *   1. connect by unix socket 
   *   2. connect by SSL implementations
   *   3. connect by compression
   */
  printd("login to database ok\n");
  return 1;
}

MYSQL* mysql_init(MYSQL *mysql)
{
#ifndef _WIN32
  /* ignore sigpipe */
  signal(SIGPIPE,SIG_IGN);
#endif
  /* allocate a new one */
  mysql = (MYSQL*)a_alloc((void*)mysql,
    sizeof(MYSQL));
  /* default is utf8 */
  mysql->charset = default_charset;
  /* client flag */
  mysql->client_cap = client_local_files;
  /* the error flag */
  mysql->last_error[0] = '\0';
  mysql->last_errno = 0;
  /* reset the command buffer */
  bzero(&mysql->cmd_buff,sizeof(container));
  mysql->cmd_buff.total  = 0;
  /* reset the column info set */
  bzero(&mysql->columns,sizeof(container)) ;
  mysql->columns.total = 0;
  /* allocate a new result object */
  mysql->res = (MYSQL_RES*)a_alloc(0,sizeof(MYSQL_RES));
  bzero(mysql->res,sizeof(MYSQL_RES));
  /* reset packet number */
  mysql->nr = 0;
  /* reset timestamp counter */
  //mysql->ts_ok = 0;
  bzero(&mysql->ts_ok,sizeof(struct timestamp_t));
  /* compress flag */
  mysql->compress = 0;
  /* the uncompress buffer management */
  bzero(&mysql->ucomp_buff,sizeof(container)) ;
  mysql->ucomp_buff.total = 0;
  bzero(&mysql->ucomp_tmp,sizeof(container)) ;
  mysql->ucomp_tmp.total = 0;
  /* fast rx mode */
  mysql->fast_rx = false;
  return mysql ;
}

void mysql_close(MYSQL *mysql)
{
  if (mysql) {
    /* send a 'com_quit' command */
    do_exec_cmd_simple(mysql,com_quit,0);
    /* release command buffer */
    if (mysql->cmd_buff.c) {
#if 0
      free(mysql->cmd_buff.c);
      mysql->cmd_buff.total = 0;
#else
      container_free(&mysql->cmd_buff);
#endif
    }
    /* release the column info set */
    if (COLUMN(mysql->columns)) {
#if 0
      free(COLUMN(mysql->columns));
#else
      container_free(&mysql->columns);
#endif
    }
    if (mysql->res) {
      mysql_free_result(mysql->res);
      /* release raw row set */
      release_raw_rows(mysql->res);
      free(mysql->res);
    }
    /* release the uncompress data buffer */
    if (mysql->compress) {
      container_free(&mysql->ucomp_buff);
      container_free(&mysql->ucomp_tmp);
    }
    /* close connection */
#ifdef _WIN32
    closesocket(mysql->sock);
#else
    close(mysql->sock);
#endif
    free(mysql);
  }
}

bool mysql_autocommit(MYSQL *mysql, bool auto_mode)
{
  char stmt[32] = "";
  
  if (check_conn(mysql)) {
    return -1;
  }
  sprintf(stmt,"set autocommit=%d",auto_mode) ;
  if (do_exec_cmd_simple(mysql,
      com_query,1,stmt,strlen(stmt))) {
    printd("failed\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

static int mysql_send_req_direct(MYSQL *mysql,
  const char *req, size_t sz)
{
  char *buff = preserve_send_buff(mysql,sz);

  /* 
   * copy the request into command buffer 
   *
   *  NOTE: the leading mysql header(4 bytes) should be skipped 
   */
  memcpy(buff,req,sz);
  /* send the command */
  if (do_send(mysql)<=0) {
    printd("failed send req\n");
    return -1;
  }
  return 0;
}

bool mysql_stmt_send_long_data_direct(MYSQL_STMT *stmt,
  const char *req, size_t len)
{
  return mysql_send_req_direct(stmt->mysql,req,len);
}

bool mysql_stmt_send_long_data(MYSQL_STMT *stmt, 
  uint16_t param_num, const char *data, uint32_t length)
{
  MYSQL *mysql = stmt->mysql;
  MYSQL_BIND *param = PARAM(stmt->params) + param_num ;
  //char *buff = 0;
  char arg[10];

  if (check_conn(mysql)) {
    return -1;
  }
  if (!param) {
    return 1;
  }
  if (param_num>=stmt->params.number) {
    set_last_error(mysql,"invalid param number",
      CR_INVALID_PARAMETER_NO, DEFAULT_SQL_STATE);
    return 1;
  }
  /* set to send long data */
  param->long_data_used = true ;
  /* fill in long data command info */
#if 0
  buff = preserve_send_buff(mysql,1+6+length);
  buff[0] = com_stmt_send_long_data ;
  ul4store(stmt->stmt_id,buff+1);
  ul2store(param_num,buff+5);
  memcpy(buff+7,data,length);
  /* execute the command */
  if (do_send(mysql)<=0) {
#else
  ul4store(stmt->stmt_id,arg);
  ul2store(param_num,arg+4);
  if (do_exec_cmd_simple(mysql,
     com_stmt_send_long_data,2,
     arg,6,data,length)) {
#endif
    printd("send long data failed\n");
    return 1;
  }
  return 0;
}

bool mysql_stmt_execute(MYSQL_STMT *stmt)
{
  MYSQL *mysql = stmt->mysql;
  container *c = 0;
  MYSQL_BIND *param = 0, *end= 0, **pp;
  char *buff = 0;
  size_t total = 0, 
    /* null bitmaps */
    sz_nbitmap = 0;

  if (check_conn(mysql)) {
    return -1;
  }
  pp = (MYSQL_BIND**)&(stmt->params.c) ;
  c  = &stmt->params ;
  sz_nbitmap = (c->number+7)/8 ;
  /* calc total payload size */
  total = 11 + sz_nbitmap + c->number*2 ;
  /* plugs each values' size */
  for (param=*pp, end=*pp+c->number;
     param<end;param++) {
    /* don't calculate long data's space
     *  which's to be sent by mysql_stmt_send_long_ data() */
    if (param->long_data_used)
      continue ;
    total += param->buffer_length ;
  }
  /* prefetch send buffer */
  buff = preserve_send_buff(mysql,total);
  /* set fixed part of 'stmt execute request' */
  buff[0] = com_stmt_execute ;
  buff++ ;
  ul4store(stmt->stmt_id,buff);
  buff+=4 ;
  /* flags */
  stmt->cursor_type = cursor_type_no_cursor/*cursor_type_read_only*/;
  buff[0] = stmt->cursor_type;
  buff+=1 ;
  /* iteration-count is always 1 */
  ul4store(1,buff);
  buff+=4 ;
  if (stmt->params.number>0) {
    /* the 'NULL-bitmap' field */
    bzero(buff,sz_nbitmap);
    buff+=sz_nbitmap ;
    /* new-params-bound-flag */
    buff[0] = 1;
    buff++ ;
    /* set param types */
  for (param=*pp, end=*pp+c->number;
       param<end;param++) {
      ul2store(param->buffer_type,buff);
      buff += 2;
    } /* for (param=) */
    /* set param values */
  for (param=*pp, end=*pp+c->number;
       param<end;param++) {
      /* if long data is sent before, then
       *  no need to encode it here*/
      if (param->long_data_used) 
        param->long_data_used = false ;
      else  buff = do_store_param(buff,param);
    } /* for (param=) */
  }
  /* send the command */
  if (do_send(mysql)<=0) {
    printd("failed send exec command\n");
    return -1;
  }
  /* get statement type reply */
  return get_stmt_exec_reply(stmt,true,
    stmt->cursor_type!=cursor_type_read_only);
}

/* execute the stmt_execute directly by the given request */
int mysql_stmt_execute_direct(MYSQL_STMT *stmt, char *req, size_t sz)
{
  if (mysql_send_req_direct(stmt->mysql,req,sz)) {
    return -1;
  }
  /* get statement type reply */
  return get_stmt_exec_reply(stmt,true,true);
}

bool mysql_stmt_bind_param(MYSQL_STMT *stmt, MYSQL_BIND *bind)
{
  container *c =0;
  int cnt = 0;
  MYSQL_BIND *param, *end, **pp ;
  size_t sz_bind = sizeof(MYSQL_BIND)*stmt->params.number;

  if (check_conn(stmt->mysql)) {
    return -1;
  }
  /* allocate binding buffer if needed */
  pp = (MYSQL_BIND**)&(stmt->params.c) ;
  c  = &stmt->params ;
  if (c->number>c->total) {
    c->total = c->number ;
    sz_bind = sizeof(MYSQL_BIND)*c->number;
    /* realloc the params buffer */
    *pp = (MYSQL_BIND*)a_alloc((void*)*pp,sz_bind) ;
  }
  /* process param */
  memcpy(*pp,bind,sz_bind);
  for (param=*pp, end=*pp+c->number;
     param<end; param++) {
    /* set param number */
    param->num = cnt++ ;
    /* reset long data send flag */
    param->long_data_used = false ;
    switch (param->buffer_type) {
      case MYSQL_TYPE_NULL:
        *param->is_null= true;
        break;
      case MYSQL_TYPE_TINY:
        param->buffer_length= 1;
        break;
      case MYSQL_TYPE_SHORT:
        param->buffer_length= 2;
        break ;
      case MYSQL_TYPE_LONG:
        param->buffer_length= 4;
        break ;
      case MYSQL_TYPE_LONGLONG:
        param->buffer_length= 8;
        break ;
      case MYSQL_TYPE_FLOAT:
        param->buffer_length= 4;
        break ;
      case MYSQL_TYPE_DOUBLE:
        param->buffer_length= 8;
        break;
      case MYSQL_TYPE_TIME:
        param->buffer_length= MAX_TIME_REP_LENGTH;
        break;
      case MYSQL_TYPE_DATE:
        param->buffer_length= MAX_DATE_REP_LENGTH;
        break;
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
        param->buffer_length= MAX_DATETIME_REP_LENGTH;
        break;
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_NEWDECIMAL:
        param->buffer_length = *(param->length) ;
        break;
      default:
        printd("unknown param type %d\n", 
          param->buffer_type);
        break ;
    }
  } /* for (ptr=stmt->params) */
  return 0 ;
}

unsigned long mysql_stmt_param_count(MYSQL_STMT *stmt)
{
  return check_conn(stmt->mysql)?0:
    stmt?stmt->params.number:0;
}

bool mysql_stmt_close(MYSQL_STMT *stmt)
{
  if (stmt) {
    if (PARAM(stmt->params)) {
#if 0
      free(PARAM(stmt->params));
#else
      container_free(&stmt->params);
#endif
    }
    if (BIND(stmt->binds)) {
#if 0
      free(BIND(stmt->binds));
#else
      container_free(&stmt->binds);
#endif
    }
    if (stmt->pholders.c) {
      container_free(&stmt->pholders);
    }
    /* release the column info set */
    if (COLUMN(stmt->columns)) {
      container_free(&stmt->columns);
    }
    if (stmt->res) {
      mysql_free_result(stmt->res);
      /* release raw row set */
      release_raw_rows(stmt->res);
      free(stmt->res);
    }
    /* notify server to close statement */
    if (stmt->mysql) {
      char arg[6];

      ul4store(stmt->stmt_id,arg);
      do_exec_cmd_simple(stmt->mysql,
        com_stmt_close,1,arg,4);
    }
    free(stmt);
  }
  return true ;
}

MYSQL_STMT* mysql_stmt_init(MYSQL *mysql)
{
  MYSQL_STMT *stmt = (MYSQL_STMT*)a_alloc(
    0,sizeof(MYSQL_STMT));

  /* 
   * TODO: add more initializations here 
   */
  stmt->mysql  = mysql ;
  bzero(&stmt->params,sizeof(container)) ;
  bzero(&stmt->binds,sizeof(container)) ;
  bzero(&stmt->pholders,sizeof(container)) ;
  /* reset the column info set */
  bzero(&stmt->columns,sizeof(container)) ;
  stmt->columns.total = 0;
  /* allocate a new result object */
  stmt->res = (MYSQL_RES*)a_alloc(0,sizeof(MYSQL_RES));
  bzero(stmt->res,sizeof(MYSQL_RES));
  /* set references to error managements in MYSQL */
  stmt->last_error = mysql->last_error ;
  stmt->sql_state  = mysql->sql_state ;
  stmt->last_errno = &mysql->last_errno ;
  /* reset 'timestamp ok' */
  //stmt->ts_ok = 0;
  bzero(&stmt->ts_ok,sizeof(struct timestamp_t));
  return stmt ;
}

int mysql_stmt_prepare(MYSQL_STMT *stmt, 
  const char *query, uint32_t length)
{
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return -1;
  }
  /* XXX: reset the raw row counter */
  if (stmt->res) {
    stmt->res->rows = 0;
  }
  /* 
   * TODO: check if there're duplicated 
   *  'statement preparations' 
   */
  /* send 'prepare' command */
  if (do_exec_cmd_simple(mysql,
     com_stmt_prepare,1,(char*)query,length)) {
    printd("send prepare command failed\n");
    return -1;
  }
  /* get reply */
  if (get_stmt_prepare_reply(stmt)) {
    /* XXX: error message here */
    return -1;
  }
  /* update timestamp when prepared ok */
  ts_update(&stmt->ts_ok);
  return 0;
}

void mysql_free_result(MYSQL_RES *result) 
{
  if (!result)
    return ;
  /* release text-ed row */
  if (result->row_data.c) {
#if 0
    free(result->row_data.c);
    result->row_data.c = 0;
#else
    container_free(&result->row_data);
#endif
  }
  if (result->data) {
    free(result->data);
    result->data = 0;
  }
  result->col_ref = 0;
  result->rp = result->results ;
}

MYSQL_RES* mysql_stmt_result_metadata(MYSQL_STMT *stmt)
{
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return 0;
  }
  /* 
   * allocates the result objects in mysql_stmt_init() 
   */
  /* set reference to stmt's column set */
  stmt->res->col_ref = &stmt->columns ;
  /* TODO: do other initializations here 
   */
  return stmt->res ;
}

unsigned int mysql_num_fields(MYSQL_RES *res)
{
  return res&&res->col_ref?res->col_ref->number:0;
}

MYSQL_FIELD* mysql_fetch_fields(MYSQL_RES *res)
{
  return res&&res->col_ref?COLUMN(*(res->col_ref)):0;
}

bool mysql_stmt_bind_result(MYSQL_STMT *stmt, MYSQL_BIND *my_bind)
{
  container *c =0;
  int cnt = 0;
  MYSQL_BIND *bind, *end, **pp ;
  size_t sz_bind = sizeof(MYSQL_BIND)*stmt->binds.number;
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return -1;
  }
  /* allocate binding buffer if needed */
  pp = (MYSQL_BIND**)&(stmt->binds.c) ;
  c  = &stmt->binds ;
  c->number = stmt->columns.number;
  if (c->number>c->total) {
    c->total = c->number ;
    sz_bind = sizeof(MYSQL_BIND)*c->number;
    /* realloc the result buffer */
    *pp = (MYSQL_BIND*)a_alloc((void*)*pp,sz_bind) ;
  }
  /* process result buffer */
  memcpy(*pp,my_bind,sz_bind);
  for (bind=*pp, end=*pp+c->number;
     bind<end; bind++) {
    /* set result number */
    bind->num = cnt++ ;
    if (!bind->error) 
      bind->error = &bind->error_value;
    if (!bind->is_null)
      bind->is_null = &bind->is_null_value ;
    if (!bind->length)
      bind->length = &bind->length_value ;
  }
  return 0 ;
}

int mysql_stmt_store_result(MYSQL_STMT *stmt)
{
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return -1;
  }
  /* set affected row number */
  mysql->affected_rows = stmt->res->rows ;
  /* the rows are stored in get_stmt_exec_reply() 
   *  and cursors are currently not yet supported */
  return 0;
}

int mysql_stmt_fetch(MYSQL_STMT *stmt)
{
  container *ct =0;
  MYSQL_BIND *bind, *end, **pp ;
  MYSQL_RESULT *res = 0;
  MYSQL *mysql = stmt->mysql ;
  char *data = 0, *null_ptr = 0 ;
  /* offset: ignore the header byte [00] */
  const size_t offs = (stmt?((stmt->columns.number+9)/8):0) + 1;
  uint8_t bit = 0x4;
  MYSQL_RES *mr = 0 ;
    
  if (check_conn(mysql)) {
    return -1;
  }
  if (!(mr=stmt->res)) {
    printd("no result set meta data created\n");
    return -1;
  }
  /* check end of result set reaches */
  if (!mr->rp) {
    printd("end of result set reached\n");
    return 1;
  }
  /* initialize read pointer, read 1 row */
  res  = container_of(mr->rp,MYSQL_RESULT,node);
  data = ((char*)(res->r.c))+offs ;
  /* check null mask of this row, ignore header [00] */
  null_ptr = (char*)(res->r.c) + 1;
  /* initiates the iterator */
  ct = &stmt->binds ;
  pp = (MYSQL_BIND**)&(ct->c) ;
  /* iterate each result buffer */
  for (bind=*pp, end=*pp+ct->number; bind<end; bind++) {
    /* if current field is null, then move to next
     *  without doing anything */
    if (*null_ptr&bit) {
      *(bind->is_null) = 1;
      goto __move_bit_itr;
    }
    *bind->is_null = 0;
    /* copy row values into result buffer */
    data = fetch_row_data(bind,data);
__move_bit_itr:
    /* update the null-bitmap iterator */
    if (!((bit<<=1)&0xff)) {
      bit = 1;
      null_ptr++ ;
    }
  } /* for (...) */
  /* move read pointer next */
  mr->rp = list_rest(mr->rp);
  /* 2017.2.1: fast receive: rx row(s) from 
   *  tcp layer */
  if (((MYSQL*)mr->mysql_obj)->fast_rx) {
    if (!mr->rp && mr->more)
      get_raw_rows(mr,1);
  }
  return 0;
}

unsigned long long mysql_affected_rows(MYSQL *mysql)
{
  return mysql->affected_rows;
}

unsigned long long mysql_num_rows(MYSQL_RES *res)
{
  return res?res->rows:0L;
}

int mysql_query(MYSQL *mysql, const char *query)
{
  if (do_exec_cmd_simple(mysql,
     com_query,1,query,strlen(query))) {
    printd("failed\n");
    return -1;
  }
  return get_stmt_exec_reply(mysql,false,true);
}

MYSQL_RES * mysql_store_result(MYSQL *mysql)
{
  MYSQL_RES *res = mysql->res ;

  if (!res) {
    return 0;
  }
  /* reset the result set reading pointer */
  res->rp = res->results ;
  /* set affected row number */
  mysql->affected_rows = mysql->res->rows ;
  /* the column info reference */
  res->col_ref = &mysql->columns ;
  return res ;
}

char** mysql_fetch_row(MYSQL_RES *mr)
{
  LIST *p = 0 ;
  MYSQL_RESULT *res = 0;
  size_t sz = 0;
  container *pc = 0;
  uint16_t col = 0;
  char *from = 0, *to = 0;
  
  /* end of result set */
  if (!mr->rp) {
    return 0;
  }
  p = mr->rp;
  res = container_of(p,MYSQL_RESULT,node);
  /* text-ed row data length = 
   *   total-raw-row-len + column-number x 2 */
  sz = (mr->col_ref->number)*2 + res->r.number ;
  pc = &(mr->row_data);
  pc->number = sz ;
  /* allocate new space for text-ed row */
  if ((size_t)pc->total<sz) {
    pc->total = sz ;
    pc->c = a_alloc(pc->c,sz);
  }
  /* allocate row pointers */
  mr->data = a_alloc((char*)mr->data,
    sizeof(char*) *(mr->col_ref->number));
  /* pointer on begining of raw data */
  from = res->r.c ;
  /* pointer on begining of text-ed buffer */
  to = pc->c ;
  /* copy text-ed row data, by each columns */
  for (col=0;col<mr->col_ref->number;col++) {
    /* get field length of current row */
    sz = from[0] ;
    from ++ ;
    mr->data[col] = to ;
    /* it's a null field */
    if ((uint8_t)sz==0xfb)  sz = 0;
    else  memcpy(to,from,sz);
    from+= sz ;
    to += sz;
    to[0] = '\0';
    to ++ ;
  }
  /* move next row */
  mr->rp = list_rest(mr->rp);
  /* 2017.2.1: fast receive: rx row(s) from 
   *  tcp layer */
  if (((MYSQL*)mr->mysql_obj)->fast_rx) {
    /* 1 row is read, reset rp for next read */
    mr->rp = 0;
    if (/*!mr->rp &&*/ mr->more) 
      get_raw_rows(mr,1);
  }
  return mr->data;
}

bool my_init(void)
{
  return 0;
}

void my_end(int infoflag)
{
}

unsigned long long mysql_stmt_num_rows(MYSQL_STMT *stmt) 
{
  MYSQL *mysql = stmt->mysql ;

  if (check_conn(mysql)) {
    return 0L;
  }
  return stmt->res?stmt->res->rows:0L;
}

int mysql_options(MYSQL *mysql,enum mysql_option option, 
  const void *arg) 
{
  int val = 0;

  switch(option) {
    case MYSQL_SET_CHARSET_NAME:
      val = get_cs_by_name((char*)arg);
      mysql->charset = val>=0?val:
        default_charset ;
      break ;
    case MYSQL_OPT_COMPRESS:
      mysql->client_cap |= client_compress ;
      break ;
    /* 2017.2.4: fast rx */
    case MYSQL_OPT_FAST_RX:
      mysql->fast_rx = !!(*(uint16_t*)arg) ;
      break ;
    default: break ;
  }
  return 0;
}

bool mysql_commit(MYSQL * mysql)
{
  if (check_conn(mysql)) {
    return -1;
  }
  if (do_exec_cmd_simple(mysql,
      com_query,1,"commit",strlen("commit"))) {
    printd("failed\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

bool mysql_rollback(MYSQL * mysql)
{
  if (check_conn(mysql)) {
    return -1;
  }
  if (do_exec_cmd_simple(mysql,
      com_query,1,"rollback",strlen("rollback"))) {
    printd("failed\n");
    return -1;
  }
  return get_simple_reply(mysql);
}

const char *mysql_stmt_error(MYSQL_STMT *stmt)
{
  MYSQL *mysql = stmt->mysql ;

  return mysql?mysql->last_error:0;
}

uint16_t mysql_stmt_errno(MYSQL_STMT *stmt)
{
  MYSQL *mysql = stmt->mysql ;

  return mysql?mysql->last_errno:0;
}

const char *mysql_error(MYSQL *mysql)
{
  return mysql?mysql->last_error:0;
}

uint16_t mysql_errno(MYSQL *mysql)
{
  return mysql?mysql->last_errno:0;
}

#if 0
void mysql_result_begin(MYSQL *mysql)
{
  mysql->res->rp = mysql->res->results;
}
#endif

static int mysql_str2bin(uint8_t type, char *in, size_t sz, 
  void *outb, size_t *o_sz)
{
  switch (type) {
    case MYSQL_TYPE_NULL:
      break;
    case MYSQL_TYPE_TINY:
      {
        sscanf(in,"%hhd",(uint8_t*)outb);
        *o_sz = sizeof(uint8_t);
      }
      break;
    case MYSQL_TYPE_SHORT:
      {
        *(short*)outb = atoi(in);
        *o_sz = sizeof(short);
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        *(long*)outb = atol(in);
        *o_sz = sizeof(long);
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        *(long long*)outb = atoll(in);
        *o_sz = sizeof(long long);
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        *(float*)outb = atof(in);
        *o_sz = sizeof(float);
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        *(double*)outb = atof(in);
        *o_sz = sizeof(double);
      }
      break;
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      break;
    default:
      break ;
  }
  return 0;
}

static int mysql_get_col_sz_by_type(char *inb, uint8_t type, 
  size_t *offs)
{
  char *pr = inb ;
  MYSQL_TIME tmp;
  MYSQL_BIND bnd ;

  /* get offset by column type */
  switch (type) {
    case MYSQL_TYPE_NULL:
      break;
    case MYSQL_TYPE_TINY:  *offs = sizeof(uint8_t);
      break;
    case MYSQL_TYPE_SHORT:  *offs = sizeof(uint16_t);
      break ;
    case MYSQL_TYPE_LONG:   *offs = sizeof(uint32_t);
      break ;
    case MYSQL_TYPE_LONGLONG:  *offs = sizeof(long long);
      break ;
    case MYSQL_TYPE_FLOAT:  *offs = sizeof(float);
      break ;
    case MYSQL_TYPE_DOUBLE:  *offs = sizeof(double);
      break;
    case MYSQL_TYPE_TIME:  *offs = timeget(&tmp,pr);
      break;
    case MYSQL_TYPE_DATE:  *offs = dateget(&tmp,pr);
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:  *offs = datetimeget(&tmp,pr);
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:  
#if 0
      *offs = getbin(&bnd,pr);
#else
      {
        char buff[4096];

        bnd.buffer = buff ;
        bnd.buffer_length = 4096 ;
        bnd.length = offs ;
        *offs = getbin(&bnd,pr);
      }
#endif
      break;
    default:
      break ;
  }
  return 0;
}

static int mysql_get_col_val_by_type(char *inb, uint8_t type, 
  uint8_t *buff, size_t *sz)
{
  char *pr = inb ;
  MYSQL_TIME tmp;
  MYSQL_BIND bnd ;

  /* get column value by type */
  switch (type) {
    case MYSQL_TYPE_NULL:
      break;
    case MYSQL_TYPE_TINY:  
      if (buff) *(uint8_t*)buff = *(uint8_t*)pr ;
      if (sz) *sz = sizeof(uint8_t);
      break;
    case MYSQL_TYPE_SHORT:  
      if (buff) *(uint16_t*)buff = byte2_2_ul(pr) ;
      if (sz) *sz = sizeof(uint16_t);
      break ;
    case MYSQL_TYPE_LONG:
      if (buff) *(uint32_t*)buff = byte4_2_ul(pr) ;
      if (sz) *sz = sizeof(uint32_t);
      break ;
    case MYSQL_TYPE_LONGLONG:
      if (buff) *(long long*)buff = byte8_2_ul(pr) ;
      if (sz) *sz = sizeof(long long);
      break ;
    case MYSQL_TYPE_FLOAT:
      if (buff) *(float*)buff = byte4_2_float(pr) ;
      if (sz) *sz = sizeof(float);
      break ;
    case MYSQL_TYPE_DOUBLE:
      if (buff) *(double*)buff = byte8_2_double(pr) ;
      if (sz) *sz = sizeof(double);
      break;
    case MYSQL_TYPE_TIME:  
      if (buff) timeget((MYSQL_TIME*)buff,pr); 
      if (sz) *sz = timeget(&tmp,pr);
      break;
    case MYSQL_TYPE_DATE:
      if (buff) dateget((MYSQL_TIME*)buff,pr); 
      if (sz) *sz = dateget(&tmp,pr);
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      if (buff) datetimeget((MYSQL_TIME*)buff,pr); 
      if (sz) *sz = datetimeget(&tmp,pr);
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
#if 0
      if (buff) getbin((MYSQL_BIND*)buff,pr); 
      if (sz) *sz = getbin(&bnd,pr);
#else
      if (sz) {
        char *in = pr ;
        size_t ln= lenenc_int_get(&in);

        *sz = ln + (in-pr);
      }
      if (buff) {
        bnd.buffer = buff ;
        bnd.buffer_length = *sz ;
        bnd.length = sz ;
        getbin(&bnd,pr);
      }
#endif
      break;
    default:
      break ;
  }
  return 0;
}

int mysql_pickup_bin_col_val(
  void *pCols, int num_cols, /* the column definitions */
  char *rec, size_t szRec,  /* the record buffer */
  const char *col_name,  /* name of targeting column */
  uint8_t *col_val, size_t *szVal,  /* buffer to hold resulting value */
  size_t *pOffs  /* offset between record beginning and real data */
  )
{
  int nCol = 0;
  MYSQL_COLUMN *pc = pCols ;
  /* skip length+sn bytes & null-bitmap bits & header[00] */
  char *pr = rec+4+(num_cols+/*9*/7)/8+1 ;
  size_t offs = 0;
  uint8_t bit = 0x4;
  /* get null-bitmap, skipping header[00] */
  char *null_bm = rec+4+1;
  bool isNull = false ;

  for (nCol=0;nCol<num_cols;nCol++,pr+=offs,pc++) {
    /* test if it's null column */
    if ((isNull=*null_bm&bit)) { offs=0; continue ; }
    /* get offset and required buffer size by column type */
    mysql_get_col_sz_by_type(pr,pc->type,&offs);
    /*printd("req col %s, col %s, type %d, offs %zu\n",
      col_name,pc->name,pc->type,offs);*/
    /* check if the column name matches */
    if (!strcmp(col_name,pc->name)) {
      if (isNull) {
        *szVal = 0;
        return 0;
      }
      /* test for value size only */
      if (!col_val) {
        mysql_get_col_val_by_type(pr,pc->type,0,szVal);
        return 0;
      }
      /* check value buffer size */
      if (*szVal<offs) 
        return -1;
      /* copy the content */
      mysql_get_col_val_by_type(pr,pc->type,col_val,szVal);
      /* the real value offset */
      if (pOffs) *pOffs = pr-rec ;
      //hex_dump(pr,offs);
      break ;
    }
    /* move bit iterator */
    bit = !((bit<<1)&0xff)?1:(bit<<1);
  } /* end for(...) */
  return 0;
}

int mysql_pickup_text_col_val(
  void *pCols, int num_cols, /* the column definitions */
  char *rec, size_t szRec,  /* the record buffer */
  const char *col_name,  /* name of targeting column */
  uint8_t *col_val, size_t *szVal,  /* buffer to hold resulting value */
  size_t *pOffs  /* offset between record beginning and real data */
  )
{
  int nCol = 0;
  MYSQL_COLUMN *pc = pCols ;
  /* skip length+sn bytes */
  char *pr = rec+4 ;
  size_t offs = 0, szReq = 0;

  for (nCol=0;nCol<num_cols;nCol++,pr+=offs,pc++) {
    /* get offset and required buffer size 
     *  by column type */
    char *o_pr = pr ;
    int sz_ln  = lenenc_int_size_get(pr);

    szReq = lenenc_int_get(&pr);
    offs  = szReq;
    /* point to begining of real value */
    pr = o_pr+ sz_ln;

    /*printf("req col %s, col %s, type %d, offs %zu\n",
      col_name,pc->name,pc->type,offs);*/
    /* check if the column name matches */
    if (!strcmp(col_name,pc->name)) {
      /* test for value size only */
      if (!col_val) {
        *szVal = szReq+1 ;
        return 0;
      }
      /* check value buffer size */
      if (*szVal<szReq) 
        return -1;
      /* copy the content */
      memcpy(col_val,pr,szReq);
      col_val[szReq] = '\0';
      *szVal = szReq ;
      /* format convert from text to bin */
      mysql_str2bin(pc->type,(char*)col_val,szReq,
        col_val,szVal);
      /* the real value offset */
      if (pOffs) *pOffs = o_pr-rec ;
      //printf("col_val: %d, sz: %zu\n",*col_val,szReq);
      //hex_dump(rec,szRec);
      break ;
    }
  } /* end for(...) */
  return 0;
}

int mysql_result_next(void *pObj, bool bStmt, 
  char **outb, size_t *sz, bool next)
{
  LIST **cur = 0 ;
  MYSQL_RESULT *res = 0;
  MYSQL_RES *pRes = 0;
  bool fast_rx = 0;

  if (bStmt) {
    pRes = ((MYSQL_STMT*)pObj)->res;
    fast_rx = ((MYSQL_STMT*)pObj)->mysql->fast_rx ;
  } else {
    pRes = ((MYSQL*)pObj)->res;
    fast_rx = ((MYSQL*)pObj)->fast_rx ;
  }
  cur = &pRes->rp ;
  /* no item left */
  if (!*cur||!pRes->more) {
    return 0;
  }
  res = container_of(*cur,MYSQL_RESULT,node);
  /* get record buffer */
  if (outb) *outb = res->r.c ;
  if (sz)   *sz   = res->r.number ;
  /* try to read next row */
  if (next==true) {
    *cur  = (*cur)->next ;
    /* try next row */
    if (fast_rx)
      get_raw_rows(pRes,1);
  }
  return 1;
}

int mysql_calc_blob_count(char *inb, int total_phs, int *nblob)
{
  /* get new-params-bound-flag bytes */
  char *params = 0;
  int i = 0;
  uint16_t t = 0;
  char *pNewBound = 0 ;

  if (!inb) {
    *nblob = 0;
    return 0;
  }

  pNewBound = inb+4+1+4+1+4+((total_phs+7)/8) ;
  if (*pNewBound!=1) {
    *nblob = 0;
    return 0;
  }
  params = pNewBound + 1;

  for (*nblob=0,i=0;i<total_phs;i++,params+=2) {
    t = byte2_2_ul(params);
    *nblob = (
      t==MYSQL_TYPE_TINY_BLOB  ||
      t==MYSQL_TYPE_MEDIUM_BLOB ||
      t==MYSQL_TYPE_LONG_BLOB   ||
      t==MYSQL_TYPE_BLOB        ||
      t==MYSQL_TYPE_VARCHAR     ||
      t==MYSQL_TYPE_VAR_STRING  ||
      t==MYSQL_TYPE_STRING      ||
      t==MYSQL_TYPE_DECIMAL     ||
      t==MYSQL_TYPE_NEWDECIMAL
    )  ? (*nblob+1):*nblob;
  }
  return 0;
}

/* get place holder's type & value from stmt_exec_req */
size_t mysql_stmt_exec_get_ph_subsequent(
  char *inb, 
  bool bGetNB, /* get new-bound-flag or not */
  int total_phs,
  int type, 
  char *val)
{
  /* get new-params-bound-flag bytes */
  char *params = 0;
  size_t offs = 0;

  if (bGetNB) {
    char *pNewBound = inb+4+1+4+1+4+((total_phs+7)/8) ;
    params = pNewBound + 1;
  } else {
    params = inb ;
  }
  offs = params - inb ;
  switch (type) {
    case MYSQL_TYPE_NULL:
      break;
    case MYSQL_TYPE_TINY:
      {
        offs +=1 ;
        *(uint8_t*)val = *(uint8_t*)params ;
      }
      break;
    case MYSQL_TYPE_SHORT:
      {
        offs += 2;
        *(uint16_t*)val = *(uint16_t*)params ;
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        offs += 4;
        *(uint32_t*)val = *(uint32_t*)params ;
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        offs += 8;
        *(uint64_t*)val = *(uint64_t*)params ;
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        offs += 4;
        *(float*)val = *(float*)params ;
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        offs += 8;
        *(double*)val = *(double*)params ;
      }
      break;
    case MYSQL_TYPE_TIME:
      {
      }
      break;
    case MYSQL_TYPE_DATE:
      {
      }
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
      {
      }
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
      {
        offs = 0;
      }
      break;
    default:
      break ;
  } /* end switch() */
  return offs;
}

/* get place holder's type & value from stmt_exec_req */
int mysql_stmt_exec_get_ph(char *inb, int total_phs, int nph,
  int *type, char *val)
{
  /* get new-params-bound-flag bytes */
  char *pNewBound = inb+4+1+4+1+4+((total_phs+7)/8) ;
  char *types = 0, *params = 0;
  int i = 0;
  uint16_t t = 0;
  size_t offs = 0;

  if (nph>=total_phs) {
    return -1;
  }
  if (*pNewBound!=1) {
    /* it's subsequent call, no 'type' items */
    return 1;
  }
  types  = pNewBound + 1;
  params = pNewBound + 1 + total_phs*2 ;
  for (i=0;i<total_phs;i++,params+=offs) {
    t = byte2_2_ul(types+i*2);
    switch (t) {
      case MYSQL_TYPE_NULL:
        break;
      case MYSQL_TYPE_TINY:
        {
          offs =1 ;
          *(uint8_t*)val = *(uint8_t*)params ;
        }
        break;
      case MYSQL_TYPE_SHORT:
        {
          offs = 2;
          *(uint16_t*)val = *(uint16_t*)params ;
        }
        break ;
      case MYSQL_TYPE_LONG:
        {
          offs = 4;
          *(uint32_t*)val = *(uint32_t*)params ;
        }
        break ;
      case MYSQL_TYPE_LONGLONG:
        {
          offs = 8;
          *(uint64_t*)val = *(uint64_t*)params ;
        }
        break ;
      case MYSQL_TYPE_FLOAT:
        {
          offs = 4;
          *(float*)val = *(float*)params ;
        }
        break ;
      case MYSQL_TYPE_DOUBLE:
        {
          offs = 8;
          *(double*)val = *(double*)params ;
        }
        break;
      case MYSQL_TYPE_TIME:
        {
        }
        break;
      case MYSQL_TYPE_DATE:
        {
        }
        break;
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
        {
        }
        break;
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_NEWDECIMAL:
        {
          offs = 0;
        }
        break;
      default:
        break ;
    } /* end switch() */
    if (i==nph) {
      *type = t;
      //*val  = params ;
      return 0;
    }
  } /* end for() */
  return -1;
}

