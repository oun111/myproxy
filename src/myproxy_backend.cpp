
#include <string>
#include "myproxy_trx.h"
#include "myproxy_backend.h"
#include "sql_parser.h"
#include "sql_router.h"
#include "simple_types.h"
#include "sock_toolkit.h"
#include "porting.h"
#include "env.h"
#include "dnmgr.h"
#include "dbug.h"

using namespace GLOBAL_ENV;
using namespace global_parser_items;

/*
 * class myproxy_backend
 */
myproxy_backend::myproxy_backend(safeLoginSessions &ss) :
  m_caches(*this),m_lss(ss)
{
  (void)mysql_cmd2str;
}

myproxy_backend::~myproxy_backend() 
{
}

int myproxy_backend::close(int cid)
{
  end_xa(cid);

  /* release the session */
  m_lss.drop_session(cid);

  /* release the statement info */
  m_stmts.drop(cid);

  return 0;
}

int myproxy_backend::schedule_close(int cid)
{
  tSessionDetails *pss = 0;
  int xaid = 0;
  xa_item *xai = 0;
  int st = st_na ;

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  /* remove related pending items */
  m_pendingQ.drop(cid);

  m_stmts.get_cmd_state(cid,st);

  /* FIXME: if no related xa, close cid immediately. BUT, 
   *  how to know before calling new_xa() ? */
  if ((xaid=pss->get_xaid())<0 || !(xai=m_xa.get_xa(xaid))) {

    this->close(cid);

    return 0;
  }

  /* the response process is done, also close cid */
  if (st==st_done) {

    this->close(cid);

    return 0;
  }

  /* there're more backends to be received, so don't close now */
  xai->set_schedule_close(1);

  return 0;
}

int 
myproxy_backend::new_xa(tSessionDetails *pss, sock_toolkit *st, 
  int cid, int cmd, char *req, size_t sz, int &xaid, int cmd_state)
{
  if ((xaid=pss->get_xaid())>0) {
    log_print("use xaid %d\n",xaid);
    return 0;
  }
  /* acquire a new transaction */
  if (m_xa.begin(cid,this,xaid)) {
    /* no xa available, so buffer this request 
     *  for later executing */
    m_pendingQ.push(st,cid,cmd,req,sz,cmd_state);
    log_print("adding redo for client %d\n", cid);
    return /*-1*/1;
  }
  pss->save_xaid(xaid);
  log_print("acquired xa %d cid %d %p\n",xaid,cid,m_xa.get_xa(xaid));
  return 0;
}

int myproxy_backend::end_xa(xa_item *xa)
{
  int xaid = 0;

  if (!xa) {
    log_print("invalid xa\n");
    return -1 ;
  }
  xaid = xa->get_xid() ;
  /* release transaction */
  log_print("try to release xaid %d(%p) cid %d\n",
    xaid,xa,xa->get_client_fd());

  m_xa.end_exec(xa);

  /* return back the cache */
  m_caches.return_cache(xa);

  /* release the xa itself */
  m_xa.release(xaid);
  log_print("xaid %d released\n",xaid);

  /* redo jobs */
  try_do_pending();

  return 0;
}

int myproxy_backend::end_xa(int cid)
{
  tSessionDetails *pss = 0;
  int xaid = 0;

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  if ((xaid=pss->get_xaid())>0) {
    xa_item *xai = m_xa.get_xa(xaid);

    end_xa(xai);

    m_lss.reset_xaid(cid);

  } else {
    log_print("warning: cid %d has no xa\n",cid);
  }
  return 0;
}

int 
myproxy_backend::get_route(int xaid, 
  int cid,
  tSqlParseItem *sp,
  bool fullroute, /* route to all datanodes or not */ 
  bool needcache,
  std::set<uint8_t> &rlist)
{
  xa_item *xai = m_xa.get_xa(xaid);
  int gid = xai->get_gid();
  auto nodes = m_xa.get_dnmgr().get_specific_group(gid);

  if (!nodes) {
    log_print("no dn group found by id %d\n",gid);
    return -1;
  }

  sql_router router(*nodes) ;
  if (fullroute?router.get_full_route_by_conf(sp,rlist):
     router.get_route(cid,sp,rlist)) {

    log_print("error calculating routes\n");

    {
      char outb[MAX_PAYLOAD] = "";
      size_t sz_out = do_err_response(0,outb,
        ER_INTERNAL_GET_ROUTE,
        ER_INTERNAL_GET_ROUTE);
      m_trx.tx(cid,outb,sz_out);
    }

    return -2;
  }

  /* acquire cache if multiple datanodes are invoked */
  if (needcache&& rlist.size()>1) {
    if (m_caches.acquire_cache(xai)) {
      return -1;
    }
  }

  return 0;
}

