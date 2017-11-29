
CPP:= g++
CC := gcc

CPP_SRCS        := $(shell find ./ -name "*.cpp")
CPP_OBJS        := $(patsubst %.cpp,%.o,$(CPP_SRCS))

TOOLSET_SRCS    := $(shell find ./toolset -name "*.c" )
C_OBJS          := $(patsubst %.c,%.o,$(TOOLSET_SRCS))

TEST_APP_SRC    := main.c
TEST_APP        := test_app #$(patsubst %.c,%,$(TEST_APP_SRC))

CLIENT_SRC   := $(shell find ./ -name "client.c")
CLIENT       := $(patsubst %.c,%,$(CLIENT_SRC))

LDFLAG          := -Wall -g -pthread -Werror -I./toolset -I./proto 
CFLAG_OBJS      := -Wall -Werror -I. -g -I./toolset -I./proto -fPIC \
                   -Wno-deprecated-declarations -Wno-misleading-indentation


MOD_VER         := $(shell grep -i "__ver_str__" ./framework.h|awk '{print $$3}'|sed 's/\"//g')

OBJS            := $(CPP_OBJS) $(C_OBJS)
TARGET_LIB      := libnf.a
TARGET_SO       := libnf.$(MOD_VER).so
TARGETS         := $(TARGET_LIB) $(TARGET_SO) $(TEST_APP) $(CLIENT)



$(TARGET_LIB): $(OBJS)
	$(AR) -r $@ $(OBJS) 

$(TARGET_SO):$(OBJS)
	#
	# generate dynamic lib 
	#
	$(CPP) $(CPP_OBJS)  -shared -o $@

.PHONY: all
all: $(TARGETS)
	#
	# update 'last modified date'
	#
	sed -i "s/Last modified: [a-zA-Z0-9]\+[\ \-]\+[0-9]\+[\,\ \-]\+[0-9]\+/Last modified: `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' ChangeLog -m1|sed 's/\://'|sed 's/\ \+//'`/g" framework.h

$(CLIENT): 
	$(CPP) $(CLIENT_SRC) -o $@ $(LDFLAG) toolset/sock_toolkit.o -I./test

$(TEST_APP): $(TARGET_LIB)
	$(CPP) $(TEST_APP_SRC) -o $@ -L. -lnf $(LDFLAG) -I./test -I.

$(CPP_OBJS):%.o:%.cpp
	$(CPP) $(CFLAG_OBJS) -c $< -o $@

$(C_OBJS):%.o:%.c
	$(CC) $(CFLAG_OBJS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGETS) $(OBJS)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

