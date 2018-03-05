
#include "sock_toolkit.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <pthread.h>
#include <netinet/tcp.h>

const int MAX_EP_EVENTS = /*4096*/10240 ;
epoll_priv_data** g_epollPrivs = 0 ;


int st_global_init(void)
{
  g_epollPrivs = malloc(MAX_EP_EVENTS*sizeof(epoll_priv_data*));
  if (!g_epollPrivs) {
    return -1;
  }
  bzero(g_epollPrivs,MAX_EP_EVENTS*sizeof(epoll_priv_data*));
  return 0;
}

void st_global_destroy(void)
{
  int i=0;

  if (!g_epollPrivs) {
    return ;
  }

  for (;i<MAX_EP_EVENTS;i++) {
    if (g_epollPrivs[i]) 
      free(g_epollPrivs[i]);
  }

  free(g_epollPrivs);
}

int init_st(sock_toolkit *st, bool bUseEp, bool block)
{
  st->m_efd  = 0;
  st->bBlock = block ;
  /* use epool or not */
  if (bUseEp && (st->m_efd=epoll_create(1))<0) {
    /* XXX: error message here */
    printf("error init epoll\n");
    return -1;
  }
  return 0;
}

int close_st(sock_toolkit *st)
{
  if (is_ep_available(st)) {
    close(st->m_efd);
  }
  return 0;
}

/* check epoll's usage */
bool is_ep_available(sock_toolkit *st)
{
  return st->m_efd>0 ;
}

int new_udp_svr(sock_toolkit *st, int port)
{
  struct sockaddr_in sa ;
  int fd = 0;

  if ((fd=socket(AF_INET,SOCK_DGRAM,0))<0) {
    /* XXX: error message here */
    return -1;
  }
  if (!st->bBlock)
    set_nonblock(fd);
  sa.sin_family     = AF_INET ;
  sa.sin_addr.s_addr= htonl(INADDR_ANY);
  sa.sin_port       = htons(port);
  if (bind(fd,(struct sockaddr*)&sa,sizeof(sa))<0) {
    /* XXX: error message here */
    printf("error bind 0x%x:%d(%s)\n", 
      sa.sin_addr.s_addr, port, strerror(errno));
    return -1;
  }
  if (is_ep_available(st) && add_to_epoll(st->m_efd,fd,0)) {
    /* XXX: error message here */
    printf("error add dispatcher to epoll(%s)\n",
      strerror(errno));
    return -1;
  }
  return fd;
}

int new_udp_client(sock_toolkit *st)
{
  int fd = 0;

  if ((fd=socket(AF_INET,SOCK_DGRAM,0))<0) {
    /* XXX: error message here */
    return -1;
  }
  return fd;
}

int 
new_tcp_svr(uint32_t address, int port, bool bBlock)
{
  uint32_t addr ;
  struct sockaddr_in sa ;
  int flag =1;
  int fd = 0;

  addr = !address?htonl(INADDR_ANY):htonl(address);
  if (addr<0) {
    /* XXX: error message here */
    return -1;
  }
  /* initialize the server socket */
  if ((fd = socket(AF_INET,SOCK_STREAM,0))<0) {
    /* XXX: error message here */
    return -1;
  }
  /* set non-blocking */
  if (!bBlock)
    set_nonblock(fd);
  /* reuse the server address */
  setsockopt(fd,SOL_SOCKET,
    SO_REUSEADDR,&flag,sizeof(int));
  /* bind server address */
  sa.sin_family      = AF_INET ;
  sa.sin_addr.s_addr = addr;
  sa.sin_port        = htons(port);
  if (bind(fd,(struct sockaddr*)&sa,sizeof(struct sockaddr))<0) {
    /* XXX: error message here */
    printf("error bind 0x%x:%d(%s)\n", 
      addr, port, strerror(errno));
    return -1;
  }
  /* do listening */
  if (listen(fd,SOMAXCONN)<0) {
    /* XXX: error message here */
    printf("error listen 0x%x:%d\n", addr, port);
    return -1;
  }
  return fd;
}

#if 1
int add_tcp_svr_2_epoll(sock_toolkit *st, int fd)
{
  if (is_ep_available(st) && add_to_epoll(st->m_efd,fd,0)) {
    /* XXX: error message here */
    printf("error add dispatcher to epoll(%s)\n",
      strerror(errno));
    return -1;
  }
  return 0;
}
#endif

