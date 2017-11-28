
#ifndef __RESIZABLE_CONTAINER_H__
#define __RESIZABLE_CONTAINER_H__

#include <stddef.h>

#if 0
#define DECLARE_RSZ_CONTAINER(tc) \
  tContainer tc = \
  { \
    /*.b =*/ 0,\
    /*.capacity =*/ 0, \
    /*.size =*/ 0,\
  }

#define DESTRUCT_RSZ_CONTAINER(tc) do {\
  tc_close(&tc);\
} while (0)

#define TC_RETURN(tc,rc) ({\
  DESTRUCT_RSZ_CONTAINER(tc);\
  return rc; \
})
#endif

class tContainer {
protected:
  char *b ;
  size_t capacity ;
  size_t size ;

public:
  tContainer(void) ;
  ~tContainer(void) ;

public:
  void tc_init(void) ;

  void tc_close(void) ;

  char *tc_data(void) ;

  size_t tc_capacity(void) ;

  size_t tc_length(void) ;

  size_t tc_free(void) ;

  void tc_resize(size_t new_sz) ;

  size_t tc_write(char *in, size_t sz) ;

  size_t tc_read(char *out, size_t sz, size_t rpos) ;

  void tc_update(size_t sz) ;

  int tc_copy(tContainer *op) ;
  tContainer& operator = (tContainer&src);

  int tc_concat(char*,size_t);
} ;

#endif /* __RESIZABLE_CONTAINER_H__*/
