
#ifndef __CONTAINER_IMPL_H__
#define __CONTAINER_IMPL_H__

#include "mysqls.h"
#include "container.h"
#include "resizable_container.h"
#include "mysqlc.h"

/* extra info of sharding key, such as value-range  */
class SHARDING_EXTRA {
public:
  char hole[1];
} ;

class SHARDING_KEY {
public:
  std::string sch ; /* name of related schema */
  std::string tbl ; /* name of related table */
  std::string col ; /* name of sharding column */
  uint8_t rule ; /* sharding rule on the column */
  SHARDING_EXTRA extra ; 
} ;

class safeShardingColumnList : public safe_container_base<uint32_t,SHARDING_KEY*>
{
protected:
  bool bCaseSensitive ;
public:
  safeShardingColumnList(bool=false) ;
  ~safeShardingColumnList(void) ;

public:
  uint32_t get_key(char*,char*,char*);
  SHARDING_KEY* get(char*,char*,char*);
  SHARDING_KEY* get(uint32_t);
  int add(char*,char*,char*,uint8_t,SHARDING_EXTRA*);
  int drop(uint32_t);
  int drop(char*,char*,char*);
  void clear(void);
};

class GLOBALCOL  {
public:
  std::string sch ; /* name of related schema */
  std::string tbl ; /* name of related table */
  std::string col ; /* name of sharding column */
  int  interval ; /* interval to increase in global id columns */
} ;

/* the global column list */
class safeGlobalIdColumnList : public safe_container_base<uint32_t,GLOBALCOL*>
{
protected:
  bool bCaseSensitive ;
public:
  safeGlobalIdColumnList(bool=false) ;
  ~safeGlobalIdColumnList(void) ;

public:
  uint32_t get_key(char*,char*);
  GLOBALCOL* get(char*,char*);
  GLOBALCOL* get(uint32_t);
  int add(char*,char*,char*,int);
  int drop(uint32_t);
  int drop(char*,char*);
  void clear(void);
};

class TABLE_NAME {
public:
  std::string sch ;
  std::string tbl ;
} ;

class unsafeTblKeyList : public safe_container_base<uint32_t,TABLE_NAME*>
{
protected:
  bool bCaseSensitive ;
  /* the table name - alias name index */
  using idx_t = std::unordered_map<uint32_t,TABLE_NAME*> ;
  idx_t m_index;
  /* the table-key list iterator */
  ITR_TYPE m_itr ;

public:
  unsafeTblKeyList(bool=false) ;
  ~unsafeTblKeyList(void);

public:
  uint32_t get_key(char*,char*);
  int add(char*,char*,char*) ;
  TABLE_NAME* get(char*,char*);
  TABLE_NAME* get(char*);
  int drop(char*,char*,bool=true);
  int drop(uint32_t,bool=true);
  void clear(void);
  TABLE_NAME* next(bool=false);
};

class sv_t {
public:
  union {
    uint8_t v8;
    uint16_t v16;
    long v32;
    long long v64;
    float vf ;
    double vd ;
  } u ;
} ;

const int MAX_SV_ITEMS = 1;
class SHARDING_VALUE {
public:
  /* the sharding column type */
  uint8_t type ; 
  /* the configured sharding column info */
  SHARDING_KEY *sk ;
  /* operator type */
  uint16_t opt ;
  /* TODO: value list of the column */
  sv_t sv[MAX_SV_ITEMS] ;
  /* value items count, such as 'in (a,b,c)' syntax */
  int num_vals ;
} ;

class unsafeShardingValueList : public safe_vector_base<SHARDING_VALUE*> {

protected:
  int m_numValueItems;
  using pholderSVMap = std::unordered_map<int,uint64_t> ;
  pholderSVMap m_map ;

public:
  unsafeShardingValueList(void);
  ~unsafeShardingValueList(void);

public:
  SHARDING_VALUE* get(int index);
  SHARDING_VALUE* get1(int nph);
  /* add a 'SHARDING_VALUE' item into list */
  int add(int &index,SHARDING_KEY*,uint8_t type,uint16_t opt) ;
  int add(int &index, SHARDING_VALUE*);
  /* get sharding value count */
  size_t get_sv_count(int);
  /* update the column's sharding value list */
  int update(int index,void*val,int nph);
  /* update the column's sharding value list by place holder number */
  int update(int nph,int type,void*val);
  int update_sv_by_type(sv_t*,int,void*);
  /* do a reset on the sharding value item */
  int reset_sv(SHARDING_VALUE*);
  void reset(void);
  /* clean up the list */
  void clear(void);
  int drop(int);
  /* copy current object */
  unsafeShardingValueList* operator =(unsafeShardingValueList&);
  /* XXX: for test only */
  void dbg_output(void);
  /* the mapping key management */
  sv_t* get_sv_by_mapping_key(uint64_t);
  void extract_mapping_key(uint64_t,int&,int&);
  uint64_t makeup_mapping_key(int,int);
} ;

