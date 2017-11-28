
#ifndef __AGG_HOOKS_H__
#define __AGG_HOOKS_H__

#include "hook.h"


class tContainer ;
class xa_item ;
class safeClientStmtInfoList ;
class agg_params {
public:
  safeClientStmtInfoList &stmts ;
  xa_item *xai ;
  int sn ; /* latest sn */
} ;

/* do aggregation processings */
class agg_hooks : public hook_object {

public:
  agg_hooks(void); 
  ~agg_hooks(void) ;

protected:
  int do_aggregates(tContainer&, agg_params*);

  int do_accumulate(xa_item *xai, int nCol, 
    tContainer & /* one record in res set */
    );

  int get_col_val(xa_item*, int nCol, char *inbuf, size_t szIn,
    uint8_t *outVal, size_t *szOut, size_t *outOffs, int &colType);

  int do_cmp(xa_item *xai, int nCol, 
    tContainer &, /* one record in res set */
    int agt /* aggregation type */
    );

  int fill_new_txt_val(tContainer &finalRes, int valOffs, 
    char newVal[]
    );

public:
  int run_hook(char *res,size_t sz,void *params);
} ;


namespace AGG_HOOK_ENV {

  //"sum", "count", "max"
  enum agg_types {
    agt_sum,
    agt_count,
    agt_max,
    agt_min,
  } ;

  extern bool is_agg_res(int,safeClientStmtInfoList&) ; 

} ;

#endif /* __AGG_HOOKS_H__*/

