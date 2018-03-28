
#ifndef __EPI_ENV_H__
#define __EPI_ENV_H__


#include "epi_list.h"


class TLS {
private:
  pthread_key_t m_tkey ;

public:
  TLS(void) {
    /* acquire the TLS */
    pthread_key_create(&m_tkey,NULL);
  }
  ~TLS(void) {
    /* release the TLS */
    pthread_key_delete(m_tkey);
  }
  void* get(void) {
    return pthread_getspecific(m_tkey) ;
  }
  void set(void* val) {
    pthread_setspecific(m_tkey,val) ;
  }
} ;

namespace EPI_ENV {

  extern epi_list g_epItems ;

  /* the TLS object */
  extern TLS m_tls ;
} ;

#endif /* __EPI_ENV_H__*/

