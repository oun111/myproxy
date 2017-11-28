
#include "resizable_container.h"
#include <stdlib.h>
#include <string.h>

tContainer::tContainer(void) :
  b(NULL),capacity(0L),size(0L)
{
}

tContainer::~tContainer(void) 
{
  tc_close();
}

void tContainer::tc_init(void) 
{
  b = NULL;
  capacity = 0L;
  size = 0L ;
}

void tContainer::tc_close(void) 
{
  if (b) 
    free(b);
  b = 0;
  capacity = 0L;
  size = 0L ;
}

char* tContainer::tc_data(void) 
{
  return b ;
}

size_t tContainer::tc_capacity(void) 
{ 
  return capacity; 
}

size_t tContainer::tc_free(void) 
{ 
  return tc_length()>=tc_capacity()?0:
    tc_capacity()-tc_length(); 
}

size_t tContainer::tc_length(void) 
{ 
  return size; 
}

int tContainer::tc_concat(char *in, size_t sz)
{
  tContainer tmp;
  size_t cap = tc_length();

  if (tc_free()<=sz) {
    tmp.tc_copy(this);
    tc_resize(cap+sz);
    tc_copy(&tmp);
  }
  tc_write(in,sz);
  return 0;
}

void tContainer::tc_resize(size_t new_sz) 
{
  if (new_sz>capacity) {
    b = (char*)realloc(b,new_sz);
    capacity = new_sz ;
  }
  tc_update(0);
}

size_t tContainer::tc_write(char *in, size_t sz) 
{
  const size_t szFree = tc_free();

  sz = sz<szFree?sz:szFree;
  memcpy(tc_data()+tc_length(),in,sz);
  tc_update(tc_length()+sz);
  return sz ;
}

size_t tContainer::tc_read(char *out, size_t sz, size_t rpos) 
{
  /* read position is invalid */
  if (rpos>=tc_length())
    return 0;
  /* get available read size */
  sz = sz<(tc_length()-rpos)?sz:(tc_length()-rpos);
  memcpy(out,tc_data()+rpos,sz);
  return sz ;
}

void tContainer::tc_update(size_t sz) 
{
  //size = sz ;
  __sync_lock_test_and_set(&size,sz);
}

int tContainer::tc_copy(tContainer *src) 
{
  size_t sz = src->tc_length();

  if (sz==0)
    return 0;
  tc_resize(sz);
  tc_write(src->tc_data(),sz);
  return sz;
}

tContainer& tContainer::operator = (tContainer &src)
{
  tc_copy(&src);
  return *this ;
}

