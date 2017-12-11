
#ifndef __DBWRAPPER_H__
#define __DBWRAPPER_H__

#if USE_UNQLITE==1
extern "C" {
 #include "unqlite.h"
}
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

constexpr uint8_t __INVALID_VALUE__ = 0xff;

class dbwrapper
{

public:
  //dbwrapper(void):__INVALID_VALUE__(0xff) {}
  virtual ~dbwrapper(void) {}

public:

  /* init the db object */
  virtual int init(char *tbl, uint8_t sort, uint8_t mode) = 0;

  /* close the db object */
  virtual void close(void) = 0;

  virtual int insert(void *key, long long  klen, void *data, long long  dlen) = 0;

  virtual int drop(void *key, long long  klen) = 0;

  virtual int fetch(void *key, long long  klen, void *data, long long  &dlen) = 0;

  /* init the cursor object */
  virtual int csr_init(void) = 0;

  /* release the cursor object */
  virtual void csr_close(void) = 0;

  /* get next record by cursor */
  virtual int csr_next(void *key, int &klen, void *data, long long  &dlen) = 0;

  /* get previous record by cursor */
  virtual int csr_prev(void *key, int &klen, void *data, long long  &dlen) = 0;

  virtual int row_count(void) = 0;

  virtual void do_truncate(void) =0 ;
} ;

#if USE_UNQLITE==1
/*
 * unqlite db wrapper
 */
class unqlite_db : public dbwrapper
{
public:
  /* use unqlite db */
  unqlite *m_db ;
protected:
  /* the unqlite db cursor */
  unqlite_kv_cursor *m_csr;
  /* indicate if the cursor points at the 1st record */
  int csrFirst ;
  /* the table name */
  char m_tbl[PATH_MAX];
  /* database mode */
  uint8_t m_mode ;

public:
  unqlite_db(char*tbl,uint8_t mode) ;
  unqlite_db(void) ;
  ~unqlite_db(void) ;

public:
  /*
   * common db interfaces
   */
  int init(char *tbl=0, uint8_t sort=__INVALID_VALUE__, 
    uint8_t mode=__INVALID_VALUE__) ;

  void close(void);

  int insert(void *key, long long  klen, void *data, long long  dlen) ;

  int drop(void *key, long long  klen) ;

  int fetch(void *key, long long  klen, void *data, long long  &dlen) ;

  /*
   * cursor interfaces
   */
  /* init the cursor object */
  int csr_init(void) ;

  /* release the cursor object */
  void csr_close(void) ;

  /* get records by cursors */
  int csr_move(void *k, int &klen, void *d,long long  &dlen, bool asc=true) ;

  /* get next record by cursor */
  int csr_next(void *key, int &klen, void *data, long long  &dlen) ;

  /* get previous record by cursor */
  int csr_prev(void *key, int &klen, void *data, long long  &dlen) ;

  int row_count(void);

  void do_truncate(void);

protected:
  unqlite* get_db(void){ return m_db; }
} ;
#endif /* USE_UNQLITE==1 */


#if USE_BDB==1
/*
 * berkeley db wrapper
 */
extern "C" {
 #include "db.h"
}
class b_db : public dbwrapper {
/*protected*/public:
  DB_ENV *m_env ;
  DB *m_db ;
  DBC *m_csr;
  /* indicate if the cursor points at the 1st record */
  int csrFirst ;
  /* record sort type, 0: asc, 1: desc */
  int m_sort;
  /* the table name */
  char m_tbl[PATH_MAX];
  /* database mode */
  uint8_t m_mode ;
  /* the output log file */
  //FILE *m_logFd;
  /* the lock */
  pthread_mutex_t m_lock ;

public:
  b_db(char*tbl,uint8_t st, uint8_t mode) ;
  b_db(void) ;
  virtual ~b_db(void) ;

public:
  int init(char *tbl=0, uint8_t sort=__INVALID_VALUE__, 
    uint8_t mode=__INVALID_VALUE__) ;

  void close(void);

  int insert(void *key, long long  klen, void *data, long long  dlen) ;

  int drop(void *key, long long  klen) ;

  int fetch(void *key, long long  klen, void *data, long long  &dlen) ;

  void do_truncate(void);
  /*
   * cursor interfaces
   */
  /* init the cursor object */
  int csr_init(void) ;

  /* release the cursor object */
  void csr_close(void) ;

  /* get records by cursors */
  int csr_move(void *k, int &klen, void *d,long long  &dlen, bool asc=true) ;

  /* get next record by cursor */
  int csr_next(void *key, int &klen, void *data, long long  &dlen) ;

  /* get previous record by cursor */
  int csr_prev(void *key, int &klen, void *data, long long  &dlen) ;

  int row_count(void);
} ;
#endif /* USE_BDB==1 */

#if USE_SQLITE==1
extern "C" {
  #include "sqliteInt.h"
  #include "btreeInt.h"
}

class sqlite_db : public dbwrapper {
protected:

