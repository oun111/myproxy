
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
  (*ep)->valid = 1 ;
  (*ep)->fd = fd ;
  (*ep)->parent = parent ;
  (*ep)->param  = param ;
  (*ep)->cache.buf   = NULL ;
  (*ep)->cache.valid = false;
  (*ep)->tid = pthread_self();
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

  if (!ep || !*ep) {
    printf("%s: no ep found for %d\n",__func__,fd);
    return -1;
  }

  (*ep)->valid = 0;
  (*ep)->parent= 0;
  (*ep)->param = 0;
  (*ep)->cache.valid = false;
  return del_from_epoll(st->m_efd,fd);
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

int close1(sock_toolkit *st, int fd, int nEvent)
{
  if (!is_ep_available(st)) {
    close(fd);
    return 0;
  }
  if (nEvent<0&& nEvent>=MAXEVENTS) {
    printf("event index %d error\n",nEvent);
    return -1;
  }
  if (do_del_from_ep(st,fd)) {
    printf("error del %d from ep %d: %s\n",fd,st->m_efd,strerror(errno));
  }
  close(fd);
  return 0;
}

int do_send(int fd, char *buf, size_t len)
{
  int ret = 0;
  size_t offs = 0;
  int32_t total=0;
  
  for (total=len;total>0;total-=ret,
     offs+=ret) {
    //ret = write(fd,buf+offs,total);
    ret = send(fd,buf+offs,total,0);
    if (errno==EINTR) 
      continue ;
    /* no data can be sent */
    //if (ret<=0 || errno==EAGAIN) {
    if (ret<0) {
    //if (ret<=0 && errno==EAGAIN) {
      //printf("error send packet:%s\n",strerror(errno));
      return -1;
    }
  }
  return 0;
}

int do_recv1(sock_toolkit *st, int fd, char *buf, int len,
  struct sockaddr *sa, socklen_t *slen)
{
  int ret = 0;
  int ln=len, total = 0;
  int flag = st->bBlock?0:MSG_DONTWAIT;

  do {
    if ((ret=recvfrom(fd,buf+total,ln,flag,sa,slen))>0) {
      ln -=ret ;
      total += ret ;
    }
  } while (ln>0 && ret>0);
  return total>0?total:ret;
}

int do_recv(sock_toolkit *st, int fd, char *buf, int len)
{
  int ret = 0;
  int ln=len, total = 0;
  int flag = st->bBlock?0:MSG_DONTWAIT;

  do {
    if ((ret=recv(fd,buf+total,ln,flag))>0) {
      ln -=ret ;
      total += ret ;
    }
  } while(ln>0 && ret>0);
  return total>0?total:ret;
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
get_event(sock_toolkit *st, int idx, int *fd, int *event, void **po)
{
  if (idx<0 || idx>=st->nEvents || idx>=MAXEVENTS)
    return -1;
  *po    = st->elist[idx].data.ptr;
  *fd    = st->elist[idx].data.fd ;
  *event = st->elist[idx].events;
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
  //log_dbg("end\n");
  return 0;
}