/* aggregation info */
class agg_info {
public:
  int nCol ;
  /* the aggregation type */
  uint8_t type ; 
} ;

class unsafeAggInfoList : public safe_vector_base<agg_info*> {
  ITR_TYPE m_itr ;

public:
  unsafeAggInfoList(void);
  ~unsafeAggInfoList(void);

public:
  int add(int nCol,uint8_t type) ;
  int add(agg_info *pa) ;
  /* clean up the list */
  void clear(void);
  agg_info* next(bool bStart=false);
  /* copy current object */
  unsafeAggInfoList* operator =(unsafeAggInfoList&);
} ;

/* sql parser item */
class tSqlParseItem {
public:
  /* sharding value list */
  unsafeShardingValueList m_svs;
  /* the place-holder type list */
  std::vector<uint8_t> m_types;
  /* the aggregated info */
  unsafeAggInfoList m_aggs ;

public:
  tSqlParseItem(void) ;
  virtual ~tSqlParseItem(void);

public:
  void reset(void) ;
  tSqlParseItem* operator = (tSqlParseItem&);
  /* sharding value management */
  int add_sv(int&,SHARDING_KEY*,uint8_t,uint16_t);
  int update_sv(int,int,void*);
  int update_sv(int,void*,int);
  int drop_sv(int);
  SHARDING_VALUE* get_sv(int index);
  SHARDING_VALUE* get_sv1(int nph);
  size_t get_num_sv(void);
  /* place-holders' types management */
  int get_type(int,int&);
  void save_type(uint8_t);
  /* add new aggregation info */
  void add_agg_info(int nCol,int t);
} ;

/*
 * statement id and group/datanode mapping list
 */
class safeDnStmtMapList : public safe_container_base<int,int> {

public:
  safeDnStmtMapList(void) { lock_init();  }
  ~safeDnStmtMapList(void) { lock_release(); }

public:
  int gen_key(int,int) const;
  int get(int grp, int dn);
  int add(int grp, int dn, int stmtid);
  void drop(int grp, int dn);
} ;

/* client-based statement info */
using stxNode = struct SyntaxTreeNode ;
class tStmtInfo {
public:
  /* the client-based logical statement id  counter */
  int lstmtid ;

  /* the prepare req */
  tContainer prep_req ;

  /* the datanode-stmtid mapping */
  safeDnStmtMapList maps ;

  /* the sql tree object */
  stxNode *m_root ;

  /* the sql parse item */
  tSqlParseItem sp ;
} ;

/* mappings of lstmtid -> tStmtInfo */
class safeStmtInfoList : public safe_container_base<int,tStmtInfo*>
{
protected:
  /* the statement key - logical statement id index */
  using keyIndex_t = std::unordered_map<uint32_t,tStmtInfo*> ;
  keyIndex_t m_keyIndex ;

public:
  safeStmtInfoList(void) { lock_init(); }
  ~safeStmtInfoList(void) { clear(); lock_release(); }

public:
  /* statement key map management */
  tStmtInfo* get(int lstmtid);
  int add_key_index(char *req, tStmtInfo*);

  tStmtInfo* get(char*req,size_t sz);

  int add_mapping(int lstmtid,int grp,int dn,int stmtid);

  int add_prep(int lstmtid,char *prep,size_t sz,stxNode*,
    tSqlParseItem&);

  int get_prep(int lstmtid, char* &prep, size_t &sz);

  tStmtInfo* new_mapping(int lstmtid);

  void clear(void);

  int drop(int lstmtid);
} ;

enum cmd_state {
  st_na,

  /* the prepare result should be 
   *  transmited to client */
  st_prep_trans,

  /* an execute should be done after
   *  this prepare command */
  st_prep_exec,

  /* statement execution */
  st_stmt_exec,

  /* query mode execution */
  //st_query,
} ;

class tClientStmtInfo {
public:
  /* client id */
  int cid ;

  /* the client-based logical statement counter */
  int lstmtid_counter;

  /* 
   * tmporary 
   */
  class currentInfo {
  public:
    /* the execute request buffer */
    tContainer exec_buf ;
    int exec_xaid ;

    /* the blob request buffer */
    tContainer blob_buf ;
    /* total received blob requests */
    int rx_blob ;
    int total_blob ;

    /* total placeholders count */
    int total_phs ;

    /* command state */
    int state ;

    /* current statement id that are operated on */
    int lstmtid ;

    /* the currently used sql parse item */
    tSqlParseItem *sp ;
  } curr;

