#ifndef __DBG_LOG_H__
#define __DBG_LOG_H__

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

class dbg_log
{
protected:
  //unsigned long long g_log_id ; 
  //FILE *g_log_fd  ;
  int g_log_fd ;
  int fno  ;
  uint32_t nline ;
  const uint32_t max_lines;
  char m_desc[128], m_path[PATH_MAX] ;
  struct tLogInfo {
    uint32_t current_date ;
    int file_no ;
  } m_logInfo ;

public:
#define mkdate(y,m,d)  (uint32_t)(\
  (((y)&0xffff)<<16) |  \
  (((m)&0xff)<<8)    |  \
  ((d)&0xff)            \
)
#define getdate(in,y,m,d) do { \
  y = ((in)>>16)&0xffff;       \
  m = ((in)>>8)&0xff;          \
  d = (in)&0xff;               \
} while(0)
  dbg_log(char*name,char*path=(char*)"/tmp/"):
    g_log_fd(0),fno(0),
    nline(0),max_lines(100000)
  {
    if (!name) strcpy(m_desc,"default_loger");
    else strcpy(m_desc,name);
    strcpy(m_path,path);
    /* create/open the log file */
    init();
  }
  ~dbg_log(void) { release(); }
  /* log file name */
  void get_log_name(char *name,char *path,
     int fileno) {
    struct tm tm1 ;
    time_t tv ;
    tv = time(0);
    gmtime_r(&tv,&tm1);
    sprintf(name,"%s/%s_%d_%4d%02d%02d_%d.log.tmp", 
      path,m_desc,getpid(),tm1.tm_year+1900, 
      tm1.tm_mon+1, tm1.tm_mday, fileno);
    /* save current log name info */
    m_logInfo.current_date = mkdate(tm1.tm_year,
      tm1.tm_mon, tm1.tm_mday) ;
    m_logInfo.file_no = fileno ;
  }
  /* log init */ 
  int init(void) {
    char lPath[PATH_MAX]=""; 
    if (g_log_fd) {
      release();
      /* remove tailing '.tmp' from 
       *  previous file */
      {
        char tmp[PATH_MAX*10]="";
        int y,m,d ;
        getdate(m_logInfo.current_date,y,m,d);
        sprintf(tmp,"%s/%s_%d_%4d%02d%02d_%d.log.tmp", 
          m_path,m_desc,getpid(),y+1900, 
          m+1, d, m_logInfo.file_no);
        strcpy(lPath,tmp);
        lPath[strlen(lPath)-4] = '\0';
        rename(tmp,lPath);
        //printf("rename : %s -> %s\n", tmp, lPath);
      }
    } 
    get_log_name(lPath,(char*)m_path,fno);
    //g_log_fd = creat(lPath,S_IRWXU);
    g_log_fd = open(lPath,O_CREAT|O_WRONLY|O_NONBLOCK,S_IRWXU);
    return !!g_log_fd ;
  }
  /* log release */ 
  void release(void) {
    if (g_log_fd) { 
      close(g_log_fd); 
      g_log_fd = 0;
    }
  }
  /* log formated output */ 
  int print(char *buf) {
    char hdr[256] ;
    struct tm tm1;
    time_t tv; 
    struct timeval tv1;
    tv = time(0);
    gmtime_r(&tv,&tm1);
    gettimeofday(&tv1,0);
    if (g_log_fd) {
      sprintf(hdr,"[%02d%02d%02d:%06ld]: ", 
        (tm1.tm_hour+8)%24,tm1.tm_min, tm1.tm_sec, 
        tv1.tv_usec);
      write(g_log_fd,hdr,strlen(hdr));
      write(g_log_fd,buf,strlen(buf));
      //dprintf(g_log_fd,"%s",buf);
      //syncfs(g_log_fd);
    }
    /* reaches line number limit, open a new 
     *  log file */
    if (nline++>max_lines) {
      fno++,nline=0;
      init();
    }
    /* reaches date ending, also open a new file */
    {
      time_t tv = time(0);

      gmtime_r(&tv,&tm1);
      if (mkdate(tm1.tm_year,tm1.tm_mon,
         tm1.tm_mday)!=m_logInfo.current_date) {
        fno=0,nline=0;
        init();
      }
    }
    return 1;
  } 
  char* get_desc(void) { 
    return (char*)m_desc; 
  }
  int get_log_fd(void) {
    return g_log_fd ;
  }
};

/* the log levels */
enum llevels {
  l_all,
  l_debug,
  l_info,
  l_none,
} ;

#define DECLARE_LOG(name) \
  dbg_log g_log((char*)name) ;\
  int g_level = l_all;

extern dbg_log g_log ;

#define log_print(fmt,arg...) do{\
  char buf[2048]="";\
  snprintf(buf, 2048,"thread(%lx): %s: " fmt,pthread_self(),\
    __func__, ## arg ); \
  g_log.print(buf);\
} while(0)

#endif /* __DBG_LOG_H__ */