  sqlite3 m_db ;
  Btree *m_btree ;
  BtCursor *m_csr;
  /* indicate if the cursor points at the 1st record */
  int csrFirst ;
  /* record sort type, 0: asc, 1: desc */
  int m_sort;
  /* the table name */
  char m_tbl[PATH_MAX];
  /* database mode */
  uint8_t m_mode ;
  /* the unique id */
  int64_t m_rowid ;
  /* the temporary buffer */
  char *m_tmpbuf ;

public:
  sqlite_db(char*tbl,uint8_t st, uint8_t mode) ;
  sqlite_db(void) ;
  virtual ~sqlite_db(void) ;

public:
  int init(char *tbl=0, uint8_t sort=__INVALID_VALUE__, 
    uint8_t mode=__INVALID_VALUE__) ;

  void close(void);

  int insert(void *key, long long  klen, void *data, long long  dlen) ;

  int drop(void *key, long long  klen) ;

  int fetch(void *key, long long  klen, void *data, long long  &dlen) ;

  void do_truncate(void);
  /*
   * cursor interfaces
   */
  /* init the cursor object */
  int csr_init(void) ;

  /* release the cursor object */
  void csr_close(void) ;

  /* get records by cursors */
  int csr_move(void *k, int &klen, void *d,long long  &dlen, bool asc=true) ;

  /* get next record by cursor */
  int csr_next(void *key, int &klen, void *data, long long  &dlen) ;

  /* get previous record by cursor */
  int csr_prev(void *key, int &klen, void *data, long long  &dlen) ;

  int row_count(void);
} ;

#endif

#if USE_LEVELDB==1

#include "leveldb/db.h"  

class leveldb_db : public dbwrapper {
protected:

  /* indicate if the cursor points at the 1st record */
  int csrFirst ;
  /* record sort type, 0: asc, 1: desc */
  int m_sort;
  /* the table name */
  char m_tbl[PATH_MAX];
  /* database mode */
  uint8_t m_mode ;
  /* the db object */
  leveldb::DB *m_db;
  /* iterator, uses for iterating the records */
  leveldb::Iterator* m_csr;

public:
  leveldb_db(char*tbl,uint8_t st, uint8_t mode) ;
  leveldb_db(void) ;
  virtual ~leveldb_db(void) ;

public:
  int init(char *tbl=0, uint8_t sort=__INVALID_VALUE__, 
    uint8_t mode=__INVALID_VALUE__) ;

  void close(void);

  int insert(void *key, long long  klen, void *data, long long  dlen) ;

  int drop(void *key, long long  klen) ;

  int fetch(void *key, long long  klen, void *data, long long  &dlen) ;

  void do_truncate(void);
  /*
   * cursor interfaces
   */
  /* init the cursor object */
  int csr_init(void) ;

  /* release the cursor object */
  void csr_close(void) ;

  /* get records by cursors */
  int csr_move(void *k, int &klen, void *d,long long  &dlen, bool asc=true) ;

  /* get next record by cursor */
  int csr_next(void *key, int &klen, void *data, long long  &dlen) ;

  /* get previous record by cursor */
  int csr_prev(void *key, int &klen, void *data, long long  &dlen) ;

  int row_count(void);
} ;

#endif

#if USE_MULTIMAP==1

#include <map>
/* the compare function type */
using comp_function_t = bool(*)(const std::string&,const std::string&);

class multimap_db : public dbwrapper {
protected:

  /* indicate if the cursor points at the 1st record */
  int csrFirst ;
  /* record sort type, 0: asc, 1: desc */
  int m_sort;
  /* the db object */
  using container_type = std::multimap<std::string,std::string,comp_function_t>;
  container_type m_db;
  /* iterator, uses for iterating the records */
  using itr_type = container_type::const_iterator ;
  itr_type m_csr ;

  pthread_mutex_t lck ;

public:
  multimap_db(char*tbl,uint8_t st, uint8_t mode) ;
  multimap_db(void) ;
  virtual ~multimap_db(void) ;

public:
  int init(char *tbl=0, uint8_t sort=__INVALID_VALUE__, 
    uint8_t mode=__INVALID_VALUE__) ;

  void close(void);

  int insert(void *key, long long  klen, void *data, long long  dlen) ;

  int drop(void *key, long long  klen) ;

  int fetch(void *key, long long  klen, void *data, long long  &dlen) ;

  void do_truncate(void);
  /*
   * cursor interfaces
   */
  /* init the cursor object */
  int csr_init(void) ;

  /* release the cursor object */
  void csr_close(void) ;

  /* get records by cursors */
  int csr_move(void *k, int &klen, void *d,long long  &dlen, bool asc=true) ;

  /* get next record by cursor */
  int csr_next(void *key, int &klen, void *data, long long  &dlen) ;

  /* get previous record by cursor */
  int csr_prev(void *key, int &klen, void *data, long long  &dlen) ;

  int row_count(void);
} ;

#endif

#endif /* __DBWRAPPER_H__*/

