
include ./Makefile.inc


REL_FILE        := RELEASE

.PHONY: all
all:  
	$(MAKE) -C src all
	$(MAKE) -C tests all
	$(MAKE) -C wrapper all
	#
	# generate 'release log' file
	# 
	/bin/echo -e "last build: "`date -R`"\nmodule version: " \
	 `grep -i "__mod_ver__" src/$(MODULE).h|awk '{print $$3}'|sed 's/\"//g'` \
	 "\nhis date:" `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' ChangeLog \
	 -m1 |sed 's/\://'|sed 's/[ ]\+//'` "\n" > $(REL_FILE)
	#
	# update 'last modified date'
	#
	sed -i "s/Last modified: [a-zA-Z0-9]\+[\ \-]\+[0-9]\+[\,\ \-]\+[0-9]\+/Last modified: `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' ChangeLog -m1|sed 's/\://'|sed 's/\ \+//'`/g" src/$(MODULE).h


.PHONY: clean
clean:  
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(MAKE) -C wrapper clean
	rm -rf $(REL_FILE)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

