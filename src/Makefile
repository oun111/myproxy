include ../Makefile.inc



CPP_SRCS        := $(shell find ./ -name "*.cpp")
CPP_OBJS        := $(patsubst %.cpp,%.o,$(CPP_SRCS))

C_SRCS        := $(shell find ./ ! -name "test.c" -a -name "*.c" )
C_OBJS        := $(patsubst %.c,%.o,$(C_SRCS))

OBJS            := $(CPP_OBJS) $(C_OBJS)

MOD_VER         := $(shell grep -i "__mod_ver__" ./$(MODULE).h|awk '{print $$3}'|sed 's/\"//g')
LDFLAG          := -Wall -g -pthread -lrt -L. -l$(MODULE) -ldl #.$(MOD_VER) 
CFLAG_OBJS      := -Wall -Werror -I. -g -UDBG_LOG -UDB_RECONN -fPIC
TARGET_LIB      := lib$(MODULE).a
TARGET_SO       := lib$(MODULE).$(MOD_VER).so

CFLAG_OBJS      += -D__LAZY_EXEC__ 

CFLAG_OBJS    += -I./DAL -I./DAL/mysql -I./toolkits
LDFLAG        +=  -lz -lcrypto

$(TARGET_LIB):$(OBJS)
	$(AR) -r $@ $(OBJS) 


$(TARGET_SO):$(OBJS)
	#
	# generate dynamic lib 
	#
	$(CPP) $(OBJS) -shared -o $@

.PHONY: all
all: $(TARGET_LIB) $(TARGET_SO) 

$(CPP_OBJS):%.o:%.cpp
	$(CPP) $(CFLAG_OBJS) -c $< -o $@

$(C_OBJS):%.o:%.c
	$(CC) $(CFLAG_OBJS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) *.so $(TARGET_LIB) $(TARGET_SO)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags
