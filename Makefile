
#include ./Makefile.inc


REL_FILE        := RELEASE

.PHONY: all
all:  
	$(MAKE) -C src all


.PHONY: clean
clean:  
	$(MAKE) -C src clean

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

