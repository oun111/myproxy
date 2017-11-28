#ifdef _WIN32
 #include "win/win.h"
#else
 #include <unistd.h>
 #include <sys/time.h>
#endif
#include "zas.h"
#include "connstr.h"
#include "dbug.h"
#include "../src/DAL/dal.h"
#ifdef DBG_LOG
  #include "dbg_log.h"
  DECLARE_DBG_LOG_EXTERN(zas);
#endif


/* do performance testing */
#define PERF_TEST 0

int main(int argc, char *argv[])
{
#if PERF_TEST==1 || defined(_WIN32)
  #define next_str(s) (s)
#else
  #define next_str(s) ({\
   (s+1)%(sizeof(streams)/sizeof(streams[0])) ;\
  })
#endif

  int strs = 0;
#ifdef _WIN32
  char *ptns = (char*)"E:\\work\\zas\\tests\\tnsnames.ora";
#else
  char *ptns = (char*)"/mnt/sda5/zyw/work/zas/tests/tnsnames.ora";
  //char *ptns = (char*)"/home/user1/work/zas/tests/tnsnames.ora";
#endif
  zas_connect cnn(ptns,dal_mysql) ;
  zas_stream streams[] = {
    /*
     * stream method selection: 
     *  true:  prepare
     *  false: query 
     */
#if 0
    zas_stream(cnn,false),
    zas_stream(cnn,false),
    zas_stream(cnn,false),
    zas_stream(cnn,false),
    zas_stream(cnn,false),
    zas_stream(cnn,false),
#else
    zas_stream(cnn,true),
    zas_stream(cnn,true),
    zas_stream(cnn,true),
    zas_stream(cnn,true),
    zas_stream(cnn,true),
    zas_stream(cnn,true),
#endif
  };
  /*
   * login connection string
   */
  char connStr[] =
#ifdef _WIN32
    "root/123@win_test"
#else
    "root/123@my_proxy" 
#endif
    /*"server=localhost;"
    "port=3306;"
    "usr=root;"
    "pwd=123;"
    "unix_socket=/tmp/mysql.sock;"
    "dbname=;"*/
    //"dbname=whui;"
    ;
  /* login to database */
  try{
    if (!cnn.rlogon(connStr))
      return -1;
  } catch(zas_exception &e) {
    printd("exception: statement: %s\n"
      "err msg: %s\n"
      "code: %d\n", e.stm_text, e.msg, e.code);
    return -1;
  }
  /********************************
   *
   * module infomations begins
   *
   ********************************/
  printd("process id: %d\n", getpid());
  printd("Module version: %s\n", __MOD_VER__);
#if 0
  printd("Character set: %s\n", 
    mysql_character_set_name(cnn.myobj_ptr()));
#endif
  printd("Building environment: '%s'\n", 
    "local"
    );
  printd("Running mode: '%s'\n",
#ifdef __LAZY_EXEC__
    "lazy"
#else
    "non-lazy"
#endif
    );
  printd("Running method: '%s'\n",
    streams[0].is_prep_method()?"prepare":
    "query"
    );
  printd("Debug log: %s\n",
#ifdef DBG_LOG
    "enabled"
#else
    "disabled"
#endif
    );
  printd("\n\n");
  /********************************
   *
   * module infomations ends
   *
   ********************************/
  /* phase 1: test oracle tns string parser */
  {
    char connStr[] = "tsdb/abc123@whui" ;
    tns_parser tp ;
    std::string tns_path(ptns);

    printd("<<< test oracle tns parser: \n");
    if (tp.parse_conn_str(connStr,tns_path)) {
      printd("proto: %s\n"
        "usr: %s\n"
        "pwd: %s\n"
        "host: %s\n"
        "port: %s\n"
        "server: %s\n"
        "service: %s\n",
        tp.get_tns(tp.tProto),
        tp.get_tns(tp.tUsr),
        tp.get_tns(tp.tPwd),
        tp.get_tns(tp.tHost),
        tp.get_tns(tp.tPort),
        tp.get_tns(tp.tSrv),
        tp.get_tns(tp.tSvc)
        );
    }
  }
  /* phase 2: test dsn string parser */
  {
    char connStr[] = "server=localhost;"
      "port=3306;"
      "usr=mysql;"
      "pwd=123;"
      "unix_socket=/tmp/mysql.sock;"
      "dbname=test_db;";
    my_dsn_parser dp;

    printd("<<< test zas dsn parser: \n");
    if (dp.parse_conn_str(connStr)) {
      printd("usr: %s\n"
        "pwd: %s\n"
        "host: %s\n"
        "port: %d\n"
        "db: %s\n",
        dp.user(),
        dp.pwd(),
        dp.host(),
        dp.port(),
        dp.db()
        );
    }
  }
  /* phase 3: test zas connecting and streaming processing */
