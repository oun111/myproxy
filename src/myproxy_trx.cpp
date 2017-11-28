
#include "myproxy_trx.h"
#include "sock_toolkit.h"


enum rx_step
{
  rx_hdr,
  rx_body
} ;

/* 
 * receive mysql packet 
 */
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

/* receive a big block of packet data */
int 
myproxy_epoll_trx::rx_blk(int fd, epoll_priv_data *priv, char* blk, 
  ssize_t &sz, const size_t capacity)
{
  ssize_t total = 0, offs = 0, ret = 0;

  if (!priv->cache.valid) {
    total= capacity;
    offs = 0;

  } else {

    if (priv->cache.pending>capacity) {
      log_print("too much pending bytes: %zu\n",
        priv->cache.pending);
      return MP_ERR;
    }
    /* move cache data to front of 'blk' */
    memcpy(blk,priv->cache.buf+priv->cache.offs,
      priv->cache.pending);
    offs = priv->cache.pending ;
    total= capacity - offs ;
    //log_print("get buff of myfd %d\n",fd);
    /* close cache */
    priv->cache.valid = false ;
    free(priv->cache.buf);
    priv->cache.buf = 0;
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

  sz += ret  ;
  /* more data should be read */
  if (ret>0 && ret<total && 
     (errno==EAGAIN || errno==EWOULDBLOCK)) {
    return MP_FURTHER;
  }

  return MP_OK ;
}

/* do release */
void myproxy_epoll_trx::end_rx(char* &buff)
{
  if (buff) {
    free(buff);
  }
}

/* send mysql packet */
int myproxy_epoll_trx::tx(int fd, char *buf, size_t sz)
{
  return do_send(fd,buf,sz);
}

