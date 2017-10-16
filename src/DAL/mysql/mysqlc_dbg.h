#ifndef __MYSQLC_DBG_H__
#define __MYSQLC_DBG_H__


#ifndef _WIN32

#if 0
  #define printd(fmt,arg...) do{\
    printf("%s(): "fmt, \
      __func__, ## arg ); \
  } while(0)
#else
  #define printd(fmt,arg...) do{\
  }while (0)
#endif

#endif


#endif /* __MYSQLC_DBG_H__ */
