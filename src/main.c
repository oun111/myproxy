#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h> 
#include "framework.h"
#include "dbug.h"
#include "myproxy_busi.h"
#include "env.h"

DECLARE_LOG("myproxy") ;

using namespace GLOBAL_ENV ;

int parse_cmdline(int argc, char **argv, char* &strConf)
{
  char ch = 0;
  const char *shortopts = "c:v";  
  const struct option longopts[] = {  
    {"conf", required_argument, NULL, 'c'},  
    {"version", no_argument, NULL, 'v'},  
    {0, 0, 0, 0},  
  };  

  while ((ch = getopt_long(argc,argv,shortopts,longopts,NULL))!=-1) {
    switch(ch) {
      case 'c':
        strConf = optarg ;
        break ;

      case 'v':
        printf("version: %s\n",__VER_STR__);
        exit(0);
        break ;

      default:
        printf("unknown option '%c'\n",ch);
        return -1 ;
    }
  }
  return 0;
}

int main(int argc,char **argv)
{
  char *strConf = const_cast<char*>("configure_dummy.json");

  if (parse_cmdline(argc,argv,strConf)) {
    printf("error parsing cmd line\n");
    return -1;
  }

  if (m_conf.read_conf(strConf)) {
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

