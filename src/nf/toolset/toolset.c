
#include "toolset.h"
#include <stdlib.h>
#include <stdio.h>

uint32_t calc_auth_key(char *strkey)
{
  uint32_t k = 0;
  char *p = strkey ;

  for (;p&&*p;k+=((*p++)&0xaa));
  k ^= 0x4a4a4a4a ;
  k &= 0xffffffff ;
  return k;
}

int get_cpu_cores(void)
{
  char buf[32];
  FILE *ps = popen("cat /proc/cpuinfo|grep processor|wc -l","r");

  fgets(buf,32,ps);
  pclose(ps);
  return atoi(buf);
}
