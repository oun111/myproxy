
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#ifndef _WIN32
#include <stdbool.h>
#endif
#include <string.h>
#include "simple_types.h"


void double8store(double v, char out[8])
{
  out[0] = ((char*)&v)[7] ;
  out[1] = ((char*)&v)[6] ;
  out[2] = ((char*)&v)[5] ;
  out[3] = ((char*)&v)[4] ;
  out[4] = ((char*)&v)[3] ;
  out[5] = ((char*)&v)[2] ;
  out[6] = ((char*)&v)[1] ;
  out[7] = ((char*)&v)[0] ;
}

/* big endian version */
void double8store_be(double v, char out[8])
{
  memcpy(out,&v,sizeof(v));
}

void float4store(float v, char out[4])
{
  out[0] = ((char*)&v)[3] ;
  out[1] = ((char*)&v)[2] ;
  out[2] = ((char*)&v)[1] ;
  out[3] = ((char*)&v)[0] ;
}

void float4store_be(float v, char out[4])
{
  memcpy(out,&v,sizeof(v));
}

void ul2store(uint16_t v, char out[2])
{
  out[0] = (char)(v&0xff);
  out[1] = (char)((v>>8)&0xff);
}

/* big-endian storage of ul2store */
void ul2store_be(uint16_t v, char out[2])
{
  out[1] = (char)(v&0xff);
  out[0] = (char)((v>>8)&0xff);
}

void ul3store(uint32_t v, char out[3])
{
  out[0] = (char)(v&0xff);
  out[1] = (char)((v>>8)&0xff);
  out[2] = (char)((v>>16)&0xff);
}

void ul4store(uint32_t v, char out[4])
{
  out[0] = (char)(v&0xff);
  out[1] = (char)((v>>8)&0xff);
  out[2] = (char)((v>>16)&0xff);
  out[3] = (char)((v>>24)&0xff);
}

/* big-endian storage of ul4store */
void ul4store_be(uint32_t v, char out[4])
{
  out[3] = (char)(v&0xff);
  out[2] = (char)((v>>8)&0xff);
  out[1] = (char)((v>>16)&0xff);
  out[0] = (char)((v>>24)&0xff);
}

/* big-endian storage of ul8store */
void ul8store_be(uint64_t v, char out[8])
{
  memcpy(out,&v,sizeof(v));
}

/* convert a 4-byte-array into a long int */
uint32_t byte4_2_ul(char b[4])
{
  return (uint32_t) ((uint32_t)(b[0]&0xff)  |
    ((uint32_t)(b[1]&0xff)<<8)  |
    ((uint32_t)(b[2]&0xff)<<16) |
    ((uint32_t)(b[3]&0xff)<<24));
}

/* convert a 3-byte-array into a long int */
uint32_t byte3_2_ul(char b[3])
{
  return (uint32_t)(((uint32_t)(b[0]&0xff))  | 
    (((uint32_t)(b[1]&0xff))<<8)  |
    (((uint32_t)(b[2]&0xff))<<16)) ;
}

/* convert a 2-byte-array into a long int */
uint32_t byte2_2_ul(char b[2])
{
  return (uint32_t)(((uint32_t)(b[0]&0xff))  | 
    (((uint32_t)(b[1]&0xff))<<8)) ;
}

/* convert a 8-byte-array into a long long int */
uint64_t byte8_2_ul(char b[8])
{
  return (long long)byte4_2_ul(b) + 
    (((long long)byte4_2_ul(b+4))<<32);
}

/* convert a 4-byte-array into a float value */
float byte4_2_float(char b[4])
{
  float val = 0;

  ((char*)&val)[0] = b[3] ;
  ((char*)&val)[1] = b[2] ;
  ((char*)&val)[2] = b[1] ;
  ((char*)&val)[3] = b[0] ;
  return val;
}

/* convert a 8-byte-array into a double value */
double byte8_2_double(char b[8])
{
  double val = 0;

  ((char*)&val)[0] = b[7] ;
  ((char*)&val)[1] = b[6] ;
  ((char*)&val)[2] = b[5] ;
  ((char*)&val)[3] = b[4] ;
  ((char*)&val)[4] = b[3] ;
  ((char*)&val)[5] = b[2] ;
  ((char*)&val)[6] = b[1] ;
  ((char*)&val)[7] = b[0] ;
  return val;
}

