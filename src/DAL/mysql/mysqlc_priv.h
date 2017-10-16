
#ifndef __MYSQLC_PRIV_H__
#define __MYSQLC_PRIV_H__


/* 
 * get parent pointer, copy from $(kernel source)include/linux/kernel.h 
 */
#ifdef _WIN32
#define container_of(ptr, type, member)  \
  (type *)( (char *)ptr - offsetof(type,member) )
#else
#define container_of(ptr, type, member) ({ \
  const typeof( ((type *)0)->member ) *__mptr = (ptr); \
  (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

/* max buffer size */
#define MAX_BUFF  /*128*/128*2
/* max name buffer length */
#define MAX_NAME_LEN 128*3 /*64*/

/* the container */
typedef struct container_t {
  /* container capacity */
  int total ;
  /* valid element numbers */
  int number ;
  /* the container object */
  void *c ;
} container ;

/* 
 * copy from $(mysql source)/include/mysql_time.h 
 */
enum enum_mysql_timestamp_type
{
  mysql_timestamp_none= -2, mysql_timestamp_error= -1, 
  mysql_timestamp_date= 0, mysql_timestamp_datetime= 1, mysql_timestamp_time= 2
};

struct timestamp_t {
  int64_t sec, usec ;
} ;

#endif /* __MYSQLC_PRIV_H__ */

