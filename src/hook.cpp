
#include "hook.h"
#include "dbg_log.h"
#include <typeinfo>
#include "env.h"
#include "hook_list.h"


hook_framework::hook_framework() : m_pNewReq(0)
{
}

hook_framework::~hook_framework(void)
{
  clear();
}

int 
hook_framework::do_normal_hooks(char **pkt, size_t &szPkt, void *param, int ht)
{
  uint16_t i=0;
  hook_object *pb = 0;

  for (;i<total_hModules;i++) {
    pb = const_cast<hook_object*>(g_hModuleList[i]) ;
    if (!pb || pb->hook_type!=ht) {
      continue ;
    }
    pb->run_hook(*pkt,szPkt,param);
  }
  return 0;
}

int 
hook_framework::do_sql_hooks(char **req, size_t &szReq, void *param,
  stxNode *tree)
{
  hook_object *pb = 0;
  uint16_t i=0;
  stxNode *root = 0;
  int ret = 0, rc=0;
  /* get statement location */
  char *pStmt = *req + 5;
  /*const*/ size_t szOrigStmt = szReq - 5;
  std::string stmt(pStmt,szOrigStmt) ;
  int cmd = 0;
  tree_serializer ts ;

  /* sql -> tree */
  root = !tree?ts.build_tree(stmt):tree;
  //log_print("stmt_test: %s(%zu,%zu)\n",stmt.c_str(),stmt.size(),szOrigStmt);

  for (;i<total_hModules;i++) {
    pb = const_cast<hook_object*>(g_hModuleList[i]) ;
    if (!pb || pb->hook_type!=h_sql) {
      continue ;
    }
    rc  = pb->run_hook(ts,root,param);
    ret = rc>=0?(ret|rc):ret;
  }

  /* ret=1 if the tree is modified */
  if (ret) {
    std::string outStmt ;
#if 0
    ts.print_tree(root,0);
    printf("\n");
#endif
    //log_print("the inputed statement is changing\n");
    /* do serialize tree here */
    ts.do_serialize(root,outStmt);

    /* get command code */
    cmd = mysqls_get_cmd(*req) ;

    szReq = 5+outStmt.size();
    /* new statement is bigger than orign one, so recreate 
     *  a new request */
    if (outStmt.size()>stmt.size()) {
      *req = m_pNewReq = (char*)malloc(szReq+10);
      pStmt= *req+5 ;
      mysqls_update_sn(*req,0);
    }

    mysqls_set_cmd(*req,cmd) ;

    memcpy(pStmt,outStmt.c_str(),outStmt.size());
    *(*req + szReq) = '\0';

    mysqls_update_req_size(*req,outStmt.size()+1);
  }

  /* release if the tree is created in crrent scope */
  if (!tree) 
    ts.destroy_tree(root);

  return 0;
}

int 
hook_framework::run(char **pkt, size_t &szPkt, void *param, 
  int htype, stxNode *tree)
{
  switch (htype) {
    case h_sql: return do_sql_hooks(pkt,szPkt,param,tree);

    case h_req: 
    case h_res: return do_normal_hooks(pkt,szPkt,param,htype);

    default: break ;
  } 

  log_print("unknown hook type %d\n",htype);
  return -1;
}

void hook_framework::clear(void)
{
  if (m_pNewReq) {
    free(m_pNewReq);
    m_pNewReq = 0;
  }
}

