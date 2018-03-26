
#include "normal_accepter.h"


normal_accepter::normal_accepter(void *client_priv) :
  m_priv(client_priv)
{
  //m_ep.parent = this ;
}

normal_accepter::~normal_accepter(void)
{
}

#if 0
epoll_priv_data** normal_accepter::get_priv(void)
{
  return &m_ep ;
}
#endif

int 
normal_accepter::rx(sock_toolkit* st,epoll_priv_data* priv,int fd) 
{
  return accept_tcp_conn(st,priv->fd,true,m_priv);
}

int 
normal_accepter::tx(sock_toolkit* st,epoll_priv_data* priv,int fd) 
{
  return 0;
}


