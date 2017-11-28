
#include <string.h>
#include "dbwrapper.h"


#if USE_UNQLITE==1
/* 
 * class unqlite_db
 */
unqlite_db::unqlite_db(char *tbl,uint8_t mode):
  m_db(0), m_csr(0), m_mode(mode)
{
  //init(tbl,mode);
  strcpy(m_tbl,tbl);
}

unqlite_db::unqlite_db(void):
  m_db(0), m_csr(0), m_mode(0)
{
  m_tbl[0] = '\0';
}

unqlite_db::~unqlite_db(void)
{
  close();
}

int unqlitedb_cmp(const void  *pUserKey, const void *pRecordKey,
  unsigned int nLen)
{
  printf("k1: %d, k2: %d\n", *(int*)pUserKey,*(int*)pRecordKey);
  return 0;
}

/* init the db operations */
int unqlite_db::init(
  char *tbl,
  uint8_t sort,
  uint8_t mode /* 0: memory, 1: disk */
  )
{
  int ret = 0;
  
  /* update parameters */
  if (tbl) strcpy(m_tbl, tbl) ;
  if (mode!=__INVALID_VALUE__) m_mode = mode;
  /* open the db object */
  ret = unqlite_open(&m_db,m_mode==0?":mem:":m_tbl,
    UNQLITE_OPEN_CREATE);
  if (ret!=UNQLITE_OK) {
    printf("%s: fail: %d\n", __func__, ret);
    return -1;
  }
#if 0
  /* register compare function */
  ret = unqlite_kv_config(m_db,UNQLITE_KV_CONFIG_CMP_FUNC,
    unqlitedb_cmp);
  if (ret!=UNQLITE_OK) {
    printf("%s: register compare function fail: %d\n", __func__, ret);
    return -1;
  }
#endif
  return 0;
}

void unqlite_db::close(void)
{
  int ret = 0;

  /* close cursor */
  csr_close();
  /* close db object */
  if (m_db &&((ret=unqlite_close(m_db))!=UNQLITE_OK)) {
    printf("%s: fail: %d\n", __func__,ret);
  }
  m_db = 0;
  m_csr= 0;
}

int unqlite_db::insert(void *key, long long  klen, void *data, long long  dlen)
{
  int ret = unqlite_kv_store(m_db,key,klen,data,dlen), rc=0;
  char *err = 0;

  if (ret!=UNQLITE_OK) {
    unqlite_config(m_db,UNQLITE_CONFIG_ERR_LOG,&err,&rc);
    printf("%s: fail: %d, err: %s\n", __func__,ret,err);
    return -1;
  }
  return 0;
}

int unqlite_db::drop(void *key, long long  klen)
{
  int ret = unqlite_kv_delete(m_db,key,klen);

  if (ret!=UNQLITE_OK) {
    printf("%s: fail: %d\n", __func__,ret);
    return -1;
  }
  return 0;
}

int unqlite_db::fetch(void *key, long long  klen, void *data, long long  &dlen)
{
  int ret = unqlite_kv_fetch(m_db,key,klen,data,&dlen);

  if (ret!=UNQLITE_OK) {
    printf("%s: fail: %d\n", __func__,ret);
    return -1;
  }
  return 0;
}
/* init the cursor object */
int unqlite_db::csr_init(void) 
{
  int ret = unqlite_kv_cursor_init(m_db,&m_csr);

  if (ret!=UNQLITE_OK) {
    printf("%s: fail: %d\n", __func__,ret);
    return -1;
  }
  csrFirst = 1 ;
  return 0;
}

/* release the cursor object */
void unqlite_db::csr_close(void) 
{
  if (m_csr) {
    unqlite_kv_cursor_release(m_db,m_csr);
  }
}

