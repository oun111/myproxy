
#ifndef __STMT_ID_H__
#define __STMT_ID_H__

#include "hook.h"
#include "container_impl.h"


using stmtIdHookParam = struct stmt_id_hook_param_t {
  /* datanode number */
  int dn ;

  /* datanode group id */
  int grp ;

  /* logical statement id <--> physical statement id map  */
  safeDnStmtMapList *maps ;
} ;

/* hook statement id */
class stmt_id_hook : public hook_object {

public:
  stmt_id_hook(void); 
  ~stmt_id_hook(void) ;

public:
  int run_hook(char *req, size_t sz, void *arg) ;
} ;


#endif /* __STMT_ID_H__*/

