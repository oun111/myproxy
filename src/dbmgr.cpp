
#include "dbmgr.h"
#include "dbg_log.h"


dbmgr::dbmgr(void): m_psize(0)
{
}

dbmgr::~dbmgr(void)
{
  release();
}

int dbmgr::initialize(int sz)
{
  int i=0;
  char str_db[PATH_MAX];
  tDbPoolObj *pp = 0;

  m_psize = sz ;
  m_pool = std::shared_ptr<tDbPoolObj>(new tDbPoolObj[m_psize]);
  for (i=0;i<m_psize;i++) {
    pp = &(*m_pool);
    pp[i].id   = i;
    pp[i].ref  = 0;
    sprintf(str_db,"/tmp/mp%d.db",i);
    pp[i].db.init(str_db,0,0);
    //pp[i].db.csr_init();
  }
  return 0;
}

void dbmgr::release(void)
{
  int i=0;

  for (i=0;i<m_psize;i++) {
    (&(*m_pool))[i].db.close();
  }

  m_pool.reset();
}

/* get suitable database object */
tDbPoolObj* dbmgr::get_db(void)
{
  int i=0;
  tDbPoolObj *po = &(*m_pool);
  dbwrapper *pcache = 0 ;


  for (i=0;i<m_psize;i++) {
    if (__sync_val_compare_and_swap(&po[i].ref,0,1)==0) {
      /* found and reset the cache */
      pcache = &po[i].db ;
      pcache->do_truncate();
      pcache->csr_init();
      return &po[i] ;
    }
  }
  log_print("no suitable db!!!\n");
  return NULL;
}

/* return back the database object */
void dbmgr::return_db(tDbPoolObj *po)
{
  __sync_lock_test_and_set(&po->ref,0);
}