int unqlite_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  int ret = 0;

  /* test fo key/value size */
  if (!dlen) {
    unqlite_kv_cursor_key(m_csr,0,&klen);
    unqlite_kv_cursor_data(m_csr,0,&dlen);
    return 0;
  }
  if (__sync_val_compare_and_swap(&csrFirst,1,0)==1) {
    /* get cursor to the initial position */
    ret = asc?unqlite_kv_cursor_first_entry(m_csr):
      unqlite_kv_cursor_last_entry(m_csr);
  } else {
    /* cursor next */
    ret = asc?unqlite_kv_cursor_next_entry(m_csr):
      unqlite_kv_cursor_prev_entry(m_csr);
  }
  if (ret!=UNQLITE_OK) {
    printf("%s: fail: %d\n", __func__,ret);
    return -1;
  }
  /* test last record */
  if (!unqlite_kv_cursor_valid_entry(m_csr)) {
    return 1;
  }
  if (k) unqlite_kv_cursor_key(m_csr,k,&klen);
  unqlite_kv_cursor_data(m_csr,d,&dlen);
  //printf("data: %s, len: %lld\n",(char*)d,dlen);
  return 0;
}

/* get next record by cursor */
int unqlite_db::csr_next(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen);
}

/* get previous record by cursor */
int unqlite_db::csr_prev(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen,false);
}

int unqlite_db::row_count(void)
{
  return 0;
}

void unqlite_db::do_truncate(void)
{
}

#endif /* USE_UNQLITE==1 */

#if USE_BDB==1
/* 
 * class b_db
 */
b_db::b_db(char *tbl,uint8_t sort,uint8_t mode):
  m_env(0), 
  m_db(0), 
  m_csr(0),
  m_sort(sort),
  m_mode(mode)
{
  //init(tbl,sort,mode);
  strcpy(m_tbl,tbl);
}

b_db::b_db(void):m_env(0),m_db(0),m_csr(0),
  m_sort(0),
  m_mode(0)
{
  m_tbl[0] = '\0';
  pthread_mutex_init(&m_lock,0);
}

b_db::~b_db(void)
{
  close();
  pthread_mutex_destroy(&m_lock);
}

int
compare_int_desc(DB *dbp, const DBT *a, const DBT *b, size_t *locp)
{
  long ai, bi;

  /* Suppress "warning: <var> not used" messages in many compilers. */
  dbp = NULL;
  locp = NULL;

  /*
   * Returns:
   * < 0 if a < b
   * = 0 if a = b
   * > 0 if a > b
   */
  memcpy(&ai, a->data, sizeof(long));
  memcpy(&bi, b->data, sizeof(long));
  //return ai>bi?1:ai<bi?-1:0; 
  return ai>bi?-1:ai<bi?1:0; 
}

int
compare_int_asc(DB *dbp, const DBT *a, const DBT *b, size_t *locp)
{
  long ai, bi;

  /* Suppress "warning: <var> not used" messages in many compilers. */
  dbp = NULL;
  locp = NULL;

  /*
   * Returns:
   * < 0 if a < b
   * = 0 if a = b
   * > 0 if a > b
   */
  memcpy(&ai, a->data, sizeof(long));
  memcpy(&bi, b->data, sizeof(long));
  //return ai>bi?1:ai<bi?-1:0; 
  return ai>bi?1:ai<bi?-1:0; 
}

/* init the db operations */
int b_db::init(
  char *tbl,
  uint8_t sort,
  uint8_t mode /* 0: memory, 1: disk */
)
{
  int ret = 0;
  size_t szPg = 65536 ;

  /* update params */
  if (tbl) strcpy(m_tbl, tbl) ;
  if (sort!=__INVALID_VALUE__) m_sort = sort ;
  if (mode!=__INVALID_VALUE__) m_mode = mode;
#if 0
  /* create environment */
  ret = db_env_create(&m_env,0);
  if (ret) {
    m_env->err(m_env, ret, "env create");
    return -1;
  }
#if 0
  ret = m_env->set_cachesize(m_env, 0, 1000000, 1);
  if (ret) {
    m_env->err(m_env, ret, "env cache set");
    return -1;
  }
#endif
  m_env->set_errfile(m_env,stderr);
  /* open the environment */
  ret = m_env->open(m_env,0,DB_CREATE | DB_INIT_MPOOL | DB_THREAD,0);
  if (ret) {
    m_env->err(m_env, ret, "env open");
    m_env->close(m_env,0);
    return -1;
  }
#endif
  /* create the database */
  ret = db_create(&m_db,m_env,0);
  if (ret) {
    m_db->err(m_db, ret, "db create");
    return -1;
  }
  /* error output */
  //m_db->set_errfile(m_db, stderr);
  m_db->set_errpfx(m_db, m_tbl);
#if 1
  /* set compare function */
  ret = m_db->set_bt_compare(m_db,
    m_sort==0?compare_int_asc:compare_int_desc);
  //ret = m_db->set_dup_compare(m_db,compare_int_asc);
  if (ret) {
    m_db->err(m_db, ret, "set compare");
    return -1;
  }
#endif
  /* page size of db */
  ret = m_db->set_pagesize(m_db,szPg);
  if (ret) {
    m_db->err(m_db, ret, "set pagesize to %d", szPg);
    return -1;
  }
  /* duplicate keys */
  ret = m_db->set_flags(m_db, DB_DUP/*|DB_DUPSORT*/) ;
  if (ret) {
    m_db->err(m_db, ret, "set flags");
    return -1;
  }
  /* open the database */
  ret = m_db->open(m_db,0,m_tbl,0,DB_BTREE,DB_CREATE|DB_TRUNCATE| DB_THREAD,0664);
  if (ret) {
    m_db->err(m_db, ret, "open");
    return -1;
  }
  return 0;
}