#if PERF_TEST==1
  while (1) {
#endif
  {
    int id = 0;
    double point = 0.0 ;
    long size = 0;
    char name[16] ;

    printd("<<< test zas streaming : \n");
    try {
#if 1
      {
        printd("test 0\n");
        cnn.change_db("test_db");
        streams[strs].open(0, "create table if not exists t1("
          "c1 int not null primary key auto_increment, "
          "c2 varchar(10) null default 0,"
          "index i3(c2),"
          "unique index i4(c1),"
          "c3 bigint default 0,"
          "foreign key i5(c1) references test_tbl(id)"
          //",primary key(c1)"
          ") engine = innodb default charset=utf8");
        streams[strs].debug();
        streams[strs].open(0, "drop table if exists t1,t2,t3 ");
        streams[strs].debug();
        return 0;
      }
#endif
      printd("test 1\n");
      streams[strs].open(0,"select * from test_db.test_tbl "
        "where id>=:f1<unsigned int> and id<:f2<int,in>");
      streams[strs]<<1;
      streams[strs]<<5;
      streams[strs].debug();
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      streams[strs].open(0,"select * from test_db.test_tbl "
        "where id>=:f1<unsigned int> and id<0x10");
      streams[strs]<<1;
      streams[strs].debug();
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      streams[strs]<< (id=2) ;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }

      {
        int i=0;

        printd("test 2\n");
        streams[strs]>>name;
        streams[strs]>>point;
        while (1&& i++<1) {
          streams[strs]>>point;
          streams[strs]>>name;
          streams[strs]<<1;
          streams[strs]>>id;
          streams[strs]>>name;
          streams[strs]>>point;
          streams[strs]>>size;
          streams[strs]>>size;
          streams[strs]>>point;
          streams[strs]>>size;
          streams[strs]<<1;
          streams[strs]<<1;
          streams[strs]>>id ;
          streams[strs]>>name ;
          streams[strs]>>point ;
          streams[strs]>>size ;
          streams[strs]<<1;
          streams[strs]>>name;
          streams[strs]>>name;
          streams[strs]>>point;
          streams[strs]>>size;
          streams[strs]>>name;
          streams[strs]>>size;
          streams[strs]>>name;
          streams[strs]>>name;
          streams[strs]<<2;
          streams[strs]>>name;
          streams[strs]>>size;
          streams[strs]<<2;
          streams[strs]<<2;
          streams[strs]<<2;
          streams[strs]<<2;
          streams[strs]<<2;
          streams[strs]<<2;
          streams[strs]>>name;
          streams[strs]>>name;
          streams[strs]>>point;
          streams[strs]<<8;
          streams[strs]<<2;
          streams[strs]>>name;
          streams[strs]<<2;
          streams[strs]>>size;
          streams[strs]>>size;
          streams[strs]>>name;
          streams[strs]>>name;
          streams[strs]>>name;
        }
      }
      streams[strs]<< (id=2) ;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      /* test next stream */
      strs = next_str(strs) ;

      printd("test 3\n");
      streams[strs].open(0, "insert into test_db.test_tbl(name,price,size) "
        "select name :#<char>,price,size from test_db.test_tbl where name "
        "like :f1<char[10]> on duplicate key update size=1;");
      streams[strs]<< "orage1" ;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      /* test next stream */
      strs = next_str(strs) ;

      streams[strs].open(0,"select * from test_db.test_tbl where name like :f1<char[129]> and id=:f2<int>");
      streams[strs]<< "orage%" ;
      streams[strs]<< 3;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      streams[strs]<< "orage1%" ;
      streams[strs]<< 3;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
    } catch(zas_exception &e) {
      printd("exception: statement: %s\n"
        "err msg: %s\n"
        "code: %d\n", e.stm_text, e.msg, e.code);
      return -1;
    }
  }
  /* phase 4: test zas multi statements execution */
  {
    int id = 0;
    float point = 0.0 ;
    long size = 0;
    char name[16] ;

    try{
      streams[strs].open(0,"select * from test_db.test_tbl where id>:id<long> "
        "or name like :name<char[10]> or size =:size<int>");
      streams[strs]<<(long)3 ;
      streams[strs]<<"orage_" ;
      streams[strs]<<3 ;
      cnn.commit();

      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
    } catch (zas_exception &e) {
      printd("exception: stmt: %s,\nexp: %s,\ncode: %d\n",
        e.stm_text, e.msg, e.code);
    }
  }
  /* phase 5: test zas streaming's types */
  try{
    int id = 0;
    float point = 0.0 ;
    long size = 0;
    char name[16] ;

    printd("<<< test zas streaming's types : \n");

    streams[strs].open(0,"select * from test_db.test_tbl where id>=:f1<int> and price like :f2<double>");
    streams[strs]<< (id=0) << (double)(point=2.12346);
    streams[strs].flush();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }

    streams[strs].open(0,"select * from test_db.test_tbl where id>=: f1 <  int > or name like :f2  < char[ 7 ] > "  "or size >: f3 < bigint    > or price < :f4  <    double > ");
    streams[strs]<< (id=0) << "orage2" << (int64_t)3<< (double)(point=2.12346);
    streams[strs].flush();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    /* test next stream */
    strs = next_str(strs) ;
    streams[strs].open(0,"select * from test_db.test_tbl where id>: f1 < int > ");
    streams[strs]<< (id=3) ;
    streams[strs].flush();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    streams[strs].open(0,"select * from test_db.test_tbl where id> 1 ");
    streams[strs].rewind();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    printd("test 4\n");
    streams[strs].open(0,"select * from test_db.test_tbl where id> nvl(0,1) ");
    streams[strs].rewind();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    {
      int val = 0 ;

      printd("test 6\n");
      streams[strs].open(0,"select * from test_db.test_tbl where "
        "id>=:f1<int> and id<50");
      streams[strs]<< (val=5) ;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>val ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          val, name, point, size);
      }
    }
    {
      printd("test 7\n");
      streams[strs].open(0, "insert into test_db.test_tbl abcd (name,price,size) "
        "select name,price,size from test_db.test_tbl where name like 'ora%';");
      streams[strs].open(0, "insert into test_db.test_tbl abcd (name,price,size) "
        "select name,price,size from test_db.test_tbl where name like 'ora%';");
      streams[strs].open(0,"delete from test_db.test_tbl where id>:f1<long>; ");
      streams[strs]<< (long)(7) ;
      streams[strs].open(0,"alter table test_db.test_tbl auto_increment=7 ");
      /* FIXME: mysql-prepare machanism considers 
       *   this as an syntax error */
      //streams[strs].open(0,"alter table test_db.test_tbl auto_increment= :f1<int>;");
      //streams[strs]<< (7) ;
      streams[strs].flush();
      streams[strs].open(0, "insert into test_db.test_tbl(name,price,size) "
        "values select name :#<char>,price,size from test_db.test_tbl where name "
        "like :f1<char[10]> on duplicate key update size=1;");
    }
    {
      //char buf[128];

      printd("test 8\n");
      streams[strs].open(0, "select sysdate from dual");
      streams[strs].debug();
#if 0
      while (!streams[strs].eof()) {
        streams[strs]>>buf ;
        printd("sysdate: %s\n", buf);
      }
#endif
    }
    {
      streams[strs].open(0,"select * from test_db.test_tbl where id>:id<long> "
        "or name like :name<char[10]> or id=:id<long> or size =:size<int> "
        "or name like :name<char[10]> or size > :size<int> or size > :size<int>");
      streams[strs]<<(long)2;
      streams[strs]<<(char*)"orage%";
      streams[strs]<<(int)10;
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
    }
    {
      printd("test 13\n");
      cnn.change_db("test_db");
      streams[strs].open(0,"select id, name from "
        "test_db.test_tbl where id>=1 and rownum<7 and rownum<8");
      streams[strs].open(0,"select id, rownum, (select name from "
        "test_db.test_tbl where id=1 and rownum<6)a from test_db.test_tbl where "
        "id in (select id from test_db.test_tbl where rownum<7 and id=1)");
      streams[strs].open(0,"select id, (select name from "
        "/*test_db.*/test_tbl where id=1 and rownum<6)a from /*test_db.*/test_tbl where "
        "id in (select id from /*test_db.*/test_tbl where rownum<7 and id=1) "
        "and rownum <9");
    }
    {
      printd("test 14\n");
      streams[strs].open(0,
        "select (trunc(sysdate) - to_date('01-01-1900','dd-mm-yyyy'))*60*60*24 "
        "- 8*60*60 +(to_char(sysdate ,'hh24'))*60*60 +(to_char(sysdate,'mi'))* 60 "
        "+to_char(sysdate,'ss') "
        "from dual "
        "where 1 = :f1<int> ") ;
      streams[strs]<<1;
      streams[strs].open(0, "select to_number('123')from dual") ;
      streams[strs].debug();
      streams[strs].open(0, "select to_number(' 123 ','xxxx')from dual") ;
      streams[strs].debug();
      streams[strs].open(0, "select to_number('123','999')from dual") ;
      streams[strs].debug();
      streams[strs].open(0, "select to_number('123.776')from dual") ;
      streams[strs].debug();
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>)) from dual where 1=1");
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>,3)) from dual where 1=1");
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>,12345,20),'99') from dual where 1=1");
    }
    {
      printd("test 15\n");
      streams[strs].open(0, "select dummy from dual where 1 = :f1<int>") ;
      streams[strs].debug();
      streams[strs]<<1;
    }
    {
      double fps = 1.22;

      printd("test 16\n");
      streams[strs].open(0,"select * from test_db.test_tbl where id>=:f1<unsigned int> "
        "and price like :fp<ldouble> or name like :F3<char[20]>");
      streams[strs]<< (long long)(id=1) ;
      streams[strs]<< (long double)fps ;
      streams[strs]<< (unsigned char*)"orage1" ;
      streams[strs].flush();
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>(unsigned char*)name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
    }
    {
      printd("test 17\n");
      streams[strs].open(0,
        "select *from test_tbl_2 c, test_tbl a "
        "where a.id=c.id(+) group by a.id");
      streams[strs].debug();
      streams[strs].open(0,
        "select *from test_tbl_2 c, (select a.size from test_tbl_1 a, test_tbl b where "
        "a.id(+)=b.id and a.name=b.name(+))b, test_tbl a "
        "where a.id=1(+) and b.size(+)=c.size and a.id(+)=1 and b.size(+)=a.size "
        "and a.size=b.size(+) and a.id(+)=1 and c.size=b.size(+) and 1=a.id(+)");
      streams[strs].open(0,
        "select *from (select a1.size from test_tbl_1 a1, test_tbl b1 where "
        "a1.id(+)=b1.id and a1.name=b1.name(+))b, test_tbl_2 c, test_tbl a "
        "where a.id=1(+) and b.size(+)=c.size ");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl_2 c, test_tbl a "
        "where a.id=b.id(+) and b.id(+)=c.id and a.name=b.name(+) and a.id=c.id(+)");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl_2 c, "
        "test_tbl a, test_tbl_1 d "
        "where a.id=b.id(+) and a.id(+)=c.id and a.name=d.name(+) and a.id=d.id(+)");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl_2 c, test_tbl a "
        "where  a.name=b.name and a.id=c.id(+)");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl a "
        "where a.id=b.id(+)");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl_2 c, test_tbl a "
        "where a.id=b.id(+) and b.id=c.id and a.name=b.name(+) and a.id>1");
      streams[strs].open(0,
        "select *from (select *from test_tbl_1)b, test_tbl_2 c, test_tbl a "
        "where a.id=b.id(+) and b.id(+)=c.id and a.name=b.name(+) and a.id>1");
      streams[strs].open(0,
        "select *from (select a.price from test_tbl_1 a, test_tbl b where a.id=b.id(+))b, "
        "test_tbl_2 c, test_tbl a, test_tbl_1 d "
        "where a.price=b.price(+) and a.id(+)=c.id and a.name=d.name(+) and a.id=d.id(+)");
      cnn.change_db("orcl");
    }
    {
      printd("test 19\n");
      streams[strs].open(0,
        "select tbl0.id, tbl0.name from test_tbl_1 , "
        "(select id,name from test_db.test_tbl_1) tbl0"
        ",test_tbl "
        ",(select id,name from test_db.test_tbl) a"
        ",test_tbl_2 "
        ",(select id,name from (select tbl0.id, tbl0.name from (select*from test_db.test_tbl)tbl0, test_tbl_1)) "
        "where 1=:f1<int> "
        "union "
        "select id, name from (select *from (select *from test_db.test_tbl_1)) "
        );
    }
    {
      printd("test 20\n");
      streams[strs].open(0,"show tables");
#if PERF_TEST==1
      continue;
#endif
    }
  } catch (zas_exception &e) {
    printd("exception: stmt: %s,\nexp: %s,\ncode: %d\n",
      e.stm_text, e.msg, e.code);
  }
  usleep(1);
#if PERF_TEST==1
  }
#endif
  printd("done\n");
  return 0;
}
