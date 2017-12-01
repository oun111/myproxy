#ifndef __DNMGR_H__
#define __DNMGR_H__

#include <memory>
#include "container_impl.h"

/* data node states */
enum dnStats {
  /* data node is invalid */
  s_invalid,

  /* data node is ok and not used */
  s_free,

  /* data node is used, or there's incompleted
   *  transaction on it */
  s_used,
} ;

/* datanode group management */
class dnGroups {
protected:
  /* data node list */
  std::shared_ptr<safeDataNodeList> m_dnGroup ;

  safeStdVector m_freeGroupIds ;

  /* maximun datanode groups count */
  const uint16_t m_maxDnGroup = 20;
  /* default datanode groups count */
  const uint16_t m_defDnGroup = 2;

  uint16_t m_nDnGroups ;

public:
  dnGroups();
  virtual ~dnGroups(void);

public:

  int create_grp_by_conf(void);

  /* get free items of data node group */
  int get_free_group(safeDataNodeList* &nodes, int &gid);

  /* return back the data node group item */
  int return_group(int gid);

  /* get specified datanode group */
  safeDataNodeList* get_specific_group(int);

  int get_num_groups(void) const;
} ;

class tDNInfo;
class tTblDetails;
class dnmgr : public dnGroups
{
public:
  dnmgr() ;
  virtual ~dnmgr() ;

protected:

  int get_tables_by_conf(void);

  int get_datanodes_by_conf(void);

  int update_dn_conn(void);

  int new_connection(tDNInfo*);

  int refresh_tbl_info(void);

  int update_tbl_struct(tDNInfo*,tTblDetails*);

  int update_tbl_extra_info(tDNInfo*,tTblDetails*);

public:
  int initialize(void);
} ;

#endif /* __DNMGR_H__*/