void b_db::do_truncate(void)
{
  /* close cursor before truncation */
  csr_close();
  /* do truncate */
  //pthread_mutex_lock(&m_lock);
  m_db->truncate(m_db,0,0,0);
  //pthread_mutex_unlock(&m_lock);
}

void b_db::close(void)
{
  int rc = 0;

  //csr_close();
  /* close database */
  if (m_db&&(rc=m_db->close(m_db,0)))  {
    m_db->err(m_db, rc, "db close");
    return ;
  }
#if 0
  /* drop database file */
  if (m_mode==0&&m_tbl[0]!='\0') {
    m_db->remove(m_db,m_tbl,0,0);
  }
#endif
  /* close environment */
  if (m_env&& (rc=m_env->close(m_env,0))) {
    m_env->err(m_env, rc, "env close");
    return ;
  }
#if 1
  /* drop database file */
  if (m_mode==0&&m_tbl[0]!='\0') {
    //remove(m_tbl);
  }
#endif
#if 1
  m_db = 0;
  m_env= 0;
  m_csr= 0;
  m_tbl[0] = '\0';
#endif
}

int b_db::insert(void *k, long long  klen, void *d, long long  dlen)
{
  int ret = 0;
  DBT key, data ;

  bzero(&key,sizeof(key));
  bzero(&data,sizeof(data));
  /* get key */
  key.data = k;
  key.size = klen ;
  /* get data */
  data.data = d;
  data.size = dlen ;
  /* write the key/data */
  //pthread_mutex_lock(&m_lock);
  ret = m_db->put(m_db,NULL,&key,&data,/*DB_NOOVERWRITE*/0);
  //pthread_mutex_unlock(&m_lock);
  if (ret) {
    m_db->err(m_db, ret, "insert");
    return -1;
  }
  return 0;
}

int b_db::drop(void *k, long long  klen)
{
  int ret = 0;
  DBT key ;

  bzero(&key,sizeof(key));
  key.data = k;
  key.size = klen ;
  ret = m_db->del(m_db,NULL,&key,0);
  if (ret) {
    m_db->err(m_db, ret, "drop");
    return -1;
  }
  return 0;
}

int b_db::fetch(void *k, long long  klen, void *d, long long  &dlen)
{
  int ret = 0;
  DBT key, data ;

  bzero(&key,sizeof(key));
  bzero(&data,sizeof(data));
  key.data  = k;
  key.size  = klen ;
  //key.flags  = DB_DBT_MALLOC ;
  data.data = d;
  data.ulen = dlen ;
  data.flags= DB_DBT_USERMEM ;
  /* get data by key */
  ret = m_db->get(m_db,NULL,&key,&data,0);
  if (ret) {
    m_db->err(m_db, ret, "fetch");
    return -1;
  }
  dlen = data.size ;
  return 0;
}

/* init the cursor object */
int b_db::csr_init(void) 
{
  int ret = m_db->cursor(m_db,NULL,&m_csr,0);

  if (ret) {
    m_db->err(m_db, ret, "cursor init");
    return -1;
  }
  csrFirst = 1;
  return 0;
}

/* release the cursor object */
void b_db::csr_close(void) 
{
  if (m_csr) {
    m_csr->close(m_csr);
    m_csr = 0;
  }
}

#if 0
int b_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  int ret = 0;
  DBT key, data;

  bzero(&key,sizeof(key));
  bzero(&data,sizeof(data));

  /* test for key/data size */
  if (!d) {
    m_csr->get(m_csr,&key,&data,DB_CURRENT);
    klen = key.size ;
    dlen = data.size;
    return 0;
  }
  /* cursor next */
  ret = asc?m_csr->get(m_csr,&key,&data,DB_NEXT):
    m_csr->get(m_csr,&key,&data,DB_PREV);
  /* mostly means record set ends */
  if (ret) {
    //m_db->err(m_db, ret, "cursor move");
    return 1;
  }
  /* fetch key */
  if (k) {
    memcpy(k,key.data,key.size);
    klen = key.size;
  }
  /* fetch data */
  memcpy(d,data.data,data.size);
  dlen = data.size;
  //printf("data: %s\n",(char*)data.data);
  return 0;
}
#else
int b_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  int ret = 0;
  DBT key, data;

  bzero(&key,sizeof(key));
  bzero(&data,sizeof(data));

  if (__sync_val_compare_and_swap(&csrFirst,1,0)==1) {
    m_csr->get(m_csr,&key,&data,asc?DB_FIRST:DB_LAST);
  }
  /* get a row at current position */
  m_csr->get(m_csr,&key,&data,DB_CURRENT);
  /* test for key/data size */
  if (!d) {
    klen = key.size ;
    dlen = data.size;
    return 0;
  }
  /* fetch key */
  if (k) {
    memcpy(k,key.data,key.size);
    klen = key.size;
  }
  /* fetch data */
  memcpy(d,data.data,data.size);
  dlen = data.size;
  //printf("data: %s\n",(char*)data.data);
  /* cursor next */
  ret = m_csr->get(m_csr,&key,&data,asc?DB_NEXT:DB_PREV);
  /* mostly means record set ends */
  if (ret) {
    //m_db->err(m_db, ret, "cursor move");
    return 1;
  }
  return 0;
}
#endif

/* get next record by cursor */
int b_db::csr_next(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen);
}

/* get previous record by cursor */
int b_db::csr_prev(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen,false);
}

int b_db::row_count(void)
{
  return 0;
}
#endif /* USE_BDB==1 */

#if USE_SQLITE==1
/* 
 * class sqlite_db
 */
sqlite_db::sqlite_db(char *tbl,uint8_t sort,uint8_t mode):
  m_btree(0),
  m_csr(0),
  csrFirst(1),
  m_sort(sort),
  m_mode(mode),
  m_rowid(0),
  m_tmpbuf(0)
{
  //init(tbl,sort,mode);
  strcpy(m_tbl,tbl);
}

sqlite_db::sqlite_db(void):m_btree(0),m_csr(0),
  csrFirst(1),
  m_sort(0),
  m_mode(0),
  m_rowid(0),
  m_tmpbuf(0)
{
  m_tbl[0] = '\0';
}

sqlite_db::~sqlite_db(void)
{
  close();
}

int sqlitedb_compare(void *arg, int nkey1,
  const void *pkey1,int nkey2, const void *pkey2)
{
  printf("key1 size: %d, key2 size: %d\n",nkey1,nkey2);
  return 1;
}

