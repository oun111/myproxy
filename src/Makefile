
include ./dbw/rules.mk

GEN_DIR:=$(shell pwd)/.gen/

CPP:= g++
CC := gcc

CPP_SRCS        := $(notdir $(shell find . -name "*.cc"))
CPP_OBJS        := $(patsubst %.cc,$(GEN_DIR)/%.o,$(CPP_SRCS))
CPP_OBJS        += $(GEN_DIR)/main.o

C_SRCS          := $(notdir $(shell find mysqls/ nf/toolset/ -name "*.c"))
C_OBJS          := $(patsubst %.c,$(GEN_DIR)/%.o,$(C_SRCS))

HOOK_PATH       := hooks/
HOOK_LIST       := ./hook_list.h

ZAS_PATH        := /home/user1/work/zas/src/
LDFLAG          := -Wall -g -pthread -Werror  \
                     -L$(ZAS_PATH) -lzas   \
                     -lz -lcrypto  $(DBWPR_LDFLAGS) 
CFLAGS          := -Wall -Werror -I. -g  -I./mysqls -I./nf \
                     -I$(ZAS_PATH)/toolkits -I$(ZAS_PATH)/DAL/mysql 
CPPFLAGS        := -Wall -Werror -I. -g \
                     -I./nf -I./nf/toolset \
                     -I./mysqls \
                     -I$(ZAS_PATH)/toolkits -I$(ZAS_PATH)/DAL/mysql \
                     -I$(ZAS_PATH) \
                     $(DBWPR_CFLAGS) 

OBJS            := $(CPP_OBJS) $(C_OBJS) 
TARGET          := myproxy_app

HOOK_SCRIPT     := $(shell pwd)/../scripts/hook_prob.py


.PHONY: all
all: mkdir hook_detect $(TARGET) 

mkdir:
	$(shell mkdir  $(GEN_DIR))

hook_clr:
	$(shell python3 $(HOOK_SCRIPT) --del $(HOOK_LIST))

hook_detect: 
	$(shell python3 $(HOOK_SCRIPT) --gen $(HOOK_PATH) $(HOOK_LIST))

Libraries: 
	make -C $(ZAS_PATH) all

$(TARGET): Libraries  $(OBJS)
	$(CPP) $(OBJS) -o $@ $(LDFLAG)

%.o:../%.c
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../%.cc
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../nf/%.cc
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../hooks/%.cc
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../dbw/%.cc
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../mysqls/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:../nf/toolset/%.c
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean: hook_clr
	rm -rf $(TARGET) $(OBJS) 

.PHONY: distclean
distclean:clean
	make -C $(ZAS_PATH) distclean
	rm -rf cscope.* 
	rm -rf tags
	rm -rf $(GEN_DIR)