int myproxy_backend::try_do_pending(void)
{
  sock_toolkit *st = 0;
  int cfd = 0;
  char req[MAX_PAYLOAD] ;
  size_t sz = MAX_PAYLOAD;
  int cmd = 0;
  int cmd_state = st_na/*, ret=0*/ ;

  if (!m_pendingQ.pop((void*&)st,cfd,cmd,req,sz,cmd_state)) {

    /* reset xaid in related session */
    m_lss.reset_xaid(cfd);

    log_print("try redo job for client %d, cmd %d\n",cfd,cmd);
    if (cmd==com_stmt_prepare) {
      m_stmts.set_cmd_state(cfd,cmd_state);

      do_stmt_prepare(st,cfd,req,sz,cmd_state);
    }
    else if (cmd==com_query)  
      do_query(st,cfd,req,sz);
    else if (cmd==com_stmt_execute) 
      do_stmt_execute(st,cfd,req,sz);
  }
  return 0;
}

int 
myproxy_backend::do_get_stree(int cid, char *req, size_t sz,
  stxNode* &pTree, bool &bNewTree)
{
  char *pSql = req + 5;
  size_t szSql = sz - 5;

  /* try to fetch the sql tree related to current statement */
  pTree = m_stmts.get_stree(cid,req,sz);
  bNewTree = !pTree;

  /* if no tree found, then parse the statement statically 
   *  and build a new tree */
  if (!pTree) {
    pTree = parse_tree(pSql,szSql);
    log_print("build new tree for %d\n",cid);
  }
  return 0;
}

#define RETURN(rc) do {\
  if (bNewTree) del_tree(pTree);\
  return rc;\
} while(0)

/* do request in query mode */
int 
myproxy_backend::do_query(sock_toolkit *st,int cid, char *req, size_t sz)
{
  int xaid = -1, rc=0 ;
  char *pSql = req + 5;
  size_t szSql = sz - 5;
  tSessionDetails *pss = 0;
  std::set<uint8_t> rlist ;
  hook_framework hooks ;
  tSqlParseItem sp ;

  /* XXX: some query requests have '0' at the end, 
   *  but some dosen't, so trim all '0' from the 
   *  packet end */
  trim(req,&sz);

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  /* try to acquire a new transaction */
  if (new_xa(pss,st,cid,com_query,req,sz,xaid)) {
    /* no xa available currently */
    return 1;
  }

  char *pDb = const_cast<char*>(pss->db.c_str());
  /* try to fetch the sql tree related to current statement */
  stxNode *pTree = 0;
  bool bNewTree = false;

  /* get syntax tree */
  if (do_get_stree(cid,req,sz,pTree,bNewTree)) {
    return -1;
  }

  normal_hook_params params(pDb,sp);

  /* dynamic request hooking */
  if (hooks.run(&req,sz,/*pDb*/&params,h_sql,pTree)) {
    log_print("error filtering sql %s\n",pSql);
    RETURN(-1) ;
  }

  /* update the statement pointer */
  pSql = req+5 ;
  szSql= sz-5;

  /* dig informations from statement */
  tContainer err ;
  sql_parser parser ;

  if (parser.scan(pSql,szSql,&sp,pDb,err,pTree)) {
    log_print("error scan statement %s\n",pSql);

    m_trx.tx(cid,err.tc_data(),err.tc_length());

    RETURN(-1) ;
  }

  /* calcualte routing infomations */
  if ((rc=get_route(xaid,cid,&sp,false,true,rlist))) {

    if (rc==-1) {
      m_pendingQ.push(st,cid,com_query,req,sz,st_na);
      log_print("adding redo for client %d\n", cid);
    }

    RETURN(-1) ;
  }

  /* save the query info */
  m_stmts.add_qry_info(cid,sp);
  /* set current 'sp' item under query mode */
  m_stmts.set_curr_sp(cid);

  log_print("try to execute sql: %s\n",pSql);
  /* send and execute the request */
  if (m_xa.execute(st,xaid,com_query,rlist,req,sz,0,this)) {

    log_print("fatal: sending xa %d for client %d\n",xaid,cid);

    RETURN(-1) ;
  }

  RETURN(0);
}

/* do the stmt_prepare request */
int 
myproxy_backend::do_stmt_prepare(sock_toolkit *st, int cid, 
  char *req, size_t sz, int cmd_state)
{
  int xaid = -1 ;
  char *pSql = req + 5;
  size_t szSql = sz - 5;
  const char *pOldReq = req ;
  const size_t szOldReq = sz ;
  tSessionDetails *pss = 0;
  std::set<uint8_t> rlist ;
  tSqlParseItem *sp = 0, tsp ;
  hook_framework hooks ;

  /* XXX: some prepare requests have '0' at the end, 
   *  but some dosen't, so trim all '0' from the 
   *  packet end */
  trim(req,&sz);

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  /* try to acquire a new transaction */
  if (new_xa(pss,st,cid,com_stmt_prepare,req,sz,xaid,cmd_state)) {
    /* no xa available currently */
    return 1;
  }

  char *pDb = const_cast<char*>(pss->db.c_str());
  /* try to fetch the sql tree related to current statement */
  stxNode *pTree = 0;
  bool bNewTree = false;

  /* get syntax tree */
  if (do_get_stree(cid,req,sz,pTree,bNewTree)) {
    return -1;
  }

  {
    normal_hook_params params(pDb,tsp);

    /* dynamic request hooking */
    if (hooks.run(&req,sz,/*pDb*/&params,h_sql,pTree)) {
      log_print("error filtering sql %s\n",pSql);

      RETURN(-1);
    }
  }

  /* update the statement pointer */
  pSql = req+5 ;
  szSql= sz-5;

  /* dig informations from tree into 'sp' */
  if (cmd_state==st_prep_trans) {

    tContainer err ;
    sql_parser parser ;

    if (parser.scan(pSql,szSql,&tsp,pDb,err,pTree)) {

      log_print("error parse statement %s\n",pSql);

      /*  send the error message to client */
      m_trx.tx(cid,err.tc_data(),err.tc_length());

      RETURN(-1);
    }

    sp = &tsp;
  } 
  /* this's a 're-prepare' request, get parser info directly */
  else {
    m_stmts.get_curr_sp(cid,sp);
  }

  /* calculate sql routes */
  if (get_route(xaid,cid,sp,true,false,rlist)) {
    log_print("fail getting route\n");

    RETURN(-1);
  }

  /* this prepare command should send response to client */
  if (cmd_state==st_prep_trans) {
    /* save prepare info */
    m_stmts.add_stmt(cid,(char*)pOldReq,(size_t)szOldReq,0,pTree,*sp);
  } 
  else {
    if (bNewTree) del_tree(pTree);
  }

  m_stmts.set_cmd_state(cid,cmd_state);

  log_print("try to prepare sql %s\n",pSql);
  /* send and execute the request */
  if (m_xa.execute(st,xaid,com_stmt_prepare,rlist,req,sz,0,this)) {
    log_print("fatal: sending xa %d for client %d\n",xaid,cid);
    return -1;
  }

  return 0;
}

/* test if this statement was prepared before */
int 
myproxy_backend::test_prepared(int cid, const char *req, size_t sz, int xaid)
{
  xa_item *xai = m_xa.get_xa(xaid);
  int gid = xai->get_gid();
  auto nodes = m_xa.get_dnmgr().get_specific_group(gid);
  safeDataNodeList::ITR_TYPE itr ;
  int lstmtid = 0, stmtid = 0;
  tDNInfo *pd = 0;

  /* get logical statement id */
  mysqls_get_stmt_prep_stmt_id(const_cast<char*>(req),sz,&lstmtid);

  /* 
   * test if this statement was prepared before 
   */
  for (pd=nodes->next(itr,true);pd;(pd=nodes->next(itr))) {
    m_stmts.get_mapping(cid,lstmtid,gid,pd->no,stmtid);
    /* the statement was prepared at this group */
    if (stmtid>=0) 
      return 0;
  }

  log_print("logical stmt %d 's not been prepared on "
    "group %d, cfd %d\n", lstmtid,gid,cid);

  return -1;
}

int myproxy_backend::save_sharding_values(tSqlParseItem *sp, 
  int total_phs, char *pReq,size_t sz)
{
  int i=0, ret = 0;
  char val[64] = "";
  int t = 0;

  for (i=0; !ret && i<total_phs;i++) {
    /* successed get a place holder from request */
    if (!(ret=mysql_stmt_exec_get_ph(pReq,total_phs,i,&t,val))) {
      /* fill back the place-holder's value if it's 
       *  sharding column */
      //log_print("%d: t %d, %u\n",i,t,*(uint32_t*)(val));
      sp->update_sv(i,t,val);
      /* it's the 1st stmt_exec on the same statement,
       *  so save the types */
      sp->save_type(t);
    }
    /* XXX: 'ret=1' means it's not the 1st time to execute 
     *   this command, the new-bound flag of command is 0 */
  }
  if (!ret) {
    return 0;
  }
  /* refill the place-holders by the types saved before*/
  for (i=0;i<total_phs;i++) {
    /* get saved types */
    sp->get_type(i,t);
    /* get each place-holder's value */
    pReq += mysql_stmt_exec_get_ph_subsequent(pReq,
      !i,total_phs,t,val);
    /* update it */
    sp->update_sv(i,t,val);
  }
  return 0;
}

safeDnStmtMapList* 
myproxy_backend::get_stmt_id_map(int cid, char* req, size_t sz) 
{
  int lstmtid = 0;
  tStmtInfo *si=0;

  mysqls_get_stmt_prep_stmt_id(req,sz,&lstmtid);
  si = m_stmts.get(cid)->stmts.get(lstmtid) ;
  if (!si) {
    log_print("can't get mapping info by cid %d, "
      "lstmtid %d\n",cid,lstmtid);
    return 0;
  }
  return &si->maps ;
}

