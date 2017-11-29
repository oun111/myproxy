include ../Makefile.inc

MODULE          := cwpr
TMPDIR          := .$(MODULE)
ZASDIR          := ../src
ZASLIB          := $(ZASDIR)/libzas.a

JNI_SRC         := $(shell find ./ -name "*.java")
JNI_OBJ         := $(patsubst %.java,%,$(JNI_SRC))
JNI_HEADER      := $(patsubst %.java,%Jni.h,$(JNI_SRC))


CPP_SRCS        := $(shell find ./ -name "*.c" )
CPP_OBJS        := $(patsubst %.c,%.o,$(CPP_SRCS))

OBJS            := $(CPP_OBJS)

MOD_VER         := $(shell grep -i "__mod_ver__" ./$(MODULE).c|awk '{print $$3}'|sed 's/\"//g')
LDFLAG          := -Wall -g -pthread -lrt -L$(ZASDIR) -lzas -ldl 
CFLAG_OBJS      := -Wall -Werror -I. -g -I$(ZASDIR) -I$(ZASDIR)/DAL/mysql \
                   -I/usr/lib/jvm/java-8-openjdk-amd64/include/ \
                   -I/usr/lib/jvm/java-8-openjdk-amd64/include/linux \
                   -I$(ZASDIR)/toolkits -I/usr/include/python3.5/ -fPIC
TARGET_LIB      := lib$(MODULE).a
TARGET_SO       := lib$(MODULE).$(MOD_VER).so


#CFLAG_OBJS    += -I./src

$(TARGET_LIB):$(CPP_OBJS)
	#
	# generate static lib 
	#
	mkdir -p $(TMPDIR)
	$(AR) -x $(ZASLIB) 
	$(AR) -t $(ZASLIB) |xargs -t -I{} mv -f {} $(TMPDIR)/ >/dev/null 2>&1 || \
	  echo 1>/dev/null
	$(AR) -r $@ $(CPP_OBJS) $(TMPDIR)/*
	rm -rf $(TMPDIR)


$(TARGET_SO):
	#
	# generate dynamic lib 
	#
	$(CPP) -shared $(CPP_OBJS) $(TARGET_LIB) -o $@  -lcrypto -fPIC

.PHONY: all
all: jni_gen $(TARGET_LIB) $(TARGET_SO) 

$(CPP_OBJS):%.o:%.c
	$(CPP) $(CFLAG_OBJS) -c $< -o $@

jni_gen:$(JNI_OBJ)

$(JNI_OBJ):%:%.java
	#
	# generating jni objects
	#
	javah -cp java -o $(JNI_HEADER) -jni $(notdir $@)
	javac $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET_LIB) $(TARGET_SO)
	rm -rf $(TMPDIR)
	rm -rf $(JNI_OBJ).class
	rm -rf $(JNI_HEADER)
	rm -rf python/__pycache__/

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags
