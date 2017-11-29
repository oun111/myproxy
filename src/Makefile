
include ./dbw/rules.mk

CPP:= g++
CC := gcc

CPP_SRCS        := $(shell find ./ ./hooks -maxdepth 1 -name "*.cpp")
CPP_OBJS        := $(patsubst %.cpp,%.o,$(CPP_SRCS))

C_SRCS        := $(shell find ./ -maxdepth 1 -name "*.c")
C_OBJS        := $(patsubst %.c,%.o,$(C_SRCS))

MYSQLS_SRCS   := $(shell find ./mysqls -maxdepth 1 -name "*.c")
MYSQLS_OBJS   := $(patsubst %.c,%.o,$(MYSQLS_SRCS))

HOOK_PATH := hooks/
HOOK_LIST := ./hook_list.h

ZAS_PATH      := ./zas/src/
LDFLAG          := -Wall -g -pthread -Werror -L./nf -lnf \
                    -L$(ZAS_PATH) -lzas   \
                    -lz -lcrypto -ldbwpr $(DBWPR_LDFLAGS) 
CFLAGS      := -Wall -Werror -I. -g  \
                     -I./mysqls \
                     -I$(ZAS_PATH)/toolkits -I$(ZAS_PATH)/DAL/mysql 
CPPFLAGS    := -Wall -Werror -I. -g \
                     -I./nf -I./nf/toolset \
                     -I./mysqls \
                     -I$(ZAS_PATH)/toolkits -I$(ZAS_PATH)/DAL/mysql \
                     -I$(ZAS_PATH) \
                     $(DBWPR_CFLAGS) \
#                    -include $(HOOK_LIST) \
                     -Wno-deprecated-declarations

OBJS            := $(CPP_OBJS) $(C_OBJS) $(MYSQLS_OBJS)
TARGET          := myproxy_app



.PHONY: all
all: gen_module_list $(TARGET) 

del_module_list:
	$(shell python3 module_gen.py --del $(HOOK_LIST))

gen_module_list: 
	$(shell python3 module_gen.py --gen $(HOOK_PATH) $(HOOK_LIST))

Libraries: 
	make -C $(ZAS_PATH) all
	make -C dbw all
	make -C nf all

$(TARGET): Libraries  $(OBJS)
	$(CPP) $(OBJS) -o $@ $(LDFLAG)

$(MYSQLS_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(C_OBJS):%.o:%.c
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(CPP_OBJS):%.o:%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean: del_module_list
	rm -rf $(TARGET) $(OBJS)

.PHONY: distclean
distclean:clean
	make -C $(ZAS_PATH) distclean
	make -C ./dbw distclean
	make -C ./nf distclean
	rm -rf cscope.* 
	rm -rf tags

