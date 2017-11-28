

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include "framework.h"
#include "busi_base.h"
#include "normal_accepter.h"
#include "epi_list.h"

epi_list g_epItems ;

/*
 * class epoll_impl
 */
epoll_impl::epoll_impl(
  void *busi,    /* client business processing */
  char *addr, int busi_port,
  size_t szThdPool) :
  /* thread pool size */
  num_thread(szThdPool),
  m_busi(busi),
  m_accepter(NULL),
  m_run(false)
{
  /* signore SIGPIPE */
  signal(SIGPIPE,SIG_IGN); 

  /* 
   * normal business network address 
   */
  m_busiAddr.ipv4 = addr?/*inet_addr*/__net_atoi(addr):0;
  m_busiAddr.port = busi_port>0&&busi_port<65535?busi_port:
    DEFAULT_BUSI_PORT ;

  /* debug information */
  init_dbug();

  /* calc a suitable thread pool size */
  calc_thd_pool_sz();

  /*
   * init the global socktoolkit items
   */
  st_global_init();

  /* 
   * init the accepter 
   */
  m_accepter = new normal_accepter(m_busi);

  /* 
   * init business network 
   */
  m_busi_fd = new_tcp_svr(m_busiAddr.ipv4,m_busiAddr.port,false);
}

epoll_impl::~epoll_impl(void)
{
  stop();

  if (m_accepter)
    delete (normal_accepter*)m_accepter ;

  /* close all handles */
  if (m_busi_fd>0) close(m_busi_fd);

  /* free the thread pool */
  if (m_threads)  delete []m_threads;

  /* destory global sock toolkit items */
  st_global_destroy();
}

void epoll_impl::calc_thd_pool_sz(void)
{
  /* 1 maintainance task + N handler tasks */
  num_thread = (num_thread<48)?(1+get_cpu_cores()*2):num_thread;
  //num_thread = 32;
  log_print("size of thread pool: %ld\n", (long)num_thread);
}

void epoll_impl::start(void)
{
  uint8_t i=0;

  /* 
   * initialize thread pool 
   */
  m_run = true ;
  /* run worker threads */
  m_threads = new thread_helper<void(epoll_impl::*)
    (int),epoll_impl>[num_thread];
  for (;i<num_thread;i++) {
    m_threads[i].bind(
      //0==i?&epoll_impl::mt_task:
      &epoll_impl::event_task,
      this,0);
    m_threads[i].start();
  }
}

void epoll_impl::stop(void)
{
  /* release the threads */
  m_run = false ;
}

/*
 * thread workers
 */
void epoll_impl::event_task(int)
{
  int i=0,fd=0,event=0;
  business_base *pb = 0;
  epoll_priv_data *pd = 0;
  int cfd = 0;
  sock_toolkit stk ;

  log_print("thread %lu entered\n",pthread_self());

  /* init the socket item */
  init_st(&stk,true,false);

  log_print("thread %lx st %d\n",pthread_self(),stk.m_efd);

  /* add listen fd to local epoll */
  int dup=0;
  do_add2epoll(&stk,m_busi_fd,m_accepter,0,&dup);
  if (dup)
    log_print("fd %d for st %d already in used\n",m_busi_fd,stk.m_efd);

  /* add to item list  */
  g_epItems.new_ep(&stk);

  /* process events */
  for (;m_run;) {
    /* poll event list */
    list_foreach_events(i,stk,&fd,&event,&pd) {

      /* get epoll private data */
      if (!pd || (pd->valid!=1) || ((cfd=pd->fd)<=0)) {
        continue ;
      } 
      pb = reinterpret_cast<business_base*>(pd->parent) ;
      if (!pb) {
        continue ;
      }
      if ((event & EPOLLERR) || 
         (event & EPOLLHUP) || 
         (event & EPOLLRDHUP)) {
        log_print("close err fd %d: %s\n",cfd,strerror(errno));
        pb->on_error(&stk,cfd);
        close1(&stk,cfd,i);
      } else { 
        /* request in */
        if (event & EPOLLIN) {
          if (pb->rx(&stk,pd,cfd)<0) {
            log_print("close fd %d\n",cfd);
            close1(&stk,cfd,i);
          }
        }
        /* ready to send */
        if (event & EPOLLOUT) {
          disable_send(&stk,cfd);
          pb->tx(&stk,pd,cfd);
        }
      }
    } /* end list_foreach... */
  } /* end for(;;) */

  /* clean up sockets */
  close_st(&stk);
}

void epoll_impl::init_dbug(void)
{
  __sync_lock_test_and_set(&m_dbg.n_reply2client,0);
  __sync_lock_test_and_set(&m_dbg.n_clientReqs,0);
  __sync_lock_test_and_set(&m_dbg.n_loginReqs,0);
}

void epoll_impl::output_dbug_info(void)
{
  static int cnt = 0;

  /* output debug info every 5 seconds */
  if (cnt++>30) {
    cnt=0;
    log_print("total logins from client: %d\n", m_dbg.n_loginReqs);
    log_print("total reqs from client: %d\n", m_dbg.n_clientReqs);
    log_print("total reply to client: %d\n", m_dbg.n_reply2client);
  }
}

void epoll_impl::mt_task(int arg)
{
  log_print("thread %lu entered\n",
    pthread_self());
  for (;;) {
    /* output debug informations */
    output_dbug_info();
    sleep(1);
  }
}

