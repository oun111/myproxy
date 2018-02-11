
#include "myproxy_trx.h"
#include "sock_toolkit.h"
#include "busi_base.h"


enum rx_step
{
  rx_hdr,
  rx_body
} ;

/* 
 * receive mysql packet 
 */
#if 0
int 
myproxy_epoll_trx::rx(int fd, epoll_priv_data *priv, char* &buf, size_t &sz)
{
  int total =0, ret = 0, offs=0, step = -1;
  char tmphdr[4];

  if (!priv) {
    log_print("fatal: %d no priv\n",fd);
  }
  /* first step, recv header */
  if (!priv->cache.valid) {
    /* TODO: how about using the stack memory, not the heap ? */
    if (!buf) buf=(char*)malloc(4) ;
    total= 4;
    offs = 0;
    step = rx_hdr ;

  } else {
    /* continue to get the pending request data */
    buf  = priv->cache.buf ;
    offs = priv->cache.offs;
    total= priv->cache.pending ;
    step = priv->cache.step ;
    //pi->valid = false;
    log_print("restart rx on %d, offs %d, total %d, priv %p\n",fd,offs,total,priv);
  }
  while (total>0) {
    ret = read(fd,buf+offs,total);
    /* error occors */
    if (ret<=0) {
      if (errno!=EAGAIN) {
        /* error, don't do recv on this socket */
        log_print("abnormal state on fd %d: %s, cache: %d, ret: %d\n",
          fd, strerror(errno), priv->valid, ret);
        /* release cache */
        if (priv->cache.valid) 
          free(priv->cache.buf);
        priv->cache.valid = false;
      }
      if (!ret) 
        return MP_ERR;
      return MP_IDLE;
    }
    /* more data should be read */
    if (ret>0 && ret<total && 
       (errno==EAGAIN || errno==EWOULDBLOCK)) {
      /* save pending receiving, and stop recv now */
      priv->cache.valid   = true ;
      priv->cache.step    = step ;
      priv->cache.buf     = buf ;
      priv->cache.offs    = offs+ret ;
      priv->cache.pending = total-ret;
      log_print("rx interrupted fd %d, ret %d, total %d, stp %d, priv %p\n",
        fd, ret, total, step, priv);
      return MP_FURTHER;
    }
    /* XXX: test */
    if (ret>0 && ret<total) {
      log_print("fd %d pkt insufficient, %d, err: %d\n",fd,ret,errno);
    }
    /* the header is received */
    if (step==rx_hdr) {
      memcpy(tmphdr,buf,4);
      /* get body size */
      total = mysqls_get_body_size(buf);
      buf   = (char*)realloc(buf,total+4+10);
      step  = rx_body ;
      offs  = 4 ;
      /* copy the header */
      memcpy(buf,tmphdr,4);

    } else {
      /* end received the packet */
      if (priv->cache.valid) {
        sz = total+offs ;
        priv->cache.valid = false;
      } else {
        sz = total + 4;
      }
      break;
    }
  } /* end while */
  return MP_OK ;
}
#else
int 
myproxy_epoll_trx::rx(sock_toolkit *st, epoll_priv_data* priv, int fd)
{
  //constexpr size_t MAX_BLK = 20000;
  int ret = 0;
  char *req =0;
  ssize_t szBlk = 0;
  size_t szReq = 0;
  char blk[/*MAX_BLK*/nMaxRecvBlk], *pblk = 0;
  bool bStop = false ;
  business_base *pb = static_cast<business_base*>(priv->parent);

  /* recv a single block of data */
  do {
    pblk= blk;
    ret = rx_blk(fd,priv,pblk,szBlk,/*MAX_BLK*/nMaxRecvBlk) ;

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

        char tmp[10] ;

        /* the header locates in cache */
        relCache = !(szRest==4&&is_epp_cache_valid(priv));

        log_print("incomplete body %zu on %d needs %zu\n",
          szRest,fd,szReq);

        /* there's a header in cache, back it up because the 
         *  create_epp_cache() call will destory the cache */
        if (!relCache) {
          memcpy(tmp,req,4);
          req = tmp ;
        }

        /* cache the partial header + body and wait to 
         *  receive the rest */
        create_epp_cache(priv,req,szRest,szReq);

        /* only the header's received and be stored in cache, so 
         *  continue to read from net and dont release cache */
        if (!relCache) {
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
#endif

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
    //log_print("fd %d rx %zu pending %zu\n",fd,offs,total);
  }

  /* read data block */
  ret = read(fd,blk+offs,total);
  sz  = offs ;

  /* error occors */
  if (ret<=0) {
    /* error, don't do recv on this socket */
    if (errno!=EAGAIN) 
      log_print("abnormal state on fd %d: %s, cache: %d\n",
        fd, strerror(errno), priv->valid);
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