/* init the db operations */
int sqlite_db::init(
  char *tbl,
  uint8_t sort,
  uint8_t mode /* 0: memory, 1: disk */
)
{
  int rc = 0;

  /* update params */
  if (tbl) {
    strcpy(m_tbl, tbl) ;
    m_tbl[strlen(m_tbl)+1] = 0;
  }
  if (sort!=__INVALID_VALUE__) m_sort = sort ;
  if (mode!=__INVALID_VALUE__) m_mode = mode;

#if 1
  /* init */
  m_db.pVfs = sqlite3_vfs_find(0);
  m_db.mutex= sqlite3MutexAlloc(SQLITE_MUTEX_RECURSIVE);
  sqlite3_mutex_enter(m_db.mutex);

  rc = sqlite3BtreeOpen(m_db.pVfs, m_tbl, &m_db, &m_btree, 0,  
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MAIN_DB /*| SQLITE_OPEN_URI |SQLITE_OPEN_TRANSIENT_DB*/);
  if (rc!=SQLITE_OK) {
    printf("error init btree: %d\n", rc);
    return -1;
  }
#else
  sqlite3_open(m_tbl,&m_db);
  m_btree = m_db->aDb[0].pBt ;
  sqlite3_mutex_enter(m_db->mutex);
#endif

  /* set cache */
  sqlite3BtreeSetCacheSize(m_btree, 102400);

  /* open transaction */
  sqlite3BtreeEnter(m_btree);
  rc = sqlite3BtreeBeginTrans(m_btree, 2);
  sqlite3BtreeLeave(m_btree);
  if (rc!=SQLITE_OK) {
    printf("error init transaction: %d\n", rc);
    return -1;
  }
  m_btree->sharable = 0;

  /* 
   * FIXME: is this collation callback invoked by 
   *  the VM layer only ?  it dosen't work here
   *
   */
  /* register comparision function */
  rc = sqlite3_create_collation(&m_db,"sqlitedb",SQLITE_UTF8,
    NULL,sqlitedb_compare);

  /* init cursor */
  if (csr_init())
    return -1;
  
  printf("init db ok\n");

  return 0;
}

void sqlite_db::close(void)
{
  csr_close();
  /* FIXME!!!: release db mutex here */
  //sqlite3_mutex_leave(m_db->mutex);
  //sqlite3_mutex_free(m_db->mutex);
  /* close database */
  sqlite3_close(&m_db);

  m_rowid = 0;
  m_btree = 0;
  m_csr   = 0;
  m_tbl[0] = '\0';
  if (m_tmpbuf) {
    free(m_tmpbuf);
    m_tmpbuf = 0;
  }
}

int sqlite_db::insert(void *k, long long  klen, void *d, long long  dlen)
{
  long long total = klen + dlen + 
    sizeof(uint32_t) + sizeof(long long);
  int rc = 0;
  BtreePayload x;
  char *end = 0;

  /*
   * data structure:
   *
   *  key length: 4 bytes
   *
   *  key data:  $(key-length) bytes
   *
   *  value length: 8 bytes
   *
   *  value:  $(value-length) bytes
   */

  bzero(&x,sizeof(x));
  /* k/v insert */
  /* prepare the 'data' buffer */
  m_tmpbuf = (char*)realloc(m_tmpbuf,total);
  end = m_tmpbuf ;
  /* key part */
  *(uint32_t*)end = klen ;
  end += sizeof(uint32_t);
  memcpy(end,k,klen);
  end += klen ;
  /* value part */
  *(long long*)end= dlen ;
  end += sizeof(long long) ;
  memcpy(end,d,dlen);

  /* fill in payload struct */
  __sync_fetch_and_add(&m_rowid,1);
  x.nKey = m_rowid;
  x.pKey = 0;
  x.pData= m_tmpbuf;
  x.nData= total;
  sqlite3_mutex_enter(m_csr->pBt->mutex);
  rc = sqlite3BtreeInsert(m_csr,&x,0,0);
  sqlite3_mutex_leave(m_csr->pBt->mutex);
  if (rc!=SQLITE_OK) {
    printf("error insert data: %d\n", rc);
    return -1;
  }
  return 0;
}

int sqlite_db::drop(void *k, long long  klen)
{
  return 0;
}

int sqlite_db::fetch(void *k, long long  klen, void *d, long long  &dlen)
{
  return 0;
}

/* init the cursor object */
int sqlite_db::csr_init(void) 
{
  int rc = 0;

  csrFirst = 1;
  if (m_csr) {
    return 0;
  }
  m_csr = (BtCursor *)malloc(sqlite3BtreeCursorSize());
  bzero(m_csr,sqlite3BtreeCursorSize());
  /* cursor init */
  rc = sqlite3BtreeCursor(m_btree, MASTER_ROOT, BTREE_WRCSR, 0, m_csr);
  if (rc!=SQLITE_OK) {
    printf("error init cursor: %d\n", rc);
    return -1;
  }
  return 0;
}

