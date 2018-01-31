
#include <string>
#include "env.h"


namespace GLOBAL_ENV {

  /* the config file item */
  myproxy_config m_conf ;

  /* the table list */
  safeTableDetailList m_tables;

  /* the myfd -> physical statement id mappings */
  safeMyFdMapList m_mfMaps ;
} ;

