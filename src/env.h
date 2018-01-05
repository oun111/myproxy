
#ifndef __ENV_H__
#define __ENV_H__

#include "myproxy_config.h"
#include "container_impl.h"
#include "hook.h"

namespace GLOBAL_ENV {

  /* the payload size */
  constexpr int MAX_PAYLOAD = 1600;

  /* the config file item */
  extern myproxy_config m_conf ;

  /* the table list */
  extern safeTableDetailList m_tables;

#define likely(x) __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)
} ;


#endif /* __ENV_H__*/
