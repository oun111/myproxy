
#include "xa_item.h"
#include "myproxy_backend.h"
#include "myproxy_trx.h"
#include "env.h"

#include "simple_types.h"
#include "dbug.h"

using namespace GLOBAL_ENV;

xa_item::xa_item(int xid) : xaid(xid),m_cfd(0),
  m_cmd(0),m_parent(NULL),m_dnGid(0),m_st(0),
  m_desireResDn(0),m_totalOk(0),m_1stRes(0),
  m_lastDnFd(0),m_lastSn(0),m_numCols(0),
  m_cache(0)
{

}

xa_item::~xa_item(void) 
{
  m_txBuff.tc_close();

  m_err.tc_close();
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
  reset_1st_res_count();
  /* reset last datanode */
  set_last_dn(-1);
  /* the column & placeholder count */
  set_col_count(0);
  set_phs_count(0);
  /* reset last sn */
  set_last_sn(0);
  /* reset relation vector */
  m_stVec.clear();
  /* reset the column defs */
  m_cols.clear();

}


void xa_item::set_desire_dn(int dn)
{
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

void xa_item::reset_1st_res_count(void)
{
  __sync_lock_test_and_set(&m_1stRes,0);
}

int xa_item::inc_1st_res_count(void)
{
  return __sync_add_and_fetch(&m_1stRes,1);
}

int xa_item::get_1st_res_count(void) const
{
  return m_1stRes;
}

void xa_item::set_last_dn(int fd)
{
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

int xa_item::get_xid(void) 
{
  return __sync_fetch_and_add(&xaid,0) ;
}

void xa_item::reset_xid(void) 
{
  __sync_lock_test_and_set(&xaid,-1) ;
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

void xa_item::dump(void)
{
  log_print("xaid: %d, cfd: %d, cmd: %d, parent: %p,"
    "gid: %d, st: %d, desire dn: %d, total ok: %d, res dn: %d,"
    "last dn fd: %d, last sn: %d, num cols: %d, phs: %d\n",
    xaid, m_cfd, m_cmd, m_parent, m_dnGid, m_st->m_efd,
    m_desireResDn,m_totalOk,m_1stRes,m_lastDnFd,m_lastSn,
    m_numCols,m_phs);
}

