
#include <string.h>
#include "id_cache.h"
#include "dbg_log.h"

id_cache::id_cache(const char *strCache) : 
  m_cacheDesc(strCache)
{
  char *desc = const_cast<char*>(m_cacheDesc.c_str());

  lock_init();

  if (!m_db.init(desc,0,1)) {
    log_print("id cache %s init ok\n", desc);
  }
}

id_cache::~id_cache(void)
{
  m_db.close();

  lock_release();
}

void id_cache::lock_init(void)
{ 
  pthread_mutex_init(&m_lk,0); 
}

void id_cache::lock_release(void) 
{
  pthread_mutex_destroy(&m_lk);
}

int id_cache::int_fetch_and_add(char *key,int &val)
{
  //char buf[32];
  long long dl = sizeof(val);
  const size_t kl = strlen(key);

  val = 0;

  try_lock();
  /* fetch original record */
  if (m_db.fetch(key,kl,&val,dl)) {

    log_print("no key %s found\n",key);

    if (insert_new_unsafe(key,0)) {
      return -1;
    }
  }
  /* inc value */
  val++ ;
  /* overwrite the key-value */
  if (insert_new_unsafe(key,val)) {
    return -1;
  }
  return 0;
}

int id_cache::int_fetch_and_sub(char *key, int &val)
{
  long long dl = 32;
  const size_t kl = strlen(key);

  val = 0;

  try_lock();
  /* fetch original record */
  if (m_db.fetch(key,kl,&val,dl)) {
    log_print("no key %s found\n",key);
    return -1;
  }
  /* dec value */
  val-- ;
  /* overwrite the key-value */
  if (insert_new_unsafe(key,val)) {
    return -1;
  }
  return 0;
}

int id_cache::insert_new_unsafe(char *key,int val)
{
  const size_t kl = strlen(key);

  /* restore new value */
  if (m_db.insert(key,kl,&val,sizeof(val))) {
    log_print("fail for key %s\n",key);
    return -1;
  }
  return 0;
}

