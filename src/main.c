#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "framework.h"
#include "dbug.h"
#include "myproxy_busi.h"
#include "env.h"

DECLARE_LOG("myproxy") ;

using namespace GLOBAL_ENV ;


int main(int argc,char **argv)
{
  if (m_conf.read_conf("configure_dummy.json")) {
    return -1;
  }

  size_t szThPool = m_conf.get_thread_pool_size();
  /* init the busi module */
  myproxy_frontend m_busi((char*)"myproxy") ;
  /* the epoll module */
  epoll_impl d1(&m_busi,0,0,szThPool) ;

  /* start the proxy */
  d1.start();
  while (1) { sleep(1); }
  return 0;
}