int myproxy_backend::get_parser_item(int cid, char *req, size_t sz, 
  tSqlParseItem* &sp)
{
  int lstmtid = 0;

  mysqls_get_stmt_prep_stmt_id(req,sz,&lstmtid);

  if (m_stmts.get_parser_item(cid,lstmtid,sp)) {
    log_print("fatal: cant get parser item, cid %d, lstmtid %d\n",
      cid,lstmtid);
    return -1;
  }

  /* set 'sp' item belonging to the statement
   *  that is about to be executed */
  m_stmts.set_curr_sp(cid,lstmtid);
  return 0;
}

int myproxy_backend::do_execute_blobs(int cid, int xaid, sock_toolkit *st, 
  std::set<uint8_t> &rlist, safeDnStmtMapList *maps)
{
  char *breq = 0;
  size_t len = 0;

  /* try to get the blob request */
  if (m_stmts.get_blob(cid,breq,len)) {
    return 0;
  }

  log_print("try sending blob list\n");

  /* iterates each blob requests in list */
  for (size_t sublen=0;len>0;len-=sublen,breq+=sublen) {

    sublen = mysqls_get_req_size(breq);

    /* send and execute the blob list */
    if (m_xa.execute(st,xaid,com_stmt_send_long_data,
        rlist,breq,sublen,(void*)maps,this)) {
      log_print("fatal: sending xa %d for client %d\n",xaid,cid);
      return -1;
    }

  } /* end for() */

  /* because the 'com_stmt_send_long_data' command has no response,
   *  so we free the myfd(s) now */
  xa_item *xai = m_xa.get_xa(xaid);

  if (xai) {
    m_xa.end_exec(xai);
  }

  return 0;
}

namespace {
  int try_close_physical_stmt(int myfd, int stmtid)
  {
    /*
     * the actual 'stmt_close' is implemented by 
     *  the idle task
     */
    m_mfMaps.add(myfd,stmtid);
    //log_print("add to close myfd %d stmtid %d\n",myfd,stmtid);

    return 0;
  }

} ;

/* do the stmt_close request */
int 
myproxy_backend::do_stmt_close(int cid, int stmtid)
{
  /* get logical statement related backend fd & physical id */
  tStmtInfo *pi = m_stmts.get(cid,stmtid);

  if (!pi) {
    log_print("found no statement maps for "
      "logical id %d\n", stmtid);
    return -1;
  }

  pi->maps.do_iterate(try_close_physical_stmt);

  return 0;
}

/* do the stmt_execute request */
int 
myproxy_backend::do_stmt_execute(sock_toolkit *st, int cid, char *req, size_t sz)
{
  int xaid = -1 ;
  int nphs = 0; /* total placeholders */
  int nblob = 0; /* total blob requests */
  tSessionDetails *pss = 0;
  std::set<uint8_t> rlist ;
  tSqlParseItem *sp = 0 ;
  safeDnStmtMapList *maps = 0 ;

  /* parse total blob-type place-holders */
  m_stmts.get_total_phs(cid,nphs);

  /* save total blob count */
  mysql_calc_blob_count(req,nphs,&nblob);
  m_stmts.set_total_blob(cid,nblob);

  /* the blob request are ready, so send the whole 
   *  execution requests */
  if (!m_stmts.is_blobs_ready(cid)) {
    /* buffer the exec request for the 
     *  send_blob req may use it*/
    m_stmts.save_exec_req(cid,0,req,sz);
    log_print("blob is not ready\n");
    return 0;
  }

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  /* try to acquire a new transaction */
  if (new_xa(pss,st,cid,com_stmt_execute,req,sz,xaid)) {
    /* no xa available currently */
    return 1;
  }

  /* prepare for calculating routes... */
  get_parser_item(cid,req,sz,sp);

  /* test whether this statement was prepared at 
   *  the datanode group before */
  if (test_prepared(cid,req,sz,xaid)) {

    /* no prepared before, just do : 
     *  save exec req body */
    m_stmts.save_exec_req(cid,xaid,req,sz);

    /* do re-prepare the statement on group */
    if (!m_stmts.get_prep_req(cid,req,sz)) {
      do_stmt_prepare(st,cid,req,sz,st_prep_exec);
    }
    return 1;
  }

  /* fill in sharding values if there're place holder(s) */
  save_sharding_values(sp,nphs,req,sz);

  {
    int ret = 0;

    /* calcualte routing infomations */
    if ((ret=get_route(xaid,cid,sp,false,true,rlist))) {

      if (ret==-1) {
        m_pendingQ.push(st,cid,com_stmt_execute,req,sz,st_stmt_exec);
        log_print("adding redo for client %d\n", cid);
      }

      return -1;
    }
  }

  /* update command state */
  m_stmts.set_cmd_state(cid,st_stmt_exec);

  maps = get_stmt_id_map(cid,req,sz);
  /* 
   * do execute request: 1. send blob list 2. send exec request 
   */
  /* send the blob requests */
  do_execute_blobs(cid,xaid,st,rlist,maps);

  /* try the request itself */
  log_print("try sending execution req\n");
  if (m_xa.execute(st,xaid,com_stmt_execute,rlist,req,sz,(void*)maps,this)) {
    log_print("fatal: sending xa %d for client %d\n",xaid,cid);
    return -1;
  }

  return 0;
}

