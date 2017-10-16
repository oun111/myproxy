#ifndef __DBG_LOG_H__
#define __DBG_LOG_H__

#ifdef DBG_LOG

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#define DECLARE_DBG_LOG(name) \
  unsigned long long g_## name ##_log_id = 0 ; \
  FILE *g_## name ##_log_fd = 0 ;\
  /* log init */ \
  int dbg_log_## name ##_init(void) {\
    char lPath[PATH_MAX]; \
    if (g_## name ##_log_fd) {\
      return 1;\
    } \
    sprintf(lPath,"/tmp/%s.%d.log", \
      #name,getpid());\
    g_## name ##_log_fd = fopen(lPath,"w");\
    return !!g_## name ##_log_fd ;\
  } \
  /* log release */ \
  void dbg_log_## name ##_release(void) {\
    if (g_## name ##_log_fd) { \
      fclose(g_## name ##_log_fd); \
      g_## name ##_log_fd = 0;\
    }\
  }\
  /* log formated output */ \
  int dbg_log_## name ##_print(char *buf) {\
    char hdr[256] ;\
    struct tm tm1;\
    time_t tv; \
    struct timeval tv1;\
    if (!g_## name ##_log_fd) \
      return 0;\
    tv = time(0);\
    gmtime_r(&tv,&tm1);\
    gettimeofday(&tv1,0);\
    sprintf(hdr,"[%d.%d.%d-%d:%d:%d.%ld, id:%lld]: ", \
      tm1.tm_year+1900, tm1.tm_mon+1, tm1.tm_mday, \
      tm1.tm_hour+8,tm1.tm_min, tm1.tm_sec, \
      tv1.tv_usec,g_## name ##_log_id&0xffff);\
    fwrite(hdr,strlen(hdr),1, \
      g_## name ##_log_fd);\
    if (buf) {\
      fwrite(buf,strlen(buf),1, \
        g_## name ##_log_fd);\
      fflush(g_## name ##_log_fd);\
    } \
    return 1;\
  } 

/* declares external usage of log */
#define DECLARE_DBG_LOG_EXTERN(name) \
  extern unsigned long long g_## name ##_log_id ; \
  extern FILE *g_## name ##_log_fd ;\
  extern int dbg_log_## name ##_print(char*);

/* 
 * log invokation interfaces
 */
/* set log object id, use to identify 
 *   each log records */
#define __log_id_set(name,id) \
  (g_## name ##_log_id = (unsigned long long)(id))
/* log machanism initialize, default to 
 *   /tmp/$(log name).$(pid).log */
#define __log_init(name) dbg_log_## name ##_init
/* log machanism release */
#define __log_rel(name)  dbg_log_## name ##_release
/* log output */
#define __log_print(name,fmt,arg...) do{\
  char buf[2048]="";\
  snprintf(buf, 2048, fmt, ## arg ); \
  dbg_log_## name ##_print(buf);\
} while(0)


/* implement log machanism with name 'zas' */
#define log_id_set(id)        __log_id_set(zas,id)
#define log_init              __log_init(zas)
#define log_rel               __log_rel(zas)
#define log_print(fmt,arg...) __log_print(zas,fmt,## arg)


#endif /* ifdef DBG_LOG */

#endif /* __DBG_LOG_H__ */

