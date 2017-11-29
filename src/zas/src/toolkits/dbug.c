
#include <sys/types.h>
#include <stdio.h>

/*
 * XXX: test
 */
void hex_dump(char *buf, size_t len)
{
  size_t i=0;
  int cnt=0;

  for (;i<len;i++) {
    printf("%02x ", buf[i]&0xff);
    if (cnt++>14) {
      cnt=0;
      printf("\n");
    }   
  }
  printf("\n");
}
