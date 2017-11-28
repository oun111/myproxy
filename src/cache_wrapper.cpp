
#include "cache_wrapper.h"
#include "xa_item.h"
#include "env.h"
#include "myproxy_backend.h"
#include "hooks/agg_hooks.h"


using namespace GLOBAL_ENV ;
using namespace AGG_HOOK_ENV ;


cache_wrapper::cache_wrapper(myproxy_backend &prt) :
  m_parent(prt)
{
  size_t sz = m_conf.get_cache_pool_size();

  log_print("total cache pools: %zu\n", sz);

  /* init the cache pool */
  m_cachePool.initialize(sz);
}

/*
 * is cache needed ?
 */
bool
cache_wrapper::is_needed(xa_item *xai)
{
  return xai->m_cache ;
}

int 
cache_wrapper::acquire_cache(xa_item *xai)
{
  /* reset the cache */
  xai->m_cache = m_cachePool.get_db();
  if (!xai->m_cache) {
    log_print("acquire fail\n");
    return -1;
  }

  log_print("acquire cache %p\n",xai->m_cache);
  return 0;
}

void cache_wrapper::return_cache(xa_item *xai)
{
  if (xai->m_cache) {
    log_print("return cache %p\n",xai->m_cache);
    m_cachePool.return_db(xai->m_cache);
    xai->m_cache = 0;
  }
}

int 
cache_wrapper::cache_row(xa_item *xai, tContainer &odrCol, char *res, size_t sz)
{
  xai->m_cache->db.insert(odrCol.tc_data(),odrCol.tc_length(),res,sz);
  return 0;
}

int 
cache_wrapper::extract_cached_rows(safeClientStmtInfoList &stmts, 
  xa_item *xai)
{
  tContainer con;
  int kl = 0;
  long long dl = 0L;
  char *buf=0;
  int cfd = xai->get_client_fd();
  dbwrapper *pdb = &xai->m_cache->db ;
  hook_framework hooks ;
  bool bAgg = is_agg_res(cfd,stmts);
  agg_params apara { 
    .stmts=stmts, .xai=xai, 
  };

  /* for those aggregated res sets, send out all 
   *  col defs in buffer before processing aggregations */
  if (bAgg) {
    tx_pending_res(xai,cfd);
    apara.sn = xai->inc_last_sn();
  }

  pdb->csr_init();
  while (1) {
    pdb->csr_next(0,kl,0,dl);
    if (dl<=0)
      break ;

    con.tc_resize(dl);
    buf = con.tc_data();

    /* no more rows */
    if (pdb->csr_next(0,kl,buf,dl)==1)
      break ;

    /* send the delayed responses */
    if (xai->m_txBuff.tc_free()<=static_cast<size_t>(dl)) 
      tx_pending_res(xai,cfd); 

    /* hooking the aggregated res */
    if (bAgg) {
      hooks.run(&buf,(size_t&)dl,(void*)&apara,h_res);
      continue ;
    }

    /* update sn */
    mysqls_update_sn(buf,xai->inc_last_sn());
    /* just buffer normal res */
    xai->m_txBuff.tc_write(buf,dl);
  }

  /* send rest row set */
  tx_pending_res(xai,cfd); 

  /* release cache */
  return_cache(xai);
  /* release buffer */
  return 0;
}

int 
cache_wrapper::tx_pending_res(xa_item *xai, int cfd)
{
  char *data = xai->m_txBuff.tc_data();
  size_t len  = xai->m_txBuff.tc_length();

  //log_print("try tx pending %zu bytes\n",len);
  if (len>0) {
    m_parent.get_trx().tx(cfd,data,len);
    xai->m_txBuff.tc_update(0);
  }
  return 0;
}

int 
cache_wrapper::do_cache_res(xa_item *xai, char *res, size_t sz)
{
  xai->m_txBuff.tc_write(res,sz);
  return 0;
}

size_t cache_wrapper::get_free_size(xa_item *xai)
{
  return xai->m_txBuff.tc_free();
}

