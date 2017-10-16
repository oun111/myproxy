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
#define PERF_TEST 1

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
#if 0
      {
        printd("test 0\n");
		streams[strs].open(0, "delete from test_db.test_tbl where id>:f1<long>; ");
		streams[strs] << (7);
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
      cnn.change_db("orcl");
      streams[strs].open(0,
        "insert into A_jinrong values(  :CustomerId_0<int,in>, 'a20050111');");
      streams[strs].debug();
        streams[strs].open(0,
          "select a.parentorgid from pg_info_org a start with a.orgid=:f0<int> /*and a.OrgType  = 2*/ connect by a.orgid=prior a.parentorgid and a.OrgType  = 2 and a.orgtype<>11 order by a.parentorgid");
      streams[strs].debug();
      streams[strs].open(0,
        "select count(1) from  pg_attribute_login a,pg_info_login b,pg_info_org c  where  a.LoginAccountId=b.LoginAccountId and a.OrgId=c.OrgId   and exists ( select 1 from ( select distinct  m.orgid  from pg_info_org m order by m.orgid start with m.orgid in  (  select j.orgid  from pg_info_org j where   j.OrgStatus=1  and exists (select 1 from pg_rl_login2org  k where  k.loginid=:f2<int> and k.OrganizationId=j.OrgId)  ) connect by m.parentorgid=prior m.orgid) n where n.orgid=a.OrgId)  and b.LoginAccount like 'admin__lyc%' escape '_' and 1=1"
        );
      cnn.change_db("orcl");
      streams[strs].open(0,"call insert_pgorginfo_bi(:f1<int>);");
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
    streams[strs].open(0,"select nvl(1,1), nvl(0,1), "
      "orcl.st_log_interest_cycle_seq.nextval, "
      "id, orcl.st_log_interest_cycle_seq.nextval "
      "from test_db.test_tbl where id like :strname<char[10]> or id>: f2 < int > ");
    streams[strs]<< "abcd" ;
    streams[strs]<< (id=4) ;
    streams[strs].debug();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    streams[strs].open(0,"select nvl(1,1), nvl(0,1), "
      "orcl.st_log_interest_cycle_seq.nextval, "
      "id, orcl.st_log_interest_cycle_seq.nextval "
      "from test_db.test_tbl where id like :strname<char[10]> or id>: f2 < int > ");
    streams[strs]<< "abc" ;
    streams[strs]<< (id=3) ;
    streams[strs].debug();
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    streams[strs].open(0,"select nvl(1,1), nvl(0,1), "
      "orcl.st_log_interest_cycle_seq.nextval, "
      "id, orcl.st_log_interest_cycle_seq.nextval "
      "from test_db.test_tbl where id like :strname<char[10]> or id>: f2 < int > ");
    streams[strs]<< "abc" ;
    streams[strs]<< (id=3) ;
    while (!streams[strs].eof()) {
      streams[strs]>>id ;
      streams[strs]>>name ;
      streams[strs]>>point ;
      streams[strs]>>size ;
      streams[strs].flush();
      printd("%d: name %s, point %f, size %ld\n",
        id, name, point, size);
    }
    printd("again: \n");
    {
      int64_t t1 = 123456789654321 ;
      streams[strs].open(0,"update orcl.st_log_settleoperation set SettlementEndTime = :f1<long>, "
        "SettlementResult = :f2<int>, CompletedStep = :f3<int>, BackupStatus = :f4<int> "
        " where settlementOperateID = :f5<long>") ;
      streams[strs]<<t1;
      streams[strs]<<3;
      streams[strs]<<4;
      streams[strs]<<5;
      streams[strs]<<t1;
      /* try again */
      streams[strs]<<t1;
      streams[strs]<<3;
      streams[strs]<<4;
      streams[strs]<<5;
      streams[strs]<<t1;
    }
    printd("test 5\n");
    streams[strs].open(0,"select DealStatus, SessionLength, SessionData, "
      "ResultCode, CreateDate, UpdateDate from orcl.st_session_settle_deal "
      "where SessionId = 0 for update nowait order by UpdateDate");
    streams[strs].open(0,"select DealStatus, SessionLength, SessionData, "
      "ResultCode, CreateDate, UpdateDate from orcl.st_session_settle_deal "
      "where SessionId = 0 for update nowait");
    streams[strs].open(0,"select DealStatus, SessionLength, SessionData, "
      "ResultCode, CreateDate, UpdateDate from orcl.st_session_settle_deal "
      "where SessionId = 0 for update nowait");
    streams[strs].open(0,"select DealStatus, SessionLength, SessionData, "
      "ResultCode, CreateDate, UpdateDate from orcl.st_session_settle_deal "
      "where SessionId = 0 for update nowait");
#if 0
    streams[strs].open(0, "exec dbms_stats.gather_index_stats(ownname => 'orcl', "
      "indname => :f1<char[%d]>,estimate_percent => '10',degree => '4')");
#endif
    streams[strs].open(0,"update orcl.ac_book_subaccount set amount+=1 "
      "where bookid=-1");

    streams[strs].open(0,"update orcl.ac_book_subaccount a, orcl.ac_book_subaccount b "
      " set a.amount+=1, b.amount=2 where a.bookid=-1");

    streams[strs].open(0,"select range, CD_FEERANGE_COMMODITY.RangeId, rangeid, key, "
      "minvalue, maxvalue from "
      "orcl.ts_order_closeposition, orcl.CD_FEERANGE_COMMODITY, "
      "orcl.CC_INFO_TOKEN");
    {
      long long t, lt, st,u ;

      printd("test 5\n");
      streams[strs].open(0,
        "select -trademode, -sum(trademode), trademode<<9, trademode >>10, ~trademode, -12,~0x9,trademode^190,trademode&5, sum(-trademode)|3, MARKETNAME||'_abc' from sd_market_info");
      streams[strs].debug();
      streams[strs].open(0,"select tradedate, lasttradedate from "
        "orcl.sd_market_info");
      streams[strs].rewind();
      while (!streams[strs].eof()) {
        streams[strs]>>t ;
        streams[strs]>>lt ;
        printd("tradedate: %lld, last: %lld\n", t, lt);
      }
      streams[strs].open(0,"select userid, trademode, nextsettlementday, "
        "status from orcl.st_status_usersettle "
        //"where status=:f1<long> "
        "where nextsettlementday=:f1<bigint> "
        "limit 10");
      streams[strs]<<(t=3642854400000000);
      //streams[strs]<<(t=1);
      while (!streams[strs].eof()) {
        streams[strs]>>u ;
        streams[strs]>>t ;
        streams[strs]>>lt ;
        streams[strs]>>st ;
        printd("userid: %lld, trademode: %lld, nxt: %lld, stat: %lld\n", u, t, lt,st);
      }
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
      streams[strs].open(0,"insert into orcl.sd_database_backup_info_his(SeqId, "
        "TradeDate, BeginTime, EndTime, BackupResult) select  SeqId, "
        "TradeDate, BeginTime, EndTime, BackupResult from "
        "orcl.sd_database_backup_info;");
      streams[strs].open(0,"insert into orcl.sd_database_backup_info_his(SeqId, "
        "TradeDate, BeginTime, EndTime, BackupResult) select  SeqId, "
        "TradeDate, BeginTime, EndTime, BackupResult from "
        "orcl.sd_database_backup_info;");
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
      streams[strs].open(0,"insert into orcl.sd_database_backup_info_his select  * from "
        "orcl.sd_database_backup_info;");
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
      printd("test 9\n");
      streams[strs].open(0, 
        "merge into orcl.cd_feeitem_commodity dst using orcl.cd_feeitem_commodity_tmp src "
        "on (dst.CommodityFeeId = :IdValue<int> and "
        "src.CommodityFeeId = dst.CommodityFeeId) "
        "when matched then update set dst.CommodityFeeGroupId = src.CommodityFeeGroupId, "
        "dst.CommodityId = src.CommodityId, "
        "dst.ChargeType = src.ChargeType, dst.CollectType = src.CollectType, "
        "dst.FeeType = src.FeeType, dst.FeeSubType = src.FeeSubType, "
        "dst.Value = src.Value, dst.EffectStatus = src.EffectStatus, "
        "dst.DescInfo = src.DescInfo, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.CommodityFeeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.CommodityFeeId, dst.CommodityFeeGroupId, dst.CommodityId, dst.ChargeType, "
        "dst.CollectType, dst.FeeType, dst.FeeSubType, dst.Value, "
        "dst.EffectStatus, dst.DescInfo, dst.CreateDate, dst.CreatorLoginId, "
        "dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.CommodityFeeId, src.CommodityFeeGroupId, "
        "src.CommodityId, src.ChargeType, "
        "src.CollectType, src.FeeType, src.FeeSubType, src.Value, "
        "src.EffectStatus, src.DescInfo, src.CreateDate, src.CreatorLoginId, "
        "src.UpdateDate, src.UpdatorLoginId) "
        "where src.CommodityFeeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int>");
      streams[strs].open(0, 
        "merge into orcl.cd_feerange_commodity dst using orcl.cd_feerange_commodity_tmp src "
        "on (dst.RangeId = :IdValue<int> and src.RangeId = dst.RangeId) "
        "when matched then update set dst.CommodityFeeGroupId = src.CommodityFeeGroupId, "
        "dst.CommodityId = src.CommodityId, "
        "dst.ChargeType = src.ChargeType, dst.FeeType = src.FeeType, "
        "dst.FeeSubType = src.FeeSubType, dst.MaxValue = src.MaxValue, "
        "dst.MinValue = src.MinValue, dst.DefaultValue = src.DefaultValue, "
        "dst.EffectStatus = src.EffectStatus, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.RangeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.RangeId, dst.CommodityFeeGroupId, dst.CommodityId, dst.ChargeType, "
        "dst.FeeType, dst.FeeSubType, dst.MaxValue, dst.MinValue, "
        "dst.DefaultValue, dst.EffectStatus, dst.CreateDate, dst.CreatorLoginId, "
        "dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.RangeId, src.CommodityFeeGroupId, src.CommodityId, src.ChargeType, "
        "src.FeeType, src.FeeSubType, src.MaxValue, src.MinValue, "
        "src.DefaultValue, src.EffectStatus, src.CreateDate, src.CreatorLoginId, "
        "src.UpdateDate, src.UpdatorLoginId) "
        "where src.RangeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int>");
      streams[strs].open(0, 
        "merge into orcl.cd_info_deliverycommodity dst using orcl.cd_info_deliverycommodity_tmp src "
        "on (dst.CommodityId = :IdValue<int> and src.CommodityId = dst.CommodityId)"
        "when matched then update set dst.CommodityName = src.CommodityName, "
        "dst.CommodityStatus = src.CommodityStatus, "
        "dst.CreateDate = src.CreateDate, dst.CreatorLoginId = src.CreatorLoginId, "
        "dst.UpdateDate = src.UpdateDate, dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.CommodityId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.CommodityId, dst.CommodityName, dst.CommodityStatus, dst.CreateDate, "
        "dst.CreatorLoginId, dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.CommodityId, src.CommodityName, src.CommodityStatus, src.CreateDate, "
        "src.CreatorLoginId, src.UpdateDate, src.UpdatorLoginId) "
        "where src.CommodityId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_rl_commodity2member dst using orcl.cd_rl_commodity2member_tmp src "
        "on (dst.Id = :IdValue<int> and src.Id = dst.Id)"
        "when matched then update set dst.CommodityId = src.CommodityId, "
        "dst.MemberId = src.MemberId, "
        "dst.quotationid = src.quotationid, dst.usepricetype = src.usepricetype, "
        "dst.QuoteCoefficient = src.QuoteCoefficient, "
        "dst.ExchangeRateType = src.ExchangeRateType, "
        "dst.QuoteExchangeRate = src.QuoteExchangeRate, dst.spreadtype = src.spreadtype, "
        "dst.fixedspread = src.fixedspread, dst.discount = src.discount, "
        "dst.EffectStatus = src.EffectStatus, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId, "
        "dst.FloatSpreadRate = src.FloatSpreadRate, "
        "dst.FloatSpreadType = src.FloatSpreadType "
        "where src.Id = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.Id, dst.CommodityId, dst.MemberId, dst.quotationid, "
        "dst.usepricetype, dst.QuoteCoefficient, dst.ExchangeRateType, "
        "dst.QuoteExchangeRate, "
        "dst.spreadtype, dst.fixedspread, dst.discount, dst.EffectStatus, "
        "dst.CreateDate, dst.CreatorLoginId, dst.UpdateDate, dst.UpdatorLoginId, "
        "dst.FloatSpreadRate, dst.FloatSpreadType) "
        "values(src.Id, src.CommodityId, src.MemberId, src.quotationid, "
        "src.usepricetype, src.QuoteCoefficient, src.ExchangeRateType, "
        "src.QuoteExchangeRate, "
        "src.spreadtype, src.fixedspread, src.discount, src.EffectStatus, "
        "src.CreateDate, src.CreatorLoginId, src.UpdateDate, src.UpdatorLoginId, "
        "src.FloatSpreadRate, src.FloatSpreadType) "
        "where src.Id = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_plan_commodity dst using orcl.cd_plan_commodity_tmp src "
        "on (dst.Id = :IdValue<int> and src.Id = dst.Id) "
        "when matched then update set dst.CommodityId = src.CommodityId, "
        "dst.StartTime = src.StartTime, "
        "dst.EndTime = src.EndTime, dst.IsNextClose = src.IsNextClose, "
        "dst.CreateDate = src.CreateDate, dst.CreatorLoginId = src.CreatorLoginId, "
        "dst.UpdateDate = src.UpdateDate, dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.Id = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.Id, dst.CommodityId, dst.StartTime, dst.EndTime, "
        "dst.IsNextClose, dst.CreateDate, dst.CreatorLoginId, dst.UpdateDate, "
        "dst.UpdatorLoginId) "
        "values(src.Id, src.CommodityId, src.StartTime, src.EndTime, "
        "src.IsNextClose, src.CreateDate, src.CreatorLoginId, src.UpdateDate, "
        "src.UpdatorLoginId) "
        "where src.Id = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_feeassign_rule dst using orcl.cd_feeassign_rule_tmp src "
        "on (dst.FeeAssignRuleId = :IdValue<int> and "
        "src.FeeAssignRuleId = dst.FeeAssignRuleId) "
        "when matched then update set dst.FeeAssignGroupId = src.FeeAssignGroupId, "
        "dst.AssignedUserType = src.AssignedUserType, "
        "dst.AssignedFeeValue = src.AssignedFeeValue, "
        "dst.CreateDate = src.CreateDate, dst.CreatorLoginId = src.CreatorLoginId, "
        "dst.UpdateDate = src.UpdateDate, dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.FeeAssignRuleId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.FeeAssignRuleId, dst.FeeAssignGroupId, dst.AssignedUserType, "
        "dst.AssignedFeeValue, "
        "dst.CreateDate, dst.CreatorLoginId, dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.FeeAssignRuleId, src.FeeAssignGroupId, src.AssignedUserType, "
        "src.AssignedFeeValue, "
        "src.CreateDate, src.CreatorLoginId, src.UpdateDate, src.UpdatorLoginId) "
        "where src.FeeAssignRuleId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_feeassign_group dst using orcl.cd_feeassign_group_tmp src "
        "on (dst.FeeAssignGroupId = :IdValue<int> and "
        "src.FeeAssignGroupId = dst.FeeAssignGroupId) "
        "when matched then update set dst.FeeAssignGroupName = src.FeeAssignGroupName, "
        "dst.GroupSequence = src.GroupSequence, dst.CommodityFeeId = src.CommodityFeeId, "
        "dst.DescInfo = src.DescInfo, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.FeeAssignGroupId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.FeeAssignGroupId, dst.FeeAssignGroupName, dst.GroupSequence, "
        "dst.CommodityFeeId, "
        "dst.DescInfo, dst.CreateDate, dst.CreatorLoginId, dst.UpdateDate, "
        "dst.UpdatorLoginId) "
        "values(src.FeeAssignGroupId, src.FeeAssignGroupName, "
        "src.GroupSequence, src.CommodityFeeId, "
        "src.DescInfo, src.CreateDate, src.CreatorLoginId, src.UpdateDate, "
        "src.UpdatorLoginId) "
        "where src.FeeAssignGroupId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_rl_user2feegroup dst using orcl.cd_rl_user2feegroup_tmp src "
        "on (dst.UserFeeGroupId = :IdValue<int> and "
        "src.UserFeeGroupId = dst.UserFeeGroupId) "
        "when matched then update set dst.CommodityFeeGroupId = src.CommodityFeeGroupId, "
        "dst.UserId = src.UserId, "
        "dst.CreateDate = src.CreateDate, dst.CreatorLoginId = src.CreatorLoginId, "
        "dst.UpdateDate = src.UpdateDate, dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.UserFeeGroupId = :IdValue<int> "
        "and src.IsPass > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.UserFeeGroupId, dst.CommodityFeeGroupId, dst.UserId, dst.CreateDate, "
        "dst.CreatorLoginId, dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.UserFeeGroupId, src.CommodityFeeGroupId, src.UserId, src.CreateDate, "
        "src.CreatorLoginId, src.UpdateDate, src.UpdatorLoginId) "
        "where src.UserFeeGroupId = :IdValue<int> "
        "and src.IsPass > :MgmEffectMin<int> ");
      streams[strs].open(0, 
        "merge into orcl.cd_feerange_commodity dst using orcl.cd_feerange_commodity_tmp src "
        "on (dst.RangeId = :IdValue<int> and src.RangeId = dst.RangeId) "
        "when matched then update set dst.CommodityFeeGroupId = src.CommodityFeeGroupId, "
        "dst.CommodityId = src.CommodityId, "
        "dst.ChargeType = src.ChargeType, dst.FeeType = src.FeeType, "
        "dst.FeeSubType = src.FeeSubType, dst.MaxValue = src.MaxValue, "
        "dst.MinValue = src.MinValue, dst.DefaultValue = src.DefaultValue, "
        "dst.EffectStatus = src.EffectStatus, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.RangeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.RangeId, dst.CommodityFeeGroupId, dst.CommodityId, dst.ChargeType, "
        "dst.FeeType, dst.FeeSubType, dst.MaxValue, dst.MinValue, "
        "dst.DefaultValue, dst.EffectStatus, dst.CreateDate, dst.CreatorLoginId, "
        "dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.RangeId, src.CommodityFeeGroupId, src.CommodityId, src.ChargeType, "
        "src.FeeType, src.FeeSubType, src.MaxValue, src.MinValue, "
        "src.DefaultValue, src.EffectStatus, src.CreateDate, src.CreatorLoginId, "
        "src.UpdateDate, src.UpdatorLoginId) "
        "where src.RangeId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs]<<12<<100 ;
      streams[strs].open(0, 
        "merge into orcl.cd_info_commodity dst using orcl.cd_info_commodity_tmp src "
        "on (dst.CommodityId = :IdValue<int> and src.CommodityId = dst.CommodityId)"
        "when matched then update set dst.CommodityCode = src.CommodityCode, "
        "dst.CommodityName = src.CommodityName, "
        "dst.CommoditySName = src.CommoditySName, dst.TradeMode = src.TradeMode, "
        "dst.CommodityType = src.CommodityType, dst.MarketStatus = src.MarketStatus, "
        "dst.CommodityStatus = src.CommodityStatus, "
        "dst.CommoditySequence = src.CommoditySequence, "
        "dst.CommodityRight = src.CommodityRight, dst.MinQuoteChangeUnit = src.MinQuoteChangeUnit, "
        "dst.WeightRate = src.WeightRate, dst.WeightUnit = src.WeightUnit, "
        "dst.QuoteMove = src.QuoteMove, dst.MinPriceUnit = src.MinPriceUnit, "
        "dst.AgreeUnit = src.AgreeUnit, dst.Currency = src.Currency, "
        "dst.DescInfo = src.DescInfo, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId, dst.BenchTimes1 = src.BenchTimes1, "
        "dst.BenchSpread1 = src.BenchSpread1, dst.BenchTimes2 = src.BenchTimes2, "
        "dst.BenchSpread2 = src.BenchSpread2, dst.BenchTimes3 = src.BenchTimes3, "
        "dst.BenchSpread3 = src.BenchSpread3, dst.PrecentSpan = src.PrecentSpan, "
        "dst.UseFiltrateEqual = src.UseFiltrateEqual, dst.FilteredFlag = src.FilteredFlag, "
        "dst.RateFlag = src.RateFlag, dst.Rate = src.Rate, "
        "dst.RateInterval = src.RateInterval, dst.CmdCode = src.CmdCode, "
        "dst.CommodityClassName = src.CommodityClassName, dst.CommodityMode = src.CommodityMode, "
        "dst.IsDisplay = src.IsDisplay, dst.WeightStep = src.WeightStep, "
        "dst.DisplayUnit = src.DisplayUnit, dst.EnterMarketType = src.EnterMarketType, "
        "dst.TransferRate = src.TransferRate, dst.TradeType = src.TradeType "
        "where src.CommodityId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.CommodityId, dst.CommodityCode, dst.CommodityName, dst.CommoditySName, "
        "dst.TradeMode, dst.CommodityType, dst.MarketStatus, dst.CommodityStatus, "
        "dst.CommoditySequence, dst.CommodityRight, dst.MinQuoteChangeUnit, dst.WeightRate, "
        "dst.WeightUnit, dst.QuoteMove, dst.MinPriceUnit, dst.AgreeUnit, "
        "dst.Currency, dst.DescInfo, dst.CreateDate, dst.CreatorLoginId, "
        "dst.UpdateDate, dst.UpdatorLoginId, dst.BenchTimes1, dst.BenchSpread1, "
        "dst.BenchTimes2, dst.BenchSpread2, dst.BenchTimes3, dst.BenchSpread3, "
        "dst.PrecentSpan, dst.UseFiltrateEqual, dst.FilteredFlag, dst.RateFlag, "
        "dst.Rate, dst.RateInterval, dst.CmdCode, dst.ClosePositionForFree, "
        "dst.CommodityClass, dst.CommodityClassName, dst.CommodityMode, dst.IsDisplay, "
        "dst.WeightStep, dst.DisplayUnit, dst.EnterMarketType, dst.TransferRate, "
        "dst.TradeType) "
        "values(src.CommodityId, src.CommodityCode, src.CommodityName, src.CommoditySName, "
        "src.TradeMode, src.CommodityType, src.MarketStatus, src.CommodityStatus, "
        "src.CommoditySequence, src.CommodityRight, src.MinQuoteChangeUnit, src.WeightRate, "
        "src.WeightUnit, src.QuoteMove, src.MinPriceUnit, src.AgreeUnit, "
        "src.Currency, src.DescInfo, src.CreateDate, src.CreatorLoginId, "
        "src.UpdateDate, src.UpdatorLoginId, src.BenchTimes1, src.BenchSpread1, "
        "src.BenchTimes2, src.BenchSpread2, src.BenchTimes3, src.BenchSpread3, "
        "src.PrecentSpan, src.UseFiltrateEqual, src.FilteredFlag, src.RateFlag, "
        "src.Rate, src.RateInterval, src.CmdCode, src.ClosePositionForFree, "
        "src.CommodityClass, src.CommodityClassName, src.CommodityMode, src.IsDisplay, "
        "src.WeightStep, src.DisplayUnit, src.EnterMarketType, src.TransferRate, "
        "src.TradeType) "
        "where src.CommodityId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int>");
      streams[strs].open(0, 
        "merge into orcl.cd_info_quotationitem dst using orcl.cd_info_quotationitem_tmp src "
        "on (dst.QuotationId = :IdValue<int> and src.QuotationId = dst.QuotationId) "
        "when matched then update set dst.ItemSymbol = src.ItemSymbol, dst.ItemName = src.ItemName, "
        "dst.ItemType = src.ItemType, dst.QuotationType = src.QuotationType, "
        "dst.Exchange = src.Exchange, dst.MemberId = src.MemberId, "
        "dst.Currency = src.Currency, dst.WeightUnit = src.WeightUnit, "
        "dst.Status = src.Status, dst.CreateDate = src.CreateDate, "
        "dst.CreatorLoginId = src.CreatorLoginId, dst.UpdateDate = src.UpdateDate, "
        "dst.UpdatorLoginId = src.UpdatorLoginId "
        "where src.QuotationId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.QuotationId, dst.ItemSymbol, dst.ItemName, dst.ItemType, "
        "dst.QuotationType, dst.Exchange, dst.MemberId, dst.Currency, "
        "dst.WeightUnit, dst.Status, dst.CreateDate, dst.CreatorLoginId, "
        "dst.UpdateDate, dst.UpdatorLoginId) "
        "values(src.QuotationId, src.ItemSymbol, src.ItemName, src.ItemType, "
        "src.QuotationType, src.Exchange, src.MemberId, src.Currency, "
        "src.WeightUnit, src.Status, src.CreateDate, src.CreatorLoginId, "
        "src.UpdateDate, src.UpdatorLoginId) "
        "where src.QuotationId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
      streams[strs]<<22<<10;
      streams[strs].open(0, 
        "merge into orcl.cd_feegroup_commodity dst using orcl.cd_feegroup_commodity_tmp src "
        "on (dst.CommodityFeeGroupId = :IdValue<int> and src.CommodityFeeGroupId = dst.CommodityFeeGroupId) "
        "when matched then update set dst.FeeGroupName = src.FeeGroupName, dst.TradeMode = src.TradeMode, "
        "dst.ApplyUserType = src.ApplyUserType, dst.CreateUserId = src.CreateUserId, "
        "dst.Status = src.Status, dst.DescInfo = src.DescInfo, "
        "dst.CreateDate = src.CreateDate, dst.CreatorLoginId = src.CreatorLoginId, "
        "dst.UpdateDate = src.UpdateDate, dst.UpdatorLoginId = src.UpdatorLoginId, "
        "dst.IsDeliveryGroup = src.IsDeliveryGroup, dst.IsDefault = src.IsDefault, "
        "dst.CreateType = src.CreateType "
        "where src.CommodityFeeGroupId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> "
        "when not matched then insert "
        "(dst.CommodityFeeGroupId, dst.FeeGroupName, dst.TradeMode, dst.ApplyUserType, "
        "dst.CreateUserId, dst.Status, dst.DescInfo, dst.CreateDate, "
        "dst.CreatorLoginId, dst.UpdateDate, dst.UpdatorLoginId, dst.IsDeliveryGroup, "
        "dst.IsDefault, dst.CreateType) "
        "values(src.CommodityFeeGroupId, src.FeeGroupName, src.TradeMode, src.ApplyUserType, "
        "src.CreateUserId, src.Status, src.DescInfo, src.CreateDate, "
        "src.CreatorLoginId, src.UpdateDate, src.UpdatorLoginId, src.IsDeliveryGroup, "
        "src.IsDefault, src.CreateType) "
        "where src.CommodityFeeGroupId = :IdValue<int> "
        "and src.ExaminationProgress > :MgmEffectMin<int> ");
    }
    {
      printd("test 10\n");
      streams[strs].open(0,"delete orcl.st_log_settleoperation a where "
        "a.TradeMode = -1 and a.SettlementType = -1 ");
      streams[strs].open(0,"delete orcl.st_log_settleoperation");
      streams[strs].open(0,"delete orcl.cd_rl_user2deposit_tmp a, "
        "orcl.sd_log_operation, orcl.rs_detail_interests_tmp cc,"
        "orcl.ts_order_closeposition_his "
        "where a.userid = :f1<int> and a.batchnum = :f2<char[33]> and "
        "sd_log_operation.LOGINACCOUNTID = -1 and cc.USERID = 0 and "
        "exists( select 1 from orcl.cd_info_commodity b where "
        "a.commodityid=b.commodityid and b.entermarkettype=:f3<int> and "
        "b.tradetype=:f4<int> and ts_order_closeposition_his.BROKERID=-1 )");
      streams[strs]<<(int)0;
      streams[strs]<<(char*)"abc";
      streams[strs]<<(int)0;
      streams[strs]<<(int)0;
      cnn.change_db("orcl");
      streams[strs].open(0,
        "delete from pg_rl_traderole2right a1 "
        " where a1.roleid in( "
        "select f.ROLEID "
         " from pg_rl_login2traderole a, "
          "     pg_info_traderole     b, "
          "     pg_rl_traderole2right c, "
          "     vi_info_customer      d, "
          "     vi_info_customer      e, "
          "     pg_rl_login2traderole f, "
          "     cm_rl_user2user       g "
        " where a.roleid = b.roleid "
          " and b.roleid = c.roleid "
          " and a.loginid = d.loginaccountid "
          " and d.loginaccounttype = 20 "
          " and b.rolestatus = 1 "
          " and e.loginaccountid = f.loginid "
          " and g.masteruserid = e.userid "
          " and g.salveuserid = d.userid "
          " and g.relationtype = 1002 "
          " and g.relationstatus = 1 "
          " and d.userid = :f1<int>) ");
      streams[strs].open(0, "truncate /*table*/ ts_order_holdposition_his");
      cnn.change_db("test_db");
    }
    {
      printd("test 11\n");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "select -trademode, -sum(trademode), sum(-trademode) from sd_market_info");
      streams[strs].open(0,
        "select bandstatus.v1, total_t.tv from (select round( count(distinct BankInfoID) / (count(distinct BankInfoID) +1 ) ) v1 from bk_status_check where BankInfoID = :BankId_0<int,in> and ReconcilDAY = :CheckDate_0<bigint,in>) bandstatus, (select count(*) tv from ( select SeqId v1, SysTime v2, BankFlowInSys v3, SysFlowInSys v4, OperateType+2 v5, BankAcctInSys v6, a.sysacctinsys v7, AmountInSys v8, 1 v9, ErrorType v10 from bk_accountitem_unpaired a where a.BankInfoID = :BankId_1<int,in> and a.TRADEDATE = :CheckDate_1<bigint,in> and a.invalidflag=0 union all select SeqId v1, BankTime v2, BankFlowInBank v3, SysFlowBInBank v4, OperateTypeInBank v5, BankAcctInBank v6, a.sysacctinbank v7, AmountInBank v8, 1 v9, ErrorType v10 from bk_accountitem_unpaired a where a.BankInfoID = :BankId_2<int,in> and a.TRADEDATE = :CheckDate_2<bigint,in> and a.invalidflag=0 )) total_t "
        );
      streams[strs].open(0,
        "select distinct p.orgid, p.OrgName, p.ParentOrgId, "
        "p.OrgType,p.OrgStatus, 2 flag, 1 orgpermissionflag "
        "from orcl.pg_info_org p  start with p.orgid in  "
        "(select a.masterorgid   from orcl.pg_rl_org2org a where a.relationtype = 1001 "
        "and exists (select 1 from  "
        "(select distinct m.orgid from orcl.pg_info_org m "
        "start with m.orgid in   (select j.orgid  from orcl.pg_info_org j  "
        " where j.OrgStatus = 1  and j.customerid =:f1<int> ) connect by "
        "m.parentorgid = prior m.orgid "
        " ) b where b.orgid = a.salveorgid)"
        ")connect by p.parentorgid = prior p.orgid ");
      streams[strs].open(0, "select distinct p.orgid, p.OrgName, p.ParentOrgId, "
        "p.OrgType,p.OrgStatus, 2 flag, 1 orgpermissionflag "
        "from orcl.pg_info_org p connect by p.parentorgid = prior p.orgid "
        "start with p.orgid in "
        "(select a.masterorgid from orcl.pg_rl_org2org a where a.relationtype = 1001 "
        "and exists (select 1 from (select distinct m.orgid from orcl.pg_info_org m "
        "where 1=1  "
        "connect by prior m.parentorgid = m.orgid "
        "start with m.orgid in (select j.orgid  from orcl.pg_info_org j "
        "where j.OrgStatus = 1  and j.customerid =:f1<int> )) "
        "b where b.orgid = a.salveorgid)) ");
      streams[strs].open(0, "select distinct p.orgid, p.OrgName, p.ParentOrgId, "
        "p.OrgType,p.OrgStatus, 2 flag, 1 orgpermissionflag "
        "from orcl.pg_info_org p connect by p.parentorgid = prior p.orgid "
        "start with p.orgid = "
        "(select a.masterorgid from orcl.pg_rl_org2org a where a.relationtype = 1001 "
        "and exists (select 1 from (select distinct m.orgid from orcl.pg_info_org m "
        "start with m.orgid in (select j.orgid  from orcl.pg_info_org j "
        "where j.OrgStatus = 1  and j.customerid =:f1<int> ) "
        "connect by prior m.parentorgid = m.orgid ) "
        "b where b.orgid = a.salveorgid)) ");
      streams[strs].open(0,
        "(select k.orgid,k.OrgName, k.ParentOrgId, k.OrgType, "
        "c.customernum, k.OrgStatus, 1 orgpermissionflag from ( "
        "select distinct m.orgid , m.OrgName,m.ParentOrgId,m.OrgType, "
        "m.customerid, m.OrgStatus from orcl.pg_info_org m start with m.orgid "
        "in (select a.orgid from orcl.pg_info_org a where a.OrgStatus=1 and "
        "exists (select 1 from orcl.pg_rl_login2org d where d.loginid=:f1<int> "
        "and d.OrganizationId=a.OrgId)) connect by m.parentorgid=prior m.orgid "
        "union all select a.OrgId,a.OrgName,a.ParentOrgId,a.OrgType,a.customerid, "
        "a.OrgStatus from orcl.pg_info_org a where   a.OrgStatus<>3  and OrgType=3) k, "
        "orcl.cm_info_customer c where k.customerid=c.customerid) "
        "union all "
        "(select a.OrgId,a.OrgName,a.ParentOrgId,a.OrgType,b.customernum, "
        "a.OrgStatus, 0 orgpermissionflag from orcl.pg_info_org a,orcl.cm_info_customer b "
        "where  a.customerid=b.customerid and  a.OrgStatus=1  and a.OrgId not in "
        "(select   k.orgid from (select distinct  m.orgid , m.OrgName,m.ParentOrgId, "
        "m.OrgType ,m.customerid from orcl.pg_info_org m start with m.orgid = (select "
        "a.orgid from orcl.pg_info_org a where a.OrgStatus=1 and exists (select 1 from "
        "orcl.pg_rl_login2org d where d.loginid=:f1<int> and d.OrganizationId=a.OrgId)) "
        "connect by m.parentorgid=prior m.orgid "
        "union all select a.OrgId,a.OrgName,a.ParentOrgId,a.OrgType,a.customerid "
        "from orcl.pg_info_org a where   a.OrgStatus<>3  and OrgType=3 ) k, "
        "orcl.cm_info_customer c where k.customerid=c.customerid)) ");
      streams[strs].open(0, "select distinct p.orgid, p.OrgName, p.ParentOrgId, "
        "p.OrgType,p.OrgStatus, 2 flag, 1 orgpermissionflag "
        "from orcl.pg_info_org p start with p.orgid = "
        "(select a.masterorgid from orcl.pg_rl_org2org a where a.relationtype = 1001 "
        "and exists (select 1 from (select distinct m.orgid from orcl.pg_info_org m "
        "connect by prior m.parentorgid = m.orgid start with "
        "m.orgid in (select j.orgid  from orcl.pg_info_org j "
        "where j.OrgStatus = 1  and j.customerid =:f1<int>)) "
        "b where b.orgid = a.salveorgid)) connect by "
        "p.parentorgid = prior p.orgid");
      streams[strs].open(0, "select distinct p.orgid, p.OrgName, p.ParentOrgId, "
        "p.OrgType,p.OrgStatus, 2 flag, 1 orgpermissionflag "
        "from orcl.pg_info_org p start with p.orgid in "
        "(select a.masterorgid from orcl.pg_rl_org2org a where a.relationtype = 1001 "
        "and exists (select 1 from (select distinct m.orgid from orcl.pg_info_org m "
        "start with m.orgid in (select j.orgid  from orcl.pg_info_org j "
        "where j.OrgStatus = 1  and j.customerid =:f1<int> ) "
        "connect by prior m.parentorgid = m.orgid ) "
        "b where b.orgid = a.salveorgid)) connect by "
        "p.parentorgid = prior p.orgid");
      streams[strs].open(0,"select distinct m.orgid from orcl.pg_info_org m "
        "start with m.orgid = :KEY<INT> connect by "
        "m.parentorgid=prior m.orgid");
      streams[strs].open(0,"select distinct m.orgid from orcl.pg_info_org m where orgid=33 "
        "connect by prior parentorgid=orgid "
        "start with orgid = :KEY<INT>");
      streams[strs].open(0,"select distinct masteruserid from orcl.cm_rl_user2user "
        "start with salveuserid = :KEY<INT> and relationtype = 1006 "
        "connect by salveuserid = prior masteruserid and relationtype = 1006");
      streams[strs].open(0,"select distinct masteruserid from orcl.cm_rl_user2user "
        "connect by salveuserid = prior masteruserid and relationtype = 1006");
      cnn.change_db("test_db");
    }
    {
      printd("test 12\n");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "with userid_all as (select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where OrgStatus = 1 and exists (select 1 from pg_rl_login2org where loginid = :LoginId_0<int,in> and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (  select distinct p.orgid from pg_info_org p start with p.orgid in (  select a.masterorgid from pg_rl_org2org a where exists ( select 1 from ( select distinct m.orgid from pg_info_org m start with m.orgid in (  select j.orgid from pg_info_org j where j.OrgStatus=1 and exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid ) b where b.orgid=a.salveorgid ) ) connect by p.parentorgid=prior p.orgid )) ) users ), "
        "orgid_all as  (select * from (select distinct m.orgid from pg_info_org m where 1=1  union select distinct m.orgid from pg_info_org m start with 1=1  connect by m.parentorgid=prior m.orgid ) orgs ), "
        "brkid_all as (select * from  (select distinct m.userid from cm_info_user m where 1=1  union select distinct m.userid from cm_info_user m where 1=1  union select distinct masteruserid userid from cm_rl_user2user start with 1=1  and relationtype = 1006 connect by salveuserid = prior masteruserid and relationtype = 1006 ) brks ), "
        "orgid_brkid_all as  (select * from (select distinct userid from cm_info_user where organizationid in (select distinct m.orgid from pg_info_org m where 1=1  union select distinct m.orgid from pg_info_org m start with 1=1  connect by m.parentorgid=prior m.orgid ) and usertype = 1001 and userstatus = 1 ) orgs_brks ), "
        "mem_brkid_all as (select * from  (select UserId from cm_info_user where usertype = 1001 and userstatus = 1 ) mems_brks ), "
        "mem_orgid_all as ( select * from (select distinct m.orgid from pg_info_org m start with m.orgid in ( select j.orgid from pg_info_org j where j.OrgStatus=1 and exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_2<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid) mem_orgs ) "
        "select count(*), sum(a.amount) from vi_info_customer b, ac_bank_signinfo c, cm_user_changelog a left join pg_info_org p1  on a.oldorgid=p1.orgid left join pg_info_org p2 on a.neworgid=p2.orgid left join cm_info_user u1 on a.oldbrokerid=u1.userid /*left*/join cm_info_customer v1 on u1.customerid=v1.customerid left join cm_info_user u2 on a.newbrokerid=u2.userid left join cm_info_customer v2 on u2.customerid=v2.customerid left join pg_info_login p3 on a.creatorloginid=p3.loginaccountid where a.booktypeid=1001 and a.newuserid=b.userid and a.customerid=c.customerid and a.createdate >= :StartDate_0<bigint,in> and a.createdate <= :EndDate_0<bigint,in> and 1=1  and 1=1  and exists (select 1 from userid_all ua where ua.userid=a.newuserid or ua.userid=a.olduserid )") ;
      streams[strs]<<0<<1;
      streams[strs]<<2;
      streams[strs]<<1000<<2000;
      streams[strs].debug();
#if 1
      streams[strs].open(0,
        "insert into cd_feeassign_group_tmp("
        " FeeAssignGroupId, FeeAssignGroupName, GroupSequence, CommodityFeeId, "
        " DescInfo, CreateDate, CreatorLoginId, UpdateDate, "
        " UpdatorLoginId, ExaminationProgress, ChangeDesc, BatchNum) "
        " values(cd_feeassign_group_seq.nextval, :Name<char[64]>, :Sequence<int>, seq_feeitemid.nextval, "
        " :DescInfo<char[128]>, :CurTime<long>, :LoginId<int>, :CurTime<long>, "
        " :LoginId<int>, :ExamProgress<int>, :ChangeDesc<char[128]>, :BatchNum<char[33]>)"          );
      streams[strs]<<"name"<<11<<"descInfo"
        <<1000<<1
        <<999+1
        <<"chDesc"
        <<"batchNum" ;
#endif
      cnn.change_db("test_db");
      streams[strs].open(0,"select * from test_db.test_tbl where id>:id<long> "
        "or id=:id<long>");
      streams[strs]<<(long)2;
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
      streams[strs]<<(long)3;
      while (!streams[strs].eof()) {
        streams[strs]>>id ;
        streams[strs]>>name ;
        streams[strs]>>point ;
        streams[strs]>>size ;
        streams[strs].flush();
        printd("%d: name %s, point %f, size %ld\n",
          id, name, point, size);
      }
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
      cnn.change_db("orcl");
      streams[strs].open(0,
        "select count(*) from  ( select k.customerid,k.customername,k.customernum,k.userid ,k.loginaccount, k.UserStatus,m.customerid cmcustomerid,n.customerid bmcustomerid from (select *from ((select g.customerid,g.customername,g.customernum,g.userid,g.loginaccount, g.UserStatus,h.SalveUserId CmUserid,j.SalveUserId BmUserid  from (select a.customerid,a.customername,a.customernum,c.loginaccount,f.userid, f.UserStatus  from cm_info_customer a ,  cm_info_user b ,   pg_info_login c,  pg_info_org d,  cm_info_user f   where b.userid = :QueryUserId_0<int,in> and b.trademode=10  and b.customerid=a.customerid and a.customerid=c.customerid and a.customerid=f.customerid and f.OrganizationId=d.orgid ) g  left join  cm_rl_user2user h  on (g.userid=h.masteruserid and h.RelationType=1002) left join  cm_rl_user2user j  on (g.userid=j.masteruserid and j.RelationType=1005) )) y ) k left join  cm_info_user m  on(m.userid=k.CmUserid )  left join  cm_info_user n  on(n.userid=k.BmUserid)  ) p left join cm_info_customer q  on( q.customerid=p.cmcustomerid) left join cm_info_customer t  on (t.customerid=p.bmcustomerid)"
        );
      streams[strs].open(0,
        "with org_in as "
        " (  "
        " select a.OrgId from orcl.pg_info_org a "
        " where  a.OrgType in (1,2) and a.OrgStatus <>3  and a.CustomerId=:f1<int> "   
        " and exists (  "
        " select 1 from ( " 
        " select distinct  m.orgid  from orcl.pg_info_org m start with m.orgid in  " 
        " (   "
        " select j.orgid  from orcl.pg_info_org j where   j.OrgStatus<>3 "   
        " and exists (select 1 from orcl.pg_rl_login2org  k where  k.loginid=:f2<int> and k.OrganizationId=j.OrgId)  " 
        " ) connect by m.parentorgid=prior m.orgid) n where n.orgid=a.OrgId)  "
        " ),"
        "org_all as "
        " (select distinct  m.orgid  from orcl.pg_info_org m where m.OrgType in (1,2) and m.OrgStatus <>3 and m.CustomerId=:f1<int> start with m.orgid =   "
        " (   "
        " select orgid from orcl.pg_attribute_login where LoginAccountId=:f2<int> "
        " ) connect by m.parentorgid=prior m.orgid "
        " )"
        "select k.OrgId,k.OrgName,k.ParentOrgId,k.OrgType,k.OrgStatus,k.CancellOperateDate,k.CancellEffectDate,f.loginusername, k.flag,  k.permissionflag from ( "
        " select a.OrgId,a.OrgName,a.ParentOrgId,a.OrgType,a.OrgStatus,a.CancellOperateDate,a.CancellEffectDate,  1 flag, 0 permissionflag,a.updatorloginid from orcl.pg_info_org a  "
        " where a.orgid in "
        " ("
        " select orgid from org_all where orgid not in (select orgid from org_in) "
        " )"
        " union all "
        " select s.OrgId,s.OrgName,s.ParentOrgId,s.OrgType,s.OrgStatus,s.CancellOperateDate,s.CancellEffectDate,  1 flag, 1 orgpermissionflag,s.updatorloginid from orcl.pg_info_org s "
        " where s.orgid in (select orgid from org_in) "
        " ) k left join orcl.pg_attribute_login f on k.updatorloginid=f.loginaccountid "
        );
        streams[strs].open(0,
          "select count(*) from (select a.TradeDate TradeDate, a.CMUserId CMUserId, a.CMCustomerNum CustomerNum, a.Commodityid Commodityid, a.CommodityCode CommodityCode, a.CommodityName CommodityName, a.COMPREUSERBUYQUANTITY s1,a.COMPREUSERBUYPL s2,a.COMPREUSERSELLQUANTITY s3,a.COMPREUSERSELLPL s4,a.COMPREUSERSUMPL s5,a.COMPREUSERLATEFEE s6, b.CumulateBUYQUANTITY s7,b.CumulateBUYPL s8,b.CumulateSELLQUANTITY s9,b.CumulateSELLPL s10,b.CumulateSumPL s11,b.CumulateLateFee s12 from rs_detail_cmhold a left join rs_detail_customerofcmhold b on a.TradeDate=b.TradeDate and a.CommodityId=b.CommodityId and a.CMUserId=b.cmuserid where a.OrderType=1 and a.StatictiscType=2 and a.TradeDate >= :BeginDate_0<bigint,in> and a.TradeDate <= :EndDate_0<bigint,in> union select b.TradeDate TradeDate, b.CMUserId CMUserId, b.CMCustomerNum CustomerNum, b.Commodityid Commodityid, b.CommodityCode CommodityCode, b.CommodityName CommodityName, a.COMPREUSERBUYQUANTITY s1,a.COMPREUSERBUYPL s2,a.COMPREUSERSELLQUANTITY s3,a.COMPREUSERSELLPL s4,a.COMPREUSERSUMPL s5,a.COMPREUSERLATEFEE s6, b.CumulateBUYQUANTITY s7,b.CumulateBUYPL s8,b.CumulateSELLQUANTITY s9,b.CumulateSELLPL s10,b.CumulateSumPL s11,b.CumulateLateFee s12 from rs_detail_customerofcmhold b left join rs_detail_cmhold a on b.TradeDate=a.TradeDate and b.CommodityId=a.CommodityId and b.CMUserId=a.cmuserid and a.OrderType=1 where b.StatictiscType=2 and b.TradeDate >= :BeginDate_1<bigint,in> and b.TradeDate <= :EndDate_1<bigint,in>) T left join (select * from rs_detail_netpositionofcmhold where TradeDate >= :BeginDate_2<bigint,in> and TradeDate <= :EndDate_2<bigint,in> and StatictiscType=2 ) d on T.commodityid=d.commodityid and T.CMUserId=d.CMUserId and T.TradeDate=d.TradeDate"
          );
        streams[strs].open(0,
          "select v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,c.NetPositionDirection, c.NetPositionQUANTITY, c.NetPositionPL from (select a.Commodityid cmdty, a.TradeDate TradeDate, a.CommodityCode v1, a.CommodityName v2, a.COMPREUSERBUYQUANTITY v3, a.COMPREUSERBUYPL v4, a.COMPREUSERSELLQUANTITY v5, a.COMPREUSERSELLPL v6, a.COMPREUSERSUMPL v7, a.COMPREUSERLATEFEE v8, b.CUMULATEBUYQUANTITY v9, b.CUMULATEBUYPL v10, b.CUMULATESELLQUANTITY v11, b.CUMULATESELLPL v12, b.CUMULATESUMPL v13, b.CUMULATELATEFEE v14 from rs_sum_cmhold a left join rs_sum_customerofcmhold b on a.TradeDate=b.TradeDate and a.CommodityId=b.CommodityId where a.OrderType=1 and a.SumType <> 0 and a.TradeDate >= :BeginDate_0<bigint,in> and a.TradeDate <= :EndDate_0<bigint,in> union select b.Commodityid cmdty, b.TradeDate TradeDate, b.CommodityCode v1, b.CommodityName v2, a.COMPREUSERBUYQUANTITY v3, a.COMPREUSERBUYPL v4, a.COMPREUSERSELLQUANTITY v5, a.COMPREUSERSELLPL v6, a.COMPREUSERSUMPL v7, a.COMPREUSERLATEFEE v8, b.CUMULATEBUYQUANTITY v9, b.CUMULATEBUYPL v10, b.CUMULATESELLQUANTITY v11, b.CUMULATESELLPL v12, b.CUMULATESUMPL v13, b.CUMULATELATEFEE v14 from rs_sum_customerofcmhold b left join rs_sum_cmhold a on b.TradeDate=a.TradeDate and b.CommodityId=a.CommodityId and a.OrderType=1 where b.SumType <> 0 and b.TradeDate >= :BeginDate_1<bigint,in> and b.TradeDate <= :EndDate_1<bigint,in> ) T left join rs_sum_netpositionofcmhold c on T.cmdty=c.Commodityid and T.TradeDate=c.TradeDate"
          );
        streams[strs].open(0,
          "select count(*) from ( select comm.usertype usertype, comm.customernum customernum, comm.customername customername, case comm.lbankid when 0 then (select BankName from bk_bank_info where BankInfoID=comm.rbankid) else (select BankName from bk_bank_info where BankInfoID=comm.lbankid) end bankname, comm.lsum + comm.rsum total, comm.lsum member_get, comm.rsum exchange_get, ifnull(a.salveuserid, comm.userid) relateid from ( select part_left.usertype, part_left.userid, part_left.customernum, part_left.customername, part_left.sum lsum, ifnull(part_left.bankid,0) lbankid, part_right.sum rsum, ifnull(part_right.bankid,0) rbankid from (select c.usertype usertype, c.userid userid, d.customernum customernum, d.customername customername, member_part_commission.sum sum, member_part_commission.bankid bankid from cm_info_customer d, cm_info_user c left join (select ifnull(sum(b.amount), 0) sum, b.payeeuserid memberid, b.payerbankid bankid from cm_rl_user2user a, st_log_settlefeeassign b where a.RelationType in(1002, 1005) and b.payeruserid=a.masteruserid and b.feetype=1 and b.payeeuserid=a.salveuserid and a.relationstatus=1 group by b.payeeuserid, b.payerbankid ) member_part_commission on c.userid=member_part_commission.memberid where c.usertype in(1003, 1004) and d.customerid=c.customerid ) part_left left join (select h.userid userid, exchange_part_commission.sum sum, exchange_part_commission.bankid bankid from cm_info_user h left join (select ifnull(sum(f.amount), 0) sum, e.payeeuserid memberid, e.payerbankid bankid from cm_rl_user2user d, st_log_settlefeeassign e, st_log_settlefeeassign f, cm_info_user g where d.RelationType in(1002, 1005) and e.payeruserid=d.masteruserid and e.feetype=1 and e.payeeuserid=d.salveuserid and d.relationstatus=1 and e.orderno=f.orderno and f.feetype=1 and f.payeeuserid=g.userid and g.usertype=1007 group by e.payeeuserid, e.payerbankid ) exchange_part_commission on h.userid=exchange_part_commission.memberid where h.usertype in(1003, 1004) ) part_right on part_left.userid=part_right.userid and part_left.bankid=part_right.bankid union select part_right.usertype, part_right.userid, part_right.customernum, part_right.customername, part_left.sum lsum, ifnull(part_left.bankid,0) lbankid, part_right.sum rsum, ifnull(part_right.bankid,0) rbankid from (select c.usertype usertype, c.userid userid, d.customernum customernum, d.customername customername, exchange_part_commission.sum sum, exchange_part_commission.bankid bankid from cm_info_customer d, cm_info_user c left join (select ifnull(sum(f.amount), 0) sum, e.payeeuserid memberid, e.payerbankid bankid from cm_rl_user2user d, st_log_settlefeeassign e, st_log_settlefeeassign f, cm_info_user g where d.RelationType in(1002, 1005) and e.payeruserid=d.masteruserid and e.feetype=1 and e.payeeuserid=d.salveuserid and d.relationstatus=1 and e.orderno=f.orderno and f.feetype=1 and f.payeeuserid=g.userid and g.usertype=1007 group by e.payeeuserid, e.payerbankid ) exchange_part_commission on c.userid=exchange_part_commission.memberid where c.usertype in(1003, 1004) and d.customerid=c.customerid ) part_right left join (select h.userid userid, member_part_commission.sum sum, member_part_commission.bankid bankid from cm_info_user h left join (select ifnull(sum(b.amount), 0) sum, b.payeeuserid memberid, b.payerbankid bankid from cm_rl_user2user a, st_log_settlefeeassign b where a.RelationType in(1002, 1005) and b.payeruserid=a.masteruserid and b.feetype=1 and b.payeeuserid=a.salveuserid and a.relationstatus=1 group by b.payeeuserid, b.payerbankid ) member_part_commission on h.userid=member_part_commission.memberid where h.usertype in(1003, 1004) ) part_left on part_right.userid=part_left.userid and part_right.bankid=part_left.bankid ) comm left join cm_rl_user2user a on a.relationstatus=1 and a.RelationType=1004 and a.masteruserid=comm.userid order by relateid, usertype )temp1 where 1=:f1<int>"
        );
        streams[strs].open(0,
          "select count(*) from cm_info_customer b, cm_info_user c, bk_bank_info d, (select tmp1.PayerUserID, tmp1.PayerBankID, tmp1.custComm, tmp1.custCommEx, tmp1.HPAmount, h.HAmout from ( select nvl(cust.SalveUserId, hp.PayerUserID) PayerUserID, nvl(cust.PayerBankID, hp.PayerBankID) PayerBankID, cust.custComm, cust.custCommEx, hp.HPAmount from (select a.PayerBankID, d.SalveUserId, sum(a.Amount) custComm, sum(case when c.usertype=1007 then a.Amount else 0.0 end) custCommEx from st_log_settlefeeassign a, cm_info_user b, cm_info_user c, cm_rl_user2user d where a.PayerUserID=d.MasterUserId and d.RelationStatus=1 and d.RelationType=1002 and a.PayerUserID=b.userid and b.usertype=1002 and a.PayeeUserID=c.userid and c.usertype in(1004,1003,1007) and a.TradeMode=10 and a.FeeType=1 group by a.PayerBankID, d.SalveUserId ) cust left join (select a.PayerBankID, a.PayerUserID, sum(a.Amount) HPAmount from st_log_settlefeeassign a, cm_info_user b, cm_info_user c where a.PayerUserID=b.userid and b.usertype=1003 and a.PayeeUserID=c.userid and c.usertype in(1006,1007) and a.TradeMode=10 and a.FeeType in(1) group by a.PayerBankID, a.PayerUserID ) hp on cust.PayerBankID=hp.PayerBankID and cust.SalveUserId=hp.PayerUserID union select nvl(cust.SalveUserId, hp.PayerUserID) PayerUserID, nvl(cust.PayerBankID, hp.PayerBankID) PayerBankID, cust.custComm, cust.custCommEx, hp.HPAmount from (select a.PayerBankID, a.PayerUserID, sum(a.Amount) HPAmount from st_log_settlefeeassign a, cm_info_user b, cm_info_user c where a.PayerUserID=b.userid and b.usertype=1003 and a.PayeeUserID=c.userid and c.usertype in(1006,1007) and a.TradeMode=10 and a.FeeType in(1) group by a.PayerBankID, a.PayerUserID ) hp left join (select a.PayerBankID, d.SalveUserId, sum(a.Amount) custComm, sum(case when c.usertype=1007 then a.Amount else 0.0 end) custCommEx from st_log_settlefeeassign a, cm_info_user b, cm_info_user c, cm_rl_user2user d where a.PayerUserID=d.MasterUserId and d.RelationStatus=1 and d.RelationType=1002 and a.PayerUserID=b.userid and b.usertype=1002 and a.PayeeUserID=c.userid and c.usertype in(1004,1003,1007) and a.TradeMode=10 and a.FeeType=1 group by a.PayerBankID, d.SalveUserId ) cust on hp.PayerBankID=cust.PayerBankID and hp.PayerUserID =cust.SalveUserId ) tmp1 left join (select a.PayerBankID, a.PayerUserID, sum(a.Amount) HAmout from st_log_settlefeeassign a, cm_info_user b, cm_info_user c where a.PayerUserID=b.userid and b.usertype=1003 and a.PayeeUserID=c.userid and c.usertype=1007 and a.TradeMode=10 and a.FeeType=1 group by a.PayerBankID, a.PayerUserID ) h on tmp1.PayerBankID=h.PayerBankID and tmp1.PayerUserID=h.PayerUserID )tmp2 where tmp2.PayerBankID=d.BankInfoID and tmp2.PayerUserID=c.UserId and c.CustomerId=b.CustomerId "
        );
    }
    {
      printd("test 13\n");
      cnn.change_db("test_db");
      streams[strs].open(0,"select  SEQ,USERID,CHANGETYPE,AMOUNT,CREATETIME,ID, "
      "LOGINID,SOURCE, ACCOUNTID from orcl.ac_cust_amtchange_rec WHERE "
      "ROWNUM <= :f1<int> and (status <> 1 or status is null) "
      "ORDER BY CREATETIME ASC");
      streams[strs].open(0,"select id, name from "
        "test_db.test_tbl where id>=1 and rownum<7 and rownum<8");
      streams[strs].open(0,"select id, rownum, (select name from "
        "test_db.test_tbl where id=1 and rownum<6)a from test_db.test_tbl where "
        "id in (select id from test_db.test_tbl where rownum<7 and id=1)");
      streams[strs].open(0,"select id, (select name from "
        "/*test_db.*/test_tbl where id=1 and rownum<6)a from /*test_db.*/test_tbl where "
        "id in (select id from /*test_db.*/test_tbl where rownum<7 and id=1) "
        "and rownum <9");
      streams[strs].open(0,"select usr.userid, usr.usertype from (select u.userid, "
        "u.usertype, rownum rn from orcl.cm_info_user u where rownum<8) usr ");
      streams[strs].open(0,"select usr.userid, usr.usertype from (select u.userid, "
        "rownum rn1, u.usertype from orcl.cm_info_user u where rownum<7) usr");
      streams[strs].open(0,"select usr.userid, usr.usertype from (select u.userid, "
        "rownum , u.usertype from orcl.cm_info_user u where rownum<7) usr ");
      streams[strs].open(0,"select u.userid, rownum , u.usertype from "
        "orcl.cm_info_user u where rownum<7");
      streams[strs].open(0,"select usr.UserId, usr.UserType from (select u.UserId, "
        "u.UserType, RowNum RN from orcl.cm_info_user u /*left*/inner join "
        "orcl.st_status_usersettle s "
        "on u.UserId = s.UserId where u.TradeMode = :f1<int> and "
        "u.UserType = :f2<int> and (s.Status is NULL or (s.NextSettlementDay "
        "in(:f3<long>, 0) and s.SettlementTimeSliceBegin in(:f4<long>, 0) "
        "and s.SettlementTimeSliceEnd in(:f5<long>, 0) and s.Status != :f6<int>)) "
        "and RowNum < :f7<int>) usr where usr.rn >= :f8<int>");
      streams[strs].debug();
      streams[strs].open(0,"select t.userid, rownum rnA, t.usertype from "
        "(select u.userid, u.usertype from "
        "orcl.cm_info_user u where u.userid=1000 order by "
        "nvl(u.usertype,rownum), rownum)t, test_db.test_tbl, "
        "(select id,name from test_db.test_tbl where rownum<7)ts, "
        "orcl.cm_info_user us1 ");
      streams[strs].open(0,"select t.userid, rownum rnA, t.usertype from "
        "(select u.userid, u.usertype from "
        "orcl.cm_info_user u where u.userid=1000 order by "
        "nvl(u.usertype,rownum), rownum)t, test_db.test_tbl, "
        "(select id,name, rownum from test_db.test_tbl where rownum<22)ts, "
        "orcl.cm_info_user us1 "
        "where rownum<7");
      streams[strs].open(0,"select t.userid, t.usertype from "
        "(select u.userid, u.usertype from "
        "orcl.cm_info_user u where u.userid<1000 order by "
        "nvl(u.usertype,rownum), rownum)t, test_db.test_tbl, "
        "(select id,name, rownum from test_db.test_tbl)ts, "
        "orcl.cm_info_user us1 "
        "where rownum<7");
      streams[strs].open(0,"select t.userid, t.usertype from "
        "(select u.userid, u.usertype from "
        "orcl.cm_info_user u where u.userid=1000 order by "
        "nvl(u.usertype,rownum), rownum)t, test_db.test_tbl, "
        "(select id,name, rownum from test_db.test_tbl)ts, "
        "orcl.cm_info_user us1 ");
      streams[strs].open(0,"select u.userid, u.usertype from "
        "(select id, name, rownum from test_db.test_tbl) t1,"
        "orcl.cm_info_user u where u.userid<1");
      streams[strs].open(0,"select u.userid, rownum, u.usertype from "
        "orcl.cm_info_user u");
      streams[strs].open(0,"select u.userid, rownum rn2, u.usertype from "
        "orcl.cm_info_user u where u.userid=1000");
      streams[strs].open(0,"select t.userid, t.usertype from "
        "(select u.userid, u.usertype from "
        "orcl.cm_info_user u where u.userid=1000 order by "
        "nvl(u.usertype,rownum), rownum)t where rownum<7");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "with userid_all_1 as "
        "( "
        "select * from "
        "(select a.userid, a.customerid, a.userstatus from "
        "cm_info_user a, cm_rl_user2user b where "
        "a.userid = b.masteruserid and b.relationtype = 1005 "
        "and b.salveuserid = :UserId_0<int,in> ) users "
        "), "
        "userid_all as "
        "( "
        "select * from "
        "(select userid_all_1.* from cm_info_user a, pg_info_org b, "
        "userid_all_1 left join (select masteruserid, "
        "b.userstatus from cm_rl_user2user a, cm_info_user b, "
        "userid_all_1 u1 where a.relationtype = 1001 and "
        "u1.userid = a.masteruserid and a.salveuserid = b.userid ) e  "
        "on userid_all_1.userid = e.masteruserid "
        "where userid_all_1.userid = a.userid and "
        "a.organizationid = b.orgid and b.orgstatus = 1 and "
        "(e.masteruserid is null or e.userstatus <> 14) ) ob_user "
        "), "
        "userid_t as "
        "( "
        "select g.userid, g.loginaccount, g.customername "
        "from userid_all u, vi_info_customer g where u.userid=g.userid and "
        "g.usertype=1002 and 1=1 and 1=1 "
        "), "
        "view1 as( "
        "select f.*, ut.loginaccount, ut.customername, "
        "L1.OrderPrice TPPrice, L2.OrderPrice SLPrice, qr.buyprice, "
        "qr.sellprice, cd.agreeunit, cd.weightunit, cd.commoditymode, "
        "cd.transferrate, cd.specificationrate from userid_t ut, "
        "ts_order_holdposition f, (select a.commodityid, a.buyprice, "
        "a.sellprice from cd_quote_realtime a, sd_market_info b "
        "where a.tradedate=b.tradedate) qr, TS_ORDER_LIMIT L1, "
        "TS_ORDER_LIMIT L2, cd_info_commodity cd where f.UserId=ut.userid "
        "and f.commodityid=qr.commodityid(+) and f.commodityid=cd.commodityid "
        "and f.holdpositionid=L1.holdpositionid(+) and "
        "L1.LimitType(+)=2 and L1.DealStatus(+)=1 and "
        "f.holdpositionid=L2.holdpositionid(+) and L2.LimitType(+)=3 and "
        "L2.DealStatus(+)=1 and f.OrderStatus=1 and 1=1 and 1=1 "
        "and 1=1 and 1=1 "
        ") "
        "select v1, v2, v3, v4, v5, v6, v7, v8, "
        "v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, "
        "v20, v21, v22, v23, v24 from "
        "(select rownum rn, v1, v2, "
        "v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, "
        "v16, v17, v18, v19, v20, v21, v22, v23, v24 from "
        "(select a.holdpositionid v1,a.loginaccount v2, a.customername v3,"
        "a.commission*a.holdquantity/a.buyprice+b.fee, a.holdquantity, " /* yzhou add */
        "nvl(a.loginaccount, '') v4,a.OpenType v5, a.commodityid v6, "
        "a.opendirector v7, a.holdquantity v8, a.openprice v9, "
        "a.holdpositionprice v10, b.fee v11, "
        "(case when a.openquantity=0 then 0 "
        "else (-1)*a.commission*a.holdquantity/a.openquantity end) v12, "
        "a.opendate v13, 0 v14, 0.0 v15, 0 v16, 0.0 v17, a.TPPrice v18, "
        "a.SLPrice v19, "
        "(case when a.opendirector=2 then "
        "(a.holdpositionprice - a.buyprice)*a.holdquantity*a.agreeunit "
        "when a.opendirector=1 then (a.sellprice - a.holdpositionprice)* "
        "a.holdquantity*a.agreeunit end) v20, "
        "(case when a.opendirector=2 then a.buyprice when a.opendirector=1 "
        "then a.sellprice end) v21, "
        "a.memberid v22, a.agreeunit*a.holdquantity*a.specificationrate/to_number( "
        "a.transferrate) v23, c.salveuserid v24 from view1 a,"
        "(select d.HoldPositionId, sum(latefee) fee from view1 c, "
        "ts_order_holdposition_his d where c.holdpositionid=d.holdpositionid(+) "
        "group by d.HoldPositionId) b, cm_rl_user2user c where "
        "a.holdpositionid=b.HoldPositionId(+) and a.userid=c.masteruserid(+) and "
        "c.relationtype(+)=1005 order by nvl(v1,rownum) desc,rownum ) temp1 "
        "WHERE 1=1) where 1=1 ");
      streams[strs].open(0,
        "select v1, v2, v3, v4, v5, v6, v7, v8 "
        "from( select rownum rn, v1, v2, v3, v4, v5, v6, v7, v8 "
        "from( select * from ( select c.customernum v1, c.customername v2, "
        "a.applydate v3,  "
        "( case when e.loginaccount is not null then e.loginaccount "
        "when e1.loginaccount is not null and e1.loginaccountid <> -1 "
        "then e1.loginaccount when e2.loginaccount is not null then e2.loginaccount "
        "else null end ) v4, "
        "d.bankname v5, (case when a.main=1 then '1' else '0' end) v6, "
        "a.amount v7, (case when a.changetype is null then 2 "
        "when a.changetype=0 then 2 when a.changetype=4 then 2 "
        "else a.changetype end) v8 from ac_bank_signinfo_his a "
        "left join (select a.updatedate, a.customerid, a.main,b.loginaccount "
        "from ac_bank_signinfo_tmp a, pg_info_login b where "
        "a.updatorloginid=b.loginaccountid) e "
        "on a.applydate=e.updatedate and a.customerid=e.customerid "
        "left join pg_info_login e1 on a.updatorloginid=e1.loginaccountid "
        "left join pg_info_login e2 on a.creatorloginid=e2.loginaccountid "
        ",cm_info_user b, cm_info_customer c, bk_bank_info d "
        "where a.customerid=b.customerid and a.customerid=c.customerid "
        "and 1=1  and a.applydate <= :EndDate_0<bigint,in> + 86400000000 "
        "and a.bankid=d.bankinfoid and b.userid = :MultipleMemberId_0<int,in> "
        "and b.usertype=1003 "
        "union "
        "select c.customernum v1, c.customername v2, a.createdate v3, "
        "e.loginaccount v4, d.bankname v5, "
        "(case when a.main=1 then '1' else '0' end) v6, 0.0 v7, a.ispass v8 "
        "from ac_bank_signinfo_tmp a left join pg_info_login e "
        "on a.updatorloginid=e.loginaccountid,cm_info_user b, cm_info_customer c, "
        "bk_bank_info d where a.customerid=b.customerid and a.customerid=c.customerid "
        "and a.bankid=d.bankinfoid and a.createdate <= :EndDate_1<bigint,in> "
        "+ 86400000000 and a.ispass=0 and b.userid = :MultipleMemberId_1<int,in> "
        "and b.usertype=1003 ) order by nvl(1,rownum) desc,rownum) temp1 "
        "WHERE ROWNUM <= :EndRow_0<int,in> )where RN >= :BeginRow_0<int,in> ");
      streams[strs].open(0,
        "select v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19 "
        "from( select rownum rn, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19 "
        "from(select TradeDate v1, BankName v2, BankAcct v3, UserId v4, UserType v5, "
        "CustomerName v6,PreDayBalance v7, CashO v8, CashI v9, ProfitOrLoss v10, "
        "Commision v11,LateFee v12,NetProfitOrLoss v13, DeliveryPayment v14, "
        "DeliveryComm v15, InterestSet v16, AgentInterestTax v17, Balance v18, "
        "LoginAccount v19 from rs_bank_exceptionalcust "
        "where TradeDate >= :BeginDate_0<bigint,in> and "
        "TradeDate <= :EndDate_0<bigint,in> and BankId = :BankId_0<int,in> "
        "order by nvl(v1,rownum) desc,rownum ) temp1  WHERE ROWNUM <= :EndRow_0<int,in> ) "
        "where RN >= :BeginRow_0<int,in> ");
      streams[strs].open(0,
       "select count(*)from (select ff.HoldPositionId HoldPositionId,"
       "f.opendate opendate, ff.commodityid commodityid,a.commodityname commodityname,"
       "ff.closequantity closequantity, f.opendirector opendirector, ff.openprice openprice,"
       "f.holdpositionprice holdpositionprice, ff.ClosePositionId  ClosePositionId, "
       "ff.closedate closedate, ff.closedirector closedirector, ff.closeprice closeprice,"
       "ff.commission commission , case f.OpenDirector when 1 then (ff.closeprice - f.holdpositionprice) "
       "* ff.closequantity * a.AgreeUnit else (f.holdpositionprice - ff.closeprice) * ff.closequantity * a.AgreeUnit "
       "end  POL from ts_order_holdposition f, ts_order_closeposition ff, cd_info_commodity a "
       "where ff.holdpositionid = f.holdpositionid and a.commodityid = ff.commodityid "
       "and ff.userid = :KEY<int> )temp1 ");
      streams[strs].open(0,
       "select  temp1.HoldPositionId v1, temp1.opendate v2,  temp1.commodityid v3, "
       "temp1.commodityname v4,  temp1.closequantity v5, "
       "temp1.opendirector v6, temp1.openprice v7, temp1.holdpositionprice v8,"
       "temp1.ClosePositionId v9,temp1.closedate v10, temp1.closedirector v11,"
       "temp1.closeprice v12, temp1.commission v13, temp1.POL v14, 0.0 v15 "
       "from (  select "
           "ff.HoldPositionId HoldPositionId, "
           "f.opendate opendate, "
           "ff.commodityid commodityid, "
           "a.commodityname commodityname, "
           "ff.closequantity closequantity, "
           "f.opendirector opendirector, "
           "ff.openprice openprice, "
           "f.holdpositionprice holdpositionprice, ff.ClosePositionId  ClosePositionId,  ff.closedate closedate, ff.closedirector closedirector,  ff.closeprice closeprice, ff.commission commission, case f.OpenDirector when 1 then (ff.closeprice - f.holdpositionprice) * ff.closequantity * a.AgreeUnit else (f.holdpositionprice - ff.closeprice) * ff.closequantity * a.AgreeUnit end  POL from ts_order_holdposition f, ts_order_closeposition ff, cd_info_commodity a  where ff.holdpositionid = f.holdpositionid and a.commodityid = ff.commodityid  and ff.userid = :KEY<INT> )temp1  order by holdpositionprice, v1 limit 0,1 "
       );
      cnn.change_db("test_db");
    }
    {
      printd("test 14\n");
#if 1
      cnn.change_db("orcl");
      streams[strs].open(0,
        " select to_number(b.ParamValue) /*Limit*/ "
        " from sd_market_param_group a, SD_MARKET_PARAM_ITEM b "
        " where a.ParamGroupId = b.ParamGroupId and a.ParamGroupCode = :GroupCode<char[128]> "         
        " and a.Status = :EnabledStatus<int> and b.Status = :EnabledStatus<int> "
        " and b.ParamType = :ParamType<int> and ( "
        "    (b.ObjectType = :NotLimited<int> and b.ObjectId = :AnyId<char[1024]>) or "
        "    (b.ObjectType = :ForCommodity<int> and b.ObjectId = to_char(:CommodityId<int>)) )");
      cnn.change_db("test_db");
#endif
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
        "select a.CommodityCode v1, a.CommodityName v2, a.DeliveryQuantity v3, "
        "a.DeliveryPayment v4, a.DeliveryCommision v5, b.CommodityName v6, "
        "c.AgreeUnit * a.DeliveryQuantity * c.specificationrate "
        " / to_number(transferrate)*to_number(a.commodityMode) v7, "
        "a.CommodityMode v8, "
        "c.AgreeUnit v9 from orcl.rs_sum_deliveryorder a, "
        "orcl.cd_info_deliverycommodity b, orcl.cd_info_commodity c "
        "where a.TradeDate >= :BeginDate_0<bigint,in> and "
        "a.TradeDate <= :EndDate_0<bigint,in> and a.SumType <> 0 "
        "and a.DeliveryCommodityId=b.CommodityId and a.CommodityId=c.CommodityId "
        "order by a.CommodityMode, a.CommodityName");
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>)) from dual where 1=1");
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>,3)) from dual where 1=1");
      streams[strs].open(0,
        "select To_number(dbms_lob.substr("
        ":OB0X_0<char[8192],in>,12345,20),'99') from dual where 1=1");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "select count(*) from cd_info_commodity a where regexp_like(a.CommodityName,  :CommodityName_0<char[64],in> /*, 'i'*/) or a.CommodityName is NULL");
      streams[strs].open(0,
        "select count(*) from cd_info_commodity a where a.CommodityName regexp binary :CommodityName_0<char[64],in>  or a.CommodityName is NULL");
      cnn.change_db("test_db");
      /* TODO: find a way to process the regexp. maybe the 
       *   APIs in regexp.c ? */
