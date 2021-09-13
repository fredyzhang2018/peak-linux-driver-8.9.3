# (c) Texas Instruments

BASE_DIR = $(abspath .)
#Default build environment, (linux only)
#if nothing is defined, default to linux as in windows this variable is defined
export OS ?= linux
CONFIG=release

MAKE = make
CGTOOLS_PATH=/usr/
AR = $(CGTOOLS_PATH)/bin/gcc-ar
CFLAGS := -Dlinux
LD_OPTS :=
EXE_EXT = out

CC = $(CGTOOLS_PATH)/bin/gcc
LD = $(CGTOOLS_PATH)/bin/gcc

SOURCES := $(wildcard *.c)
OBJS := $(addprefix ,$(notdir $(SOURCES:.c=.o)))
# EXEC_NAME=eth_tx.$(EXE_EXT)

# CFLAGS += -MMD -c -Wall
CFLAGS += -Wall
INCLUDES :=

ifeq ($(CONFIG), release)
CFLAGS+=
OPTI_OPTS=-O3
DEFINE=
else
CFLAGS+=-g
OPTI_OPTS=
DEFINE=-DDEBUG
endif

.PHONY: all clean

recv_1722 : recv_1722.c
	$(CC) $(CFLAGS) $(OPTI_OPTS) $(INCLUDES) -o recv_1722.$(EXE_EXT) $< $(LD_OPTS) $(LIBS)

send_1722 : send_1722.c
	$(CC) $(CFLAGS) $(OPTI_OPTS) $(INCLUDES) -o send_1722.$(EXE_EXT) $< $(LD_OPTS) $(LIBS)

all: send_1722 recv_1722 

clean:
	rm -f recv_1722.out send_1722.out


