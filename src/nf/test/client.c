
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "dummy_proto.h"
#include "sock_toolkit.h"

#if 0
typedef struct tHostAddr {
  uint32_t ipv4 ;
  uint32_t port ;
} addr_t;
#endif
struct tGlobalPara
{
  addr_t local;
  addr_t remote;
  addr_t other ;
  int mode ; /* mode: 0 test client, 1 out-standing server */
} g_param;

int extract_addr(char *in, addr_t *out)
{
  char *p = 0;

  p = strchr(in,':');
  if (!p)
    return -1;
  /* get ip address */
  *p = '\0';
  out->ipv4 = !strchr(in,'.')?0:
    __net_atoi(in);
  /* get port */
  *p = ':';
  out->port = atoi(p+1);
  return 0;
}

int parse_args(int argc, char**argv)
{
  int opt = 0;  
  int option_index = 0;
  char *optstring = (char*)"o:l:r:m:?";  
  static struct option long_options[] = {  
    {"mode", required_argument, NULL, 'm'},  
    {"local", required_argument, NULL, 'l'},  
    {"remote-host", required_argument, NULL, 'r'},  
    {"outstanding-server", required_argument, 
      NULL, 'o'},  
#if 0
    {"optarg", optional_argument, NULL, 'o'},  
    {"noarg",  no_argument,       NULL, 'n'},  
#endif
    {0, 0, 0, 0}  
  };  

  while((opt=getopt_long(argc,argv,optstring,
     long_options,&option_index))!=-1) {  
    switch (opt)
    {
      case 'm':
        g_param.mode = atoi(optarg);
        break ;
      case 'l':
        extract_addr(optarg,&g_param.local);
        break ;
      case 'r':
        extract_addr(optarg,&g_param.remote);
        break ;
      case 'o':
        extract_addr(optarg,&g_param.other);
        break ;
      case '?':
        printf("help message\n");
        exit(0);
      default:
        /* TODO: help messages here */
        break ;
    }
  } 
  return 0;
}

int init_defaults(void)
{
  memset(&g_param,0,sizeof(g_param));
  g_param.remote.ipv4 = 0x7f000001 ;
  g_param.remote.port = 23300 ;
  return 0;
}

int test_business(void)
{
  int fd = 0;
  struct timeval tv;
  tDummyBusiReq *req ;
  tDummyBusiResp resp ;
  char buf[10000];
  sock_toolkit st;
  
  init_st(&st,false,true) ;
  /* connect to remote server */
  fd = new_tcp_client(&st,g_param.remote.ipv4,
    g_param.remote.port);
  if (fd<=0) {
    printf("fail connect to host\n");
    return -1;
  }
  req = (tDummyBusiReq*)buf ;
  memset(req,0,sizeof(*req));
  /* generate test request */
  gettimeofday(&tv,0);
  req->timestamp = tv.tv_sec*1000000 + 
    tv.tv_usec ;
  sprintf(req->msg,"test request");
  req->len = 13 ;
  req->direction = 0;
  req->bConfirm = false ;
  /* fillin out-standing server field,
   *  if needed */
  if (g_param.other.port>0) {
    req->ossAddr = g_param.other.ipv4;
    req->ossPort = g_param.other.port;
  }
  printf("send request to server, size %zu\n",sizeof(*req));
  /* XXX: test send body */
  {
    char *ptr = buf+sizeof(tDummyBusiReq);
    int i=0;
    const int sz_body = 8000;

    req->sz_body = sz_body ;
    for (;i<sz_body;i++) {
      ptr[i] = i ;
    }
    srand(time(0));
    req->px = rand()%sz_body;
    req->py = rand()%sz_body ;
    printf("x: %x, y: %x, hdr: %zu, body: %d\n",ptr[req->px],
      ptr[req->py], sizeof(tDummyBusiReq),sz_body);
    send(fd,(char*)buf,sizeof(tDummyBusiReq)+req->sz_body,0);
  }
  /* get feedback */
  recv(fd,(char*)&resp,sizeof(resp),0);
  printf("recv a response: \n");  
  /* dump feedback */
  printf("timestamp: %llu\n", (long long unsigned)resp.timestamp);
  printf("msg: %s, len: %ld\n", resp.msg,(long)resp.len);
  printf("direction: %d\n", resp.direction);
  printf("body len: %d\n", resp.sz_body);
  return 0;
}

