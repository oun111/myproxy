
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "container_impl.h"
#include "simple_types.h"
#include "crypto.h"
#include "xa_item.h"
#include "dbg_log.h"
#include "connstr.h"
#include "sql_tree.h"
#include "dbug.h"


/*
 * class safeShardingColumnList
 */
safeShardingColumnList::safeShardingColumnList(bool bCs):
  bCaseSensitive(bCs)
{
  lock_init();
}

safeShardingColumnList::~safeShardingColumnList(void)
{
  clear();

  lock_release();
}

uint32_t safeShardingColumnList::get_key(char *schema, 
  char *tbl, char *col)
{
  char *psch = bCaseSensitive?schema:to_lower(schema),
    *ptbl = bCaseSensitive?tbl:to_lower(tbl),
    *pcol = bCaseSensitive?col:to_lower(col);

  return (str2id(psch)<<16) | (str2id(ptbl)<<8) | str2id(pcol);
}

SHARDING_KEY* safeShardingColumnList::get(char *schema, char *tbl, char *col)
{
  try_read_lock();
  return find(get_key(schema,tbl,col));
}

SHARDING_KEY* safeShardingColumnList::get(uint32_t k)
{
  try_read_lock();
  return find(k);
}

int safeShardingColumnList::add(char *schema, char *tbl, 
  char *col, uint8_t rule, SHARDING_EXTRA *pse)
{
  uint32_t k = get_key(schema,tbl,col);
  SHARDING_KEY *sk = get(k);

  if (!sk) {
    sk = new SHARDING_KEY ;
    try_write_lock();
    insert(k,sk);
  }
  {
    try_write_lock();
    sk->sch = schema ;
    sk->tbl = tbl ;
    sk->col = col ;
    sk->rule= rule ;
    if (pse) {
      memcpy(&sk->extra,pse,sizeof(SHARDING_EXTRA));
    }
  }
  return 0;
}

int safeShardingColumnList::drop(uint32_t k)
{
  SHARDING_KEY *sk = get(k);

  if (!sk) {
    return -1;
  }
  {
    try_write_lock();
    safe_container_base<uint32_t,SHARDING_KEY*>::drop(k);
  }
  delete sk ;
  return 0;
}

int safeShardingColumnList::drop(char *schema,char *tbl, char *col)
{
  return drop(get_key(schema,tbl,col));
}

void safeShardingColumnList::clear(void)
{
  try_write_lock();
  list_foreach_container_item {
    delete i.second;
  }
}


/*
 * class safeGlobalIdColumnList
 */
safeGlobalIdColumnList::safeGlobalIdColumnList(bool bCs):
  bCaseSensitive(bCs)
{
  lock_init();
}

safeGlobalIdColumnList::~safeGlobalIdColumnList(void)
{
  clear();

  lock_release();
}

uint32_t 
safeGlobalIdColumnList::get_key(char *schema, char *tbl)
{
  char *psch = bCaseSensitive?schema:to_lower(schema),
    *ptbl = bCaseSensitive?tbl:to_lower(tbl);

  return (str2id(psch)<<16) | (str2id(ptbl));
}

GLOBALCOL* safeGlobalIdColumnList::get(char *schema, char *tbl)
{
  try_read_lock();
  return find(get_key(schema,tbl));
}

GLOBALCOL* safeGlobalIdColumnList::get(uint32_t k)
{
  try_read_lock();
  return find(k);
}

int safeGlobalIdColumnList::add(char *schema, char *tbl, 
  char *col, int interval)
{
  uint32_t k = get_key(schema,tbl);
  GLOBALCOL *sk = get(k);

  if (!sk) {
    sk = new GLOBALCOL ;
    try_write_lock();
    insert(k,sk);
  }
  {
    try_write_lock();
    sk->sch = schema ;
    sk->tbl = tbl ;
    sk->col = col ;
    sk->interval= interval ;
  }
  return 0;
}

int safeGlobalIdColumnList::drop(uint32_t k)
{
  GLOBALCOL *sk = get(k);

  if (!sk) {
    return -1;
  }
  {
    try_write_lock();
    safe_container_base<uint32_t,GLOBALCOL*>::drop(k);
  }
  delete sk ;
  return 0;
}

int safeGlobalIdColumnList::drop(char *schema,char *tbl)
{
  return drop(get_key(schema,tbl));
}

void safeGlobalIdColumnList::clear(void)
{
  try_write_lock();
  list_foreach_container_item {
    delete i.second;
  }
}

/*
 * class unsafeTblKeyList
 */
unsafeTblKeyList::unsafeTblKeyList(bool cs):
  bCaseSensitive(cs)
{
  lock_init();
}

unsafeTblKeyList::~unsafeTblKeyList(void)
{
  clear();
  lock_release();
}

uint32_t unsafeTblKeyList::get_key(char *item0, char *item1)
{
  char *p0 = bCaseSensitive?item0:to_lower(item0),
    *p1 = bCaseSensitive?item1:to_lower(item1);

  return (str2id(p0)<<16) | str2id(p1);
}

/* get table name by schema+table */
TABLE_NAME* unsafeTblKeyList::get(char *schema, char *tbl)
{
  return find(get_key(schema,tbl));
}

/* get table name by alias */
TABLE_NAME* unsafeTblKeyList::get(char *tbl_a)
{
  idx_t::iterator itr = m_index.find(get_key(tbl_a,0));

  if (itr==m_index.end()) {
    return NULL;
  }
  return itr->second;
}

int unsafeTblKeyList::add(
  char *schema, 
  char *tbl, 
  char *tbl_a /* table alias name */
)
{
  uint32_t k = get_key(schema,tbl);
  TABLE_NAME *pt = find(k);
  uint32_t id = 0;

  /* new table name */
  if (!pt) {
    pt = new TABLE_NAME ;
    insert(k,pt);
  }
  pt->sch = schema ;
  pt->tbl = tbl ;
  /* try to add alias index */
  if (!tbl_a) {
    return 0;
  }
  id = get_key(tbl_a,0);
  m_index[id] = pt ;
  return 0;
}

int unsafeTblKeyList::drop(uint32_t k, bool bRemove)
{
  TABLE_NAME *pt = find(k);

  if (!pt) {
    return 1;
  }
  delete pt ;
  if (bRemove) {
    safe_container_base<uint32_t,TABLE_NAME*>::drop(k);
  }
  return 0;
}

int unsafeTblKeyList::drop(char *schema,char *tbl,bool bRemove)
{
  return drop(get_key(schema,tbl),bRemove);
}

void unsafeTblKeyList::clear(void)
{
  list_foreach_container_item {
    drop(i.first,false);
  }
}

TABLE_NAME* unsafeTblKeyList::next(bool bStart)
{
  /* XXX: the 'next' call needs a lock initializing */
  return safe_container_base<uint32_t,TABLE_NAME*>::next(m_itr,bStart);
}

/*
 * class unsafeShardingValueList
 */
unsafeShardingValueList::unsafeShardingValueList(void) :
  m_numValueItems(0)
{
  // lock_init();
}

unsafeShardingValueList::~unsafeShardingValueList(void)
{
  clear();
  // lock_release();
}

SHARDING_VALUE* unsafeShardingValueList::get(int index)
{
  return find(index);
}

SHARDING_VALUE* unsafeShardingValueList::get1(int nph)
{
  pholderSVMap::iterator itr ;
  int index = 0, pos = 0;
  //SHARDING_VALUE *psv = 0;
  //sv_t *psv = 0;

  itr = m_map.find(nph);
  /* no such place holder number found */
  if (itr==m_map.end()) {
    return 0;
  }
  extract_mapping_key(itr->second,index,pos);
  /* get sharding value */
  return get(index);
}

void unsafeShardingValueList::clear(void)
{
  SHARDING_VALUE *psv = 0;
  safe_vector_base<SHARDING_VALUE*>::ITR_TYPE itr ;
  int ret = 0;

  for (ret=next(itr,true);!ret;ret=next(itr)) {
    psv = *itr;
    delete psv;
  }
  m_map.clear();
  m_list.clear();
}

int unsafeShardingValueList::reset_sv(SHARDING_VALUE *psv)
{
  int i=0;

  psv->sk = 0;
  for (i=0;i<MAX_SV_ITEMS;i++) {
    //psv->sv[i].nPlace_holder = -1;
  }
  psv->num_vals = 0;
  return 0;
}

void unsafeShardingValueList::reset(void)
{
  SHARDING_VALUE *psv = 0;
  safe_vector_base<SHARDING_VALUE*>::ITR_TYPE itr ;
  int ret = 0;

  if (safe_vector_base<SHARDING_VALUE*>::size()<=0) {
    return ;
  }
  for (ret=next(itr,true);!ret;ret=next(itr)) {
    psv = *itr ;
    reset_sv(psv);
  }
  m_numValueItems = 0;
  m_map.clear();
}

size_t unsafeShardingValueList::get_sv_count(int index)
{
  SHARDING_VALUE *psv = get(index);

  if (!psv) {
    return 0;
  }
  return psv->num_vals ;
}

#if 0
int unsafeShardingValueList::add(int &index, SHARDING_VALUE *ps)
{
  SHARDING_VALUE *psv = get(index);

  if (!psv) {
    psv = new SHARDING_VALUE ;
    insert(psv);
    /* new value item */
    m_numValueItems ++ ;
    /* last one of list */
    index = m_numValueItems-1;
  }
  psv->type = ps->type ;
  psv->sk   = ps->sk ;
  psv->opt  = ps->opt ;
  psv->num_vals= ps->num_vals ;
  memcpy(psv->sv,ps->sv,sizeof(ps->sv));
  return 0;
}
#endif

int unsafeShardingValueList::add(int &index, SHARDING_KEY *psk,
  uint8_t type,uint16_t opt)
{
  SHARDING_VALUE *psv = get(index);

  if (!psv) {
    psv = new SHARDING_VALUE ;
    insert(psv);
    /* new value item */
    m_numValueItems ++ ;
    /* last one of list */
    index = m_numValueItems-1;
  }
  /* 
   * update the value 
   */
  /* column type */
  psv->type = type ;
  /* sharding column item */
  psv->sk   = psk ;
  /* operator type */
  psv->opt  = opt ;
  /* valid value count */
  psv->num_vals = 0;
  return 0;
}

int unsafeShardingValueList::drop(int index)
{
  return safe_vector_base<SHARDING_VALUE*>::drop(index);
}

int unsafeShardingValueList::update_sv_by_type(sv_t *psv, 
  int type, void *val)
{
  /* assign sharding values */
  switch (type) {
    case MYSQL_TYPE_TINY:  
      psv->u.v8 = *(uint8_t*)val ;   
      //log_print("val8: %d\n",psv->u.v8);
      break;

    case MYSQL_TYPE_SHORT: 
      psv->u.v16 = *(uint16_t*)val ;
      //log_print("val16: %d\n",psv->u.v16);
      break ;

    case MYSQL_TYPE_LONG:
      psv->u.v32 = *(/*uint32_t*/long*)val ;
      //log_print("val32: %ld\n",psv->u.v32);
      break ;

    case MYSQL_TYPE_LONGLONG:
      psv->u.v64 = *(long long*)val ;
      //log_print("val64: %lld\n",psv->u.v64);
      break ;

    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_NEWDECIMAL:
      psv->u.vf = *(float*)val ;
      //log_print("valf: %f\n",psv->u.vf);
      break ;

    case MYSQL_TYPE_DOUBLE:
      psv->u.vd = *(double*)val ;
      //log_print("vald: %f\n",psv->u.vd);
      break ;

    default:
      /* XXX: unsupported column type */
      log_print("unknown type %d\n",type);
      return -1 ;
  }
  return 0;
}

int unsafeShardingValueList::update(int nph,int type,void*val)
{
  pholderSVMap::iterator itr ;
  sv_t *psv = 0;

  itr = m_map.find(nph);
  /* no such place holder number found */
  if (itr==m_map.end()) {
    return -1;
  }
  /*get the sv_t item */
  psv = get_sv_by_mapping_key(itr->second);
  if (!psv) {
    return -1;
  }
  /* assign sharding values */
  if (update_sv_by_type(psv,type,val)) {
    return -1;
  }
  return 0;
}

