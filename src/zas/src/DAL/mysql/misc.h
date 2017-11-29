
#ifndef __MISC_H__
#define __MISC_H__

/*
 * copy from zlib source
 */
extern int do_uncompress(uint8_t *inb, int sz_in,
  uint8_t *outb, int *sz_out);

extern int do_compress(uint8_t *inb, int sz_in, uint8_t *outb,
  int *sz_out);

extern void hex_dump(char *buf, size_t sz);

extern size_t timestore(MYSQL_TIME *tm, char *out);

extern size_t timeget(MYSQL_TIME *tm, char *in);

extern size_t datetimestore(MYSQL_TIME *tm, char *out);

extern size_t dateget(MYSQL_TIME *tm, char *in);

extern size_t datetimeget(MYSQL_TIME *tm, char *in);

extern size_t getbin(MYSQL_BIND *bind, char *in);

extern size_t getstr(MYSQL_BIND *bind, char *in);

/* do malloc with size alignment */
extern void* a_alloc(char *p,size_t s);

extern void container_free(container *pc);

extern char* str_store(char *ptr, const char *str);

extern int set_last_error(MYSQL *mysql, const char *msg, 
  int no, const char *sqlstat);

extern int new_tcp_myclient(MYSQL *mysql, const char *host, 
  uint16_t port);

extern char* preserve_send_buff(MYSQL *mysql, 
  size_t size);

extern void reassign_send_buff_size(MYSQL *mysql, 
  const size_t size);

extern int send_by_len(MYSQL *mysql, char *buff,
  const int len);

extern int recv_by_len(MYSQL *mysql, char *buff, 
  size_t const total);

extern bool ts_compare(struct timestamp_t *t0, 
  struct timestamp_t *t1);

extern void ts_update(struct timestamp_t *t);

#endif /* __MISC_H__*/