#if 0
      streams[strs].open(0,
        "select To_number(dbms_lob.substr(regexp_substr( "
        ":OB0X_0<char[8192],in>, '[^,]+', 1, x.n )),'999') from dual a, "
        //"(select rownum n from dual connect by rownum < 200) x",
        "(select rownum n from dual where rownum < 200) x");
      streams[strs].debug();
      return 0;
#endif
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
          "select a.TradeDate,a.CommodityId,a.CommodityCode,"
          "a.CommodityName,a.COMPREUSERBUYQUANTITY,a.COMPREUSERBUYPL,"
          "a.COMPREUSERSELLQUANTITY,a.COMPREUSERSELLPL,a.COMPREUSERSUMPL,"
          "b.CUMULATEBUYQUANTITY,b.CUMULATEBUYPL,b.CUMULATESELLQUANTITY,"
          "b.CUMULATESELLPL,b.CUMULATESUMPL "
          "from orcl.rs_sum_cmclose a, orcl.rs_sum_customerofcmclose b " 
          "where a.TradeDate=b.TradeDate and a.CommodityId=b.CommodityId(+) " 
          "and a.OrderType=2 and a.SumType > 0 "
          "union "
          "select a.TradeDate,a.CommodityId,a.CommodityCode,"
          "a.CommodityName,a.COMPREUSERBUYQUANTITY,a.COMPREUSERBUYPL,"
          "a.COMPREUSERSELLQUANTITY,a.COMPREUSERSELLPL,a.COMPREUSERSUMPL,"
          "b.CUMULATEBUYQUANTITY,b.CUMULATEBUYPL,b.CUMULATESELLQUANTITY,"
          "b.CUMULATESELLPL,b.CUMULATESUMPL "
          "from orcl.rs_sum_cmclose a, orcl.rs_sum_customerofcmclose b " 
          "where a.TradeDate=b.TradeDate and a.CommodityId=b.CommodityId(+) " 
          "and a.OrderType=1 and a.SumType <> 0"
          );
      streams[strs].open(0, 
          "select  b.Amount+nvl(ar.sa,0)  "
          "from orcl.ac_account_sub a, orcl.ac_book_subaccount b, "
          "(select userid, sum(amount) sa from "
          "orcl.ac_cust_amtchange_rec group by userid) ar  "
          "where a.main=:f0<int> and a.SubAccountId = b.SubAccountId "
          "and b.BookTypeId = :f1<int> " 
          "and exists (select 1 from orcl.cm_info_user c where  "
          "c.userid=a.userid and c.customerid=:f2<int> and "
          "c.trademode=:f3<int> and c.UserStatus<>13) and a.UserId = ar.userid(+) "
          "and a.userid<>0 ");
      streams[strs].open(0, 
          "select  b.Amount+nvl(ar.sa,0)  "
          "from orcl.ac_account_sub a, orcl.ac_book_subaccount b, "
          "(select userid, sum(amount) sa from "
          "orcl.ac_cust_amtchange_rec group by userid) ar  "
          "where a.main=:f0<int> and a.SubAccountId = b.SubAccountId "
          "and b.BookTypeId = :f1<int> " 
          "and exists (select 1 from orcl.cm_info_user c where  "
          "c.userid=a.userid and c.customerid=:f2<int> and "
          "c.trademode=:f3<int> and c.UserStatus<>13) and a.UserId = ar.userid(+) "
          "group by a.userid ");
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
      streams[strs].open(0, 
        "with users_cmdy as(select a.userid, b.commodityid from cm_info_user a, cd_info_commodity b where usertype = 1003), "
        "distinct_userid as(select distinct (userid) userid from users_cmdy) "
        "select distinct_userid.userid, "
        "temp3.amount,temp3.FreezenThreshold,temp3.WarnThreshold,temp3.PostThreshold,MG.margin,temp4.close_pl, temp5.close_pl_member "
        "from distinct_userid, "
        "(select /*+ index(c,IDX_CM_ATTRIBUTE_MEMBER_UID) */ user_id, "
        "sum( abs((a.v1-a.v5)-(a.v2-a.v6)) * " //净头寸
        "( "
        "case   when (a.v1-a.v5)-(a.v2-a.v6)>0 and ((a.v3+a.v7)>0 and (a.v1+a.v5)>0) then (a.v3+a.v7)/(a.v1+a.v5) " //净头寸为买，且买单总额>0，且买单量>0，则按买方向算
        "       when (a.v1-a.v5)-(a.v2-a.v6)>0 and ((a.v3+a.v7)=0 or (a.v1+a.v5)=0) and ((a.v4+a.v8)>0 and (a.v2+a.v6)>0) then (a.v4+a.v8)/(a.v2+a.v6) " //净头寸为买，但(买单总额=0或买单量=0)，且卖单总额>0且卖单量>0则按卖方向算
        "       when (a.v1-a.v5)-(a.v2-a.v6)<0 and ((a.v4+a.v8)>0 and (a.v2+a.v6)>0) then (a.v4+a.v8)/(a.v2+a.v6) " //净头寸为卖，且卖单总额>0，且卖单量>0，则按卖方向算
        "       when (a.v1-a.v5)-(a.v2-a.v6)<0 and ((a.v4+a.v8)=0 or (a.v2+a.v6)=0) and ((a.v3+a.v7)>0 and (a.v1+a.v5)>0) then (a.v3+a.v7)/(a.v1+a.v5) " //净头寸为卖，但(卖单总额=0或卖单量=0)，且买单总额>0且买单量>0则按买方向算
        "       else 0.0 "
        "end "
        ") * b.Value / nullif(c.RishRate,0) ) margin "
        "from ( "
        "    select a1.commodityid v0,a1.user_id, "
        "    nvl(a1.s1, 0) v1,nvl(a1.s2, 0) v2,nvl(a1.s3, 0) v3,nvl(a1.s4, 0) v4, "
        "    nvl(a2.s1, 0) v5,nvl(a2.s2, 0) v6,nvl(a2.s3, 0) v7,nvl(a2.s4, 0) v8 "
        "    from "
        "    (select /*+ index(a,IDX_ACHOLDPOSITION_STATUS_MTC) */ a.commodityid, "
        "    a.makeorderuserid user_id, "
        "    sum(a.buyholdposition) s1,sum(a.sellholdposition) s2,sum(a.buytotalamt) s3,sum(a.selltotalamt) s4 "
        "    from ac_holdposition_status a, users_cmdy b "
        "    where a.makeorderuserid = b.userid "
        "    and a.commodityid = b.commodityid "
        "    group by a.commodityid, a.makeorderuserid) a1, "
        "    (select /*+ index(a,IDX_AC_HOLDPOSITION_STATUS_TC) */ a.commodityid, "
        "    a.takeorderuserid user_id, "
        "    sum(a.buyholdposition) s1,sum(a.sellholdposition) s2,sum(a.buytotalamt) s3,sum(a.selltotalamt) s4 "
        "    from ac_holdposition_status a, users_cmdy b "
        "    where a.takeorderuserid = b.userid "
        "    and a.commodityid = b.commodityid "
        "    group by a.commodityid, a.takeorderuserid) a2 "
        "    where a1.commodityid = a2.commodityid(+) and a1.user_id = a2.user_id(+) "
#if 1
        "  union "
        "    select a2.commodityid v0,a2.user_id, "
        "    nvl(a1.s1, 0) v1,nvl(a1.s2, 0) v2,nvl(a1.s3, 0) v3,nvl(a1.s4, 0) v4, "
        "    nvl(a2.s1, 0) v5,nvl(a2.s2, 0) v6,nvl(a2.s3, 0) v7,nvl(a2.s4, 0) v8 "
        "    from "
        "    (select /*+ index(a,IDX_ACHOLDPOSITION_STATUS_MTC) */ a.commodityid, "
        "    a.makeorderuserid user_id, "
        "    sum(a.buyholdposition) s1,sum(a.sellholdposition) s2,sum(a.buytotalamt) s3,sum(a.selltotalamt) s4 "
        "    from ac_holdposition_status a, users_cmdy b "
        "    where a.makeorderuserid = b.userid "
        "    and a.commodityid = b.commodityid "
        "    group by a.commodityid, a.makeorderuserid) a1, "
        "    (select /*+ index(a,IDX_AC_HOLDPOSITION_STATUS_TC) */ a.commodityid,a.takeorderuserid user_id, "
        "    sum(a.buyholdposition) s1,sum(a.sellholdposition) s2,sum(a.buytotalamt) s3,sum(a.selltotalamt) s4 "
        "    from ac_holdposition_status a, users_cmdy b "
        "    where a.takeorderuserid = b.userid "
        "    and a.commodityid = b.commodityid "
        "    group by a.commodityid, a.takeorderuserid) a2 "
        "    where a2.commodityid = a1.commodityid(+) and a2.user_id = a1.user_id(+) "
#endif
        "  ) a,  "
        "  (select b.commodityid, b.Value "
        "  from cd_feegroup_commodity a, cd_feeitem_commodity b "
        "  where a.ApplyUserType = 1002 "
        "  and a.IsDefault = 1 "
        "  and a.status = 1 "
        "  and a.CommodityFeeGroupId = b.CommodityFeeGroupId "
        "  and b.FeeType = 15 "
        "  and b.FeeSubType = 10) b,  "
        "  cm_attribute_member c "
        "where a.v0 = b.commodityid "
        "and a.user_id = c.userid "
        "group by user_id "
        ") MG, "
        "(select "
        " /*+ leading(e) USE_NL(e,a)  */ "  //0402增加hint
        "a.userid UserID, a.usertype, "
        "case "
        "  when b.marginfreezenthresholdflag = 0 then MarginFreezenThreshold "
        "  when a.usertype = 1003 and b.marginfreezenthresholdflag = 1 then b.lowestmarginthreshold*b.marginfreezenthreshold*0.01 "
        "  when a.usertype = 1006 and b.marginfreezenthresholdflag = 1 then b.lowestmarginthreshold*b.marginfreezenthreshold*0.01 "
        "  else -0.0 "
        "end FreezenThreshold, "
        "case "
        "  when b.MarginWarnThresholdflag = 0 then MarginWarnThreshold "
        "  when a.usertype = 1003 and b.MarginWarnThresholdflag = 1 then b.lowestmarginthreshold*b.MarginWarnThreshold*0.01 "
        "  when a.usertype = 1006 and b.MarginWarnThresholdflag = 1 then b.lowestmarginthreshold*b.MarginWarnThreshold*0.01 "
        "  else -0.0 "
        "end WarnThreshold, "
        "case "
        "  when b.MarginPostThresholdflag = 0 then MarginPostThreshold "
        "  when a.usertype = 1003 and b.MarginPostThresholdflag = 1 then b.lowestmarginthreshold*b.MarginPostThreshold*0.01 "
        "  when a.usertype = 1006 and b.MarginPostThresholdflag = 1 then b.lowestmarginthreshold*b.MarginPostThreshold*0.01 "
        "  else -0.0 "
        "end PostThreshold, "
        "a.userstatus UserStatus, " 
        "i.CustomerNum CustomerNum, "
        "(h.amount+nvl(ar.sa,0)) Amount "
        "from cm_attribute_member b, "
        "pg_info_login c, "
        "pg_rl_login2user d, "
        "ac_account_sub g, "
        "ac_book_subaccount h, "
        "cm_info_customer i, "
        "distinct_userid e, "  //0402以e为驱动
        "cm_info_user a "               
        "left join " 
        "(select "   //获得会员下客户的上日余额
        "  a.userid memberid, "
        "  CumulateBalance settlementmoney "
        "  from rs_sum_customerofcmfund a, "
        "  sd_market_info b, "
        "  distinct_userid g " //加distinct_userid 条件进来
        "  where g.userid = a.userid and a.tradedate = b.lasttradedate "
        "  and CumulatePreDayBal >0) customer_last_fund "
        "on a.userid = customer_last_fund.memberid "
        "left join (select rec.userid, sum(rec.amount) sa "
        "  from ac_mem_amtchange_rec rec ,distinct_userid "
        " where rec.userid = distinct_userid.userid "
        " group by rec.userid) ar "
        "on a.userid = ar.userid "
        "where a.userid = e.userid "  //0402增加条件
        "and a.userid = b.userid "
        "and a.userid = d.UserId and d.LoginId = c.LoginAccountId and c.LoginAccountType = 20 "
        "and a.userid = g.userid and g.subaccountid = h.subaccountid and h.booktypeid = 1001 "
        "and (a.UserType = 1003) "
        "and a.CustomerId = i.CustomerId "
        ") temp3, "
        "(select memberid UserID, sum(closeprofitloss) close_pl "
        "from ts_order_closeposition "
        "group by memberid) temp4 ,"
        "(select UserID UserID, sum(closeprofitloss) close_pl_member "
        "from ts_order_closeposition "
        "group by UserID) temp5 "
        "where distinct_userid.userid = MG.user_id(+) "
        "and distinct_userid.userid = temp3.userid(+) "
        "and distinct_userid.userid = temp4.userid(+) "
        "and distinct_userid.userid = temp5.userid(+) "
        "and 1=:fn1<int>");
    }
    {
      printd("test 18\n");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "with userid_all_1 as "
        "( select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a, cm_rl_user2user b where a.usertype = 1002 and a.userid = b.masteruserid and b.relationtype = 1002 and 1=1  and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where  exists (select 1 from pg_rl_login2org where 1=1  and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (  select distinct p.orgid from pg_info_org p start with p.orgid in ( select a.masterorgid from pg_rl_org2org a where exists ( select 1 from (  select distinct m.orgid from pg_info_org m start with m.orgid in (  select j.orgid from pg_info_org j where  exists (select 1 from pg_rl_login2org k where 1=1  and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid ) b where b.orgid=a.salveorgid ) ) connect by p.parentorgid=prior p.orgid )) ) users ), "
        "userid_all as "
        "(select * from (select userid_all_1.* from cm_info_user a, pg_info_org b, userid_all_1 left join (select masteruserid, b.userstatus from cm_rl_user2user a, cm_info_user b,userid_all_1 u1 where a.relationtype = 1001 and u1.userid = a.masteruserid and a.salveuserid = b.userid ) e  on userid_all_1.userid = e.masteruserid where userid_all_1.userid = a.userid and a.organizationid = b.orgid and b.orgstatus = 1 and (e.masteruserid is null or e.userstatus <> 14) ) ob_user ) "
        "select count(*) from (select a.LoginAccount v1,b.CustomerName v2,b.Nationality v3,c.OrganizationId v4,f.SalveUserId v5,nvl(d.SignStatus,1) v6,c.UserStatus v7,d.bankid v8,b.CustomerId v9, h.CommodityFeeGroupId v10,h.createdate v11,p1.loginaccount v12,p1.usertype v13, h.updatedate v14,p2.loginaccount v15,p2.usertype v16, c.UserId v17,a.LoginAccountId v18,j.customername v19,b.CustomerNum v20,d.BankAccount v21,d.SignAccount v22 from userid_all ua, pg_info_login a,cm_info_customer b "
        "LEFT JOIN ac_bank_signinfo d on( d.customerid=b.customerid),cm_info_user c "
        "LEFT JOIN cm_rl_user2user f on (c.UserId=f.MasterUserId and f.RelationType=1001) "
        "left join cm_info_user e on (f.SalveUserId=e.userid and e.userstatus <> 13) "
        "left join cm_info_customer j on e.customerid=j.customerid "
        "LEFT JOIN cm_info_user g on(g.UserId=c.UserId and c.UserStatus=2) "
        "LEFT JOIN cd_rl_user2feegroup h on c.userid=h.userid "
        "left join vi_info_customer p1 on h.creatorloginid=p1.loginaccountid "
        "left join vi_info_customer p2 on h.UpdatorLoginid=p2.loginaccountid, cd_feegroup_commodity i "
        "where ua.userid=c.UserId and a.customerid=b.customerid and b.customerid=c.customerid and a.LoginAccountType=10 and 1=1  and c.UserStatus<>13 and h.commodityfeegroupid=i.commodityfeegroupid and i.isdeliverygroup=0 and exists ( select 1 from pg_info_org i where i.OrgId=c.OrganizationId and 1=1 ) "
        "and exists ( "
        "select 1 from (select distinct m.orgid from pg_info_org m start with m.orgid in(select j.orgid from pg_info_org j where j.OrgStatus=1 and exists (select 1 from pg_rl_login2org k where 1=1  and k.OrganizationId=j.OrgId)) connect by m.parentorgid=prior m.orgid) n where n.orgid=c.OrganizationId) and 1=1 and 1=1 "
        "union all "
        "select a.LoginAccount v1,b.CustomerName v2,b.Nationality v3,c.OrganizationId v4,f.SalveUserId v5,nvl(d.SignStatus,1) v6,c.UserStatus v7,d.bankid v8,b.CustomerId v9, h.CommodityFeeGroupId v10,h.createdate v11,p1.loginaccount v12,p1.usertype v13,h.updatedate v14,p2.loginaccount v15,p2.usertype v16, c.UserId v17,a.LoginAccountId v18,j.customername v19,b.CustomerNum v20,d.BankAccount v21,d.SignAccount v22 "
        "from userid_all ua, pg_info_login a, cm_info_customer b "
        "LEFT JOIN ac_bank_signinfo d on( d.customerid=b.customerid),cm_info_user c "
        "LEFT JOIN cm_rl_user2user f on (c.UserId=f.MasterUserId and f.RelationType=1001) "
        "left join cm_info_user e on (f.SalveUserId=e.userid and e.userstatus <> 13) "
        "left join cm_info_customer j on e.customerid=j.customerid "
        "LEFT JOIN cm_info_user g on(g.UserId=c.UserId and c.UserStatus=2) "
        "LEFT JOIN cd_rl_user2feegroup h on c.userid=h.userid "
        "left join vi_info_customer p1 on h.creatorloginid=p1.loginaccountid "
        "left join vi_info_customer p2 on h.UpdatorLoginid=p2.loginaccountid, cd_feegroup_commodity i "
        "where ua.userid=c.UserId and a.customerid=b.customerid and b.customerid=c.customerid and a.LoginAccountType=10 and 1=1  and c.UserStatus<>13 and h.commodityfeegroupid=i.commodityfeegroupid and i.isdeliverygroup=0 and exists( "
        "select 1 from (select r.orgid from (select distinct p.orgid, p.OrgName, p.ParentOrgId, p.OrgType, 2 flag from pg_info_org p start with p.orgid in (select a.masterorgid from pg_rl_org2org a where a.relationtype=1001 and exists (select 1 from (select distinct m.orgid from pg_info_org m start with m.orgid in (select j.orgid from pg_info_org j where j.OrgStatus=1 and 1=1  ) connect by m.parentorgid=prior m.orgid) b where b.orgid=a.salveorgid))connect by p.parentorgid=prior p.orgid) r) y where y.orgid =c.OrganizationId) "
        "and 1=1 and 1=1 and 1=:f1<int>) ");
      streams[strs].open(0,
        "with org_in as "
        " (  "
        " select a.OrgId from orcl.pg_info_org a "
        " where  a.OrgType in (1,2) and a.OrgStatus <>3  and a.CustomerId=:f1<int> "   
        " and exists (  "
        " select 1 from ( " 
        " select distinct  m.orgid  from orcl.pg_info_org m start with m.orgid in  " 
        " (   "
        " select j.orgid  from orcl.pg_info_org j where   j.OrgStatus<>3 "   
        " and exists (select 1 from orcl.pg_rl_login2org  k where  k.loginid=:f2<int> and k.OrganizationId=j.OrgId)  " 
        " ) connect by m.parentorgid=prior m.orgid) n where n.orgid=a.OrgId)  "
        " ),"
        "org_all as "
        " (select distinct  m.orgid  from orcl.pg_info_org m where m.OrgType in (1,2) and m.OrgStatus <>3 and m.CustomerId=:f1<int> start with m.orgid =   "
        " (   "
        " select orgid from orcl.pg_attribute_login where LoginAccountId=:f2<int> "
        " ) connect by m.parentorgid=prior m.orgid "
        " )"
        "select k.OrgId,k.OrgName,k.ParentOrgId,k.OrgType,k.OrgStatus,k.CancellOperateDate,k.CancellEffectDate,f.loginusername, k.flag,  k.permissionflag from ( "
        " select a.OrgId,a.OrgName,a.ParentOrgId,a.OrgType,a.OrgStatus,a.CancellOperateDate,a.CancellEffectDate,  1 flag, 0 permissionflag,a.updatorloginid from orcl.pg_info_org a  "
        " where a.orgid in "
        " ("
        " select orgid from org_all where orgid not in (select orgid from org_in) "
        " )"
        " union all "
        " select s.OrgId,s.OrgName,s.ParentOrgId,s.OrgType,s.OrgStatus,s.CancellOperateDate,s.CancellEffectDate,  1 flag, 1 orgpermissionflag,s.updatorloginid from orcl.pg_info_org s "
        " where s.orgid in (select orgid from org_in) "
        " ) k left join orcl.pg_attribute_login f on k.updatorloginid=f.loginaccountid "
        );
      streams[strs]<<1<<2;
      printd("rpc: %d\n", (int)streams[strs].get_rpc());
      streams[strs].open(0,
        "with users_cmdy as (select a.userid :#<bigint>, b.commodityid :#<int> "
        "from orcl.cm_info_user a, orcl.cd_info_commodity b where usertype = 1003) "
        "select users_cmdy.userid, users_cmdy.commodityid, "
        "temp1.TakeBuyOrderAmount,temp1.TakeSellOrderAmount,temp1.TakeBuyPositionSum,temp1.TakeSellPositionSum, "
        "temp2.MakeBuyOrderAmount,temp2.MakeSellOrderAmount,temp2.MakeBuyPositionSum,temp2.MakeSellPositionSum "
        "from users_cmdy , "
        "(select "
        "sum(a.buytotalamt) TakeBuyOrderAmount,sum(a.selltotalamt) TakeSellOrderAmount,sum(a.buyholdposition) TakeBuyPositionSum,sum(a.sellholdposition) TakeSellPositionSum, "
        "a.commodityid CommodityID,a.takeorderuserid UserID "
        "from orcl.ac_holdposition_status a ,users_cmdy b "
        "where a.takeorderuserid = b.userid "
        "and a.commodityid = b.commodityid "
        "group by (a.takeorderuserid, a.commodityid)) temp1, "
        "(select "
        "sum(a.buytotalamt) MakeBuyOrderAmount,sum(a.selltotalamt) MakeSellOrderAmount,sum(a.buyholdposition) MakeBuyPositionSum,sum(a.sellholdposition) MakeSellPositionSum, "
        "a.commodityid Commodityid,a.makeorderuserid UserID "
        "from orcl.ac_holdposition_status a ,users_cmdy b "
        "where a.makeorderuserid = b.userid "
        "and a.commodityid = b.commodityid "
        "group by (a.makeorderuserid, a.commodityid)) temp2 "
        "where "
        "    users_cmdy.userid = temp1.userid(+) and users_cmdy.commodityid = temp1.commodityid(+) "
        "and users_cmdy.userid = temp2.userid(+) and users_cmdy.commodityid = temp2.commodityid(+) ") ;
      streams[strs].open(0,
        "with main_info as(select H.HOLDPOSITIONID,H.COMMODITYID,H.OPENTYPE, "
        "H.ORDERSTATUS,H.SWITCHHOLDPOSITIONID,H.OPENDIRECTOR,H.OPENPRICE, "
        "H.holdquantity,H.OPENDATE,H.HOLDPOSITIONPRICE, L1.LimitOrderId TPOdrId,"
        "L1.OrderPrice TPOdrPrice,L2.LimitOrderId SLOdrId, L2.OrderPrice SLOdrPrice,"
        "H.commission, H.MemberId, H.holdmargin ma1, 0.0 from orcl.TS_ORDER_HOLDPOSITION H "
        "left join orcl.TS_ORDER_LIMIT L1 on H.HOLDPOSITIONID=L1.HOLDPOSITIONID "
        "and L1.LimitType=2 and H.USERID=L1.USERID and L1.HOLDPOSITIONID <> 0 "
        "and L1.DealStatus=1 left join orcl.TS_ORDER_LIMIT L2 "
        "on H.HOLDPOSITIONID=L2.HOLDPOSITIONID and L2.LimitType=3 "
        "and H.USERID=L2.USERID  and L2.HOLDPOSITIONID <> 0  and L2.DealStatus=1 "
        "where H.UserId = :UserId_0<int,in> and H.Orderstatus=1 order by "
        "H.HOLDPOSITIONID) select m.*, order_latefee.fee from main_info m "
        "left join (select HoldPositionId, sum(latefee) fee from "
        "orcl.ts_order_holdposition_his group by HoldPositionId) order_latefee "
        "on m.holdpositionid=order_latefee.HoldPositionId "
        "order by m.HOLDPOSITIONID");
      streams[strs].open(0,
        "with users as( " 
        "select vic.userid, cc.customernum " 
        "from orcl.vi_info_customer vic, orcl.cm_info_customer cc " 
        "where " 
        //"vic.userid[SettleMemberId]=:KEY<INT> and vic.UserType=1006 and vic.loginaccounttype=20 " 
        "vic.userid=:KEY<INT> and vic.UserType=1006 and vic.loginaccounttype=20 " 
        "and vic.customerid=cc.customerid " 
        ") " 
        "select users.customernum, curr_amount.t_amount, total_magin.margin, " 
        "order_info.commodityid, order_info.sum_buytotalamt, order_info.sum_buyholdposition, order_info.sum_selltotalamt, order_info.sum_sellholdposition, " 
        "close_info.sum_pl " 
        "from " 
        "users, " 
        "( select a.userid, (b.amount+nvl(ar.sa,0)) t_amount from "
        "orcl.ac_account_sub a, orcl.ac_book_subaccount b, " 
        "(select userid, sum(amount) sa from orcl.ac_mem_amtchange_rec "
        "group by userid) ar " 
        "where a.subaccountid=b.subaccountid and b.booktypeid=1001 and a.userid=ar.userid(+) and a.main=1 " 
        ") curr_amount, " 
        "(select a.u1, " 
        "sum( abs((a.v1-a.v5)-(a.v2-a.v6)) * " 
        "(case when (a.v1-a.v5)-(a.v2-a.v6)>0 and ((a.v3+a.v7)>0 and (a.v1+a.v5)>0) then (a.v3+a.v7)/(a.v1+a.v5) " 
        "when (a.v1-a.v5)-(a.v2-a.v6)>0 and ((a.v3+a.v7)=0 or (a.v1+a.v5)=0) and ((a.v4+a.v8)>0 and (a.v2+a.v6)>0) then (a.v4+a.v8)/(a.v2+a.v6) " 
        "when (a.v1-a.v5)-(a.v2-a.v6)<0 and ((a.v4+a.v8)>0 and (a.v2+a.v6)>0) then (a.v4+a.v8)/(a.v2+a.v6) " 
        "when (a.v1-a.v5)-(a.v2-a.v6)<0 and ((a.v4+a.v8)=0 or (a.v2+a.v6)=0) and ((a.v3+a.v7)>0 and (a.v1+a.v5)>0) then (a.v3+a.v7)/(a.v1+a.v5) " 
        "else 0.0 " 
        "end " 
        ") * b.Value / nullif(c.RishRate,0) ) margin from " 
        "( " 
        "select a1.u1, a1.commodityid v0,nvl(a1.s1,0) v1,nvl(a1.s2,0) v2,nvl(a1.s3,0) v3,nvl(a1.s4,0) v4,nvl(a2.s1,0) v5,nvl(a2.s2,0) v6,nvl(a2.s3,0) v7,nvl(a2.s4,0) v8 " 
        "from " 
        "(select makeorderuserid u1, commodityid, sum(buyholdposition) s1,sum(sellholdposition) s2,sum(buytotalamt) s3,sum(selltotalamt) s4 " 
        "from orcl.ac_holdposition_status " 
        "group by makeorderuserid, commodityid) a1, " 
        "(select takeorderuserid u1, commodityid, sum(buyholdposition) s1,sum(sellholdposition) s2,sum(buytotalamt) s3,sum(selltotalamt) s4 " 
        "from orcl.ac_holdposition_status " 
        "group by takeorderuserid, commodityid) a2 " 
        "where a1.u1=a2.u1(+) and a1.commodityid=a2.commodityid(+) " 
        "union " 
        "select a2.u1, a2.commodityid v0,nvl(a1.s1,0) v1,nvl(a1.s2,0) v2,nvl(a1.s3,0) v3,nvl(a1.s4,0) v4,nvl(a2.s1,0) v5,nvl(a2.s2,0) v6,nvl(a2.s3,0) v7,nvl(a2.s4,0) v8 " 
        "from (select makeorderuserid u1, commodityid, sum(buyholdposition) s1,sum(sellholdposition) s2,sum(buytotalamt) s3,sum(selltotalamt) s4 " 
        "from orcl.ac_holdposition_status " 
        "group by makeorderuserid, commodityid) a1, " 
        "(select takeorderuserid u1, commodityid, sum(buyholdposition) s1,sum(sellholdposition) s2,sum(buytotalamt) s3,sum(selltotalamt) s4 " 
        "from orcl.ac_holdposition_status " 
        "group by takeorderuserid, commodityid) a2 " 
        "where a2.u1=a1.u1(+) and a2.commodityid=a1.commodityid(+) " 
        ") a, " 
        "(select b.commodityid, b.Value from orcl.cd_feegroup_commodity a, "
        "orcl.cd_feeitem_commodity b " 
        "where a.ApplyUserType=1006 and a.IsDefault=1 and a.status=1 " 
        "and a.CommodityFeeGroupId=b.CommodityFeeGroupId " 
        "and a.isdeliverygroup=0 " 
        "and b.FeeType=15 and b.FeeSubType=17) b, " 
        "orcl.cm_attribute_member c " 
        "where a.u1=c.userid and a.v0=b.commodityid " 
        "group by a.u1 " 
        ") total_magin, " 
        "(select f.takeorderuserid, f.commodityid commodityid, "
        "sum(f.buytotalamt) sum_buytotalamt, "
        "sum(f.buyholdposition) sum_buyholdposition, "
        "sum(f.selltotalamt) sum_selltotalamt, "
        "sum(f.sellholdposition) sum_sellholdposition "
        "from orcl.ac_holdposition_status f "
        "group by f.takeorderuserid, f.commodityid "
        ")order_info, "
        "(select memberid, sum(closeprofitloss) sum_pl from orcl.ts_order_closeposition "
        "group by memberid "
        ")close_info "
        "where "
        "users.userid=curr_amount.userid(+) "
        "and users.userid=total_magin.u1(+) "
        "and users.userid=order_info.takeorderuserid(+) "
        "and users.userid=close_info.memberid(+) ");
      streams[strs].open(0,
        "with userid_all_1 as "
        "( select * from "
        "(select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where  exists (select 1 from pg_rl_login2org where loginid = :LoginId_0<int,in> and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (  select distinct p.orgid from pg_info_org p start with p.orgid in (  select a.masterorgid from pg_rl_org2org a where exists ( select 1 from ( select distinct m.orgid from pg_info_org m start with m.orgid in (  select j.orgid from pg_info_org j where  exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid ) b where b.orgid=a.salveorgid ) ) connect by p.parentorgid=prior p.orgid )) ) users "
        "), "
        "userid_all as "
        "(select * from "
        "(select * from userid_all_1 ) ob_user ),"
        "orgid_all as "
        "( select * from "
        "(select distinct m.orgid from pg_info_org m where m.orgid in (:Org0_0<int,in>) union select distinct m.orgid from pg_info_org m start with m.orgid in (:OrgX0_0<int,in>) connect by m.parentorgid=prior m.orgid ) orgs ), "
        "brkid_all as "
        "( select * from "
        "(select distinct m.userid from cm_info_user m where 1=1  union select distinct m.userid from cm_info_user m where 1=1  union select distinct masteruserid userid from cm_rl_user2user start with 1=1  and relationtype = 1006 connect by salveuserid = prior masteruserid and relationtype = 1006 ) brks ), "
        "orgid_brkid_all as "
        "( select * from "
        "(select distinct userid from cm_info_user where organizationid in (select distinct m.orgid from pg_info_org m where m.orgid in (:Org0_1<int,in>) union select distinct m.orgid from pg_info_org m start with m.orgid in (:OrgX0_1<int,in>) connect by m.parentorgid=prior m.orgid ) and usertype = 1001 and userstatus = 1 ) orgs_brks ), "
        "mem_brkid_all as "
        "( select * from "
        "(select UserId from cm_info_user where usertype = 1001 and userstatus = 1 ) mems_brks ), "
        "mem_orgid_all as "
        "( select * from "
        "(select distinct m.orgid from pg_info_org m start with m.orgid in ( select j.orgid from pg_info_org j where j.OrgStatus=1 and exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_2<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid) mem_orgs ) "
        "select count(*), sum(a.amount) from vi_info_customer b, "
        "ac_bank_signinfo c, cm_user_changelog a "
        "left join pg_info_org p1 on a.oldorgid=p1.orgid "
        "left join pg_info_org p2 on a.neworgid=p2.orgid "
        "left join cm_info_user u1 on a.oldbrokerid=u1.userid "
        "left join cm_info_customer v1 on u1.customerid=v1.customerid "
        "left join cm_info_user u2 on a.newbrokerid=u2.userid "
        "left join cm_info_customer v2 on u2.customerid=v2.customerid "
        "left join pg_info_login p3 on a.creatorloginid=p3.loginaccountid "
        "where a.booktypeid=1001 and a.newuserid=b.userid and "
        "a.customerid=c.customerid and a.createdate >= :StartDate_0<bigint,in> "
        "and a.createdate <= :EndDate_0<bigint,in> and "
        "b.loginaccount = :LoginAccount_0<char[8192],in> and 1=1 "
        "and exists (select 1 from userid_all ua where ua.userid=a.newuserid or ua.userid=a.olduserid ) "
        "and a.updateflag in (1,3) and "
        "exists (select 1 from mem_orgid_all moa where moa.orgid=a.neworgid or moa.orgid=a.oldorgid ) ");
      streams[strs].open(0,
        "with userid_all as "
        "( select * from "
        "(select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where  exists (select 1 from pg_rl_login2org where loginid = :LoginId_0<int,in> and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (  select distinct p.orgid from pg_info_org p start with p.orgid in (  select a.masterorgid from pg_rl_org2org a where exists ( select 1 from ( select distinct m.orgid from pg_info_org m start with m.orgid in (  select j.orgid from pg_info_org j where  exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid ) b where b.orgid=a.salveorgid ) ) connect by p.parentorgid=prior p.orgid )) ) users ) "
        "select v1, v2, v3, v4, v5, v6, v7, v8, v9 "
        "from( select rownum rn, v1, v2, v3, v4, v5, v6, v7, v8, v9 "
        "from( select * from ( "
        "select a.loginaccount v1, a.customername v2, 0 v3, 0 v4, a.CommodityClass v5, 0 v6, 0 v7, 0.0 v8, "
        "(case "
        "when b.applieddepositrate is null then a.Value "
        "when b.applieddepositrate > b.exchangedepositrate then b.applieddepositrate "
        "else b.exchangedepositrate "
        "end) v9 "
        "from (select e.userid, a1.commodityid, c.loginaccount, f.customername, "
        "a1.CommodityClass, d1.Value from cd_info_commodity a1, "
        "cd_feeitem_commodity d1, cd_rl_user2feegroup e1,cd_feegroup_commodity f1, "
        "pg_info_login c, pg_rl_login2user e, cm_info_customer f, userid_all ua  "
        "where ua.userid=e.userid and e.loginid=c.loginaccountid and "
        "c.loginaccount = :LoginAccount_0<char[8192],in> "
        "and ua.customerid=f.customerid and 1=1 and e.userid=e1.userid "
        "and e1.CommodityFeeGroupId=d1.CommodityFeeGroupId and "
        "d1.CommodityFeeGroupId=f1.CommodityFeeGroupId and "
        "d1.CommodityId=a1.CommodityId and d1.FeeType=15 and "
        "d1.FeeSubType=10) a "
        "left join cd_rl_user2deposit b on a.userid=b.userid and "
        "a.commodityid=b.commodityid "
        "union "
        "select c.loginaccount v1, f.customername v2, a.createdate v3, a.netvalueonapplication v4, cd.commodityclass v5, a.applieddepositrate v6, a.result v7, "
        "(case "
        "when a.applieddepositrate is null then 0.0 "
        "when a.lastappliedrate > a.lastexchangerate then a.lastappliedrate "
        "else a.lastexchangerate "
        "end) v8, "
        "-1.0 v9 "
        "from cd_rl_user2deposit_his a, pg_info_login c, userid_all ua, "
        "pg_rl_login2user e, cm_info_customer f , cd_info_commodity cd "
        "where ua.userid=e.userid and e.loginid=c.loginaccountid and "
        "a.CommodityId=cd.CommodityId and "
        "c.loginaccount = :LoginAccount_1<char[8192],in> "
        "and ua.customerid=f.customerid and 1=1 and ua.userid=a.userid and "
        "a.createdate >= :StartDate_0<bigint,in> and "
        "a.createdate <= :EndDate_0<bigint,in> + 86400000000 ) "
        "order by nvl(v3,rownum) desc,rownum) temp1 WHERE "
        "ROWNUM <= :EndRow_0<int,in> )where RN >= :BeginRow_0<int,in> ") ;
      cnn.change_db("test_db");
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
      streams[strs].open(0,
        "select CommodityId, CommodityCode, CommodityName, CommoditySName, "
        "TradeMode, CommodityType, CommodityRight, AgreeUnit, Currency, "
        "MinQuoteChangeUnit, WeightRate, WeightUnit, fixedspread, MinPriceUnit, "
        "TradeRight, CommoditySequence, CommodityClass, CommodityClassName, "
        "CommodityMode, IsDisplay, CommodityGroupId, CommodityGroupName, "
        "weightstep, buyprice, sellprice, DisplayUnit, EnterMarketType, "
        "TransferRate, TradeType, specificationrate, specificationunit "
        "from (select distinct a.CommoditySequence, a.CommodityId, "
        "a.CommodityCode, a.CommodityName, a.CommoditySName, a.TradeMode, "
        "a.CommodityType, a.CommodityRight, a.AgreeUnit, a.Currency, "
        "a.MinQuoteChangeUnit, a.WeightRate, a.WeightUnit, a.CommodityClass, "
        "a.CommodityClassName, a.CommodityMode, a.IsDisplay, e.CommodityGroupId, "
        "e.CommodityGroupName, a.weightstep, qr.buyprice, qr.sellprice, a.DisplayUnit, "
        "a.EnterMarketType, a.TransferRate, a.specificationrate, a.specificationunit, "
        "CM.fixedspread, a.MinPriceUnit, c.TradeRight, a.TradeType from "
        "orcl.pg_rl_login2traderole b, orcl.pg_rl_traderole2right c, "
        "orcl.cd_info_CommodityClass e, "
        "orcl.cd_info_commodity a left join ( select distinct CommodityId, fixedspread "
        "from orcl.cd_rl_commodity2member where MemberId = 0 and MemberId <> -100 "
        "union select distinct CommodityId, fixedspread from orcl.cd_rl_commodity2member "
        "where MemberId=-100 and CommodityId not in (select distinct CommodityId "
        "from orcl.cd_rl_commodity2member where MemberId = 0 and MemberId <> -100) ) CM "
        "on a.CommodityId=CM.CommodityId left join (select a.commodityid, "
        "a.buyprice, a.sellprice from orcl.cd_quote_realtime a, orcl.sd_market_info b "
        "where a.tradedate=b.tradedate) qr on a.commodityid=qr.commodityid "
        "where b.LoginId = 0 and b.RoleId=c.RoleId and c.CommodityId=a.CommodityId "
        "and a.CommodityStatus=1 and a.commodityclass=e.commodityclass "
        "order by a.CommoditySequence asc) ");
    }
    {
      printd("test 20\n");
      cnn.change_db("orcl");
#if 1
      streams[strs].open(0,
        "with userid_all_1 as ("
        "select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a, cm_rl_user2user b where a.userid = b.masteruserid and b.relationtype = 1005 and b.salveuserid = 1) users), "
        "userid_all as ("
        "select * from (select userid_all_1.* from cm_info_user a, pg_info_org b, userid_all_1 left join (select masteruserid, b.userstatus from cm_rl_user2user a, cm_info_user  b, userid_all_1 u1 where a.relationtype = 1001 and u1.userid = a.masteruserid and a.salveuserid = b.userid) e on userid_all_1.userid = e.masteruserid where userid_all_1.userid = a.userid and a.organizationid = b.orgid and b.orgstatus = 1 and (e.masteruserid is null or e.userstatus <> 14)) ob_user),"
        "userid_t as ("
        "select /*+ leading(u) USE_NL(u, g) */ g.userid, g.loginaccount, g.customername from userid_all u, vi_info_customer g where u.userid = g.userid and g.usertype = 1002 and 1 = 1 and 1 = 1), "
        "view1 as ("
        "select /*+ leading(ut) USE_NL(ut, f) */ f.*, ut.loginaccount, ut.customername, L1.OrderPrice TPPrice, L2.OrderPrice SLPrice, qr.buyprice,qr.sellprice, cd.agreeunit, cd.weightunit, cd.commoditymode, cd.transferrate, cd.specificationrate from userid_t ut, ts_order_holdposition f, (select a.commodityid, a.buyprice, a.sellprice from cd_quote_realtime a, sd_market_info b where a.tradedate = b.tradedate) qr, TS_ORDER_LIMIT L1, TS_ORDER_LIMIT L2, cd_info_commodity cd where f.UserId = ut.userid and f.commodityid = qr.commodityid(+) and f.commodityid = cd.commodityid and f.holdpositionid = L1.holdpositionid(+) and L1.LimitType(+) = 2 and L1.DealStatus(+) = 1 and f.holdpositionid = L2.holdpositionid(+) and L2.LimitType(+) = 3 and L2.DealStatus(+) = 1 and f.OrderStatus = 1 and 1 = 1 and 1 = 1 and 1 = 1 and 1 = 1) "
        "select v1, v2, v3,v4,v5,v6,v7, v8, v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24 from (select rownum rn, v1, v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24 from (select a.holdpositionid v1,a.loginaccount v2,a.customername v3,nvl(a.loginaccount, '') v4,a.OpenType v5,a.commodityid v6,a.opendirector v7,a.holdquantity v8,a.openprice v9,a.holdpositionprice v10,b.fee v11,(case when a.openquantity = 0 then 0 else (-1) * a.commission * a.holdquantity /  a.openquantity end) v12, a.opendate v13, 0 v14, 0.0 v15, 0 v16, 0.0 v17, a.TPPrice v18, a.SLPrice v19, (case when a.opendirector = 2 then (a.holdpositionprice - a.buyprice) * a.holdquantity * a.agreeunit when a.opendirector = 1 then (a.sellprice - a.holdpositionprice) * a.holdquantity *a.agreeunit end) v20, (case when a.opendirector = 2 then a.buyprice when a.opendirector = 1 then a.sellprice end) v21, a.memberid v22, a.agreeunit * a.holdquantity * a.specificationrate / to_number(a.transferrate) v23, c.salveuserid v24 from view1 a, (select /*+ leading(c) USE_NL(c, d) */ d.HoldPositionId, sum(latefee) fee from view1 c, ts_order_holdposition_his d where c.holdpositionid = d.holdpositionid(+) group by d.HoldPositionId) b, cm_rl_user2user c where a.holdpositionid = b.HoldPositionId(+) and a.userid = c.masteruserid(+) and c.relationtype(+) = 1005  order by nvl(v1, rownum) desc, rownum) temp1 WHERE 1 = 1) where 1 = 1");
#endif
      streams[strs].open(0,
        "with userid_all_1 as ("
        "select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where exists (select 1 from pg_rl_login2org where loginid = :LoginId_0 < int, in > and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (select distinct p.orgid from pg_info_org p start with p.orgid in (select a.masterorgid from pg_rl_org2org a where exists (select 1 from (select distinct m.orgid from pg_info_org m start with m.orgid in (select j.orgid from pg_info_org j where exists (select 1 from pg_rl_login2org k where k.loginid =:LoginId_1 < int, in > and k.OrganizationId = j.OrgId)) connect by m.parentorgid = prior m.orgid) b where b.orgid = a.salveorgid)) connect by p.parentorgid = prior p.orgid))) users), "
        "userid_all as (select *from (select userid_all_1.* from cm_info_user a, pg_info_org b, userid_all_1 left join (select masteruserid, b.userstatus from cm_rl_user2user a, cm_info_user    b, userid_all_1    u1 where a.relationtype = 1001 and u1.userid = a.masteruserid and a.salveuserid = b.userid) e on userid_all_1.userid = e.masteruserid where userid_all_1.userid = a.userid and a.organizationid = b.orgid and b.orgstatus = 1 and (e.masteruserid is null or e.userstatus <> 14)) ob_user),"
        "userid_t as ("
        "select g.userid, g.loginaccount, g.customername from userid_all u, vi_info_customer g where u.userid = g.userid and g.usertype = 1002 and 1 = 1 and g.loginaccount = :LoginAccount_0 < char [ 8192 ], in >) select v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,v14, v15, v16, v17, v18, v19,v20,v21,v22,v23,v24,v25 from (select rownum rn,v1,v2,v3, v4, v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25 from (select userid_t.loginaccount v1, userid_t.customername v2, nvl(g2.loginaccount, '') v3, nvl(g3.loginaccount, '') v4, ff.commodityid v5, ff.closequantity v6, (-1) * (ff.commission) v7, ff.HoldPositionId v8, f.opendirector v9, ff.openprice v10, f.holdpositionprice v11, f.opendate v12, f.OpenType v13, ff.ClosePositionId v14, ff.closedirector v15, ff.closeprice v16, ff.closedate v17, ff.closetype v18, ff.memberid v23, c.salveuserid v25, cd.agreeunit * ff.closequantity * cd.specificationrate / to_number(cd.transferrate) v24, 0.0 v19, ff.closeprofitloss v20,(case when ff.openquantity = 0 then 0 else ff.closeprofitloss - (nvl(ff.commission, 0) + nvl(ff.opencommission / ff.openquantity * ff.closequantity, 0)) end) v21, (case when ff.openquantity = 0 then 0 else (-1) * (ff.opencommission / ff.openquantity * ff.closequantity) end) v22 from pg_info_login g2, pg_info_login g3, cd_info_commodity cd, ts_order_holdposition  f, ts_order_closeposition ff, userid_t, cm_rl_user2user c where ff.UserId = userid_t.userid and ff.holdpositionid = f.holdpositionid and ff.commodityid = cd.commodityid and f.creatorloginid = g2.loginaccountid(+) and ff.CreatorLoginId = g3.loginaccountid(+) and 1 = 1 and 1 = 1 and 1 = 1 and 1 = 1 and ff.userid = c.masteruserid(+) and c.relationtype(+) = 1005 and 1 = 1 order by nvl(v1, rownum) desc, rownum) temp1 WHERE ROWNUM <= :EndRow_0 < int, in >) where RN >= :BeginRow_0 < int, in >"
        );
      streams[strs].open(0,
        "with userid_all as "
        "( select * from "
        "(select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ( "
        "(select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where  exists (select 1 from pg_rl_login2org where loginid = :LoginId_0<int,in> and OrganizationId = OrgId)) connect by parentorgid = prior orgid) "
        "union all (  select distinct p.orgid from pg_info_org p start with p.orgid in (  select a.masterorgid from pg_rl_org2org a where exists ( select 1 from ( select distinct m.orgid from pg_info_org m start with m.orgid in (  select j.orgid from pg_info_org j where  exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1<int,in> and k.OrganizationId=j.OrgId) ) connect by m.parentorgid=prior m.orgid ) b where b.orgid=a.salveorgid ) ) connect by p.parentorgid=prior p.orgid )) ) users ), "
        "data_view as ( "
        "select /*+ LEADING(ua) USE_NL(ua,b) */ "
        "a.ChgLogId, a.ChangeType, a.BeforeAmount, a.Amount, a.AfterAmount, a.CreateDate, b.userid, a.OpLoginid, a.id "
        "from userid_all ua, ac_account_sub b, ac_book_changelog_his a "
        "where a.IsMasterAccount=0 and a.accountid=b.SubAccountId and b.userid=ua.userid "
        "and a.Amount <> 0 and 1=1 and 1=1 and a.tradedate >= :BeginDate_0<bigint,in> "
        "and a.tradedate <= :EndDate_0<bigint,in> ) "
        "select sum(data_view.Amount), count(*) from data_view, vi_info_customer c, "
        "cm_info_user d, cm_info_customer e, pg_info_login f1, ac_bank_signinfo g, "
        "cm_rl_user2user r where data_view.userid=d.userid and "
        "d.customerid=e.customerid and 1=1 and data_view.userid=c.userid and 1=1 "
        "and data_view.OpLoginid=f1.loginaccountid(+) and data_view.userid=r.masteruserid "
        "and r.RelationType=1002 and d.customerid=g.customerid and "
        "g.IsReckonBank=1 and 1=1 ");
      streams[strs].open(0,
        "with userid_all_1 as (select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a, cm_rl_user2user b where a.usertype = 1002 and a.userid = b.masteruserid and b.relationtype = 1002 and b.salveuserid = :UserId_0 < int, in > and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where exists (select 1 from pg_rl_login2org where loginid = :LoginId_0 < int, in > and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (select distinct p.orgid from pg_info_org p start with p.orgid in (select a.masterorgid from pg_rl_org2org a where exists (select 1 from (select distinct m.orgid from pg_info_org m start with m.orgid in (select j.orgid from pg_info_org j where exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1 < int, in > and k.OrganizationId =j.OrgId)) connect by m.parentorgid = prior m.orgid) b where b.orgid = a.salveorgid)) connect by p.parentorgid = prior p.orgid))) users), "
        "userid_all as (select * from (select userid_all_1.* from cm_info_user a, pg_info_org b, userid_all_1 left join (select masteruserid, b.userstatus from cm_rl_user2user a, cm_info_user    b, userid_all_1    u1 where a.relationtype = 1001 and u1.userid = a.masteruserid and a.salveuserid = b.userid) e on userid_all_1.userid = e.masteruserid where userid_all_1.userid = a.userid and a.organizationid = b.orgid and b.orgstatus = 1 and (e.masteruserid is null or e.userstatus <> 14)) ob_user),"
        "userid_t as (select /*+ LEADING(ua) USE_NL(ua,f) */ ua.userid, g1.loginaccount, g1.customername from userid_all ua, vi_info_customer g1 where ua.userid = g1.userid and g1.usertype = 1002 and 1 = 1 and 1 = 1) "
        "select v1, v2, v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21 from (select rownum rn,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21 from (select /*+ LEADING(ut) USE_NL(ut,f) */ f.limitorderid v1, ut.loginaccount v2,ut.customername v3,nvl(g2.loginaccount, '') v4,f.ordertype v5,f.commodityid v6,f.opendirector v7,f.limittype v8,f.openquantity v9,f.orderprice v10,f.slprice v11,f.tpprice v12,f.expiretype v13,f.createdate v14,f.updatedate v15,f.holdpositionid v16,f.marketprice v17,f.throughprice v18,f.dealstatus v19,f.frozenmargin v20,cd.agreeunit * f.openquantity * cd.specificationrate /to_number(cd.transferrate) v21 from cd_info_commodity cd,pg_info_login     g2,userid_t          ut,ts_order_limit    f where f.UserId = ut.userid and f.CreatorLoginId = g2.loginaccountid(+) and 1 = 1 and 1 = 1 and 1 = 1 and 1 = 1 and f.limittype = :OrderType_0 < int, in > and 1 = 1 and f.commodityid = cd.commodityid and 1 = 1 order by nvl(v14, rownum) desc, rownum) temp1 WHERE ROWNUM <= :EndRow_0 < int, in >) where RN >= :BeginRow_0 < int, in > ") ;
      streams[strs].open(0,
        "with userid_all_1 as ("
        "select * from (select a.userid, a.customerid, a.userstatus from cm_info_user a where a.usertype = 1002 and a.organizationid in ((select distinct orgid from pg_info_org start with orgid in (select orgid from pg_info_org where exists (select 1 from pg_rl_login2org where loginid = :LoginId_0 < int, in > and OrganizationId = OrgId)) connect by parentorgid = prior orgid) union all (select distinct p.orgid  from pg_info_org p start with p.orgid in (select a.masterorgid from pg_rl_org2org a where exists (select 1 from (select distinct m.orgid from pg_info_org m start with m.orgid in (select j.orgid from pg_info_org j where exists (select 1 from pg_rl_login2org k where k.loginid = :LoginId_1 < int, in > and k.OrganizationId = j.OrgId)) connect by m.parentorgid = prior m.orgid) b where b.orgid = a.salveorgid)) connect by p.parentorgid = prior p.orgid))) users), "
        "userid_all as ("
        "select * from (select userid_all_1.* from cm_info_user a, pg_info_org b, userid_all_1 left join (select masteruserid, b.userstatus from cm_rl_user2user a, cm_info_user    b, userid_all_1    u1 where a.relationtype = 1001 and u1.userid = a.masteruserid and a.salveuserid = b.userid) e on userid_all_1.userid = e.masteruserid where userid_all_1.userid = a.userid and a.organizationid = b.orgid and b.orgstatus = 1 and (e.masteruserid is null or e.userstatus <> 14)) ob_user) "
        "select v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19 from (select rownum rn,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19 from (select b.loginaccount v1,b.customername v2, c.OrganizationId v3, a.DeliveryOrderId v4, a.ApplyDate v5, a.DeliveryDate v6, a.DeliveryStatus v7, 0 v8, a.commodityid v9, a.DeliveryQuality v10, a.HoldPositionId v11, a.DeliveryPrice v12, a.deliveryAmount v13, a.commission v14, a.AppDesc v15, a.DealLoginId v16, b.userid v17, f.CommodityName v18, cd.agreeunit * a.DeliveryQuality * cd.specificationrate / to_number(cd.transferrate) v19 from cd_info_commodity cd, vi_info_customer b, cm_info_user c, userid_all, cd_info_deliverycommodity f, ts_order_delivery a where a.appuserid = userid_all.userid and a.appuserid = b.userid and a.appuserid = c.UserId and b.usertype = 1002 and 1 = 1 and 1 = 1 and 1 = 1 and 1 = 1 and a.DeliveryCommodityId = f.CommodityId and a.commodityid = cd.commodityid and 1 = 1 order by nvl(v6, rownum) desc, rownum) temp1 WHERE ROWNUM <= :EndRow_0 < int, in >) where RN >= :BeginRow_0 < int, in > ") ;
    streams[strs].open(0,"show tables");
#if PERF_TEST==1
    continue;
#endif
      cnn.change_db("orcl");
    }
    {
      long long id1 = 0;
      int rng = 0, od = 1, lt = 1;
      int counter = 0;
      bool found ;

      printd("test 21: test autocommit on/off\n");
      cnn.change_db("orcl");
      streams[strs].open(0,
        "select a.LimitOrderId, a.OpenDirector, a.LimitType "
        ", nvl(b.SessionId, 'limit_order') from ts_order_limit a "
        "left join ts_session_limit_order_deal b on  a.LimitOrderId = b.LimitOrderId "
        " where a.QuotationId = :f1<int> and a.CommodityId = :f2<int> and "
        "(a.LimitType = :f3<int> or a.LimitType = :f4<int>) and "
        "a.OpenDirector = :f5<int> and a.DealStatus = 1 and "
        "a.InternalStatus = 0 and a.OrderPrice > :f6<double> and "
        "a.OrderPrice <= :f7<double>");
      printd("searching ");
      while (counter<10) {
        streams[strs]<<(int)(-100);
        streams[strs]<<(int)(100000002);
        streams[strs]<<(int)(1);
        streams[strs]<<(int)(lt/*=(lt==1?2:lt==2?3:1)*/);
        streams[strs]<<(int)(od/*=(od==1?2:1)*/);
        streams[strs]<<(double)rng;
        streams[strs]<<(double)(rng+1);
        for (found=false;!streams[strs].eof();) {
          streams[strs]>>id ;
          streams[strs]>>lt ;
          streams[strs]>>size ;
          streams[strs]>>id1 ;
          found = true ;
          printd("oid: %d, director: %d, limitype: %ld, sid: %lld\n",
            id, lt, size, id1);
        }
        if (found || !rng) {
          printf(".");
          counter++ ;
        }
        //usleep(100);
        rng = (rng+1)%4000 ;
      } /* end while() */
      printd("\n");
      cnn.change_db("test_db");
    }
    {
      int cnt = 0;
      int to_str = /*next_str(strs)*/strs+1;

      printd("test 22: test timeout reconnection to db\n");
      streams[strs].open(0, "select name :#<char>,price,size from "
        "test_db.test_tbl where name like :f1<char[10]> ");
      streams[to_str].open(0, "set @@wait_timeout=1");
      while (cnt++<5) {
        streams[strs]<< "orage1" ;
        streams[strs].flush();
        while (!streams[strs].eof()) {
          streams[strs]>>name ;
          streams[strs]>>point ;
          streams[strs]>>size ;
          streams[strs].flush();
          printd("name %s, point %f, size %ld\n",
            name, point, size);
        }
        streams[to_str].open(0, "set @@wait_timeout=1");
        sleep(3);
      }
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
