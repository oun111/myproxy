
#ifndef __DBMGR_H__
#define __DBMGR_H__

#include <memory>
#include "dbwrapper.h"


typedef struct db_pool_object_t {
  /*b_db*/multimap_db db;
  int lock ;
  int ref ;
  int id ;
} tDbPoolObj;

class dbmgr
{
protected:
  /* the database pool object */
  std::shared_ptr<tDbPoolObj> m_pool ;
  /* database pool size */
  int m_psize ;

public:
  dbmgr(void);
  ~dbmgr(void);

public:
  int initialize(int);

  void release(void);

  /* get suitable database object */
  tDbPoolObj* get_db(void);

  /* return back the database object */
  void return_db(tDbPoolObj*);
} ;

#endif /* __DBMGR_H__*/

