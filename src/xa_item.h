
#ifndef __XA_ITEM_H__
#define __XA_ITEM_H__

#include "busi_base.h"
#include "dbwrapper.h"
#include "myproxy_trx.h"

typedef struct db_pool_object_t tDbPoolObj;

class xa_item  {

protected:


  /* the xa id */
  int xaid ;

  /* client fd */
  int m_cfd ;

  /* command code */
  int m_cmd ;

  /* parent, normaly to the executor */
  void *m_parent ;

  /* used data node group id */
  int m_dnGid ;

  sock_toolkit *m_st;

  /* total datanode count to reply responses */
  int m_desireResDn ;

  /* total datanode count that received complete responses */
  int m_totalOk ;

  /* datanode count that response for the request */
  int m_resDn;

  /* the last datanode that responses */
  int m_lastDnFd ;

  uint8_t m_lastSn ;

  /* column count in row set */
  int m_numCols ;
  /* placeholder count */
  int m_phs ;

  /* the packet serial number counter */
  int sn_count ;
  int sn_count_lock ;

public:
  /* the cache object */
  tDbPoolObj *m_cache ;

  /* the per-client data tx cache */
  tContainer m_txBuff ;

  /* relations of st& myfds */
  using STM_PAIR = std::pair<sock_toolkit*,int> ;
  //std::vector<STM_PAIR> m_stVec ;
  safeStMapList m_stVec ;

public:

  /* the cached column definitions base on datanode numbers */
  //tContainer m_cols ;
  safeColDefGroup m_cols ;
  //
  safeRxStateList m_states;

public:

  xa_item(int) ;
  virtual ~xa_item(void) ;

public:
  void reset(sock_toolkit*,int,int);

  int lock_sn_count(void);
  void unlock_sn_count(void);

  void reset_sn_count(void);
  void set_sn_count(int sn);
  int get_sn_count(void);
  int get_sn_count1(void);
  int inc_sn_count(void);

  int set_cmd(int);
  int get_cmd(void) const;

  void set_last_dn(int);
  int get_last_dn(void) const;

  void reset_res_dn(void);
  int inc_res_dn(void);
  int get_res_dn(void) const;

  void reset_ok_count(void);
  int inc_ok_count(void);
  int get_ok_count(void) const;

  void set_desire_dn(int);
  int get_desire_dn(void) const;

  int set_xa_info(int,int,void*);
  int get_gid(void) const;
  int get_xid(void) const;
  void reset_xid(void) ;
  int get_client_fd(void) const;

  void set_col_count(int);
  int get_col_count(void) const;

  void set_phs_count(int);
  int get_phs_count(void) const;

  void set_last_sn(int);
  int get_last_sn(void) const;
  uint8_t inc_last_sn(void);

  void set_sock(sock_toolkit*);
  sock_toolkit* get_sock(void) const;

} ;

#endif /* __XA_ITEM_H__*/

