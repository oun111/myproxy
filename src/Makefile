
include ./dbw/rules.mk

GEN_DIR:=$(shell pwd)/.gen/

CPP:= g++
CC := gcc

ENTRY_SRC       := $(wildcard main.c)
CPP_SRCS        := $(wildcard *.cpp)
CPP_SRCS        += $(notdir $(wildcard hooks/*.cpp))
CPP_SRCS        += $(notdir $(wildcard nf/*.cpp))
CPP_SRCS        += $(notdir $(wildcard dbw/*.cpp))
CPP_OBJS        := $(patsubst %.cpp,$(GEN_DIR)/%.o,$(CPP_SRCS))
CPP_OBJS        += $(patsubst %.c,$(GEN_DIR)/%.o,$(ENTRY_SRC))

C_SRCS          := $(notdir $(wildcard mysqls/*.c))
C_SRCS          += $(notdir $(wildcard nf/toolset/*.c))
C_OBJS          := $(patsubst %.c,$(GEN_DIR)/%.o,$(C_SRCS))

HOOK_PATH       := hooks/
HOOK_LIST       := ./hook_list.h

ZAS_PATH        := ./zas/src/
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



.PHONY: all
all: mkdir gen_module_list $(TARGET) 

mkdir:
	$(shell mkdir  $(GEN_DIR))

del_module_list:
	$(shell python3 module_gen.py --del $(HOOK_LIST))

gen_module_list: 
	$(shell python3 module_gen.py --gen $(HOOK_PATH) $(HOOK_LIST))

Libraries: 
	make -C $(ZAS_PATH) all

$(TARGET): Libraries  $(OBJS)
	$(CPP) $(OBJS) -o $@ $(LDFLAG)

%.o:../%.c
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../nf/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../hooks/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../dbw/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.o:../mysqls/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:../nf/toolset/%.c
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean: del_module_list
	rm -rf $(TARGET) $(OBJS) 

.PHONY: distclean
distclean:clean
	make -C $(ZAS_PATH) distclean
	rm -rf cscope.* 
	rm -rf tags
	rm -rf $(GEN_DIR)