sv_t* unsafeShardingValueList::get_sv_by_mapping_key(uint64_t key)
{
  int index = 0, pos = 0;
  SHARDING_VALUE *psv = 0;

  extract_mapping_key(key,index,pos);
  /* get sharding value */
  psv = get(index);
  /* no sharding value item found */
  if (!psv) {
    return 0;
  }
  /* the sv position is too big */
  if (pos>=psv->num_vals ||pos>=MAX_SV_ITEMS) {
    return 0;
  }
  //printf("sv type: %d\n",psv->type);
  return &psv->sv[pos] ;
}

void unsafeShardingValueList::extract_mapping_key(uint64_t key, int &index, int &pos)
{
  index = key>>32;
  pos   = key&0xffffffff ;
}

uint64_t unsafeShardingValueList::makeup_mapping_key(int index, int pos)
{
  return (((uint64_t)index)<<32)|(pos&0xffffffff) ;
}

int unsafeShardingValueList::update(int index,void*val,int nph)
{
  SHARDING_VALUE *psv = get(index);
  int &pos = psv->num_vals ;

  /* not found */
  if (!psv) {
    return -1;
  }
  /* value list's full */
  if (pos>=MAX_SV_ITEMS) {
    return 1;
  }
  if (!val) {
    /* set no values but create 'place-holder-number' -> 'map-key' mapping */
    m_map[nph] = makeup_mapping_key(index,pos++) ;
    return 0;
  }
  /* assign sharding values */
  if (update_sv_by_type(&psv->sv[pos],psv->type,val)) {
    return -1;
  }
  pos ++ ;
  return 0;
}

#if 0
unsafeShardingValueList* unsafeShardingValueList::operator =(unsafeShardingValueList &lst)
{
  int i = 0;
  SHARDING_VALUE *psv = 0;

  reset();
  for (i=0;i<lst.m_numValueItems;i++) {
    psv = lst.get(i);
    this->add(i,psv);
    //this->m_map = lst.m_map ;
  }
  this->m_map = lst.m_map ;
  return this;
}
#endif

unsafeShardingValueList* unsafeShardingValueList::operator =(unsafeShardingValueList &&lst)
{
  int i = 0;
  SHARDING_VALUE *psv = 0;

  clear();
  for (i=0;i<lst.m_numValueItems;i++) {
    /* take over items in lst */
    psv = lst.get(i);
    insert(psv);
  }
  m_map = std::move(lst.m_map) ;

  m_numValueItems = lst.m_numValueItems ;

  /* set the 'move from' items to already-freed */
  lst.m_numValueItems = 0;
  lst.m_list.clear();
  lst.m_map.clear();

  return this;
}

void unsafeShardingValueList::dbg_output(void)
{
  SHARDING_VALUE *psv = 0;
  ITR_TYPE itr; 
  int i=0, ret=0;

  for (ret=next(itr,true);!ret;ret=next(itr),i++) {
    psv = *itr ;
    log_print("sv %d: type %d, opt %d, sk: %s.%s.%s\n",
      i,psv->type,psv->opt,psv->sk->sch.c_str(),
      psv->sk->tbl.c_str(),psv->sk->col.c_str());
  }
}

/*
 * class unsafeAggInfoList
 */
unsafeAggInfoList::unsafeAggInfoList(void)
{
}

unsafeAggInfoList::~unsafeAggInfoList(void)
{
  clear();
}

int unsafeAggInfoList::add(int nCol,uint8_t type) 
{
  agg_info *pa = new agg_info ;

  pa->nCol = nCol ;
  pa->type = type ;
  insert(pa);
  return 0;
}

#if 0
int unsafeAggInfoList::add(agg_info *pa) 
{
  agg_info *ptr = new agg_info ;

  memcpy(ptr,pa,sizeof(*pa));
  insert(ptr);
  return 0;
}
#endif

/* clean up the list */
void unsafeAggInfoList::clear(void)
{
  for (auto i:m_list) {
    delete i ;
  }
  m_list.clear();
}

agg_info* unsafeAggInfoList::next(bool bStart)
{
  if (safe_vector_base<agg_info*>::next(m_itr,bStart)) {
    return NULL ;
  }

  return *m_itr;
}

#if 0
/* copy current object */
unsafeAggInfoList* unsafeAggInfoList::operator =(unsafeAggInfoList &lst)
{
  clear();

  for (auto pa : lst.m_list) {
    add(pa);
  }
  return this;
}
#endif

/* move objects */
unsafeAggInfoList* unsafeAggInfoList::operator =(unsafeAggInfoList &&lst)
{
  clear();

  for (auto pa : lst.m_list) {
    insert(pa);
  }
  lst.m_list.clear();

  return this;
}

/* 
 * class tSqlParseItem 
 */
tSqlParseItem::tSqlParseItem(void) 
{
}

tSqlParseItem::~tSqlParseItem(void)
{ 
}

void tSqlParseItem::reset(void) 
{
  m_svs.reset();
  m_types.clear();
  m_aggs.clear();
}

tSqlParseItem* tSqlParseItem::operator = (tSqlParseItem &ps)
{
#if 0
  m_svs   = ps.m_svs ;
  m_types = ps.m_types;
  m_aggs  = ps.m_aggs;
#else
  m_svs   = std::move(ps.m_svs) ;
  m_types = std::move(ps.m_types);
  m_aggs  = std::move(ps.m_aggs);
#endif
  return this;
}

/* update sharding value by its index */
int tSqlParseItem::update_sv(int idx, void *data, int nph)
{
  return m_svs.update(idx,data,nph);
}

/* update sharding value by place holder's number */
int tSqlParseItem::update_sv(int nph, int type, void *data)
{
  return m_svs.update(nph,type,data);
}

int tSqlParseItem::add_sv(int &index, SHARDING_KEY *psk,
  uint8_t type,uint16_t opt)
{
  return m_svs.add(index,psk,type,opt);
}

