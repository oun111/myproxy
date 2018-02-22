
#include "myproxy_trx.h"
#include "sock_toolkit.h"
#include "busi_base.h"


/* 
 * receive mysql packet 
 */
int 
myproxy_epoll_trx::rx(sock_toolkit *st, epoll_priv_data* priv, int fd)
{
  int ret = 0;
  char *req =0;
  ssize_t szBlk = 0;
  size_t szReq = 0;
  char blk[nMaxRecvBlk], *pblk = 0;
  bool bStop = false ;
  business_base *pb = static_cast<business_base*>(priv->parent);
  tContainer tmp ;

  /* recv a single block of data */
  do {
    pblk= blk;
    ret = rx_blk(fd,priv,pblk,szBlk,nMaxRecvBlk) ;

    if (ret==MP_ERR) {
      return -1;
    }

    /* if there're pending bytes in cache, means the 
     *  packet in cache is incompleted, so go ahead 
     *  to receive the rest */
    if (is_epp_data_pending(priv)) {
      return 0;
    }

    const char *pBlkEnd = pblk + szBlk ;
    bool relCache = true ;

    /* get requests out of pblk one by one */
    for (req=pblk;!bStop && req<pBlkEnd;req+=szReq) {

      const size_t szRest = pBlkEnd-req ;

      if (szRest<4) {
        log_print("incomplete header %zu on %d\n",szRest,fd);

        /* cache the partial header */
        create_epp_cache(priv,req,szRest,4);

        return 0 ;
      }

      /* current req size pointed to by req */
      szReq = mysqls_get_req_size(req);

      if (szRest<szReq) {

        log_print("incomplete body %zu on %d needs %zu,ret %d\n",
          szRest,fd,szReq,ret);

        /* there're contents in cache already, so 
         *  back it up first */
        if (is_epp_cache_valid(priv)) {
          tmp.tc_resize(szRest+10);
          tmp.tc_write(req,szRest);
          req = tmp.tc_data() ;
        }

        /* resize the cache and wait to receive the rest */
        create_epp_cache(priv,req,szRest,szReq);
        relCache = false ;

        /* continue to read from net and dont release cache */
        if (ret==MP_OK) {
          break ;
        }

        return 0 ;
      }

      if (!mysqls_is_packet_valid(req,szReq)) {
        log_print("myfd %d packet err\n",fd);
        return -1;
      }

      /* process the incoming packet */
      if (pb->deal_pkt(fd,req,szReq,priv->param)) {
        /* TODO: no need to recv any data */
        bStop = true ;
      }

    } /* end for */

    /* all received datas are completely processed, 
     *  try to release cache if there is */
    if (relCache) {
      free_epp_cache(priv);
    }

  } while (!bStop && ret==MP_OK);

  return 0;
}

/* receive a big block of packet data */
int 
myproxy_epoll_trx::rx_blk(int fd, epoll_priv_data *priv, char* &blk, 
  ssize_t &sz, const size_t capacity)
{
  ssize_t total = 0, offs = 0, ret = 0;

  if (!is_epp_cache_valid(priv)) {
    total= capacity;
    offs = 0;
  } else {
    /* receive the cached partial data first */
    get_epp_cache_data(priv,&blk,&offs,&total);
  }

  /* read data block */
  ret = read(fd,blk+offs,total);
  sz  = offs ;

  /* error occors */
  if (ret<=0) {
    /* error, don't do recv on this socket */
    if (errno!=EAGAIN) 
      log_print("abnormal state on fd %d: %s, cache: %d\n",
        fd, strerror(errno), priv->cache.valid);
    if (!ret) 
      return MP_ERR;
    return MP_IDLE;
  }

  /* update cache if it has */
  update_epp_cache(priv,ret);

  sz += ret  ;
  /* more data should be read */
  if (ret>0 && ret<total && 
     (errno==EAGAIN || errno==EWOULDBLOCK)) {
    return MP_FURTHER;
  }

  return MP_OK ;
}

/* send mysql packet */
int myproxy_epoll_trx::tx(int fd, char *buf, size_t sz)
{
  return do_send(fd,buf,sz);
}

