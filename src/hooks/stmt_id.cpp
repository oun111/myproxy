
#include "stmt_id.h"
#include "dbg_log.h"


stmt_id_hook::stmt_id_hook(void) : hook_object(h_req)
{
}

stmt_id_hook::~stmt_id_hook(void)
{
}

int stmt_id_hook::run_hook(char *req, size_t sz, void *param)
{
  stmtIdHookParam *ptr = (stmtIdHookParam*)param ;
  int pstmtid = 0, dn=0, grp = 0;
  
  if (!ptr) {
    log_print("invalid params\n");
    return -1;
  }
  
  dn = ptr->dn ;
  grp= ptr->grp ;
  pstmtid = ptr->maps->get(grp,dn);

  if (pstmtid<0) {
    log_print("found no mapping by grp %d dn %d\n",grp,dn);
    return -1;
  }
  /* update request by the mapped physical statement id */
  mysqls_update_stmt_prep_stmt_id(req,sz,pstmtid);
  log_print("updating stmtid to %d\n",pstmtid);

  /* update the 'new bound' flag to '1' */
  //mysqls_update_stmt_newbound_flag(req,sz,1);
  return 0;
}


HMODULE_IMPL (
  stmt_id_hook,
  stmt_id
);

