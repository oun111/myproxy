
#ifndef __STATIC_PARSE_H__
#define __STATIC_PARSE_H__

#include <memory>
#include "hook.h"
#include "connstr.h"
#include "adaptors.h"


/* static parsing on sql */
class static_parse : public hook_object {

private:
  mysql_adpt m_adpt ;

public:
  static_parse(void); 
  ~static_parse(void) ;

public:
  int run_hook(sql_tree &st,stxNode *root,void *params);
} ;


#endif /* __STATIC_PARSE_H__*/

