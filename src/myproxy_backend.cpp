
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
  log_print("xaid %d(%p) cid %d released\n",xaid,xa,xa->get_client_fd());
  m_xa.end_exec(xa);
  /* return back the cache */
  m_caches.return_cache(xa);
  /* release the xa itself */
  m_xa.release(xaid);
  //m_lss.reset_xaid(cid);
  /* redo jobs */
  try_do_pending();
  return 0;
}

int myproxy_backend::end_xa(int cid)
{
  tSessionDetails *pss = 0;
  int xaid = 0;
  xa_item *xai = 0;

  /* get current connection region */
  pss  = m_lss.get_session(cid);
  if (!pss) {
    log_print("fatal: no connection state "
      "found for client %d\n", cid);
    return -1 ;
  }

  /* try redo pending jobs */
  if ((xaid=pss->get_xaid())>0) {
    xai = m_xa.get_xa(xaid);
    end_xa(xai);
    m_lss.reset_xaid(cid);
    try_do_pending();

  } else {
    log_print("warning: cid %d has no xa\n",cid);
  }
  return 0;
}

int 
myproxy_backend::get_route(int xaid, 
  int cid,
  tSqlParseItem &sp,
  bool fullroute, /* route to all datanodes or not */ 
  bool needcache,
  std::vector<uint8_t> &rlist)
{
  xa_item *xai = m_xa.get_xa(xaid);
  int gid = xai->get_gid();
  auto nodes = m_xa.get_dnmgr().get_specific_group(gid);

  if (!nodes) {
    log_print("no dn group found by id %d\n",gid);
    return -1;
  }

  sql_router router(*nodes) ;
  if (fullroute?router.get_full_route(rlist):
     router.get_route(cid,&sp,rlist)) {
    log_print("error calculating routes\n");
    /* return back the datanodes */
    return -1;
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
  size_t sz = 0;
  int cmd = 0;
  int cmd_state = st_na/*, ret=0*/ ;

  //while (!ret && !m_pendingQ.pop((void*&)st,cfd,cmd,req,sz,cmd_state)) {
  if (!m_pendingQ.pop((void*&)st,cfd,cmd,req,sz,cmd_state)) {

    /* reset xaid in related session */
    m_lss.reset_xaid(cfd);

    log_print("try redo job for client %d, cmd %d\n",cfd,cmd);
    if (cmd==com_stmt_prepare) {
      m_stmts.set_cmd_state(cfd,cmd_state);

      /*ret =*/ do_stmt_prepare(st,cfd,req,sz,cmd_state);
    }
    else if (cmd==com_query)  
      /*ret =*/ do_query(st,cfd,req,sz);
    else if (cmd==com_stmt_execute) 
      /*ret =*/ do_stmt_execute(st,cfd,req,sz);
  }
  return 0;
}

/* do request in query mode */
int 
myproxy_backend::do_query(sock_toolkit *st,int cid, char *req, size_t sz)
{
  int xaid = -1, ret=0 ;
  char *pSql = req + 5;
  size_t szSql = sz - 5;
  tSessionDetails *pss = 0;
  std::vector<uint8_t> rlist ;
  std::string newStmt ;
  tContainer err ;
  tContainer buff ;
  tSqlParseItem sp ;
  sql_parser parser ;
  hook_framework hooks ;

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

  stxNode* pTree = parse_tree(pSql,szSql);
  char *pDb = const_cast<char*>(pss->db.c_str());

  /* dynamic request hooking */
  if (hooks.run(&req,sz,pDb,h_sql,pTree)) {
    log_print("error filtering sql %s\n",pSql);
    ret = -1;
    goto __end_do_query ;
  }

  /* update the statement pointer */
  pSql = req+5 ;
  szSql= sz -5;

  /* dig informations from statement */
  if (parser.do_dig(pSql,szSql,&sp,pDb,err,pTree)) {
    log_print("error parse statement %s\n",pSql);
    /*  send the error message to client */
    do_send(cid,err.tc_data(),err.tc_length());
    ret = -1;
    goto __end_do_query ;
  }

  /* calcualte routing infomations */
  if (get_route(xaid,cid,sp,false,true,rlist)) {
    m_pendingQ.push(st,cid,com_query,req,sz,st_na);
    log_print("adding redo for client %d\n", cid);

    ret = -1;
    goto __end_do_query ;
  }

  /* save the query info */
  m_stmts.add_qry_info(cid,sp);
  /* set current 'sp' item under query mode */
  m_stmts.set_curr_sp(cid);

  log_print("try to execute sql %s\n",pSql);
  /* send and execute the request */
  if (m_xa.execute(st,xaid,com_query,rlist,req,sz,0,this)) {
    log_print("fatal: sending xa %d for client %d\n",xaid,cid);
    //return -1;
    ret = -1;
    goto __end_do_query ;
  }

__end_do_query:
  del_tree(pTree);

  return ret;
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
  std::vector<uint8_t> rlist ;
  tContainer err ;
  sql_parser parser ;
  tSqlParseItem sp ;
  hook_framework hooks ;
  auto inner_del_tree = [&](auto &bNew,auto &t) 
  {
    if (bNew){
      /* the tree is newly created and is no used any more */
      del_tree(t);
    }
  } ;

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
  stxNode *pTree = m_stmts.get_stree(cid,req,sz);
  bool bNewTree = !pTree;

  /* if no tree found, then parse the statement statically 
   *  and build a new tree */
  if (!pTree) {
    pTree = parse_tree(pSql,szSql);
    log_print("build new tree for %d\n",cid);
  }

  /* dynamically modify the tree by hook modules */
  if (hooks.run(&req,sz,pDb,h_sql,pTree)) {
    log_print("error filtering sql %s\n",pSql);

    inner_del_tree(bNewTree,pTree);

    return -1;
  }

  /* update the statement pointer */
  pSql = req+5 ;
  szSql= sz -5;

  /* dig informations from tree into 'sp' */
  if (cmd_state==st_prep_trans && 
     parser.do_dig(pSql,szSql,&sp,pDb,err,pTree)) {

    log_print("error parse statement %s\n",pSql);

    inner_del_tree(bNewTree,pTree);
    /* 
     * TODO: send the error message to client 
     */
    return -1;
  }

  if (get_route(xaid,cid,sp,true,false,rlist)) {
    log_print("fail getting route\n");

    inner_del_tree(bNewTree,pTree);

    return -1;
  }

  /* this prepare command should send response to client */
  if (cmd_state==st_prep_trans) {
    /* save prepare info */
    m_stmts.add_stmt(cid,(char*)pOldReq,(size_t)szOldReq,0,pTree,sp);
  } 
  else {
    /* the tree is newly created and is no used any more */
    inner_del_tree(bNewTree,pTree);
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

int myproxy_backend::save_sv_by_placeholders(tSqlParseItem *sp, 
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

int myproxy_backend::deal_parser_item(int cid, char *req, size_t sz, 
  tSqlParseItem* &sp)
{
  int lstmtid = 0;

  mysqls_get_stmt_prep_stmt_id(req,sz,&lstmtid);
  if (m_stmts.get_parser_item(cid,lstmtid,sp)) {
    return -1;
  }

  /* set 'sp' item belonging to the statement
   *  that is about to be executed */
  m_stmts.set_curr_sp(cid,lstmtid);
  return 0;
}

/* do the stmt_execute request */
int 
myproxy_backend::do_stmt_execute(sock_toolkit *st, int cid, char *req, size_t sz)
{
  int xaid = -1 ;
  int nphs = 0; /* total placeholders */
  int nblob = 0; /* total blob requests */
  char *breq = 0;
  size_t len = 0;
  tSessionDetails *pss = 0;
  std::vector<uint8_t> rlist ;
  tSqlParseItem *sp = 0 ;
  safeDnStmtMapList *maps = 0 ;

  /* parse total blob-type place-holders */
  m_stmts.get_total_phs(cid,nphs);
  mysql_calc_blob_count(req,nphs,&nblob);
  /* save total blob count */
  m_stmts.set_total_blob(cid,nblob);

  /* the blob request are enough */
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

  /* prepare for calculating routes... */
  deal_parser_item(cid,req,sz,sp);
  /* fill in sharding values if it's place holder */
  save_sv_by_placeholders(sp,nphs,req,sz);

  /* calcualte routing infomations */
  if (get_route(xaid,cid,*sp,false,true,rlist)) {
    m_pendingQ.push(st,cid,com_stmt_execute,req,sz,st_stmt_exec);
    log_print("adding redo for client %d\n", cid);
    return -1;
  }

  /* update command state */
  m_stmts.set_cmd_state(cid,st_stmt_exec);

  maps = get_stmt_id_map(cid,req,sz);
  /* 
   * do execute request: 1. send blob list 2. send exec request 
   */
  /* try to get the blob request */
  if (!m_stmts.get_blob(cid,breq,len)) {
    size_t sublen=0;

    log_print("try sending blob list\n");
    /* iterates each blob requests in list */
    for (sublen=0;len>0;len-=sublen,breq+=sublen) {

      sublen = mysqls_get_req_size(breq);

      /* send and execute the blob list */
      if (m_xa.execute(st,xaid,com_stmt_send_long_data,
          rlist,breq,sublen,(void*)maps,this)) {
        log_print("fatal: sending xa %d for client %d\n",xaid,cid);
        return -1;
      }
    } /* end for() */
  }

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
	pf(pCols,numCols,res,sz,chOdrCol,reinterpret_cast<uint8_t*>(colVal.tc_data()),&szVal,0);
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
myproxy_backend::deal_query_res(
  xa_item *xai,  /* transaction object */
  int myfd, /* backend db fd that responses the packet */
  char *res, 
  size_t sz
  )
{
  int cfd = xai->get_client_fd(), nCols = 0;
  int sn = mysqls_extract_sn(res);

  /* check validation of xaid */
  if (!m_xa.get_xa(xai->get_xid())) {
    log_print("invalid xaid %d %p from %d\n", 
      xai->get_xid(),xai,myfd);
    return -1;
  }

  /* 1st packet of response */
  if (sn==1 && xai->m_states.get(myfd)==0) {

    /* add new state of db fd */
    xai->m_states.add(myfd);

    /* get total columns */
    nCols = mysqls_extract_column_count(res,sz);

    /* reset stuffs that needed by the column defs */
    xai->set_col_count(nCols);
    if (xai->get_last_sn()==0) {
      xai->m_cols.reset(myfd);
      /* beginning serial number of column defs */
      xai->set_last_sn(2);
    }

    /* set number of last datanode */
    if (xai->inc_res_dn()<xai->get_desire_dn()) {
      return 0;
    }
    xai->set_last_dn(myfd);

    /* no columns found in res set or ERROR OCCORS */
    if (!nCols || mysqls_is_error(res,sz)) {
      m_xa.end_exec(xai);

      /* return back cache if it has */
      m_caches.return_cache(xai);

      /*return*/ m_trx.tx(cfd,res,sz);
      return 0;
    }
  }
  /* the column def are not yet recv */
  else if (xai->m_states.get(myfd)==rs_none) {

    /* end of column defs */
    if (mysqls_is_eof(res,sz)) {
      xai->m_states.next(myfd);
      /* save last sn */
      xai->set_last_sn(sn);
    } 
    else {
      /* caching the column definitions */
      if (m_caches.is_needed(xai)) 
        save_col_def_by_dn(xai,cfd,myfd,res,sz);
    }

    /* packet is not from last data node, discard it */
    if (myfd!=xai->get_last_dn()) {
      return 0;
    }
  }
  /* recv the row sets */
  else if (xai->m_states.get(myfd)==rs_col_def) {

    /* end of row sets */
    if (mysqls_is_eof(res,sz)) {
      xai->m_states.next(myfd);
      /* not the last eof recv, just go back */
      if (xai->inc_ok_count()<xai->get_desire_dn()) 
        return 0;
      log_print("recv last eof on %d, cid %d\n",myfd, cfd);
    }
    /* cache row set if it's not from last datanode */
    else if (m_caches.is_needed(xai)) {
      do_cache_row(xai,myfd,res,sz);
      return 0;
    }
  }

  /* last packet of last datanode recv */
  if (xai->m_states.get(myfd)==rs_ok) {

    /* there maybe rows in cache, free them */
    if (m_caches.is_needed(xai)) {
      m_caches.extract_cached_rows(m_stmts,xai);
      /* update sn of last eof frame */
      mysqls_update_sn(res,xai->inc_last_sn());
    }
    /* finishing command execution */
    m_xa.end_exec(xai);

    /* reset if it's under statement prepare mode */
    m_stmts.reset(cfd);
  }

  /* send the delayed responses */
  if (m_caches.get_free_size(xai)<=sz ||
     xai->m_states.get(myfd)==rs_ok) 
  {
    m_caches.tx_pending_res(xai,cfd); 
  }

  /* buffer data if not the eof */
  if (xai->m_states.get(myfd)!=rs_ok) {
    m_caches.do_cache_res(xai,res,sz);
  /* send the data */
  } else {
    //sn = mysqls_extract_sn(res);
    m_trx.tx(cfd,res,sz);
  }

  return 0;
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

int myproxy_backend::prep_done(xa_item *xai, int cfd)
{
  /* get db fds out of epoll */ 
  //m_xa.end_exec(xai); 

  /* release the xa */ 
  //end_xa(cfd); 
  end_xa(xai); 

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
  m_stmts.add_mapping(cfd,lstmtid,gid,pd->no,stmtid);

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

  /* check validation of xaid */
  if (!m_xa.get_xa(xai->get_xid())) {
    log_print("invalid xaid %d %p from %d\n", 
      xai->get_xid(),xai,myfd);
    return -1;
  }

  m_stmts.get_cmd_state(cfd,st) ;

  /* 
   * add statement id mapping according to 
   *  first response packet
   */
  if (sn==1&& xai->m_states.get(myfd)==0) {
    /* 
     * check return code for errors 
     */
    if (mysqls_is_error(res,sz)) {
      /* send error messages to client */
      return m_trx.tx(cfd,res,sz);
    }

    /*
     * add new state of db fd 
     */
    xai->m_states.add(myfd);

    /* 
     * get total columns, total placeholders, and physical statement id
     */
    mysqls_extract_prepared_info(res,sz,&stmtid,&nCols,&nPhs);
    /* get currently operated logical statement id */
    if (m_stmts.get_lstmtid(cfd,lstmtid)) {
      return -1;
    }

    /* 
     * add lstmtid -> stmtid mapping here 
     */
    do_add_mapping(myfd,cfd,stmtid,lstmtid,xai);

    /*
     * save column & placeholder count
     */
    xai->set_col_count(nCols);
    xai->set_phs_count(nPhs);
    /* save total placeholder */
    m_stmts.set_total_phs(cfd,nPhs);

    /* replace the out-sending statement id with 
     *  client-based statement id */
    mysqls_update_stmt_prep_stmt_id(res,sz,lstmtid);
  }

  /* cache response packets, only 1 cpu thread can 
   *  run them */
  if (sn==xai->get_sn_count() && !xai->lock_sn_count()) {

    /*
     * for state st_prep_trans, the prepared results should be sent to client
     * for st_prep_execute, it should not
     */
    /* this prepare response should be sent to client */
    if (st==st_prep_trans) {
      m_caches.do_cache_res(xai,res,sz);
    }

    /* switch to next serial number */
    xai->set_sn_count(sn+1);
    xai->unlock_sn_count();
  }

  /* 
   * last response packet reached 
   */
  //log_print("sn %d, col %d\n",sn,xai->get_col_count());
  if (sn==get_last_sn(xai) && xai->inc_ok_count()==xai->get_desire_dn()) {

    log_print("reach last eof of %d\n", cfd);

    /* XXX: should reset session related xaid before sending
     *  last prepare response to client under multi-thread envirogments */
    m_lss.reset_xaid(cfd);

    /* this prepare response should be sent to client */
    if (st==st_prep_trans) {
      m_caches.tx_pending_res(xai,cfd); 
    }

    sock_toolkit *sock = xai->get_sock();
    int xaid = xai->get_xid();

    /* should release xa as soon as possible */
    prep_done(xai,cfd);

    try_pending_exec(cfd,xaid,sock);

    ret = 1;
  }

  return ret;
}

int myproxy_backend::deal_stmt_execute_res(xa_item *xai, int cid, char *res, size_t sz)
{
  return deal_query_res(xai,cid,res,sz);
}

int myproxy_backend::xa_rx(xa_item *xai, int myfd, char *res, size_t sz)
{
  int cmd = xai->get_cmd();
  //int cid = xai->get_client_fd();

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
  constexpr size_t MAX_BLK = 20000;
  //myproxy_backend *pexec = (myproxy_backend*)m_parent;
  int ret = 0;
  char *req =0;
  ssize_t szBlk = 0, szReq = 0;
  char blk[MAX_BLK];
  bool bStop = false ;
  xa_item *xa = static_cast<xa_item*>(priv->param);

  /* recv a single block of data */
  do {
    ret = m_trx.rx_blk(fd,priv,blk,szBlk,MAX_BLK) ;

    if (ret==MP_ERR) {
      return -1;
    }

    const char *pBlkEnd = blk + szBlk ;

    /* get requests out of blk one by one */
    for (req=blk;!bStop && req<pBlkEnd;req+=szReq) {

      if ((pBlkEnd-req)<4) {
        //log_print("incompleted header %zu on %d\n",(pBlkEnd-req),fd);
        break ;
      }

      /* current req size pointed to by req */
      szReq = mysqls_get_req_size(req);

      if ((pBlkEnd-req)<szReq) {
        //log_print("incompleted body %zu on %d\n",(pBlkEnd-req),fd);
        break ;
      }

      if (!mysqls_is_packet_valid(req,szReq)) {
        log_print("datanode %d packet err\n",fd);
        return -1;
      }

      /* process the incoming packet */
      if (xa_rx(xa,fd,req,szReq)) {
        /* TODO: no need to recv any data */
        bStop = true ;
        //break ;
      }
    } /* end for */

    /* check if need to cache data */
    if (pBlkEnd>req) {
      ssize_t szRest = pBlkEnd - req ;

      priv->cache.buf = (char*)malloc(szRest);
      priv->cache.offs = 0;
      priv->cache.pending = szRest ;
      memcpy(priv->cache.buf,req,szRest);
      priv->cache.valid = true ;
    }

  } while (!bStop && ret==MP_OK);

  return 0;
}

myproxy_epoll_trx&
myproxy_backend::get_trx(void)
{
  return m_trx ;
}