/* do the send_long_data request */
int myproxy_backend::do_send_blob(int cid, char *req, size_t sz)
{
  auto pss = m_lss.get_session(cid);
  int xaid = 0;
  xa_item *xai = 0;

  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  xaid = pss->get_xaid();
  xai  = m_xa.get_xa(xaid);

  /* buffer the current blob request */
  m_stmts.save_blob(cid,req,sz);
  /* test if it's ready to execute statement */
  if (!m_stmts.is_blobs_ready(cid) || 
     !m_stmts.is_exec_ready(cid)) {
    log_print("blobs or exec not ready\n");
    return 0;
  }

  /* get buffered statement exec req */
  m_stmts.get_exec_req(cid,xaid,req,sz);

  /* try execute process */
  do_stmt_execute(xai->get_sock(),cid,req,sz);
  return 0;
}

int 
myproxy_backend::get_ordering_col(char *cols, int num_cols, char **name)
{
  int i=0, digit_col = -1;
  char *db=0, *tbl=0, *col=0;
  SHARDING_KEY *psk = 0; 
  uint8_t type = 0; 

  for (i=0;i<num_cols;i++) {
    if (mysqls_get_col_full_name(cols,i,&db,&tbl,&col,&type)) {
      log_print("can't get column full name\n");
      continue ;
    }    
    /* search for the 1st 'digit-type' column */
    if (digit_col<0 && mysqls_is_digit_type(type)) {
      digit_col = i ;
    }    
    /* match sharding column name */
    psk = m_conf.m_shds.get(db,tbl,col);
    if (!psk) {
      continue ;
    }    
    *name = col ;
    return 0;
  }
  /* retrive the 1st digit-type column name if 
   *  no sharding columns are matched */
  mysqls_get_col_full_name(cols,digit_col,0,0,name,0) ;
  return 0;
}

int
myproxy_backend::do_cache_row(xa_item *xai, int myfd, char *res, size_t sz)
{
  char *chOdrCol = 0;
  int cmd = xai->get_cmd();
  size_t numCols = xai->get_col_count(), szVal = 0;
  char *pCols = 0;
  tContainer colVal, *pc = xai->m_cols.get(myfd) ; ;
  auto pf = cmd==com_query?mysql_pickup_text_col_val:
    cmd==com_stmt_execute?mysql_pickup_bin_col_val:NULL;

  if (!pf) {
    log_print("fatal: invalid command %d\n", cmd);
    return -1;
  }

  if (!pc) {
    log_print("found no col def of fd %d\n",myfd);
    return -1;
  }

  if (pc->tc_length()==0) {
    log_print("fatal: no column definitions found on %d!!\n", myfd);
    return -1;
  }

  pCols = pc->tc_data();

  /* get name of order column */
  get_ordering_col(pCols,numCols,&chOdrCol);
  //log_print("ordering column name: %s\n", chOdrCol);

  /* 
   * get ordering column value by command type 
   */
  pf(pCols,numCols,res,sz,chOdrCol,0,&szVal,0);
  colVal.tc_resize(szVal);
  pf(pCols,numCols,res,sz,chOdrCol,reinterpret_cast<uint8_t*>
    (colVal.tc_data()),&szVal,0);
  colVal.tc_update(szVal);

	m_caches.cache_row(xai,colVal,res,sz);
  return 0;
}

int 
myproxy_backend::save_col_def_by_dn(xa_item *xai, int cfd, int myfd, 
  char *res, size_t sz)
{
  MYSQL_COLUMN mc ;
  auto pss = m_lss.get_session(cfd);

  if (!pss) {
    log_print("no login session found by %d\n",cfd);
    return -1;
  }

  /* transform protocol format to MYSQL_COLUMN format */
  mysqls_save_one_col_def(&mc,res,const_cast<char*>(pss->db.c_str()));
  xai->m_cols.add(myfd,(char*)&mc,sizeof(mc));

  return 0;
}

