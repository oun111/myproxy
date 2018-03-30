
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

/* FIXME: make the cache size configurable */
const size_t MAX_TX_CACHE = 10240*5;


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
    if (g_epollPrivs[i]) {
      free_epp_cache(g_epollPrivs[i]);
      release_tx_cache(g_epollPrivs[i]);

      free(g_epollPrivs[i]);
    }
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
  init_tx_cache(*ep);
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

  {
    epoll_priv_data **ep = get_epp(fd);

    free_epp_cache(*ep);
    release_tx_cache(*ep);
  }

  close(fd);
  return 0;
}

#if 0
int do_send(int fd, char *buf, size_t len)
{
  epoll_priv_data **ep = get_epp(fd);

  /* if there're pending data in cache, due to the sequences 
   *  of the data, also cache the new ones */
  if (is_epp_tx_cache_valid(*ep)) {
    append_epp_tx_cache(*ep,buf,len);
    //return 0;
    goto __triger_tx ;
  }

  size_t ret = send(fd,buf,len,0);

  if (ret<=0) {
    printf("%s: send no data on %d: %s\n",__func__,fd,strerror(errno));
    //return -1;
  }

  size_t rest = (ret<=0)?len:(len-ret) ;

  /* if there're rest data not be send, cache them */
  if (rest>0) {
    //append_epp_tx_cache(*ep,buf+ret,rest);
    create_epp_tx_cache(*ep,buf+ret,rest,rest);
  }

__triger_tx:
  /* triger the pending tx data to be send if there're */
  triger_epp_cache_flush(fd);

  return 0;
}
#else
ssize_t do_send(int fd, char *buf, size_t len)
{
  return send(fd,buf,len,0);
}
#endif

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
  int ret = 0;
  epoll_priv_data **ep = get_epp(fd);

  if (!ep || !*ep) {
    return -1;
  }

  /* XXX: test */
  st = (*ep)->tx_cache.tx_st ;

  if (st && is_ep_available(st)) {
    if ((ret=del_epo(st->m_efd,fd,*ep)))
      printf("error disable tx on fd %d: %s\n",fd,
        strerror(errno));
  }
  return ret;
}

int enable_send(sock_toolkit *st, int fd)
{
  epoll_priv_data **ep = get_epp(fd);
  int ret = 0;

  if (!ep || !*ep) {
    return -1;
  }
  
  /* XXX: test */
  if ((*ep)->tx_cache.tx_st)
    st = (*ep)->tx_cache.tx_st;

  if (st && is_ep_available(st)) {
    ret = add_epo(st->m_efd,fd,*ep) ;
    if (ret) {
      /*printf("error enable tx on fd %d: %s\n",fd,
        strerror(errno));*/
    } else {
      (*ep)->tx_cache.tx_st = st;
    }
  }
  return ret;
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

#define move_state_until(ep,news,tills) do {\
  int ret = 0; \
  while ((ret=__sync_val_compare_and_swap(&(ep)->tx_cache.flag, \
      tx_free,(news)))!=tx_free) { \
    if (ret==(tills)) \
      return 0; \
  } \
} while(0)

#define reset_state(ep) do {\
  __sync_lock_test_and_set(&(ep)->tx_cache.flag,tx_free);\
} while(0)

void init_tx_cache(epoll_priv_data *ep)
{
  bzero(&ep->tx_cache,sizeof(ep->tx_cache));

  ep->tx_cache.buf = malloc(MAX_TX_CACHE);

  /* set to 'free' state */
  reset_state(ep);
}

int release_tx_cache(epoll_priv_data *ep)
{
  /* wait for 'free' state and move to 'del' state */
  move_state_until(ep,tx_del,-1);

  if (ep->tx_cache.buf) 
    free(ep->tx_cache.buf);

  bzero(&ep->tx_cache,sizeof(ep->tx_cache));

  return 0;
}

size_t get_tx_cache_free_size(int fd)
{
#if 0
  epoll_priv_data **epp = get_epp(fd), *ep = 0;
  size_t wo=0,ro=0 ;
  ssize_t ln=0;

  if (!epp) {
    return 0;
  }

  ep = *epp ;
  wo = __sync_fetch_and_add(&ep->tx_cache.wo,0);
  ro = __sync_fetch_and_add(&ep->tx_cache.ro,0);

  ln = ro-wo ;

  return (ln>0?ln:(MAX_TX_CACHE+ln))-1;
#else
  size_t sz = 0;

  get_tx_cache_data(fd,NULL,&sz);

  return MAX_TX_CACHE-sz ;
#endif
}

int get_tx_cache_data(int fd, char *buf, size_t *sz)
{
  ssize_t ln = 0;
  epoll_priv_data **epp = get_epp(fd), *ep = 0;
  size_t wo=0,ro=0;

  if (!epp) {
    return -1;
  }

  /* need 'sz' to receive data size */
  if (!sz) 
    return -1;

  ep = *epp ;
  wo = __sync_fetch_and_add(&ep->tx_cache.wo,0);
  ro = __sync_fetch_and_add(&ep->tx_cache.ro,0);

  ln = wo-ro ;
  *sz= ln>=0?ln:(MAX_TX_CACHE+ln);

  /* get data size only */
  if (!buf) 
    return 0;

  /* no data avaiable */
  if (*sz==0)
    return 0;

  /* wait for 'free' and move to 'read/write' state */
  move_state_until(ep,tx_rw,tx_init);

  /* get data */
  if (ln>0) {
    memcpy(buf,ep->tx_cache.buf+ro,ln);
  }
  else {
    ln = MAX_TX_CACHE-ro ;
    memcpy(buf,ep->tx_cache.buf+ro,ln);
    memcpy(buf+ln,ep->tx_cache.buf,*sz-ln);
  }

  /* back to 'free' state */
  reset_state(ep);

  return 0;
}

int append_tx_cache(int fd, char *data, size_t sz)
{
  epoll_priv_data **epp = get_epp(fd), *ep = 0;
  size_t wo = 0, ro=0, ln=0, sz1 = 0;

  if (!epp) {
    return -1;
  }

  ep = *epp ;  
  wo = __sync_fetch_and_add(&ep->tx_cache.wo,0);
  ro = __sync_fetch_and_add(&ep->tx_cache.ro,0);

  /* wait for 'free' and move to 'read/write' state */
  move_state_until(ep,tx_rw,tx_init);

  sz1 = MAX_TX_CACHE-wo ;
  if (wo<ro || sz<=sz1) {
    memcpy(ep->tx_cache.buf+wo,data,sz);
  }
  else {
    memcpy(ep->tx_cache.buf+wo,data,sz1);
    ln = sz-sz1;
    memcpy(ep->tx_cache.buf,data+sz1,ln);
  }

  /* back to 'free' state */
  reset_state(ep);

  __sync_lock_test_and_set(&ep->tx_cache.wo,(wo+sz)%MAX_TX_CACHE);

  return 0;
}

int update_tx_cache_ro(int fd, ssize_t sz)
{
  epoll_priv_data **epp = get_epp(fd), *ep = 0;
  size_t ro = 0;

  if (!epp) {
    return -1;
  }

  ep = *epp ;  
  ro = __sync_fetch_and_add(&ep->tx_cache.ro,0);
  __sync_lock_test_and_set(&ep->tx_cache.ro,(ro+sz)%MAX_TX_CACHE);

  return 0;
}