/* release the cursor object */
void sqlite_db::csr_close(void) 
{
  int rc = 0;

  if (m_csr) {
    sqlite3_mutex_enter(m_csr->pBt->mutex);
    rc = sqlite3BtreeCloseCursor(m_csr);
    sqlite3_mutex_leave(m_csr->pBt->mutex);
    if (rc!=SQLITE_OK) {
      printf("error close cursor\n");
    }
    free(m_csr);
    m_csr = 0;
  }
}

int sqlite_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  int rc = 0, res = 0;
  char *end = 0;
  uint32_t ln = 0;

  /*
   * data structure:
   *
   *  key length: 4 bytes
   *
   *  key data:  $(key-length) bytes
   *
   *  value length: 8 bytes
   *
   *  value:  $(value-length) bytes
   */

  klen = 0;
  if (__sync_val_compare_and_swap(&csrFirst,1,0)==1) {
    /* get cursor to the initial position */
    sqlite3_mutex_enter(m_csr->pBt->mutex);
    rc = asc?sqlite3BtreeFirst(m_csr,&res):
      sqlite3BtreeLast(m_csr,&res);
    sqlite3_mutex_leave(m_csr->pBt->mutex);
    /* no more data or record ends */
    if (res) 
      return 1;
  } 
  /* test data length */
  if (!d) {
    sqlite3_mutex_enter(m_csr->pBt->mutex);
    ln = sqlite3BtreePayloadSize(m_csr);
    sqlite3_mutex_leave(m_csr->pBt->mutex);
    /* no data */
    if (ln<=0) {
      dlen = klen = 0;
      return 0;
    }
    m_tmpbuf = (char*)realloc(m_tmpbuf,ln);
    /* fetch whole payload */
    sqlite3_mutex_enter(m_csr->pBt->mutex);
    sqlite3BtreePayload(m_csr,0,ln,m_tmpbuf);
    sqlite3_mutex_leave(m_csr->pBt->mutex);
    end= (char*)m_tmpbuf;
    /* the key length */
    klen = *(uint32_t*)end ;
    end += klen+4;
    /* data length */
    dlen = *(long long*)end ;
    return 0;
  }
  /* cursor next */
  if (!csrFirst) {
    sqlite3_mutex_enter(m_csr->pBt->mutex);
    rc = asc?sqlite3BtreeNext(m_csr,&res):
      sqlite3BtreePrevious(m_csr,&res);
    sqlite3_mutex_leave(m_csr->pBt->mutex);
    if (rc) {
      printf("error moving cursor: %d\n", rc);
    }
    /* no more data or record ends */
    if (res) {
      printf("no more data\n");
      return 1;
    }
  }
  sqlite3_mutex_enter(m_csr->pBt->mutex);
  ln = sqlite3BtreePayloadSize(m_csr);
  sqlite3_mutex_leave(m_csr->pBt->mutex);
  /* no data */
  if (ln<=0) {
    return 1;
  }
  m_tmpbuf = (char*)realloc(m_tmpbuf,ln);
  /* fetch whole payload */
  sqlite3_mutex_enter(m_csr->pBt->mutex);
  sqlite3BtreePayload(m_csr,0,ln,m_tmpbuf);
  sqlite3_mutex_leave(m_csr->pBt->mutex);
  /* copy key & data */
  end= (char*)m_tmpbuf;
  /* the key */
  klen = *(uint32_t*)end ;
  end += sizeof(uint32_t);
  memcpy(k,end,klen);
  end += klen;
  /* data */
  dlen = *(long long*)end ;
  end += sizeof(long long);
  memcpy(d,end,dlen);

  return 0;
}

/* get next record by cursor */
int sqlite_db::csr_next(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen);
}

/* get previous record by cursor */
int sqlite_db::csr_prev(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen,false);
}

int sqlite_db::row_count(void)
{
  return 0;
}

void sqlite_db::do_truncate(void)
{
}
#endif

#if USE_LEVELDB==1

/* 
 * class leveldb_db
 */
leveldb_db::leveldb_db(char *tbl,uint8_t sort,uint8_t mode):
  csrFirst(1),
  m_sort(sort),
  m_mode(mode),
  m_csr(0)
{
  //init(tbl,sort,mode);
  strcpy(m_tbl,tbl);
}

