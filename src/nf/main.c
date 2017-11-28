
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "framework.h"
#include "dummy_business.h"


DECLARE_LOG("nf");

int main(int argc,char **argv)
{
  dummy_busi busi((char*)"dummy business") ;
  epoll_impl *d1 = 0;
  
  d1 = new epoll_impl(&busi) ;
  d1->start();
  while (1) { sleep(1); }
  return 0;
}