int tSqlParseItem::drop_sv(int index)
{
  return m_svs.drop(index);
}

SHARDING_VALUE* tSqlParseItem::get_sv1(int nph)
{
  return m_svs.get1(nph);
}

SHARDING_VALUE* tSqlParseItem::get_sv(int index)
{
  return m_svs.get(index);
}

size_t tSqlParseItem::get_num_sv(void)
{
  return m_svs.size();
}

/* get place holder's type by index */
int tSqlParseItem::get_type(int nph,int &t)
{
  if (nph>=(int)m_types.size()) {
    return -1;
  }
  t = m_types[nph];
  return 0;
}

void tSqlParseItem::save_type(uint8_t t)
{
  m_types.push_back(t);
}

void tSqlParseItem::add_agg_info(int nCol,int t)
{
  m_aggs.add(nCol,t);
}

/*
 * class tSessionDetails
 */
int tSessionDetails::get_xaid(void) 
{
  //return xaid ;
  return __sync_fetch_and_add(&xaid,0) ;
}

void tSessionDetails::save_xaid(int id)
{
  //xaid = id ;
  __sync_lock_test_and_set(&xaid,id);
}

/*
 * class safeLoginSessions
 */
tSessionDetails* safeLoginSessions::get_session(int cid)
{
  try_read_lock();

  return find(cid);
}

int safeLoginSessions::add_session(int cid, tSessionDetails *pv)
{
  tSessionDetails *v = get_session(cid);

  /* not exist */
  if (!v) {
    v = new tSessionDetails ;
    try_write_lock();
    insert(cid,v);
  }
#if 0
  /* update content */
  memcpy(v,pv,sizeof(*pv));
#else
  memcpy(v->scramble,pv->scramble,pv->sc_len);
  v->sc_len = pv->sc_len ;
  v->status = pv->status;
  v->db = pv->db;
  v->usr= pv->usr;
  v->pwd= pv->pwd;
  v->xaid = -1;
#endif
  return 0 ;
}

tSessionDetails* safeLoginSessions::add_session(int cid)
{
  tSessionDetails *v = get_session(cid);

  if (v) {
    return v;
  }
  /* not exist */
  v = new tSessionDetails ;
  mysqls_gen_rand_string(v->scramble,AP_LENGTH-1);
  v->scramble[AP_LENGTH] = 0;
  v->sc_len = AP_LENGTH-1;
  v->status = cs_init ;
  v->xaid = -1;
  {
    try_write_lock();
    insert(cid,v);
  }
  return v ;
}

int safeLoginSessions::drop_session(int cid)
{
  tSessionDetails *v = get_session(cid);

  if (!v)
    return -1;
  {
    try_write_lock();
    drop(cid);
  }
  delete v ;
  return 0;
}

void safeLoginSessions::clear(void)
{
  try_write_lock();

  list_foreach_container_item {
    delete i.second ;
  }
  m_list.clear();
}

int safeLoginSessions::get_xaid(int cid)
{
  tSessionDetails *v = get_session(cid);

  /* not exist */
  if (!v) {
    return -1;
  }
  return v->get_xaid() ;
}

int safeLoginSessions::save_xaid(int cid,int id)
{
  tSessionDetails *v = get_session(cid);

  /* not exist */
  if (!v) {
    return -1;
  }
  v->save_xaid(id) ;
  return 0;
}

int safeLoginSessions::reset_xaid(int cid)
{
  return save_xaid(cid,-1);
}


/*
 * class safeDataNodeList
 */
/* get related data node connection info */
tDNInfo* safeDataNodeList::get(int dn) 
{
  try_read_lock();

  return find(dn);
}

tDNInfo* safeDataNodeList::get_by_fd(int myfd) 
{
  size_t dn=0;
  tDNInfo *pd = 0;

  try_read_lock();
  for (dn=0;dn<m_list.size();dn++) {
    pd = find(dn);
    if (!pd) continue ;
    if (pd->mysql->sock==myfd)
      return pd ;
  }
  return 0;
}

int safeDataNodeList::add(int dn, tDNInfo *info)
{
  tDNInfo *pd = get(dn);

  /* not exist */
  if (!pd) {
    pd = new tDNInfo ;
    try_write_lock();
    insert(dn,pd);
  }
  {
    try_write_lock();
    memcpy(pd,info,sizeof(tDNInfo));
  }
  return 0;
}

/* add new connection info */
int safeDataNodeList::add(int dn, uint32_t addr, int port,
  char *schema, char *usr, char *pwd)
{
  tDNInfo *pd = get(dn);

  /* not exist */
  if (!pd) {
    pd = new tDNInfo ;
    try_write_lock();
    insert(dn,pd);
  }
  {
    try_write_lock();
    pd->addr=addr;
    pd->port=port ;
    strcpy(pd->schema,schema);
    strcpy(pd->usr,usr);
    strcpy(pd->pwd,pwd);
  }
  return 0;
}

/* delete connection info */
int safeDataNodeList::drop(int dn)
{
  if (!get(dn))
    return -1;
  {
    try_write_lock();
    safe_container_base<int,tDNInfo*>::drop(dn);
  }
  return 0;
}

/* clear all items */
void safeDataNodeList::clear(void)
{
  try_write_lock();

  list_foreach_container_item {
    if (i.second->mysql) {
      mysql_close(i.second->mysql);
    }
    delete i.second ;
  }
  m_list.clear();
}

/* inactivate all items */
void safeDataNodeList::update_batch_stats(int st)
{
  try_read_lock();

  list_foreach_container_item {
    i.second->stat = st;
  }
}

/*
 * class safeTableDetailList
 */
int safeTableDetailList::add(char *schema, 
  char *table, char *phy_schema, int dn)
{
  uint64_t key = gen_key(schema,table);
  tTblDetails *td = get(key);

  if (!td) {
    td = new tTblDetails ;
    td->key     = key ;
    td->columns = 0;
    td->conf_dn = 0;
    try_write_lock();
    insert(key,td);
  } else {
    log_print("warning: found existing entry for %lu\n",key);
  }
  /* update table info */
  td->schema      = schema ;
  td->table       = table ;
  td->dn          = dn ;
  td->num_cols    = 0;
  if (phy_schema)
    td->phy_schema= phy_schema ;
  return 0;
}

