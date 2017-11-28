
include rules.mk

CPP:= g++
CC:= gcc
AR:= ar

C_SRCS        := $(shell find ./ -maxdepth 1 -name "*.c" -a ! -name "main.c" )

ifeq ($(USE_UNQLITE),1)
  C_SRCS        += $(shell find $(UNQLITE_BASE) -name "*.c" )
endif
C_OBJS        := $(patsubst %.c,%.o,$(C_SRCS))

CPP_SRCS      := $(shell find ./ -name "*.cpp" )
CPP_OBJS      := $(patsubst %.cpp,%.o,$(CPP_SRCS))

TEST_APP_SRC    := main.c
TEST_APP        := test #$(patsubst %.c,%,$(TEST_APP_SRC))

LDFLAG          := -Wall -g -Werror $(DBWPR_CFLAGS)
CFLAG           := -Wall -I. -g   #-Werror
CPPFLAG         := -Wall -I. $(DBWPR_CFLAGS) $(DBWPR_LDFLAGS) -g -Werror

OBJS            := $(C_OBJS) $(CPP_OBJS)
TARGETS         := $(TEST_APP)
TARGET_LIB      := libdbwpr.a


.PHONY: all
all: $(TARGETS)
	#
	# update 'last modified date'
	#
#	sed -i "s/Last modified: [a-zA-Z0-9]\+[\ \-]\+[0-9]\+[\,\ \-]\+[0-9]\+/Last modified: `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' HISTORY.TXT -m1|sed 's/\://'|sed 's/\ \+//'`/g" mysqlc.h

$(TARGET_LIB):$(OBJS)
	$(AR) -r $(TARGET_LIB) $(OBJS)


$(TEST_APP): $(TARGET_LIB)
	$(CPP) $(TEST_APP_SRC) -o $@ $(LDFLAG) -L. -ldbwpr $(DBWPR_LDFLAGS)

$(C_OBJS):%.o:%.c
	$(CC) $(CFLAG) -c $< -o $@

$(CPP_OBJS):%.o:%.cpp
	$(CPP) $(CPPFLAG) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGETS) $(OBJS) $(TARGET_LIB)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags
	rm -rf __db.*