int new_tcp_client(sock_toolkit *st, uint32_t addr, int port)
{
  int fd=0;
  struct sockaddr_in sa;

  if ((fd=socket(AF_INET,SOCK_STREAM,0))<0) {
    /* XXX: error message here */
    return -1;
  }
  sa.sin_family      = AF_INET;
  sa.sin_addr.s_addr = htonl(addr);
  sa.sin_port        = htons(port);
  if (connect(fd,(struct sockaddr*)&sa,
     sizeof(struct sockaddr))<0) {
    printf("cant connect to host: %s\n",
      strerror(errno));
    close(fd);
    return -1;
  }
  /* make socket async */
  if (!st->bBlock)
    set_nonblock(fd);
  /* add this socket to epoll */
  add_to_epoll(st->m_efd,fd,0);
  return fd;
}

/*static*/ epoll_priv_data** get_epp(int fd)
{
  if (fd<0 || fd>=MAX_EP_EVENTS) {
    printf("%s: much too connections(>%d)\n",
      __func__,MAX_EP_EVENTS);
    return NULL;
  }
  return &g_epollPrivs[fd];
}

#if 0
int do_modepoll(sock_toolkit *st, int fd, void *priv)
{
  epoll_priv_data **ep = get_epp(fd);
  
  if (!ep || !(*ep)) {
    printf("%s: no ep found for %d\n",__func__,fd);
    return -1;
  }
  (*ep)->valid = 1 ;
  (*ep)->fd = fd ;
  (*ep)->parent = priv ;

  return 0;
}
#endif

int 
do_add2epoll(sock_toolkit *st, int fd, void *parent, void *param, int *dup)
{
  epoll_priv_data **ep = get_epp(fd);
  
  if (!ep) {
    printf("%s: no ep found for %d\n",__func__,fd);
    return -1;
  }

  //printf("%s: added ep fd %d\n",__func__,fd);
  if (!*ep) {
    *ep = (epoll_priv_data*)malloc(sizeof(epoll_priv_data));

  } else if ((*ep)->valid) {
    *dup = 1;
    printf("%s(%lu): warning: ep priv %d 's already in used\n",
      __func__,pthread_self(),fd);
    printf("duplicated: orig: fd %d, parent %p, tid %lu\n",
      (*ep)->fd,(*ep)->parent,(*ep)->tid);
  }

  /* make socket async */
  if (!st->bBlock)
    set_nonblock(fd);
  (*ep)->valid = 0 ;
  (*ep)->fd = fd ;
  (*ep)->parent = parent ;
  (*ep)->param  = param ;
  (*ep)->cache.buf   = NULL ;
  (*ep)->cache.valid = false;
  (*ep)->tid = pthread_self();
  (*ep)->valid = 1 ;
  (*ep)->st = st ;
  (*ep)->tx_cache.valid = 0 ;
  (*ep)->tx_cache.buf = 0 ;
  /* add this socket to epoll */
  if (is_ep_available(st) && add_to_epoll(st->m_efd,fd,*ep)) {
    /* XXX: error message here */
    printf("error add client to epoll(%s)\n",
      strerror(errno));
    return -1 ;
  }
  return 0;
}

int 
do_del_from_ep(sock_toolkit *st, int fd)
{
  epoll_priv_data **ep = get_epp(fd) ;
  int ret = 0;

  if (!ep || !*ep) {
    printf("%s: no ep found for %d\n",__func__,fd);
    return -1;
  }

  ret = del_from_epoll(st->m_efd,fd);

  (*ep)->valid = 0;
  (*ep)->parent= 0;
  (*ep)->param = 0;
  (*ep)->cache.valid = false;

  return ret ;
}

int 
accept_tcp_conn(sock_toolkit *st,int sfd,
  bool initiative, /* initiative or not to send a packet after 
                       accepting a new client */
  void *priv  /* private data related to epoll events */
  )
{
  struct sockaddr in_addr;  
  socklen_t in_len = sizeof(in_addr);  
  int fd = 0;
  int dup = 0;
  
  while (1) {
    /* no more incoming connections */
    if ((fd = accept(sfd,&in_addr,&in_len))<0 /*&& errno==EAGAIN*/) {
      return /*-1*/1 ;
    }
    /* set tcp no delay */
    {
      int flag = 1;
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
      flag = 0;
      setsockopt(fd, IPPROTO_TCP, TCP_CORK, &flag, sizeof(int));
    }

    //printf("fd: %d %d\n",fd,htons(((struct sockaddr_in*)&in_addr)->sin_port));

    dup = 0;
    if (do_add2epoll(st,fd,priv,0,&dup)) {
      close(fd);
      continue ;
    }
    /* triger a send event */
    if (initiative)
      enable_send(st,fd);
  }
  return 0;
}

int close1(sock_toolkit *st, int fd)
{
  if (!is_ep_available(st)) {
    close(fd);
    return 0;
  }
  if (do_del_from_ep(st,fd)) {
    printf("error del %d from ep %d: %s\n",fd,st->m_efd,strerror(errno));
  }
  close(fd);
  return 0;
}

