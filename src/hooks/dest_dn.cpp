
#include "dest_dn.h"
#include "dbg_log.h"

using namespace STREE_TYPES;

dest_dn_hook::dest_dn_hook(void) : hook_object(h_sql)
{
}

dest_dn_hook::~dest_dn_hook(void)
{
}

int dest_dn_hook::run_hook(sql_tree &st,stxNode *root,void *params) 
{
  stxNode *nd = 0;
 
  /* not 'show xxx' statements */
  nd = st.find_in_tree(root,mktype(m_list,s_show_lst));
  if (!nd) {
    return 0;
  }

  size_t ln = nd->op_lst.size();
  stxNode *pn0 = nd->op_lst[ln-1];

  /* check if last element in tree is datanode number */
  if (!pn0 || pn0->type!=mktype(m_endp,s_c_int)) {
    return 0;
  }

  /* save dest dn */
  {
    normal_hook_params *prm = (normal_hook_params*)params ;

    /* the tree node of destination datanode will be removed
     *  next, so save the destination number here */
    prm->m_sp.dest_dn = atoi(pn0->name);

    //printf("dest dn : %d\n",prm->m_sp.dest_dn);
  }

  /* drop last element from tree */
  st.detach(pn0,ln-1);

  return 1;
}


HMODULE_IMPL (
  dest_dn_hook,
  dest_dn
);

