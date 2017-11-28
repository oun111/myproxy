
#include "static_parse.h"
#include "dbg_log.h"


static_parse::static_parse(void) : hook_object(h_sql)
{
}

static_parse::~static_parse(void)
{
}

int static_parse::run_hook(sql_tree &st,stxNode *root,void *params)
{
  /* 
   * XXX: do statically adaptions on the inputing tree, 
   *  but adaptions on placeholders are NOT supported yet
   */
#if 0
  /* do sql syntax checking */
  {
    sql_checker t_chk ;

    if (!t_chk.do_checking(root))
      return -1;
  }

  /* 
   * gather place holder informations from
   *  tree
   */
  p_alist->reset();
  p_alist->reset_counters();
  if (!m_adpt.get_placeholder_info(root,p_alist)) {
    return -1;
  }
  /* create place holder orders */
  p_alist->build_order_map();
#endif

  /* 
   * translate the tree to mysql style 
   */
  m_adpt.mysqlize_tree(root,0);

#if 0
  /* after syntax adaptions,
   *  re-gather place holder informations 
   *  from attribute list, cause the number
   *  and order of place holders may have been
   *  changed
   */
  p_alist->reset_counters();
  if (!m_adpt.get_placeholder_info(root,p_alist)) {
    return -1;
  }
  /* re-create place holder orders */
  p_alist->rebuild_order_map();
  /* adapts the place holders here */
  m_adpt.do_placeholder_adpt(root);
#endif

  return 1;
}

HMODULE_IMPL (
  static_parse,
  s_parse
);

