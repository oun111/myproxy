
#include "xamgr.h"
#include "simple_types.h"
#include "container_impl.h"
#include "epi_list.h"
#include "env.h"
#include "hooks/stmt_id.h"

using namespace GLOBAL_ENV;

extern epi_list g_epItems;


/*
 * class xamgr
 */
xamgr::xamgr(void)
{
}

xamgr::~xamgr(void) 
{
  m_xaList.clear();
}

/* acquire xa channel, begins a new transaction */
int 
xamgr::begin(
  int cfd,  /* client fd */
  void *parent,
  int &xid, /* the unique xa id */
  int mode /* 0 all, 1 single data node, no transaction */
  )
{
  int gid = 0;
  safeDataNodeList *nodes = 0;
  
  if (m_dnMgr.get_free_group(nodes,gid)) {
    log_print("no free groups found\n");
    return -1;
  }
  /* create a new transaction */
  if ((xid=m_xaList.add(cfd,gid,parent))<0) {
    log_print("cant create new transaction "
      "by group %d\n",gid);
    return -1;
  }
  return 0;
}

/* 
 * send requests to xa channel 
 */
int 
xamgr::execute(sock_toolkit *st, 
  int xid, /* xa id */
  int cmd, /* the request command id */
  std::vector<uint8_t> &dn_set, /* datanode set to execute request */
  char *req, size_t szReq, /* the request */
  void *maps,
  void *parent
  )
{
  xa_item *xai = m_xaList.get(xid);
  tDNInfo *pd = 0;
  int myfd = 0;
  uint16_t i=0;

  if (!xai) {
    log_print("no transaction %d found\n",xid);
    return -1;
  }
  /* reset xa item */
  xai->reset(st,cmd,dn_set.size());
  /* reset datanode states */
  xai->m_states.clear();
  /* get datanodes */
  auto nodes = m_dnMgr.get_specific_group(xai->get_gid()) ;
  if (!nodes) {
    log_print("cant get datanodes by group id %d\n",
      xai->get_gid());
    return -1;
  }

  xai->m_stVec.clear();

  /* send and execute sql by required datanode set */
  for (i=0;i<dn_set.size();i++) {
    pd = nodes->get(dn_set[i]);
    if (!pd) 
      continue ;
    myfd = pd->mysql->sock ;

    /* add to local thread epoll */
    {
      sock_toolkit *p = /*st*/0;

      /* save relations of st & myfd */
      if (g_epItems.get_idle(p)) {
        log_print("fail to get idle ep items\n");
      }
      xai->m_stVec.add(xa_item::STM_PAIR(p,myfd));

      /* add to epoll */
      int dup = 0;
      do_add2epoll(p,myfd,(void*)parent,(void*)xai,&dup) ;
      pd->add_ep = 1;

      log_print("execute on myfd %d -> %d, dn %d%s\n",myfd, 
        p->m_efd, dn_set[i], dup?" is dup!":"");

      //do_modepoll(st,myfd,(void*)xai);
    }

    /* do necessary update the out-sending packet */
    if (maps) {
      stmtIdHookParam sParam  ;
      hook_framework hooks ;

      sParam.grp = xai->get_gid() ;
      sParam.dn  = pd->no ;
      sParam.maps= (safeDnStmtMapList*)maps ;

      hooks.run(&req,szReq,&sParam,h_req);
    }
    /* send the request */
    do_send(myfd,req,szReq);
  }
  return 0;
}

int remove_from_ep(STM_PAIR &pair)
{
  auto st = pair.first ;
  auto myfd = pair.second ;

  /* remove from local thread epoll */
  if (do_del_from_ep(st,myfd)) 
    log_print("error remove myfd %d -> %d: %s\n",
      myfd,st->m_efd,strerror(errno));
  else log_print("ok removed %d -> %d\n",myfd,st->m_efd);

  /* free the st */
  g_epItems.return_back(st);

  return 0;
}

int xamgr::end_exec(xa_item *xai)
{
  /* remove mysql fd in group from epoll */
  xai->m_stVec.do_iterate(remove_from_ep); 
  return 0;
}

/* release the xa channel */
int 
xamgr::release(int xid)
{
  xa_item *xai = get_xa(xid);
  int gid = 0;

  if (!xai) {
    log_print("error: xa %d not found\n",xid);
    return -1;
  }
  /* release the transaction realated
   *  data node group */
  gid = xai->get_gid();
  m_dnMgr.return_group(gid);
  /* release the transaction */
  m_xaList.drop(xid);
  return 0;
}

xa_item* xamgr::get_xa(int xid)
{
  return m_xaList.get(xid) ;
}

dnmgr& xamgr::get_dnmgr(void)
{
  return m_dnMgr ;
}

