
#include "dbwrapper.h"
#include <time.h>
#include <string.h>
#include <sys/time.h>

int test_one_row(dbwrapper &db)
{
  char buf[512];
  long long len = 512;
  int nkey = 500 ;

  printf("%s: key %d\n",__func__,nkey);
  db.insert(&nkey,sizeof(nkey),(char*)"aaaaaaddd",10);
  if (!db.fetch(&nkey,sizeof(nkey),buf,len)) {
    buf[len] = '\0';
    printf("value: %s, len: %lld\n",buf,len);
  }
  return 0;
}

int test_cursor(dbwrapper &db)
{
  char k[16], d[512];
  int ln = 16, ret=0;
  long long  lln = /*512*/10;

  printf("%s: \n",__func__);
  db.csr_init();
  while (!ret) {
    db.csr_next(0,ln,0,lln);
    ret = db.csr_next(k,ln,d,lln);
    if (ret) 
      break ;
    //k[ln] = '\0';
    //d[lln] = '\0';
    printf("kl: %d, dl: %lld\n",ln,lln);
    printf("%s / %d => %s\n","",*(int*)k,d);
  } 
  return 0;
}

void test_duplicate(dbwrapper &db)
{
  int i=0;
  char/* strNum[16],*/ val[512], testKey[] = "163ad,";

  srand(time(0));
  for (i=0;i<5;i++) {
    sprintf(val,"%d",rand());
    db.insert(&i,sizeof(i),val,sizeof(val));
  }
  /* test */
  i=2;
  sprintf(val,"%d",rand());
  db.insert(&(i),sizeof(i),val,sizeof(val));
  i++;
  sprintf(val,"%d",rand());
  db.insert(&(i),sizeof(i),val,sizeof(val));
  i++ ;
  sprintf(val,"%d",rand());
  db.insert(&(i),sizeof(i),val,sizeof(val));
  i++;
  sprintf(val,"%d",rand());
  db.insert(&(i),sizeof(i),val,sizeof(val));
  for (int i=9;i<20;i++) {
    sprintf(val,"%d",rand());
    db.insert(&i,sizeof(i),val,sizeof(val));
  }
  db.insert(testKey,strlen(testKey)+1,(char*)"aaaaxaaaaaddd",10);
  db.insert(testKey,strlen(testKey)+1,(char*)"aaaaxaaaaaddd",10);
  (void)testKey;
}

void test_batch(dbwrapper &db)
{
  long long  lln = 0;
  uint32_t i=1;
  int klen = 0;
  char val[512], key[128];
  struct timeval tv, tv1;
  uint32_t ndata = 1000000; 

  /* batch insert */
  gettimeofday(&tv,0);
  for (i=1000;i<(ndata+1000);i++) {
    sprintf(val,"%ld",lrand48());
    db.insert(&i,sizeof(i),val,sizeof(val));
  }
  gettimeofday(&tv1,0);
  printf("insert %u rows costs: %lu us\n", 
    ndata, ((tv1.tv_sec-tv.tv_sec)*1000000 + (tv1.tv_usec-tv.tv_usec)));
  /*
   * batch query
   */
  db.csr_init();
  gettimeofday(&tv,0);
  for (ndata=0;!db.csr_next((void*)key,klen,val,lln);ndata++) ; 
  gettimeofday(&tv1,0);
  printf("query %u rows costs: %lu us\n", 
    ndata, ((tv1.tv_sec-tv.tv_sec)*1000000 + (tv1.tv_usec-tv.tv_usec)));
}

void test_db(dbwrapper &db)
{
  test_duplicate(db);
  /* test 1 row fetch */
  test_one_row(db);
  /* test cursor fetch */
  test_cursor(db);
  /* test batch insert and query */
  test_batch(db);
}

int main(void)
{
#if USE_UNQLITE==1
  /* unqlite test */
  {
    /* mode: 0: memory, 1: disk file */
    unqlite_db db;
    
    db.init((char*)"/tmp/db1",0,0) ;
    test_db(db);
    db.close();
  }
#endif

#if USE_BDB==1
  /* bdb test */
  {
    b_db db1;

    db1.init((char*)"/tmp/db2",1,0) ;
    test_db(db1);
    db1.close();
  }
#endif

#if USE_SQLITE==1
  /* bdb test */
  {
    sqlite_db db1;

    db1.init((char*)"/tmp/db3",1,0) ;
    test_db(db1);
    db1.close();
  }
#endif

#if USE_LEVELDB==1
  /* leveldb test */
  {
    leveldb_db db1;

    db1.init((char*)"/tmp/db4",1,0) ;
    test_db(db1);
    db1.close();
  }
#endif

#if USE_MULTIMAP==1
  /* multimap test */
  {
    multimap_db db1;

    db1.init(0,0,0) ;
    test_db(db1);
    db1.close();
  }
#endif
  return 0;
}