int safeTableDetailList::add(char *schema, 
  char *table, char *column, uint16_t charset, uint32_t maxlen, 
  uint8_t type, uint16_t flags)
{
  uint64_t key = gen_key(schema,table);
  tTblDetails *td = get(key);
  tColDetails *cd = 0, *prev = 0;
  uint16_t nCol = 0;

  if (!td) {
    td = new tTblDetails ;
    td->key = key ;
    try_write_lock();
    insert(key,td);
  }
  /* iterate column list */
  for (cd=td->columns;nCol<td->num_cols&&cd/*&&cd->next*/;
     prev=cd,cd=cd->next,nCol++) {
    if (!strcasecmp(column,cd->col.name))
      break ;
  }
  if (nCol>=td->num_cols) {
    /* table's empty, create the first one */
    if (!td->columns) {
      td->columns = new tColDetails ;
      cd = td->columns ;
      cd->next = NULL;
    /*log_print("insert entry for %s.%s, col %s, pid: %d\n",
      schema,table,column,getpid());*/
    } else if (!cd/*->next*/) {
      /* add to tail of column list */
      cd = new tColDetails ;
      cd->next  = NULL;
      prev->next= cd ;
    /*log_print("insert1 entry for %s.%s, col %s, pid: %d\n",
      schema,table,column,getpid());*/
    }
    td->num_cols ++ ;
  }
  /* update the column */
  strcpy(cd->col.schema,schema);
  strcpy(cd->col.tbl,table);
  strcpy(cd->col.name,column);
  cd->col.charset = charset ;
  cd->col.len     = maxlen ;
  cd->col.type    = type ;
  cd->col.flags   = flags ;
  /* update table info */
  td->schema      = schema ;
  td->table       = table ;
  return 0;
}

int safeTableDetailList::add(char *schema, 
  char *table, char *col, char *dtype, 
  bool null_able, uint8_t key_type, char *def_val, 
  char *ext)
{
  uint64_t key = gen_key(schema,table);
  tTblDetails *td = get(key);
  tColDetails *cd = 0;
  uint16_t nCol = 0;

  if (!td) {
    /* FATAL: no table entry found */
    log_print("fatal: no entry for %lu found\n",key);
    return -1;
  }
  try_read_lock();
  /* iterate column list */
  for (cd=td->columns;nCol<td->num_cols&&cd&&cd->next;
     cd=cd->next,nCol++) {
    if (!strcmp(cd->col.name,col))
      break ;
  }
  if (nCol>=td->num_cols) {
    /* no specific column found */
    return -1;
  }
  /* update the column extra information */
  bzero(&cd->ext,sizeof(cd->ext));
  cd->ext.null_able = null_able ;
  cd->ext.key_type  = key_type ;
  if (dtype) 
    strcpy(cd->ext.display_type,dtype);
  if (def_val) 
    strcpy(cd->ext.default_val,def_val);
  if (ext) 
    strcpy(cd->ext.extra,ext);
  return 0;
}

int safeTableDetailList::add(char *schema, 
  char *table, uint8_t dn, uint8_t io_type)
{
  uint64_t key = gen_key(schema,table);
  tTblDetails *td = get(key);
  tDnMappings *pm = 0, *dm = 0;

  if (!td) {
    /* FATAL: no table entry found */
    log_print("fatal: no entry for %lu found\n",key);
    return -1;
  }
  /* find suitable item */
  for (pm=td->conf_dn;pm&&pm->next&&
    pm->dn!=dn; pm=pm->next) ;
  /* found matched space */
  if (pm&&pm->dn==dn) {
    pm->io_type = io_type ;
  } else {
    /* no data node config, then add one */
    dm = new tDnMappings ;
    dm->next   = 0;
    dm->dn     = dn;
    dm->io_type= io_type;
    if (!td->conf_dn) td->conf_dn = dm ;
    else if (!pm->next) pm->next = dm ;
  }
  if (td->dn<0) 
    td->dn = dm->dn ;
  return 0;
}

tColDetails* safeTableDetailList::get_col(char *sch, char *tbl, tColDetails *prev)
{
  uint64_t key = gen_key(sch,tbl);
  tTblDetails *td = 0;

  td = get(key);
  if (!td) {
    return 0;
  }
  return !prev?td->columns:prev->next;
}

tColDetails* safeTableDetailList::get_col(char *sch, char *tbl, char *col)
{
  uint64_t key = gen_key(sch,tbl);
  tTblDetails *td = 0;
  tColDetails *pc = 0;
  uint16_t ncol = 0;

  td = get(key);
  if (!td) {
    return 0;
  }
  /* iterates the column list */
  for (pc=td->columns,ncol=0;pc&&ncol<td->num_cols;
     pc=pc->next,ncol++) {
    if (!strcasecmp(pc->col.name,col)) 
      return pc ;
  } /* end for(...) */
  return 0;
}

tTblDetails* safeTableDetailList::get(uint64_t key)
{
  try_read_lock();
  return find(key);
}

int safeTableDetailList::drop(uint64_t key)
{
  tTblDetails *td = get(key);
  tColDetails *cd = 0, *pc = 0;
  tDnMappings *pd=0, *dm=0 ;

  if (!td)
    return -1;
  {
    try_write_lock();
    safe_container_base<uint64_t,tTblDetails*>::drop(key);
  }
  /* release columns */
  for (pc=td->columns;pc;) {
    cd = pc ;
    pc=pc->next ;
    delete cd ;
  }
  /* release data node mappings */
  for (pd=td->conf_dn;pd;) {
    dm = pd ;
    pd = pd->next ;
    delete dm ;
  }
  delete td ;
  log_print("droping entry key %lu\n",key);
  return 0;
}

