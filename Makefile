
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
	$(MAKE) -C src distclean
	rm -rf cscope.* 
	rm -rf tags

