#
# pkgutil program
#
# makefile
#
ifndef INITRD_DIR
    INITRD_DIR = $(PWD)/../../initrd_tmp
    export INITRD_DIR
endif

INST_DIR = $(INITRD_DIR)/bin/

#PROG = pkgutil
PROG_MKPKG = mkpkg
PROG_EXPKG = expkg
PROG_XPKG = pkgutil

#CFLAGS += -D_GUN_SOURCE
#CFLAGS += -D_XOPEN_SOURCE
CFLAGS += -Wall
CFLAGS += -g
LDFLAGS += -lssl

MKPKG_SRCS = mkpkg.c pkgutil.c
MKPKG_OBJS = $(MKPKG_SRCS:.c=.o)

EXPKG_SRCS = expkg.c pkgutil.c
EXPKG_OBJS = $(EXPKG_SRCS:.c=.o)

XPKG_SRCS = xpkg.c pkgutil.c
XPKG_OBJS = $(XPKG_SRCS:.c=.o)

all: $(PROG_MKPKG) $(PROG_EXPKG) $(PROG_XPKG)

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<
 
$(PROG_MKPKG): $(MKPKG_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PROG_EXPKG): $(EXPKG_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PROG_XPKG): $(XPKG_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: all
	mkdir -p $(INST_DIR)
	strip $(PROG_MKPKG) $(PROG_EXPKG) $(PROG_XPKG)
	cp -f $(PROG_EXPKG) $(INST_DIR)

env: $(PROG_MKPKG) $(PROG_EXPKG) $(PROG_XPKG)
	strip $(PROG_XPKG)
	cp -f $(PROG_XPKG) /sbin/

clean:
	rm -f *.o
	rm -f $(PROG_MKPKG) $(PROG_EXPKG) $(PROG_XPKG)