/* responses processing */
int 
myproxy_backend::deal_query_res_single_path(
  xa_item *xai,  /* transaction object */
  int myfd, /* backend db fd that responses the packet */
  char *res, 
  size_t sz
  )
{
  int cfd = xai->get_client_fd(), nCols = 0;
  int sn = mysqls_extract_sn(res);
  int ret = 0;
  int st = xai->m_states.get(myfd);

  /* 1st packet of response */
  if (sn==1 && st==0) {

    /* add new state of myfd */
    xai->m_states.add(myfd);

    /* get total columns */
    nCols = mysqls_extract_column_count(res,sz);

    /* no columns found in res set or ERROR OCCORS */
    if (!nCols || mysqls_is_error(res,sz)) {
      /* this's the last packet from current myfd, 
       *  jump to last state */
      xai->m_states.set(myfd,rs_ok);
    }

  }
  else if ((st==rs_none || st==rs_col_def) && 
      mysqls_is_eof(res,sz)) {

    xai->m_states.next_state(myfd);
  }

  /* last packet of last datanode recv */
  st = xai->m_states.get(myfd);
  if (st==rs_ok) {

    /* XXX: command exec ends!!! NO data should be received
     *  from this fd !!! */
    m_xa.end_exec(xai);

    /* reset if it's under statement prepare mode */
    m_stmts.reset(cfd);

    /* reset session command states */
    m_lss.set_cmd(cfd,st_idle,NULL,0);

    log_print("recv last eof on %d, cid %d\n",myfd, cfd);

    /* exit current myfd receiving as soon as possible!! */
    ret = 1;
  }

  /* send the delayed responses */
  if (m_caches.get_free_size(xai)<=sz || st==rs_ok) {
    m_caches.tx_pending_res(xai,cfd); 
  }

  /* buffer data if not the eof */
  if (st!=rs_ok) {
    m_caches.do_cache_res(xai,res,sz);
  } 
  else {
    /* this's the LAST packet */
    m_trx.tx(cfd,res,sz);

    /* triger the pending tx data to be send if there're */
    triger_epp_cache_flush(cfd);
  }

  return /*0*/ret;
}

int myproxy_backend::force_rollback(xa_item *xai,int cfd)
{
  tContainer tmp ;

  tmp.tc_resize(32);

  char *pb = tmp.tc_data();
  size_t ln = mysqls_gen_rollback(pb);

  /* don't reply to client */
  m_stmts.set_cmd_state(cfd,st_silence);

  log_print("force silence and rollback all "
    "backends cfd %d\n",cfd);

  /* emit rollback */
  do_query(xai->get_sock(),cfd,pb,ln);

  return 0;
}

int myproxy_backend::do_send_res(xa_item *xai, int cfd, char *res, size_t sz)
{
  tContainer tmp ;
  int xaid= xai->get_xid();
  int st = st_na ;

  /* get command states */
  m_stmts.get_cmd_state(cfd,st);

#if 0
  /* receive an error, try to rollback all backends */
  if (st!=st_silence && m_caches.is_err_pending(xai)) {

    force_rollback(xai,cfd);

    return 0;
  }
#endif

  /* reset to normal */
  m_stmts.set_cmd_state(cfd,st_na);

  /* 
   * case 1: error occours; 
   * case 2: single packet in response, such as response to 'insert' statements
   */
  if (m_caches.is_err_pending(xai) || xai->get_col_count()==0) {

    /* move 'err buffer' or 'tx buffer' -> tmp */
    m_caches.move_buff(xai,tmp);

    res = tmp.tc_data();
    sz  = tmp.tc_length();
  }
  /*
   * case 3: multi packets in response
   */
  else {
    /* tx the col defs */
    m_caches.tx_pending_res(xai,cfd);

    /* there maybe rows set in cache, tx them */
    m_caches.extract_cached_rows(m_stmts,xai);

    mysqls_update_sn(res,xai->inc_last_sn());
  }

  /* remove related pending items */
  m_pendingQ.drop(cfd);

  /* end current xa if: commit/rollback */
  if (m_stmts.test_xa_end(cfd)) {

    log_print("force to end xa %d cid %d\n",xaid,cfd);

    end_xa(cfd);
  }

  /* all are ok */
  m_stmts.set_cmd_state(cfd,st_done);

  /* if schedule to close this fd before, it's 
   *  time to close it now */
  if (xai->is_schedule_close()) {

    log_print("closing schedule cid %d\n",cfd);

    this->close(cfd);
  }

  /* send the last packet to client here.. */
  m_trx.tx(cfd,res,sz);

  /* triger the pending tx data to be send if there're */
  triger_epp_cache_flush(cfd);

  return 1;
}