void safeTableDetailList::clear(void)
{
  tColDetails *cd = 0, *pc = 0;
  tDnMappings *pd=0, *dm=0 ;

  try_write_lock();

  list_foreach_container_item {
    /* release columns */
    for (pc=i.second->columns;pc;) {
      cd = pc ;
      pc=pc->next ;
      delete cd ;
    }
    /* release data node mappings */
    for (pd=i.second->conf_dn;pd;) {
      dm = pd ;
      pd = pd->next ;
      delete dm ;
    }
    /* release table */
    delete i.second ;
  }
  m_list.clear();
}

uint64_t safeTableDetailList::gen_key(char *schema, char *tbl)
{
  return ((uint64_t)str2id(schema))<<32|
    str2id(tbl);
}

/*
 * class safeDnStmtMapList
 */
int safeDnStmtMapList::gen_key(int grp, int dn) const
{
  return ((grp<<16)&0xffff0000) | (dn&0xffff) ;
}

int safeDnStmtMapList::get(int grp, int dn)
{
  const int key = gen_key(grp,dn);
  int stmtid = 0;

  {
    try_read_lock();
    stmtid = find(key) ;
  }
  return !stmtid?-1:(stmtid-1);
}

int safeDnStmtMapList::add(int grp, int dn, int stmtid)
{
  const int key = gen_key(grp,dn);
  
  try_write_lock();
  insert(key,stmtid+1);
  return 0;
}

void safeDnStmtMapList::drop(int grp, int dn)
{
  const int key = gen_key(grp,dn);
  
  try_write_lock();
  safe_container_base<int,int>::drop(key);
}

/* 
 * class safeStmtInfoList
 */
tStmtInfo* safeStmtInfoList::get(int lstmtid)
{
  try_read_lock();
  return find(lstmtid);
}

tStmtInfo* safeStmtInfoList::get(char *req, size_t sz)
{
  char *stmt = req + 5;
  const uint32_t skey = str2id(stmt);
  keyIndex_t::iterator i ;

  {
    try_read_lock();
    i = m_keyIndex.find(skey);
  }
  /* key value not found */
  if (i==m_keyIndex.end()) {
    return 0;
  }
  return i->second ;
}

int safeStmtInfoList::add_key_index(char *req, tStmtInfo *info)
{
  char *stmt = req + 5;
  const uint32_t skey = str2id(stmt);

  try_write_lock();
  m_keyIndex[skey] = info;
  return 0;
}

tStmtInfo* safeStmtInfoList::new_mapping(int lstmtid)
{
  tStmtInfo *ps = new tStmtInfo ;

  ps->lstmtid = lstmtid ;
  ps->m_root  = 0;
  {
    try_write_lock();
    safe_container_base<int,tStmtInfo*>::insert(lstmtid,ps);
  }
  return ps ;
}

int safeStmtInfoList::add_mapping(
  int lstmtid, //logical statement id
  int grp, //datanode group id
  int dn, //datanode id
  int stmtid //physical statement id
  )
{
  tStmtInfo *ps = get(lstmtid);

  if (!ps) {
    ps = new_mapping(lstmtid);
  }
  ps->maps.add(grp,dn,stmtid);
  log_print("added mapping: lstmtid %d -> grp %d "
    "dn %d stmtid %d\n", lstmtid, grp, dn, stmtid);
  return 0;
}

int 
safeStmtInfoList::add_prep(int lstmtid, char *prep, size_t sz, 
  stxNode *stree, tSqlParseItem &sp)
{
  tStmtInfo *ps = get(lstmtid);

  if (!ps) {
    ps = new_mapping(lstmtid) ;
  }
  ps->prep_req.tc_resize(sz+1) ;
  ps->prep_req.tc_write(prep,sz+1) ;
  ps->m_root = stree ;
  ps->sp = sp ;
  /* add statement key mapping */
  add_key_index(prep,ps);
  return 0;
}

int safeStmtInfoList::get_prep(int lstmtid, char* &prep, size_t &sz)
{
  tStmtInfo *ps = get(lstmtid);

  if (!ps) {
    return -1 ;
  }
  prep = ps->prep_req.tc_data();
  sz   = ps->prep_req.tc_length();
  return 0;
}

int safeStmtInfoList::drop(int lstmtid)
{
  tStmtInfo *ps = get(lstmtid);

  if (!ps) {
    return -1 ;
  }
  {
    try_write_lock();
    safe_container_base<int,tStmtInfo*>::drop(lstmtid);
    delete ps ;
  }
  return 0;
}

void safeStmtInfoList::clear(void)
{
  sql_tree st ;

  try_write_lock();
  list_foreach_container_item {
    if (i.second->m_root) {
      st.destroy_tree(i.second->m_root,true);
      i.second->m_root = 0;
    }
    delete i.second;
  }
}

/*
 * safeClientStmtInfoList
 */
tClientStmtInfo* safeClientStmtInfoList::get(int cid) 
{
  try_read_lock();
  return find(cid);
}

int safeClientStmtInfoList::get_lstmtid(int cid, int &lstmtid) 
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    log_print("get current logical statement id from "
      "client %d error\n",cid);
    return -1;
  }

  lstmtid = __sync_fetch_and_add(&ci->curr.lstmtid,0) ;

  return 0;
}

int safeClientStmtInfoList::get_cmd_state(int cid, int &st)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  st = __sync_fetch_and_add(&ci->curr.state,0) ;
  return 0;
}

int safeClientStmtInfoList::set_cmd_state(int cid, int st)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  {
    __sync_lock_test_and_set(&ci->curr.state,st);
  }
  return 0;
}

int safeClientStmtInfoList::get_blob(int cid, char* &req, size_t &sz)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  {
    try_read_lock();
    req = ci->curr.blob_buf.tc_data();
    sz  = ci->curr.blob_buf.tc_length();
  }
  return sz>0?0:1;
}

int safeClientStmtInfoList::save_blob(int cid, char *req, size_t sz)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  {
    try_write_lock();
    ci->curr.blob_buf.tc_concat(req,sz);
  }
  __sync_fetch_and_add(&ci->curr.rx_blob,1);
  return 0;
}

int safeClientStmtInfoList::set_total_blob(int cid, int nblob)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  {
    try_write_lock();
    __sync_lock_test_and_set(&ci->curr.total_blob,nblob);
  }
  return 0;
}