  /* the statement list */
  safeStmtInfoList stmts;

  /* infos in query mode */
  class queryInfo {
  public:
    /* the sql parse item used in query mode */
    tSqlParseItem sp ;

  } qryInfo;

} ;

/* mappings of  client id -> tClientStmtInfo */
class safeClientStmtInfoList : public safe_container_base<int,tClientStmtInfo*>
{

public:
  safeClientStmtInfoList(void) { lock_init(); }
  ~safeClientStmtInfoList(void) { clear(); lock_release(); }

public:
  tClientStmtInfo* get(int) ;

  int add_stmt(int cid, char *prep_req, size_t sz, uint16_t nPhs, 
    stxNode*, tSqlParseItem &sp) ;

  int add_qry_info(int cid, tSqlParseItem &sp);
  int get_qry_info(int cid, tSqlParseItem &sp);

  int add_mapping(int cid,int lstmtid,int grp,int dn,int stmtid);

  int get_lstmtid(int cid, int &lstmtid) ;

  int get_mapping(int cid, int lstmtid, int grp, int dn, int &stmtid);

  int get_cmd_state(int cid, int &st);

  int set_cmd_state(int cid, int st);

  int save_exec_req(int cid, int xaid, char *req, size_t sz);
  int get_exec_req(int cid, int &xaid, char* &req, size_t &sz);
  int get_prep_req(int cid, char* &req, size_t &sz);
  bool is_exec_ready(int cid);

  int get_parser_item(int cid, int lstmtid, tSqlParseItem*&);

  int get_total_phs(int cid, int &nphs);
  int set_total_phs(int cid, int nphs);

  bool is_blobs_ready(int cid);
  int get_blob(int cid, char* &req, size_t &sz);
  int save_blob(int cid, char *req, size_t sz);
  int set_total_blob(int cid, int nblob);

  stxNode* get_stree(int cid, char *req, size_t sz);

  int set_curr_sp(int cid, int lstmtid=-1);
  int get_curr_sp(int cid, tSqlParseItem*&);

  int drop(int);
  int reset(int);
  void clear(void);
} ;

/*
 * class safeLoginSessions 
 */
class tSessionDetails
{
public:
  /* cramble */
  char scramble[AP_LENGTH] ;
  size_t sc_len ;
  /* status of current connection */
  int status ;
  /* currently used db */
  std::string db ;
  /* currently login user */
  std::string usr;
  /* related login pwd */
  std::string pwd;
  /* session related xa identifier */
  int xaid ;

public:
  /* xaid management */
  int get_xaid(void) ;
  void save_xaid(int);
} ;


class safeLoginSessions : public safe_container_base<int,tSessionDetails*>
{
public:
  safeLoginSessions(void) { lock_init(); }
  ~safeLoginSessions(void) { lock_release(); }

public:
  /* get variable value */
  tSessionDetails* get_session(int) ;
  /* add new variable by fd */
  int add_session(int,tSessionDetails*);
  /* delete fd's variable */
  int drop_session(int);
  /* clear all items */
  void clear(void);
  /* session related xaid management */
  int get_xaid(int);
  int save_xaid(int,int);
  int reset_xaid(int);
} ;

/* extra column details */
class tColExtra {
public:
  /* column may be null */
  bool null_able ;
  char default_val[253];
  char extra[253];
  char display_type[253];
  /* key type: 0: primary, 1 multiple, 2 unique, 0xff n/a */
  uint8_t key_type ;
} ;

/* table column definitions */
class tColDetails {
public:
  /* mainly column structure */
  MYSQL_COLUMN col ;
  /* extra column infomations */
  tColExtra ext;
  tColDetails *next ;
} ;

class tDnMappings {
public:
  uint8_t dn ;
  uint8_t io_type ;
  tDnMappings *next ;
} ;

class tTblDetails {
public:
  /* combined with: schema-id + table-id */
  uint64_t key ;
  /* data node number to execute sql, only processor 
   *  uses this bytes */
  int dn ;
  /* physical schema name */
  std::string phy_schema ;
  /* logical schema name */
  std::string schema ;
  /* table name */
  std::string table ;
  /* column list */
  tColDetails *columns ; 
  size_t num_cols ;
  /* configured data node mappings */
  tDnMappings *conf_dn ;
  /* TODO: add index definitions here */
} ;


/*
 * class safeTableDetailList
 */