int 
myproxy_backend::deal_query_res_multi_path(
  xa_item *xai,  /* transaction object */
  int myfd, /* backend db fd that responses the packet */
  char *res, 
  size_t sz
  )
{
  int cfd = xai->get_client_fd(), nCols = 0;
  int sn = mysqls_extract_sn(res);
  int st = st_na ;
  int ret = 0;

  /* get command states */
  m_stmts.get_cmd_state(cfd,st);

  /* 1st packet of response */
  if (sn==1 && xai->m_states.get(myfd)==0) {

    /* add new state of db fd */
    xai->m_states.add(myfd);

    /* get total columns */
    nCols = mysqls_extract_column_count(res,sz);
    if (nCols<255) {
      xai->set_col_count(nCols);
    }

    /* reset stuffs that needed by the column defs */
    if (xai->get_last_sn()==0) {
      xai->m_cols.reset(myfd);
      /* beginning with the 'sn' of column defs packets */
      xai->set_last_sn(2);
    }

    /* no columns found in res set or ERROR OCCORS */
    if (!nCols || mysqls_is_error(res,sz)) {

      /* if errors, save it */
      if (mysqls_is_error(res,sz)) {
        m_caches.save_err_res(xai,res,sz);
        log_print("err myfd %d cid %d\n",myfd,cfd);
      }

      /* this's the last packet from current myfd, 
       *  jump to last state */
      xai->m_states.set(myfd,rs_ok);

      /* for error or single response packet, sn is 1 */
      xai->set_last_sn(0);

      xai->inc_ok_count();

      /* there's only 1 packet in this response  */
      ret = 1;
    }

    /* find out last datanode */
    if (xai->inc_1st_res_count()<xai->get_desire_dn()) {
      return ret;
    }
    xai->set_last_dn(myfd);

    /* cache the 1st packet except:  error packet, 
     *  no silence state */
    if (!mysqls_is_error(res,sz) && st!=st_silence) {
      m_caches.do_cache_res(xai,res,sz);
    }
  }

  /* rx column defs */
  else if (xai->m_states.get(myfd)==rs_none) {

    /* end of column defs */
    if (mysqls_is_eof(res,sz)) {
      xai->m_states.next_state(myfd);
      /* save last sn */
      xai->set_last_sn(sn);
    } 
    else {
      /* caching the column definitions */
      save_col_def_by_dn(xai,cfd,myfd,res,sz);
    }

    /* packet is not from last data node, discard it */
    if (myfd!=xai->get_last_dn()) {
      return 0;
    }

    /* cache the col def */
    m_caches.do_cache_res(xai,res,sz);
  }

  /* recv the row sets */
  else if (xai->m_states.get(myfd)==rs_col_def) {

    /* end of row sets */
    if (mysqls_is_eof(res,sz)) {
      xai->m_states.next_state(myfd);
      //log_print("myfd %d eof\n",myfd);
      /* all response from current myfd is completed, but other
       *  myfds are not */
      if (xai->inc_ok_count()<xai->get_desire_dn()) 
        return /*0*/1;
    }
    /* cache row set if it's not from last datanode */
    else {
      do_cache_row(xai,myfd,res,sz);
      return 0;
    }
  }

  if (xai->m_states.get(myfd)!=rs_ok) 
    return 0;

  /* 
   * this's the last response packet received 
   */

  log_print("recv last eof on %d, cid %d\n",myfd, cfd);

  /* stop recv from current datanodes */
  m_xa.end_exec(xai);

  /* reset if it's under statement prepare mode */
  m_stmts.reset(cfd);

  /* reset session command states */
  m_lss.set_cmd(cfd,st_idle,NULL,0);

  /* send all responses to client */
  return do_send_res(xai,cfd,res,sz);

  //return 1;
}

int 
myproxy_backend::deal_query_res(
  xa_item *xai,  /* transaction object */
  int myfd, /* backend db fd that responses the packet */
  char *res, 
  size_t sz
  )
{
  int xaid = xai->get_xid();

  /* check validation of xaid */
  if (xaid<0 || !m_xa.get_xa(xaid)) {
    log_print("invalid xaid %d %p from %d\n", xaid,xai,myfd);
    xai->dump();
    return -1;
  }

  int nDn = xai->get_desire_dn();

  if (unlikely(nDn<=0)) {
    log_print("fatal: no source datanode!\n");
    return -1;
  }

  /* packets are from a single datanode */
  if (nDn==1) {
    return deal_query_res_single_path(xai,myfd,res,sz);
  }

  /* from multi datanodes */
  return deal_query_res_multi_path(xai,myfd,res,sz);
}

int myproxy_backend::try_pending_exec(int cfd,int xaid,sock_toolkit *st)
{
  char *req = 0;
  size_t sz = 0;
  int cmd_state = 0;
  int exec_xaid = 0;

  m_stmts.get_cmd_state(cfd,cmd_state) ;

  /* state = st_prep_exec, means an execute request 
   *  is pending, do it */
  if (cmd_state!=st_prep_exec) {
    return 0;
  }

  log_print("try the pending execute req\n");
  /* get the pending execute request */
  m_stmts.get_exec_req(cfd,exec_xaid,req,sz);
  if (exec_xaid!=xaid) {
    //log_print("xaid %d != %d\n",exec_xaid,xaid);
    return 0;
  }
  //log_print("exec_xid sz %zu\n",sz);
  if (sz>0) {
    /* execute it  */
    do_stmt_execute(st,cfd,req,sz);
  }

  return 0;
}

int 
myproxy_backend::get_last_sn(xa_item *xai)
{
  int end = xai->get_col_count()+xai->get_phs_count()+1;

  /* the serial number of eof packet */
  if (xai->get_col_count()>0) end += 1;
  if (xai->get_phs_count()>0) end += 1;

  return end ;
}

int 
myproxy_backend::do_add_mapping(int myfd, int cfd, 
  int stmtid, int lstmtid, xa_item *xai)
{
  int gid = xai->get_gid();
  auto nodes = m_xa.get_dnmgr().get_specific_group(gid);
  tDNInfo *pd = nodes->get_by_fd(myfd);

  log_print("try add_mapping for cid %d myfd %d\n",cfd,myfd);
  m_stmts.add_mapping(cfd,lstmtid,gid,myfd,pd->no,stmtid);

  return 0;
}

