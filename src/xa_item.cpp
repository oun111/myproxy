
#include "xa_item.h"
#include "myproxy_backend.h"
#include "myproxy_trx.h"
#include "env.h"

#include "simple_types.h"
#include "dbug.h"

using namespace GLOBAL_ENV;

xa_item::xa_item(int xid) : xaid(xid),m_cfd(0),
  m_cmd(0),m_parent(NULL),m_dnGid(0),m_st(0),
  m_desireResDn(0),m_totalOk(0),m_resDn(0),
  m_lastDnFd(0),m_lastSn(0),m_numCols(0),
  sn_count(0),m_cache(0)
{
  //priv.parent = this ;
  
#if 1
  /* init the tx buffer */
  m_txBuff.tc_init();
  m_txBuff.tc_resize(MAX_PAYLOAD*10);
#endif
}

xa_item::~xa_item(void) 
{
  m_txBuff.tc_close();
}

int xa_item::set_xa_info(int fd, int gid, void *parent)
{
  m_cfd   = fd ;
  m_dnGid = gid ;
  m_parent= parent ;
  return 0;
}

int xa_item::set_cmd(int cmd)
{
  //m_cmd = cmd;
  __sync_lock_test_and_set(&m_cmd,cmd);
  return 0;
}

void xa_item::reset(sock_toolkit *st, int cmd, int total)
{
  /* set sock info */
  set_sock(st);
  /* set the requested command id */
  set_cmd(cmd);
  /* set count of datanodes that should 
   *  return responses */
  set_desire_dn(total);
  /* reset ok reponse count */
  reset_ok_count();
  /* reset reponse data node count */
  reset_res_dn();
  /* reset last datanode */
  set_last_dn(-1);
  /* the column & placeholder count */
  set_col_count(0);
  set_phs_count(0);
  /* reset last sn */
  set_last_sn(0);
  /* reset sn counter */
  reset_sn_count();
  /* reset relation vector */
  m_stVec.clear();
  /* reset the column defs */
  m_cols.clear();
}

void xa_item::set_desire_dn(int dn)
{
  //m_desireResDn = dn ;
  __sync_lock_test_and_set(&m_desireResDn,dn);
}

int xa_item::get_desire_dn(void) const
{
  return m_desireResDn ;
}

void xa_item::reset_ok_count(void)
{
  __sync_lock_test_and_set(&m_totalOk,0);
}

int xa_item::inc_ok_count(void)
{
  return __sync_add_and_fetch(&m_totalOk,1);
}

int xa_item::get_ok_count(void) const
{
  return m_totalOk;
}

void xa_item::reset_res_dn(void)
{
  __sync_lock_test_and_set(&m_resDn,0);
}

int xa_item::inc_res_dn(void)
{
  return __sync_add_and_fetch(&m_resDn,1);
}

int xa_item::get_res_dn(void) const
{
  return m_resDn;
}

void xa_item::set_last_dn(int fd)
{
  //m_lastDnFd = fd ;
  __sync_lock_test_and_set(&m_lastDnFd,fd);
}

int xa_item::get_last_dn(void) const
{
  return m_lastDnFd ;
}

void xa_item::set_phs_count(int n)
{
  __sync_lock_test_and_set(&m_phs,n);
}

int xa_item::get_phs_count(void) const
{
  return m_phs ;
}

void xa_item::set_col_count(int n)
{
  __sync_lock_test_and_set(&m_numCols,n);
}

int xa_item::get_col_count(void) const
{
  return m_numCols ;
}

void xa_item::set_last_sn(int sn)
{
  //m_lastSn = sn;
  __sync_lock_test_and_set(&m_lastSn,sn);
}

uint8_t xa_item::inc_last_sn(void)
{
  return __sync_add_and_fetch(&m_lastSn,1);
}

int xa_item::get_last_sn(void) const
{
  return m_lastSn ;
}

void xa_item::set_sock(sock_toolkit *st)
{
  m_st = st ;
  //__sync_lock_test_and_set(&m_st,st);
}

sock_toolkit* xa_item::get_sock(void) const
{
  return m_st;
}

int xa_item::get_xid(void) const
{
  return xaid ;
}

void xa_item::reset_xid(void) 
{
  xaid =-1 ;
}

int xa_item::get_cmd(void) const
{
  return m_cmd ;
}

int xa_item::get_client_fd(void) const
{
  return m_cfd ;
}


int xa_item::get_gid(void) const
{
  return m_dnGid ;
}

int xa_item::lock_sn_count(void)
{
  return __sync_val_compare_and_swap(&sn_count_lock,0,1);
}

void xa_item::unlock_sn_count(void)
{
  __sync_lock_test_and_set(&sn_count_lock,0);
}

void xa_item::reset_sn_count(void)
{
  unlock_sn_count();
  set_sn_count(1);
}

void xa_item::set_sn_count(int sn)
{
  __sync_lock_test_and_set(&sn_count,sn);
}

int xa_item::get_sn_count(void)
{
  return __sync_fetch_and_add(&sn_count,0);
}

int xa_item::get_sn_count1(void)
{
  return __sync_fetch_and_add(&sn_count,0);
}

int xa_item::inc_sn_count(void)
{
  return __sync_add_and_fetch(&sn_count,1);
}

