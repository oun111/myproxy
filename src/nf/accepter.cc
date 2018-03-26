
#include "accepter.h"


accepter::accepter(void *client_priv) :
  m_priv(client_priv)
{
  //m_ep.parent = this ;
}

accepter::~accepter(void)
{
}

#if 0
epoll_priv_data** accepter::get_priv(void)
{
  return &m_ep ;
}
#endif

int 
accepter::rx(sock_toolkit* st,epoll_priv_data* priv,int fd) 
{
  return accept_tcp_conn(st,priv->fd,true,m_priv);
}

int 
accepter::tx(sock_toolkit* st,epoll_priv_data* priv,int fd) 
{
  return 0;
}


