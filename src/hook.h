
#ifndef __HOOK_H__
#define __HOOK_H__

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <stack>
#include <regex.h>
#include <limits.h>
#include "mysqlc.h"
#include "connstr.h"
#include "sql_tree.h"
#include "container.h"
#include "dbg_log.h"

enum hook_type
{
  /* hook sql statement */
  h_sql,

  /* hook request header */
  h_req,

  /* hook responses */
  h_res,
} ;

class hook_object /*: public sql_tree*/ {

public:
  const int hook_type ;

public: 
  hook_object(int t = h_sql, char *s=0) : hook_type(t) {}
  virtual ~hook_object(void) {}

public: 
  /* do hook on sql */
  virtual int run_hook(sql_tree&,stxNode*root,void *param) { return 0; }

  /* do hook on request header */
  virtual int run_hook(char *req, size_t sz, void *param) { return 0; }
} ;

class hook_framework {

  char *m_pNewReq ;

public:
  hook_framework(void) ;
  ~hook_framework(void) ;

protected:
  int do_sql_hooks(char **req, size_t &szReq, void *param,
    stxNode *tree);

  int do_normal_hooks(char **pkt, size_t &szPkt, void *param, int htype);

public:
  /* request hooks */
  int run(char **pkt, size_t &szPkt, void *param, 
    int htype=h_sql, stxNode *tree=0);

  void clear(void);
} ;


#define HMODULE_NAME(__name)  g_ ##__name## _mod

#define HMODULE_IMPL(__type,__name) __type HMODULE_NAME(__name)

#define HMODULE_DECL(__type,__name)  extern __type HMODULE_NAME(__name) 

#define HMODULE_UNIMPL(__type,__name) 

#endif /* __HOOK_H__*/