leveldb_db::leveldb_db(void):
  csrFirst(1),
  m_sort(0),
  m_mode(0),
  m_csr(0)
{
  m_tbl[0] = '\0';
}

leveldb_db::~leveldb_db(void)
{
  close();
}

#if 0
int sqlitedb_compare(void *arg, int nkey1,
  const void *pkey1,int nkey2, const void *pkey2)
{
  printf("key1 size: %d, key2 size: %d\n",nkey1,nkey2);
  return 1;
}
#endif

/* init the db operations */
int leveldb_db::init(
  char *tbl,
  uint8_t sort,
  uint8_t mode /* 0: memory, 1: disk */
)
{
  leveldb::Options options;
  leveldb::Status status ;

  /* update params */
  if (tbl) strcpy(m_tbl, tbl) ;
  if (sort!=__INVALID_VALUE__) m_sort = sort ;
  if (mode!=__INVALID_VALUE__) m_mode = mode;

  options.create_if_missing = true;

  status = leveldb::DB::Open(options, m_tbl, &m_db);
  if (!status.ok()) {
    printf("init db %s failed\n", m_tbl);
    return -1;
  }
  printf("init db ok\n");
  return 0;
}

void leveldb_db::close(void)
{
  /* close iterator */
  csr_close();

  /* close db object */
  if (m_db) {
    delete m_db ;
    m_db = 0;
  }

  m_tbl[0] = '\0';
}

int leveldb_db::insert(void *k, long long  klen, void *d, long long  dlen)
{
  leveldb::Status st ;
  std::string key(static_cast<char*>(k),klen);
  std::string data(static_cast<char*>(d),dlen);

  st = m_db->Put(leveldb::WriteOptions(),key,data);
  if (!st.ok()) {
    printf("insert failed\n");
    return -1;
  }
  return 0;
}

int leveldb_db::drop(void *k, long long  klen)
{
  leveldb::Status st ;
  std::string key(static_cast<char*>(k),klen);

  st = m_db->Delete(leveldb::WriteOptions(),key);
  if (!st.ok()) {
    printf("delete fail\n");
    return -1;
  }
  return 0;
}

int 
leveldb_db::fetch(void *k, long long  klen, void *d, long long &dlen)
{
  std::string key(static_cast<char*>(k),klen);
  std::string val ;
  leveldb::Status st ;

  st = m_db->Get(leveldb::ReadOptions(), key, &val);
  if (!st.ok()) {
    printf("fetch fail\n");
    return -1;
  }

  dlen = val.length();
  /* get data size only */
  if (!d) {
    return 0;
  }
  memcpy(d,val.data(),dlen);
  return 0;
}

/* init the cursor object */
int leveldb_db::csr_init(void) 
{
  csrFirst = 1;
  m_csr = m_db->NewIterator(leveldb::ReadOptions());
  return m_csr->Valid()?0:-1;
}

/* release the cursor object */
void leveldb_db::csr_close(void) 
{
  if (m_csr) {
    delete m_csr ;
    m_csr = 0;
  }
}

int leveldb_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  if (__sync_val_compare_and_swap(&csrFirst,1,0)==1) {
    if (asc) m_csr->SeekToFirst();
    else m_csr->SeekToLast(); 
  }
  /* to the end */
  if (!m_csr->Valid()) {
    return -1;
  }
  /* fetch key */
  klen = m_csr->key().ToString().length();
  if (k) {
    memcpy(k,m_csr->key().ToString().data(),klen);
  }
  /* fetch data */
  dlen = m_csr->value().ToString().length();
  if (d) {
    memcpy(d,m_csr->value().ToString().data(),dlen);
  } else {
    /* test data length */
    return 0;
  }
  /* move next */
  if (asc) m_csr->Next();
  else m_csr->Prev();
  return 0;
}

/* get next record by cursor */
int leveldb_db::csr_next(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen);
}

/* get previous record by cursor */
int leveldb_db::csr_prev(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen,false);
}

int leveldb_db::row_count(void)
{
  return 0;
}

void leveldb_db::do_truncate(void)
{
}

#endif

#if USE_MULTIMAP==1

/* 
 * class multimap_db
 */
