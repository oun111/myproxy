#ifndef __SOCK_TOOLKIT_H__
#define __SOCK_TOOLKIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
//#include "dbg_log.h"



/* defines host address:port pair */
typedef struct tHostAddr {
  uint32_t ipv4 ;
  uint32_t port ;
} addr_t;

#define __net_atoi(s) ({\
  int i0,i1,i2,i3;\
  sscanf(s,"%d.%d.%d.%d", &i0,&i1,&i2,&i3);\
  (i0<<24)|(i1<<16)|(i2<<8)|i3 ;\
})
  /* add handle to epoll */
#define add_to_epoll(ef,tf,po) ({     \
  struct epoll_event event;           \
  if (po) event.data.ptr = (void*)((uintptr_t)po) ; \
  else event.data.fd = tf ;      \
  event.events  = EPOLLIN|EPOLLET  ;  \
  epoll_ctl((ef),EPOLL_CTL_ADD,(tf),&event);\
})
  /* remove handle from epoll */
#define del_from_epoll(ef,tf) ({     \
  /*struct epoll_event event;*/          \
  epoll_ctl((ef),EPOLL_CTL_DEL,(tf),NULL) ;\
})
  /* modify epoll event */
#define mod_epoll(ef,tf,ev,po) ({       \
  struct epoll_event event;          \
  if (po)  event.data.ptr = (void*)((uintptr_t)po); \
  else event.data.fd = tf ;            \
  event.events  = /*EPOLLIN|EPOLLET|*/(ev) ;     \
  epoll_ctl((ef),EPOLL_CTL_MOD,(tf),&event) ;\
})
  /* add 'EPOLLOUT' event */
#define add_epo(ef,tf,po)  mod_epoll(ef,tf,EPOLLOUT|EPOLLET,po)
  /* del 'EPOLLOUT' event */
#define del_epo(ef,tf,po)  mod_epoll(ef,tf,/*0*/EPOLLIN|EPOLLET,po)
  /* make handle non-blocking */
#define set_nonblock(fd)  \
  fcntl((fd),F_SETFL, \
    fcntl((fd),F_GETFL,0)| O_NONBLOCK);
/* iterates each epoll events */
#define list_foreach_events(i,so,fd,ev,po)      \
  for (i=0,get_svr_events(&so);i<num_events(&so) &&  \
    !get_event(&so,i,fd,ev,(void**)po);i++) 

typedef struct tEPollPrivData
{
  int fd ;
  void *parent ;
  void *param ;
  int valid ;

  /* the fd-based rx cache */
  struct {
    char *buf;
    size_t offs ;
    size_t pending;
    uint8_t step ;
    bool valid;
  } cache ;

  pthread_t tid ;
} epoll_priv_data ;

typedef struct tSockToolkit 
{
  /* the epoll handle */
  int m_efd ;
  /* the i/o mode flag */
  bool bBlock ;
  /* the epoll event list */
#define MAXEVENTS  128 /* 1024*/ 
  struct epoll_event elist[MAXEVENTS];
  int nEvents;
} sock_toolkit  ;

extern int st_global_init(void);
extern void st_global_destroy(void);
extern int num_events(sock_toolkit*);
extern int new_udp_svr(sock_toolkit*,int);
extern int new_udp_client(sock_toolkit*);
extern int new_tcp_svr(uint32_t,int,bool);
extern int add_tcp_svr_2_epoll(sock_toolkit*,int);
extern int new_tcp_client(sock_toolkit*,uint32_t,int);
extern int do_add2epoll(sock_toolkit*,int,void*,void*,int*);
extern int do_del_from_ep(sock_toolkit*,int);
extern int do_modepoll(sock_toolkit*,int,void*);
/*static*/ extern epoll_priv_data** get_epp(int fd);
extern int accept_tcp_conn(sock_toolkit*,int,bool,void*);
extern bool is_ep_available(sock_toolkit*);
extern int do_send(int, char*, size_t);
extern int do_recv(sock_toolkit*,int,char*,int);
extern int do_recv1(sock_toolkit*,int,char*,int,
  struct sockaddr*, socklen_t*);
extern int get_svr_events(sock_toolkit*);
extern int get_event(sock_toolkit*,int,int*,int*,void**);
extern int disable_send(sock_toolkit*,int);
extern int enable_send(sock_toolkit*,int);
extern int close1(sock_toolkit*,int,int);
extern int init_st(sock_toolkit*,bool,bool);
extern int close_st(sock_toolkit*);
extern int brocast_tx(int,int,char*,size_t);

#ifdef __cplusplus
}
#endif

#endif /* __SOCK_TOOLKIT_H__ */

