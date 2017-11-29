
CPP:= gcc
AR:= ar

C_SRCS        := $(shell find ./ -name "*.c" -a ! -name "test.c")
C_OBJS        := $(patsubst %.c,%.o,$(C_SRCS))

TOOLKITS_SRCS   := $(shell find ../toolkits -name "*.c" )
TOOLKITS_OBJS   := $(patsubst %.c,%.o,$(TOOLKITS_SRCS))

TEST_APP_SRC    := test.c
TEST_APP        := $(patsubst %.c,%,$(TEST_APP_SRC))

LDFLAG          := -Wall -g -pthread -Werror -lcrypto -lz 
CFLAG_OBJS      := -Wall -Werror -I. -g -I../toolkits -fPIC  #-DNDEBUG

OBJS            := $(C_OBJS) $(TOOLKITS_OBJS)
TARGETS         := $(TEST_APP)
TARGET_LIB      := libmysqlc.a


.PHONY: all
all: $(TARGETS)
	#
	# update 'last modified date'
	#
	sed -i "s/Last modified: [a-zA-Z0-9]\+[\ \-]\+[0-9]\+[\,\ \-]\+[0-9]\+/Last modified: `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' HISTORY.TXT -m1|sed 's/\://'|sed 's/\ \+//'`/g" mysqlc.h

$(TARGET_LIB):$(OBJS)
	$(AR) -r $(TARGET_LIB) $(OBJS)


$(TEST_APP): $(TARGET_LIB)
	$(CPP) $(TEST_APP_SRC) $(TARGET_LIB) -o $@ $(LDFLAG) -I../toolkits

$(OBJS):%.o:%.c
	$(CPP) $(CFLAG_OBJS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGETS) $(OBJS) $(TARGET_LIB)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