int 
myproxy_backend::deal_stmt_prepare_res(xa_item *xai, int myfd, char *res, size_t sz)
{
  int ret = 0;
  int lstmtid = 0, stmtid=0, st=st_na; 
  int nCols = 0, nPhs = 0;
  const int cfd = xai->get_client_fd();
  const int sn = mysqls_extract_sn(res);
  int xaid = xai->get_xid();
  bool isErr = (sn==1 && mysqls_is_error(res,sz)) ;

  /* check validation of xaid */
  if (xaid<0 || !m_xa.get_xa(xaid)) {
    log_print("invalid xaid %d %p from %d\n", xaid,xai,myfd);
    xai->dump();
    return -1;
  }

  m_stmts.get_cmd_state(cfd,st) ;

  if (isErr) {
    /* save the error message */
    m_caches.save_err_res(xai,res,sz);
  }
  else {

    /* the 1st normal response packet */
    if (sn==1&& xai->m_states.get(myfd)==0) {

      /* update backend fd states */
      xai->m_states.add(myfd);

      /* get currently operated logical statement id */
      mysqls_extract_prepared_info(res,sz,&stmtid,&nCols,&nPhs);
      if (m_stmts.get_lstmtid(cfd,lstmtid)) {
        log_print("error get logical statement id from %d\n",cfd);
        return -1;
      }

      /*  add lstmtid -> stmtid mapping here */
      do_add_mapping(myfd,cfd,stmtid,lstmtid,xai);

      /* save column & placeholder count */
      xai->set_col_count(nCols);
      xai->set_phs_count(nPhs);
      m_stmts.set_total_phs(cfd,nPhs);

      /* replace the out-sending statement id with 
       *  logical statement id */
      mysqls_update_stmt_prep_stmt_id(res,sz,lstmtid);

      if (xai->inc_1st_res_count()<xai->get_desire_dn()) {
        return 0;
      }

      xai->set_last_dn(myfd);
    }

    /* cache response packets from lastest datanode */
    if (st==st_prep_trans && myfd==xai->get_last_dn()) {
      m_caches.do_cache_res(xai,res,sz);
    }

  }

  /* 
   * last response packet reached 
   */
  if (isErr || sn==get_last_sn(xai)) {

    /* all response packets are received finished */
    if (xai->inc_ok_count()==xai->get_desire_dn()) {

      log_print("reach last eof of %d\n", cfd);

      /* XXX: should reset session related xaid before sending
       *  last prepare response to client under multi-thread envirogments */
      m_lss.reset_xaid(cfd);

      /* test if error occours */
      isErr = m_caches.is_err_pending(xai);

      /* sent to client */
      if (isErr) {
        m_caches.tx_pending_err(xai,cfd);
        log_print("trans err\n");
      } 
      else if (st==st_prep_trans) {
        log_print("prep trans sz: %zu\n",xai->m_txBuff.tc_length());
        m_caches.tx_pending_res(xai,cfd); 
      }

      sock_toolkit *p_stk = xai->get_sock();

      /* should release xa as soon as possible */
      end_xa(xai); 

      /* run any pending stmt_exec request only if no errors */
      if (!isErr) {
        try_pending_exec(cfd,xaid,p_stk);
      }

    }

    /* end current myfd receiving as soon as possible */
    ret = 1;
  }

  return ret;
}

int myproxy_backend::deal_stmt_execute_res(xa_item *xai, int cid, char *res, size_t sz)
{
  return deal_query_res(xai,cid,res,sz);
}

int myproxy_backend::deal_pkt(int myfd, char *res, size_t sz, void *arg)
{
  xa_item *xai = static_cast<xa_item*>(arg);
  int cmd = 0;

  if (!xai) {
    log_print("warning: no xa in myfd %d\n",myfd);
    return 0;
  }

  cmd = xai->get_cmd();

  switch (cmd) {
    case com_query:
      return deal_query_res(xai,myfd,res,sz);
      //break;

    case com_stmt_prepare:
      return deal_stmt_prepare_res(xai,myfd,res,sz);
      //break;

    case com_stmt_execute:
      return deal_stmt_execute_res(xai,myfd,res,sz);
      //break;

    /* no response for 'com_stmt_send_long_data:' */
#if 0
    case com_stmt_send_long_data:
      break;
#endif

    default:
      log_print("unknown response command %d from %d\n", cmd,myfd);
      break ;
  }
  return 0;
}

int 
myproxy_backend::tx(sock_toolkit *st, epoll_priv_data *priv, int fd)
{
  return 0;
}

int 
myproxy_backend::rx(sock_toolkit *st, epoll_priv_data* priv, int fd)
{
  m_trx.rx(st,priv,fd);
  return 0;
}

myproxy_epoll_trx&
myproxy_backend::get_trx(void)
{
  return m_trx ;
}