bool mmap_cmp(const std::string &ls, const std::string &rs)
{
  int lv = *(int*)ls.c_str(), rv = *(int*)rs.c_str();
  return lv<rv;
}

multimap_db::multimap_db(char *tbl,uint8_t sort,uint8_t mode):
  csrFirst(1),
  m_sort(sort),
  m_db(mmap_cmp)
{
}

multimap_db::multimap_db(void):
  csrFirst(1),
  m_sort(0),
  m_db(mmap_cmp)
{
  pthread_mutex_init(&lck,0);
}

multimap_db::~multimap_db(void)
{
  close();

  pthread_mutex_destroy(&lck);
}

/* init the db operations */
int multimap_db::init(
  char *tbl,
  uint8_t sort,
  uint8_t mode /* 0: memory, 1: disk */
)
{
  /* update params */
  if (sort!=__INVALID_VALUE__) 
    m_sort = sort ;

  printf("init db ok\n");
  return 0;
}

void multimap_db::close(void)
{
  m_db.clear();
}

int multimap_db::insert(void *k, long long  klen, void *d, long long  dlen)
{
  std::string key(static_cast<char*>(k),klen);
  std::string data(static_cast<char*>(d),dlen);

  pthread_mutex_lock(&lck);
  m_db.emplace(key,data);
  //m_db.insert(std::pair<std::string,std::string>(key,data));
  pthread_mutex_unlock(&lck);

  return 0;
}

int multimap_db::drop(void *k, long long  klen)
{
  std::string key(static_cast<char*>(k),klen);

  pthread_mutex_lock(&lck);
  m_db.erase(key);
  pthread_mutex_unlock(&lck);

  return 0;
}

int 
multimap_db::fetch(void *k, long long  klen, void *d, long long &dlen)
{
  std::string key(static_cast<char*>(k),klen);
  std::string val ;
  itr_type i = m_db.end();

  pthread_mutex_lock(&lck);
  if ((i=m_db.find(key))==m_db.end()) {
    printf("fetch fail\n");
    pthread_mutex_unlock(&lck);
    return -1;
  }

  val = (*i).second;

  dlen = val.length();
  /* get data size only */
  if (!d) {
    pthread_mutex_unlock(&lck);
    return 0;
  }
  memcpy(d,val.data(),dlen);
  pthread_mutex_unlock(&lck);

  return 0;
}

/* init the cursor object */
int multimap_db::csr_init(void) 
{
  int ret = 0;

  pthread_mutex_lock(&lck);
  m_csr = m_db.cbegin();
  ret = m_csr==m_db.end()?-1:0;
  pthread_mutex_unlock(&lck);

  return ret ;
}

/* release the cursor object */
void multimap_db::csr_close(void) 
{
}

int multimap_db::csr_move(void *k, int &klen, void *d, 
  long long  &dlen, bool asc) 
{
  pthread_mutex_lock(&lck);
  if (__sync_val_compare_and_swap(&csrFirst,1,0)==1) {
    if (asc) m_csr = m_db.cbegin();
    else m_csr = m_db.cend(); 
  }
  /* to the end */
  if (m_csr==m_db.end()) {
    pthread_mutex_unlock(&lck);
    return 1;
  }
  /* fetch key */
  klen = (*m_csr).first.length();
  if (k) {
    memcpy(k,(*m_csr).first.data(),klen);
  }
  /* fetch data */
  dlen = (*m_csr).second.length();
  if (d) {
    memcpy(d,(*m_csr).second.data(),dlen);
  } else {
    /* test data length */
    pthread_mutex_unlock(&lck);
    return 0;
  }
  /* move next */
  if (asc) ++m_csr;
  else --m_csr;

  pthread_mutex_unlock(&lck);

  return 0;
}

/* get next record by cursor */
int multimap_db::csr_next(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen);
}

/* get previous record by cursor */
int multimap_db::csr_prev(void *k, int &klen, void *d, long long  &dlen) 
{
  return csr_move(k,klen,d,dlen,false);
}

int multimap_db::row_count(void)
{
  return m_db.size();
}

void multimap_db::do_truncate(void)
{
  pthread_mutex_lock(&lck);
  m_db.clear();
  pthread_mutex_unlock(&lck);
}
#endif