class safeTableDetailList : public safe_container_base<uint64_t,tTblDetails*>
{
public:
  safeTableDetailList (void) { lock_init(); }
  ~safeTableDetailList(void) { clear(); lock_release(); }

public:
  uint64_t gen_key(char*,char*);
  tTblDetails* get(uint64_t);
  tColDetails* get_col(char*,char*,char*);
  tColDetails* get_col(char*,char*,tColDetails*);
  /* add empty table */
  int add(char *schema, char *table, 
    char *phy_schema=0, int dn=-1);
  /* add a column of a table of a schema */
  int add(char *schema, char *table, 
    char *colum, uint16_t charset, uint32_t maxlen, 
    uint8_t type, uint16_t flags);
  int add(char *schema, char *table, 
    char *column, char* display_type,
    bool null_able, uint8_t key_type, 
    char *def_val, char *ext);
  int add(char *schema, char *table, 
    uint8_t dn, uint8_t io_type);
  /* delete */
  int drop(uint64_t);
  void clear(void);
} ;


/*
 * class safeDataNodeList
 */
class tDNInfo {
public:
  /* data node's number */
  uint8_t no ;
  uint32_t addr ;
  int port ;
  char schema[64];
  char usr[64];
  char pwd[64];
  MYSQL *mysql ;
  uint8_t stat ;
  //void *ep ;
  int add_ep:1;
} ;

class safeDataNodeList : public safe_container_base<int,tDNInfo*>
{

public:
  safeDataNodeList (void) { lock_init(); }
  ~safeDataNodeList  (void) { clear(); lock_release(); }

public:
  /* get related dtanode info */
  tDNInfo* get(int) ;
  tDNInfo* get_by_fd(int) ;
  /* add new processor/client mapping */
  int add(int,uint32_t,int,char*,char*,char*);
  int add(int,tDNInfo*);
  /* delete mapping */
  int drop(int);
  /* clear all items */
  void clear(void);
  /* update all items' states */
  void update_batch_stats(int);
  /* get next data node item */
  //tDNInfo* next(bool=false);
  /* get data node count */
  //size_t count(void);
} ;

class safeStdVector : safe_queue_base<int> {
public:
  safeStdVector(void) { lock_init(); }
  ~safeStdVector(void) { lock_release(); }

public:
  void push(int);
  int pop(int&); 
} ;

class tSchedule {
public:
  void *st;
  int cfd ;
  int cmd;
  char *req ;
  size_t sz ;
  int cmd_state;
} ;

class safeScheQueue : safe_queue_base<tSchedule*> {
public:
  safeScheQueue(void) { lock_init(); }
  ~safeScheQueue(void) { lock_release(); }

public:
  void push(void*,int,int,char*,size_t,int);
  int pop(void*&,int&,int&,char*,size_t&,int&); 
} ;


class xa_item ;
class safeXAList : public safe_container_base<int,xa_item*>
{
protected:
  /* the latest xaid, may be overflow */
  int m_xaid ;

public:
  safeXAList (void):m_xaid(0) { lock_init(); m_list.clear(); }
  ~safeXAList (void) { lock_release(); }


public:
  /* get related xa item */
  xa_item* get(int) ;
  /* add new xa */
  int add(int,int,void*);
  /* delete xa */
  int drop(int);
  /* clear all items */
  void clear(void);
} ;

/* rx states */
enum {
  rs_none=1,
  rs_col_def=2, /* column definitions are recv */
  rs_ok=3, /* all parts are recv */
} ;

class safeRxStateList : public safe_container_base<int,uint8_t>
{
public:
  safeRxStateList (void) { lock_init(); }
  ~safeRxStateList (void) { lock_release(); }

public:
  /* get related state item */
  uint8_t get(int) ;
  /* set new state */
  int set(int fd, uint8_t new_st);
  /* add new state */
  int add(int);
  /* delete */
  int drop(int);
  /* clear all items */
  void clear(void);
  /* to next state */
  uint8_t next(int);
} ;

/* 
 * container gruops 
 */
class safeColDefGroup : public safe_container_base<int,tContainer*>
{
public:
  safeColDefGroup (void) { clear(); lock_init(); }
  ~safeColDefGroup (void) { lock_release(); }

public:
  /* get related column def item by myfd */
  tContainer* get(int) ;
  tContainer* get_first(void) ;
  /* add attach column def contents */
  int add(int,char*,size_t);
  /* reset item */
  int reset(int);
  /* clear all items */
  void clear(void);
} ;


typedef struct tSockToolkit sock_toolkit;
using STM_PAIR = std::pair<sock_toolkit*,int> ;

using ITR_FUNC = int(*)(STM_PAIR&);

class safeStMapList : public safe_vector_base<STM_PAIR> {
protected:
  ITR_TYPE m_itr ;

public:
  safeStMapList (void) { lock_init(); }
  ~safeStMapList (void) { clear(); lock_release(); }

public:
  int add(STM_PAIR);

  //int next(STM_PAIR &pair, bool bStart=false);
  int do_iterate(ITR_FUNC);

  void clear(void);
};


#endif /* __CONTAINER_IMPL_H__ */