int do_send(int fd, char *buf, size_t len)
{
  epoll_priv_data **ep = get_epp(fd);

  /* if there're pending data in cache, due to the sequences 
   *  of the data, also cache the new ones */
  if (is_epp_tx_cache_valid(*ep)) {
    append_epp_tx_cache(*ep,buf,len);
    return 0;
  }

  size_t ret = send(fd,buf,len,0);

  if (ret<=0) {
    printf("%s: send no data on %d: %s\n",__func__,fd,strerror(errno));
    //return -1;
  }

  size_t rest = (ret<=0)?len:(len-ret) ;

  /* if there're rest data not be send, cache them */
  if (rest>0) {
    append_epp_tx_cache(*ep,buf+ret,rest);
  }

  return 0;
}

int do_recv(int fd, char **blk, ssize_t *sz, size_t capacity)
{
  ssize_t total = 0, offs = 0, ret = 0;
  epoll_priv_data **ep = get_epp(fd);

  if (!is_epp_cache_valid(*ep)) {
    total= capacity;
    offs = 0;
  } else {
    /* receive the cached partial data first */
    get_epp_cache_data(*ep,blk,&offs,&total);
  }

  /* read data block */
  ret = read(fd,(*blk)+offs,total);
  *sz  = offs ;

  /* error occors */
  if (ret<=0) {
    /* error, don't do recv on this socket */
    if (errno!=EAGAIN && errno!=EWOULDBLOCK) {
      /*log_print("abnormal state on fd %d: %s, cache: %d\n",
        fd, strerror(errno), (*ep)->cache.valid);*/
    }
	/* XXX: test */
    if (total==0 && ret==0) {
      printf("invalid sz on fd %d\n",fd);
    }
    if (!ret) 
      return /*MP_ERR*/-1;
    return /*MP_IDLE*/2;
  }

  /* update cache if it has */
  update_epp_cache(*ep,ret);

  *sz += ret  ;
  /* more data should be read */
  if (ret>0 && ret<total && 
     (errno==EAGAIN || errno==EWOULDBLOCK)) {
    return /*MP_FURTHER*/1;
  }
  return /*MP_OK*/0 ;
}

int disable_send(sock_toolkit *st, int fd)
{
  epoll_priv_data **ep = get_epp(fd);

  if (!ep || !*ep) {
    return -1;
  }
  if (is_ep_available(st)) {
    if (del_epo(st->m_efd,fd,*ep))
      printf("error disable tx on fd %d: %s\n",fd,
        strerror(errno));
  }
  return 0;
}

int enable_send(sock_toolkit *st, int fd)
{
  epoll_priv_data **ep = get_epp(fd);

  if (!ep || !*ep) {
    return -1;
  }
  if (is_ep_available(st)) {
    if (add_epo(st->m_efd,fd,*ep))
      printf("error enable tx on fd %d: %s\n",fd,
        strerror(errno));
  }
  return 0;
}

int get_svr_events(sock_toolkit *st)
{
  st->nEvents = epoll_wait(st->m_efd,st->elist,MAXEVENTS,-1);
  return st->nEvents<0?-1:0;
}

int 
get_event(sock_toolkit *st, int idx, int *event, void **po)
{
  if (idx<0 || idx>=st->nEvents || idx>=MAXEVENTS)
    return -1;
  *po    = st->elist[idx].data.ptr;
  *event = st->elist[idx].events;
  //*fd    = st->elist[idx].data.fd ;
  return 0;
}

int num_events(sock_toolkit *st)
{
  return st->nEvents;
}

int brocast_tx(int fd, int bcast_port, char *data, size_t len)
{
  int i = 0;
  struct ifreq *ifr;
  struct ifconf ifc;
  char buf[1024];
  struct sockaddr_in sa ;

  /* iterate all local interfaces */
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(fd,SIOCGIFCONF,(char*)&ifc)<0) {
    /* XXX: error message here */
    return -1;
  }
  /* send advertisement to each interface */
  sa.sin_family = AF_INET;
  sa.sin_port   = htons(bcast_port);
  for (i=ifc.ifc_len/sizeof(struct ifreq),
    ifr = ifc.ifc_req; --i>=0; ifr++) 
  {
    if (ioctl(fd,SIOCGIFBRDADDR,ifr)<0)
      continue ;
    /* set broadcast address */
    sa.sin_addr.s_addr = ((struct sockaddr_in*)&ifr->
      ifr_broadaddr)->sin_addr.s_addr ;
    /* enable broadcast */
    setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&sa,
      sizeof(sa));
    /* send the packet */
    sendto(fd,(char*)data,len,MSG_DONTWAIT,
      (struct sockaddr*)&sa,sizeof(sa));
  }

  return 0;
}

/*
 * rx cache
 */
