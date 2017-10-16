
#ifndef __DBUG_H__
#define __DBUG_H__


#ifdef	__cplusplus
extern "C" {
#endif

extern void hex_dump(char*,size_t);

#ifdef _WIN32
#define printd printf
#else
#ifdef printd
#undef printd
#endif
#define printd(fmt,arg...) do{\
  fprintf(stdout,"[%s]: " fmt, \
    __func__,  ## arg ); \
  fflush(stdout);\
} while(0)
#endif

#ifdef	__cplusplus
}
#endif

#endif /* __DBUG_H__*/