bool safeClientStmtInfoList::is_blobs_ready(int cid)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return false;
  }
  try_read_lock();
  return ci->curr.total_blob==ci->curr.rx_blob ;
}

bool safeClientStmtInfoList::is_exec_ready(int cid)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return false;
  }
  try_read_lock();
  return ci->curr.state==st_stmt_exec ;
}

int safeClientStmtInfoList::save_exec_req(
  int cid,
  int xaid,
  char *req, 
  size_t sz
  )
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    try_write_lock();
    ci->curr.exec_buf.tc_resize(sz);
    ci->curr.exec_buf.tc_write(req,sz);
  }
  __sync_lock_test_and_set(&ci->curr.state,st_stmt_exec);
  __sync_lock_test_and_set(&ci->curr.exec_xaid,xaid);
  return 0;
}

int 
safeClientStmtInfoList::get_prep_req(int cid, char* &req, size_t &sz)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    ci->stmts.get_prep(ci->curr.lstmtid,req,sz);
  }
  return 0;
}

int 
safeClientStmtInfoList::get_exec_req(int cid, int &xaid, char* &req, size_t &sz)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    try_read_lock();
    req= ci->curr.exec_buf.tc_data();
    sz = ci->curr.exec_buf.tc_length();
  }
  xaid = __sync_fetch_and_add(&ci->curr.exec_xaid,0);
  return 0;
}

int safeClientStmtInfoList::get_parser_item(int cid, int lstmtid, 
  tSqlParseItem* &sp)
{
  tClientStmtInfo *ci = get(cid);
  tStmtInfo *si = 0;

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  si = ci->stmts.get(lstmtid);
  if (!si) {
    return -1;
  }
  {
    try_read_lock();
    sp = &si->sp ;
  }

  return 0;
}

int safeClientStmtInfoList::set_curr_sp(int cid, int lstmtid)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  /* query mode */
  if (lstmtid<0) {
    try_read_lock();
    ci->curr.sp = &ci->qryInfo.sp ;
  }
  /* prepare mode */
  else {
    tStmtInfo *si = ci->stmts.get(lstmtid);

    if (!si) {
      return -1;
    }
    try_read_lock();
    ci->curr.sp = &si->sp ;
  }
  return 0;
}

int safeClientStmtInfoList::get_curr_sp(int cid, tSqlParseItem* &sp)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }

  sp = ci->curr.sp ;
  return 0;
}

int safeClientStmtInfoList::set_total_phs(int cid, int nphs)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    __sync_lock_test_and_set(&ci->curr.total_phs,nphs);
  }
  return 0;
}

int safeClientStmtInfoList::get_total_phs(int cid, int &nphs)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    nphs = ci->curr.total_phs ;
  }
  return 0;
}

int safeClientStmtInfoList::reset(int cid)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    try_read_lock();
    ci->curr.rx_blob = 0;
    ci->curr.total_blob = 0;
    ci->curr.blob_buf.tc_update(0);
    ci->curr.exec_buf.tc_update(0);
  }
  return 0;
}

int 
safeClientStmtInfoList::add_stmt(int cid, char *prep_req, size_t sz, 
  uint16_t nPhs, stxNode *stree, tSqlParseItem &sp)
{
  tClientStmtInfo *ci = get(cid);
  int lstmtid = 0;

  /* test if statement is already added */
  if (ci && ci->stmts.get(prep_req,sz)) {
    return 0;
  }
  if (!ci) {
    ci = new tClientStmtInfo ;
    ci->lstmtid_counter = 0;
    ci->cid = cid ;
    try_write_lock();
    safe_container_base<int,tClientStmtInfo*>::insert(cid,ci);
  }
  ci->curr.rx_blob   = 0;
  ci->curr.total_blob= 0;
  ci->curr.total_phs = static_cast<int>(nPhs);
  ci->curr.state= st_na ;
  ci->curr.blob_buf.tc_update(0);
  ci->curr.exec_buf.tc_update(0);
  ci->curr.exec_xaid = 0;
  /* the statement was not add prepared, create 
   *  new statement id */
  lstmtid = __sync_fetch_and_add(&ci->lstmtid_counter,1);
  ci->stmts.add_prep(lstmtid,prep_req,sz,stree,sp);
  ci->curr.lstmtid = lstmtid ;
  log_print("saved lstmtid %d\n",lstmtid);
  return 0;
}

int 
safeClientStmtInfoList::add_qry_info(int cid, tSqlParseItem &sp)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    ci = new tClientStmtInfo ;
    ci->lstmtid_counter = 0;
    ci->cid = cid ;
    try_write_lock();
    safe_container_base<int,tClientStmtInfo*>::insert(cid,ci);
  }

  {
    try_write_lock();
    ci->qryInfo.sp = sp ;
  }

  return 0;
}

int 
safeClientStmtInfoList::get_qry_info(int cid, tSqlParseItem &sp)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }

  {
    try_read_lock();
    sp = ci->qryInfo.sp ;
  }
  return 0;
}

int
safeClientStmtInfoList::add_mapping(int cid, int lstmtid, int grp, int dn, int stmtid)
{
  tClientStmtInfo *ci = get(cid);

  if (!ci) {
    return -1;
  }
  return ci->stmts.add_mapping(lstmtid,grp,dn,stmtid);
}

int 
safeClientStmtInfoList::get_mapping(int cid, int lstmtid, int grp, int dn, int &stmtid)
{
  tStmtInfo *si = 0;
  tClientStmtInfo *ci = get(cid);

  stmtid = -1;
  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  /* get statement info by client-based logical 
   *  statement id */
  si = ci->stmts.get(lstmtid);
  if (!si) {
    return -1;
  }
  stmtid = si->maps.get(grp,dn);
  if (stmtid<0)
    return -1;
  return 0;
}

stxNode* safeClientStmtInfoList::get_stree(int cid, char *req, size_t sz)
{
  tClientStmtInfo *ci = get(cid);
  tStmtInfo *si = 0;

  /* get client statements info by cid */
  if (!ci) {
    log_print("found no client info %d\n",cid);
    return NULL;
  }
  si = ci->stmts.get(req,sz);
  return si?si->m_root:NULL ;
}