int 
create_epp_cache(epoll_priv_data *ep, char *data, size_t sz, 
  const size_t capacity)
{
  ep->cache.valid= true;
  ep->cache.offs = 0;
  ep->cache.buf  = realloc(ep->cache.buf,capacity+10);
  ep->cache.pending = capacity;

  if (data && sz>0) {
    memcpy(ep->cache.buf,data,sz);
    ep->cache.offs += sz ;
    ep->cache.pending -= sz;
  }

  return 0;
}

int update_epp_cache(epoll_priv_data *ep, size_t sz_in)
{
  if (!ep->cache.valid)
    return -1;

  ep->cache.offs += sz_in ;
  ep->cache.pending -= sz_in ;

  return 0;
}

bool is_epp_cache_valid(epoll_priv_data *ep)
{
  return ep->cache.valid;
}

bool is_epp_data_pending(epoll_priv_data *ep)
{
  if (!ep->cache.valid)
    return false;

  return ep->cache.pending>0 ;
}

int 
get_epp_cache_data(epoll_priv_data *ep, char **data, ssize_t *sz, ssize_t *szPending)
{
  if (!ep->cache.valid)
    return -1;

  *data = ep->cache.buf ;
  *sz   = ep->cache.offs ;

  if (szPending) *szPending = ep->cache.pending;

  return 0;
}

int free_epp_cache(epoll_priv_data *ep)
{
  if (!ep->cache.valid)
    return -1;

  ep->cache.valid = false ;
  free(ep->cache.buf);
  ep->cache.buf = NULL ;
  ep->cache.offs = ep->cache.pending = 0;

  return 0;
}

/* 
 * tx cache 
 */

bool is_epp_tx_cache_valid(epoll_priv_data *ep)
{
  return ep->tx_cache.valid;
}

int append_epp_tx_cache(epoll_priv_data *ep, char *data, size_t sz)
{
  size_t ln = 0;
  char *tmp = 0;

  if (is_epp_tx_cache_valid(ep)) 
    ln  = ep->tx_cache.len ;

  if (!is_epp_tx_cache_valid(ep) || (ep->tx_cache.capacity-ep->tx_cache.len)<=sz) {

    /* not enough space in tx cache */
    if (is_epp_tx_cache_valid(ep)) {
      tmp = malloc(ln);
      memcpy(tmp,ep->tx_cache.buf,ln);
    }

    create_epp_tx_cache(ep,tmp,ln,ln+sz);

    if (tmp)
      free(tmp);
  }

  memcpy(ep->tx_cache.buf+ln,data,sz);
  ep->tx_cache.len += sz ;

  return 0;
}

int 
create_epp_tx_cache(epoll_priv_data *ep, char *data, size_t sz, 
  const size_t capacity)
{
  const size_t szRedundant = 10 ;

  ep->tx_cache.valid= true;
  ep->tx_cache.ro   = 0;
  ep->tx_cache.len  = 0;
  ep->tx_cache.capacity  = capacity+szRedundant;
  ep->tx_cache.buf  = realloc(ep->tx_cache.buf,capacity+szRedundant);

  if (data && sz>0) {
    memcpy(ep->tx_cache.buf,data,sz);
    ep->tx_cache.len = sz ;
  }

  return 0;
}

int free_epp_tx_cache(epoll_priv_data *ep)
{
  if (!ep->tx_cache.valid)
    return -1;

  ep->tx_cache.valid = false ;
  free(ep->tx_cache.buf);
  ep->tx_cache.buf = NULL ;
  ep->tx_cache.ro = ep->tx_cache.len = 0;

  return 0;
}

int flush_epp_tx_cache(sock_toolkit *st, epoll_priv_data *priv, int fd)
{
  if (!is_epp_tx_cache_valid(priv)) {
    return -1;
  }

  char *buf  = priv->tx_cache.buf + priv->tx_cache.ro ;
  size_t sz  = priv->tx_cache.len-priv->tx_cache.ro ;
  size_t ret = send(fd,buf,sz,0);

  if (ret<=0) {
    printf("%s: send no data on %d: %s\n",__func__,fd,strerror(errno));
    //return -1;
  }

  size_t rest = ret<=0?sz:(sz-ret) ;

  //printf("%s: flush %zu rest %zu fd %d\n",__func__,ret,rest,fd);
  if (!rest) {
    free_epp_tx_cache(priv);
  } else {
    /* not all the data are send, enable for 
     *  the last tx event */
    priv->tx_cache.ro += ret ;
    enable_send(st,fd);
  }

  return 0;
}

int triger_epp_cache_flush(int cfd)
{
  epoll_priv_data **ep = get_epp(cfd);

  if (is_epp_tx_cache_valid(*ep)) {
    enable_send((sock_toolkit*)(*ep)->st,cfd);
  }

  return 0;
}

