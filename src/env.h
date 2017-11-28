
#ifndef __ENV_H__
#define __ENV_H__

#include "myproxy_config.h"
#include "container_impl.h"
#include "hook.h"

namespace GLOBAL_ENV {

  /* the config file item */
  extern myproxy_config m_conf ;

  /* the table list */
  extern safeTableDetailList m_tables;
} ;


#endif /* __ENV_H__*/