int test_business1(void)
{
  sock_toolkit st;
  int sfd = 0, fd=0, ret=0;
  tOssPkt tp ;
  struct sockaddr in_addr;  
  socklen_t in_len = sizeof(in_addr);  

  init_st(&st,false,true);
  sfd = new_tcp_svr(0, g_param.local.port,false);
  if (sfd<=0) {
    printf("fail init server mode\n");
    return -1;
  }
  add_tcp_svr_2_epoll(&st,sfd);
  while (1) {
    fd = accept(sfd,&in_addr,&in_len);
    if (fd<=0)
      continue ;
    do {
      if ((ret=recv(fd,(char*)&tp,sizeof(tp),0))<=0) {
        continue ;
      }
      printf("recv an oss packet: \n");
      printf("timestamp: %llu\n", (unsigned long long)tp.timestamp);
      printf("counter: %u\n", tp.counter);
      /* send back */
      tp.type = 2;
      ret = send(fd,(char*)&tp,sizeof(tp),0);
    } while(ret>0);
  } /* end while */
  return 0;
}

int test_business2(void)
{
  int fd = 0;
  char req[32] ;
  char resp[32] ;
  sock_toolkit st; 
  
  init_st(&st,false,true) ;
  /* connect to remote server */
  fd = new_tcp_client(&st,g_param.remote.ipv4,
    g_param.remote.port);
  if (fd<=0) {
    printf("fail connect to host\n");
    return -1;
  }
  memset(&req,0,sizeof(req));
  req[0] = 0x15 ;
  req[1] = 20;
  printf("send request2 to server\n");
  send(fd,(char*)&req,sizeof(req),0);
  /* get feedback */
  recv(fd,(char*)&resp,sizeof(resp),0);
  printf("recv a response: \n");  
  /* dump feedback */
  printf("resp: %d\n", *(int*)resp);
  return 0;
}

int test_business3(void)
{
  int fd = 0;
  char req[32] ;
  char resp[32] ;
  sock_toolkit st;
  
  init_st(&st,false,true) ;
  /* connect to remote server */
  fd = new_tcp_client(&st,g_param.remote.ipv4,
    g_param.remote.port);
  if (fd<=0) {
    printf("fail connect to host\n");
    return -1;
  }
  req[0] = 0x17 ;
  printf("send request3 to server\n");
  send(fd,(char*)&req,1,0);
  /* get feedback */
  recv(fd,(char*)&resp,sizeof(resp),0);
  printf("recv a response: \n");  
  /* dump feedback */
  printf("resp: %s\n", resp+1);
  return 0;
}

int test_business4(void)
{
  int fd = 0, i=0, ret=0, cnt=0, ln=0, offs=0;
  struct timeval tv;
  tDummyBusiReq *req ;
  tDummyBusiResp *resp ;
  char buf[10000];
  sock_toolkit st;

  init_st(&st,false,true) ;
  /* connect to remote server */
  fd = new_tcp_client(&st,g_param.remote.ipv4,
    g_param.remote.port);
  if (fd<=0) {
    printf("fail connect to host\n");
    return -1;
  }
  req = (tDummyBusiReq*)buf ;
  memset(req,0,sizeof(*req));
  /* generate test request */
  gettimeofday(&tv,0);
  req->timestamp = tv.tv_sec*1000000 + 
    tv.tv_usec ;
  sprintf(req->msg,"test multiple replies");
  req->len = 13 ;
  req->direction = 0;
  req->bConfirm = false ;
  /* test for myltiple replies */
  req->type = 1 ;
  /* number of frames to reply */
  req->px = req->py = cnt =4;
  req->sz_body = 0;
  printf("send request to server, size %zu\n",sizeof(*req));
  send(fd,(char*)buf,sizeof(tDummyBusiReq),0);
  /* get feedback */
  printf("recv response: \n");  
  /* dump feedback */
  for (i=0;i<cnt;i++) {
    resp = (tDummyBusiResp*)buf ;
    bzero(resp,sizeof(*resp));
    for (offs=0,ln=sizeof(*resp);ln>0;) {
      ret = recv(fd,(char*)buf+offs,ln,0);
      ln -= ret;
      offs+=ret;
      //usleep(1);
    }
    printf("timestamp: %llu\n", (long long unsigned)resp->timestamp);
    printf("msg: %s, len: %ld\n", resp->msg,(long)resp->len);
    printf("direction: %d\n", resp->direction);
    printf("hdr: %d, body len: %d\n", ret,resp->sz_body);
    for (offs=0,ln=resp->sz_body;ln>0;) {
      ret = recv(fd,(char*)buf+offs,ln,0);
      ln -= ret ;
      offs+=ret;
      //usleep(1);
    }
  }
  return 0;
}

int main(int argc, char**argv)
{
  /* init default arguments */
  init_defaults();
  /* parse arguments from command line */
  parse_args(argc,argv);
  /* do test */
  switch (g_param.mode) {
    case 0:
    {
      /* simulates the business process */
      test_business();
    }
    break ;

    case 1:
    {
      /* the out-standing server mode */
      test_business1();
    }
    break ;

    case 2:
    {
      /* split packet server mode */
      test_business2();
    }
    break ;

    case 3:
    {
      /* fast reply mode */
      test_business3();
    }
    break ;

    case 4:
    {
      /* multiple replies */
      test_business4();
    }
    break ;
  } 
  return 0;
}

