
#include <stdint.h>
#include <zlib.h>
/*
 * copy from zlib source
 */
int do_uncompress(uint8_t *inb, int sz_in,
  uint8_t *outb, int *sz_out)
{
  z_stream stream;
  int err;

  stream.next_in = (uint8_t*)inb;
  stream.avail_in = (uInt)sz_in;
  stream.next_out = outb;
  stream.avail_out = (uInt)*sz_out;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  err = inflateInit(&stream);
  if (err != Z_OK) return err;

  err = inflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    inflateEnd(&stream);
    if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
        return Z_DATA_ERROR;
    return err;
  }
  *sz_out = stream.total_out;

  err = inflateEnd(&stream);
  return err;
}

int do_compress(uint8_t *inb, int sz_in, uint8_t *outb,
  int *sz_out)
{
  z_stream stream;
  int err;

  stream.next_in = inb;
  stream.avail_in = sz_in;
  stream.next_out = outb;
  stream.avail_out = *sz_out ;
  if (stream.avail_out != *sz_out)
    return Z_BUF_ERROR;

  stream.zalloc = 0;
  stream.zfree = 0;
  stream.opaque = (voidpf)0;

  err = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
  if (err != Z_OK) return -1;

  err = deflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    deflateEnd(&stream);
    return -1;
  }
  *sz_out = stream.total_out;
  err = deflateEnd(&stream);
  return err!=Z_OK;
}
