
include ../Makefile.inc

C_SRCS          := $(shell ls *.c)
C_OBJS          := $(patsubst %.c,%.o,$(C_SRCS))

J_SRCS          := $(shell ls *.java)
J_OBJS          := $(patsubst %.java,%,$(J_SRCS))

LIB_PATH        := ../src/
LDFLAG          := -Wall -g -pthread -lrt -L$(LIB_PATH) -l$(MODULE) -ldl 
CFLAG_OBJS      := -Wall -Werror -I. -g  -fPIC -I$(LIB_PATH)/DAL/mysql  \
                   -I$(LIB_PATH)/toolkits -I$(LIB_PATH)
LDFLAG        +=  -lz -lcrypto

TARGET          := test 
TARGET_LIB      := $(LIB_PATH)/lib$(MODULE).a

.PHONY: all
all:  $(TARGET) 
#java

java:$(J_OBJS)
$(J_OBJS):%:%.java
	#
	# set correct path of zas.class before compiling
	#
	$(shell export CLASSPATH=$(CLASSPATH):$(shell pwd)/../wrapper/java)
	javac $<

$(TARGET):$(C_OBJS) 
	$(CPP) $(C_OBJS) -o $@ $(LDFLAG) 

$(C_OBJS):%.o:%.c
	$(CPP) $(CFLAG_OBJS) -c $< -o $@ 

.PHONY: clean
clean:
	rm -f *.o *.class
	rm -rf $(TARGET) 

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags
