
CPP:= gcc
AR:= ar

C_SRCS        := $(shell find ./ -name "*.c")
C_OBJS        := $(patsubst %.c,%.o,$(C_SRCS))

LDFLAG          := -Wall -g  -Werror # -lcrypto -lz
CFLAG_OBJS      := -Wall -Werror -I. -g 

OBJS            := $(C_OBJS)
TARGET_LIB      := libtoolkits.a


.PHONY: all
all: $(TARGET_LIB)
	#
	# update 'last modified date'
	#
	sed -i "s/Last modified: [a-zA-Z0-9]\+[\ \-]\+[0-9]\+[\,\ \-]\+[0-9]\+/Last modified: `grep -i '[a-z]\+\ [0-9]\+\, [0-9]\+\:' ChangeLog -m1|sed 's/\://'|sed 's/\ \+//'`/g" version.h

$(TARGET_LIB):$(C_OBJS)
	$(AR) -r $(TARGET_LIB) $(C_OBJS)

$(C_OBJS):%.o:%.c
	$(CPP) $(CFLAG_OBJS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET_LIB)

.PHONY: distclean
distclean:clean
	rm -rf cscope.* 
	rm -rf tags

