#ifndef __WIN_H__
#define __WIN_H__


/*
 * transplant difference from gcc to cl
 */
#ifdef _WIN32

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>

#ifndef ssize_t
  #define ssize_t long long
#endif

#ifndef atoll
  #define atoll   _atoi64
#endif

#ifndef printd
/* build mysqlc */
#ifdef __MYSQLC_UNDER_WIN32__
  #define printd 
#else
  #define printd   printf 
#endif
#endif

#ifndef printd0
  #define printd0   printf
#endif

#ifndef PATH_MAX
  #define PATH_MAX   _MAX_PATH
#endif

#ifndef getpid
  #define getpid   GetCurrentProcessId
#endif

#ifndef usleep
  #define usleep   Sleep
#endif

#ifndef sleep
  #define sleep   Sleep
#endif

#ifndef __attribute__
  #define __attribute__(x)
#endif

/* build mysqlc */
#ifdef __MYSQLC_UNDER_WIN32__
  typedef unsigned char bool ;
  #define true  1
  #define false 0
#endif

#ifndef __func__
  #define __func__ __FUNCTION__
#endif

#ifndef bzero
  #define bzero(b,n)  memset(b,0,n)
#endif

#ifndef strcasecmp
  #define strcasecmp _stricmp
#endif

#ifndef strncasecmp
  #define strncasecmp _strnicmp
#endif

#ifndef __builtin_expect
  #define __builtin_expect(x, expected_value) (x)
#endif


static char* strcasestr(char *src, char *ptn)
{
  char *src0 = 0, *ptn0 = 0, *p = 0 ;

  if (!src)
    return NULL ;
  /* copy the source string */
  //src0 = new char[strlen(src)+1];
  src0 = (char*)malloc(strlen(src)+1);
  strcpy(src0,src);
  /* make lower case */
  for (p=src0;p&&*p;p++)
    *p = tolower(*p);
  /* copy the pattern */
  //ptn0 = new char[strlen(ptn)+1];
  ptn0 = (char*)malloc(strlen(ptn)+1);
  strcpy(ptn0,ptn);
  for (p=ptn0;p&&*p;p++)
    *p = tolower(*p);
  /* find pattern occurance */
  p = strstr(src0,ptn0);
  /* release stirngs */
  /*delete []src0;
  delete []ptn0;*/
  free(src0);
  free(ptn0);
  /* get actual position */
  return !p?0:(src+(p-src0));
}

static char* stpcpy(char *dst, char *src)
{
  char *ptr = 0;

  strcpy(dst,src);
  for (ptr=dst;ptr&&*ptr;ptr++);
  return ptr ;
}

static char* stpncpy(char *dst, char *src, size_t n)
{
  size_t i=0;
  char *ptr = 0;

  strncpy(dst,src,n);
  for (ptr=dst;ptr&&*ptr&&i<n;ptr++,i++);
  return ptr ;
}


#endif /* ifdef _WIN32 */

#endif  /* __WIN_H__ */

