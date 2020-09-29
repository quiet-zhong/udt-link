CROSS_COMPILE=mips-linux-uclibc-gnu-

VERSION = 0.1
LIB_NAME = libzlink
LIB_SO_NAME = $(LIB_NAME).so

LIB_SO	= $(LIB_SO_NAME).$(VERSION)
LIB_A	= $(LIB_NAME).a

CC	= $(CROSS_COMPILE)gcc
CXX	= $(CROSS_COMPILE)g++
AR	= $(CROSS_COMPILE)ar


arch = MIPS

CFLAGS	= -fPIC -Wall
CFLAGS	+= -g
CFLAGS	+= -DLINUX -Wextra  -finline-functions -O0 -fno-strict-aliasing -fvisibility=hidden

ifeq ($(arch), IA32)
	CFLAGS += -DIA32
else ifeq ($(arch), POWERPC)
	CFLAGS += -mcpu=powerpc
else ifeq ($(arch), SPARC)
	CFLAGS += -DSPARC
else ifeq ($(arch), IA64)
	CFLAGS += -DIA64
else ifeq ($(arch), AMD64)
	CFLAGS += -DAMD64
else
	CFLAGS += -D$(arch)
endif



INCLUDE = -I./include -I./udt -I./zlink

COBJS	+= $(patsubst %.c, %.o, $(wildcard *.c))
CXXOBJS	+= $(patsubst %.cpp, %.o, $(wildcard udt/*.cpp))
CXXOBJS	+= $(patsubst %.cpp, %.o, $(wildcard zlink/*.cpp))

TARGET:$(LIB_SO) $(LIB_A)

$(LIB_SO):$(COBJS) $(CXXOBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(INCLUDE)

$(LIB_A):$(COBJS) $(CXXOBJS)
	$(AR) -rcs $@ $^ $(LIBS)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ $(INCLUDE)

$(CXXOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $^ $(INCLUDE)


.PHONY:clean

clean:
	rm -f $(COBJS) $(CXXOBJS) $(LIB_SO) $(LIB_A)