double byte8_2_double_be(char b[8])
{
  double val = 0;

  ((char*)&val)[0] = b[0] ;
  ((char*)&val)[1] = b[1] ;
  ((char*)&val)[2] = b[2] ;
  ((char*)&val)[3] = b[3] ;
  ((char*)&val)[4] = b[4] ;
  ((char*)&val)[5] = b[5] ;
  ((char*)&val)[6] = b[6] ;
  ((char*)&val)[7] = b[7] ;
  return val;
}

char* strend(char *s)
{
  for (;s&&*s&&*s!='\0';s++);
  return s;
}

uint32_t lenenc_int_size_get(char *end)
{
  if (end[0]<0xfb) {
    return 1;
  } else if (end[0]==0xfc) {
    return 3;
  } else if (end[0]==0xfd) {
    return 3;
  } else if (end[0]==0xfe) {
    return 9;
  }
  return 0;
}

uint32_t lenenc_int_get(char **end)
{
  uint32_t ln = 0;

  if ((*end[0]&0xff)<0xfb) {
    ln = (*end[0])&0xff ;
    (*end) ++ ;
  } else if ((*end[0]&0xff)==0xfc) {
    (*end) ++ ;
    ln = byte2_2_ul(*end);
    (*end) += 2;
  } else if ((*end[0]&0xff)==0xfd) {
    (*end) ++ ;
    ln = byte3_2_ul(*end);
    (*end) += 3;
  } else if ((*end[0]&0xff)==0xfe) {
    (*end) ++ ;
    ln = byte8_2_ul(*end);
    (*end) += 8;
  }
  return ln;
}

int lenenc_int_set(uint64_t val, char *outb)
{
  char *end = outb ;

  if (val<0xfb) {
    end[0] = (uint8_t)val ;
    end++;
  } else if (val<(1<<16)) {
    end[0] = 0xfc ;
    end++ ;
    ul2store((uint16_t)val,end);
    end+=2;
  } else if (val<(1<<24)) {
    end[0] = 0xfd ;
    end++ ;
    ul3store((uint32_t)val,end);
    end+=3;
  } else {
    end[0] = 0xfe ;
    end++ ;
    ul8store_be(val,end);
    end+=8;
  }
  return end-outb;
}

size_t get_int_lenenc_size(uint64_t val)
{
  if (val<0xfb) {
    return 1;
  } else if (val<(1<<16)) {
    return 3;
  } else if (val<(1<<24)) {
    return 4;
  } else {
    return 9;
  }
}

int lenenc_str_set(char *inb, char *str)
{
  char *end = inb;
  int ln = strlen(str);

#if 0
  end[0] = (uint8_t)ln ;
  end++ ;
#else
  end+=lenenc_int_set(ln,end);
#endif
  memcpy(end,str,ln);
  end += ln ;
  return end-inb;
}

#if 0
char* lenenc_str_get(char *in, char *out)
{
  uint8_t len = (uint8_t)in[0] ;
  size_t sz = 0;

  if (out) {
    sz = (len>=MAX_NAME_LEN)?(MAX_NAME_LEN-1):
      len ;
    strncpy(out,in+1,sz);
    out[sz] = '\0';
  }
  return (char*)in+(len+1);
}
#else
char* lenenc_str_get(char *in, char *out, int capacity)
{
  uint32_t len = lenenc_int_get(&in);
  size_t sz = 0;

  if (out) {
    sz = (len>=capacity)?(capacity-1):
      len ;
    strncpy(out,in,sz);
    out[sz] = '\0';
  }
  return (char*)in+(len);
}
#endif

size_t lenenc_str_size_get(char *in)
{
  char *end = in ;

  return lenenc_int_size_get(in) + lenenc_int_get(&end);
}

size_t get_str_lenenc_size(char *in)
{
  size_t ln = strlen(in);

  return ln + get_int_lenenc_size(ln);
}

void ul2ipv4(char *outb, uint32_t ip)
{
  sprintf(outb,"%d.%d.%d.%d", (ip>>24)&0xff,
    (ip>>16)&0xff,(ip>>8)&0xff,ip&0xff);
}

char* to_lower(char *str)
{
  char *p = str;

  for (;p&&*p;p++) 
    *p = tolower(*p);
  return str;
}

