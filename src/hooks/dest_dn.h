
#ifndef __DEST_DN_H__
#define __DEST_DN_H__

#include "hook.h"
#include "container_impl.h"


/* hook destination datanode number */
class dest_dn_hook : public hook_object {

public:
  dest_dn_hook(void); 
  ~dest_dn_hook(void) ;

public:
  int run_hook(sql_tree &st,stxNode *root,void *params) ;
} ;


#endif /* __DEST_DN_H__*/