int safeClientStmtInfoList::drop(int cid)
{
  tClientStmtInfo *ci = get(cid);

  /* get client statements info by cid */
  if (!ci) {
    return -1;
  }
  {
    try_write_lock();
    safe_container_base<int,tClientStmtInfo*>::drop(cid);
  }
  delete ci ;
  return 0;
}

void safeClientStmtInfoList::clear(void)
{
  try_write_lock();
  list_foreach_container_item {
    delete i.second;
  }
}

/*
 * class safeStdVector
 */
void safeStdVector::push(int v)
{
  try_write_lock();
  safe_queue_base<int>::push(v);
}

int safeStdVector::pop(int &v)
{
  {
    try_write_lock();
    if (!m_queue.empty()) {
      v = safe_queue_base<int>::top();
      safe_queue_base<int>::pop();
    } else {
      return -1;
    }
  }
  return 0;
}

/*
 * class safeScheQueue
 */
void 
safeScheQueue::push(void *st,int cfd, int cmd, 
  char *req, size_t sz, int cmd_state)
{
  tSchedule *ps = new tSchedule ;

  ps->st  = st;
  ps->cfd = cfd;
  ps->cmd = cmd;
  ps->req = new char[sz] ;
  memcpy(ps->req,req,sz);
  ps->sz  = sz ;
  ps->cmd_state = cmd_state ;
  {
    try_write_lock();
    safe_queue_base<tSchedule*>::push(ps);
  }
}

int 
safeScheQueue::pop(void *&st, int &cfd, int &cmd, 
  char* req, size_t &sz, int &cmd_state)
{
  tSchedule *ps = 0;

  {
    try_write_lock();
    if (!m_queue.empty()) {
      ps = safe_queue_base<tSchedule*>::top();
      safe_queue_base<tSchedule*>::pop();
    } else {
      return -1;
    }
  }
  if (!ps) {
    return -1;
  }
  st = ps->st;
  cfd= ps->cfd ;
  cmd= ps->cmd ;
  memcpy(req,ps->req,ps->sz);
  sz = ps->sz ;
  cmd_state = ps->cmd_state ;
  delete []ps->req ;
  delete ps ;
  return 0;
}

/*
 * class safeXAList
 */
xa_item* safeXAList::get(int xaid) 
{
  try_read_lock();
  return find(xaid);
}

int safeXAList::add(int clientfd, int gid, void *parent)
{
  int id = __sync_add_and_fetch(&m_xaid,1);
  xa_item *px = new xa_item(id);

  px->set_xa_info(clientfd,gid,parent);
  {
    try_write_lock();
    insert(id,px);
  }
  return id;
}

int safeXAList::drop(int xid)
{
  xa_item *px = get(xid) ;

  if (!px) {
    return -1;
  }
  {
    try_write_lock();
    safe_container_base<int,xa_item*>::drop(xid);
  }
  px->reset_xid();
  px->m_cols.clear();
  delete px ;
  return 0;
}

void safeXAList::clear(void)
{
  try_write_lock();
  list_foreach_container_item {
    delete i.second;
  }
}


/* 
 * class safeRxStateList
 */

/* get related state item */
uint8_t safeRxStateList::get(int fd) 
{
  try_read_lock();
  return find(fd);
}

/* add new state */
int safeRxStateList::add(int fd)
{
  if (get(fd)>0) {
    try_write_lock();
    m_list[fd] = rs_none;
    return 0;
  }
  try_write_lock();
  insert(fd,rs_none);
  return 0;
}

int safeRxStateList::set(int fd, uint8_t new_st)
{
  if (!get(fd)) {
    return -1;
  }
  try_write_lock();
  m_list[fd] = new_st;
  return 0;
}

uint8_t safeRxStateList::next(int fd)
{
  uint8_t st = get(fd);
  
  if (!st) {
    return 0;
  }
  try_write_lock();
  insert(fd,++st);
  return st;
}

/* delete */
int safeRxStateList::drop(int fd)
{
  try_write_lock();
  safe_container_base<int,uint8_t>::drop(fd);
  return 0;
}

/* clear all items */
void safeRxStateList::clear(void)
{
  try_write_lock();
  m_list.clear();
}

/*
 * class safeColDefGroup
 */

/* get related column def item by myfd */
tContainer* safeColDefGroup::get(int myfd) 
{
  try_read_lock();
  return find(myfd);
}

/* get 1st column def item in list */
tContainer* safeColDefGroup::get_first(void) 
{
  try_read_lock();

  ITR_TYPE i;

  return next(i,true);
}

/* add attach column def contents */
int safeColDefGroup::add(int myfd, char *res, size_t sz)
{
  tContainer *pc = get(myfd);

  if (!pc) {
    pc = new tContainer;
    try_write_lock();
    m_list[myfd] = pc ;
  }
  {
    try_write_lock();
    pc->tc_concat(res,sz);
  }
  return 0;
}

/* reset item */
int safeColDefGroup::reset(int myfd)
{
  tContainer *pc = get(myfd);

  if (!pc) {
    return -1;
  }

  {
    try_read_lock();
    pc->tc_update(0);
  }
  return 0;
}

/* clear all items */
void safeColDefGroup::clear(void)
{
  list_foreach_container_item {
    i.second->tc_close();
    delete i.second ;
  }
  m_list.clear();
}


/*
 * class safeStMapList
 */
int safeStMapList::add(STM_PAIR pair)
{
  try_write_lock();
  insert(pair);
  return 0;
}

#if 0
int safeStMapList::next(STM_PAIR &pair, bool bStart)
{
  try_read_lock();

  int ret = safe_vector_base<STM_PAIR>::next(m_itr,bStart);

  if (ret)
    return -1;

  pair = *m_itr ;
  return 0;
}
#else
int safeStMapList::do_iterate(ITR_FUNC f)
{
  try_read_lock();

  for (auto i:m_list) {
    f(i);
  }
  return 0;
}
#endif

void safeStMapList::clear(void)
{
  try_write_lock();
  m_list.clear();
}


