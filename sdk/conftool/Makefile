LDFLAGS?=
LIBS?=

CFLAGS:=-Og -g -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -I../../userland/lib/gcf/include

GCF_HOST_LIB:=../../userland/lib/gcf/gcf-host.a

DESTDIR:=../bin/

CC:=gcc
AR:=ar
AS:=as

CFLAGS:=$(CFLAGS)
CPPFLAGS:=$(CPPFLAGS)

OBJS=\
main.o \

BINARIES=conftool

.PHONY: all clean install install-headers
.SUFFIXES: .o .c .S

all: $(BINARIES)

conftool: $(OBJS) $(GCF_HOST_LIB)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(GCF_HOST_LIB) $(LDFLAGS)

$(GCF_HOST_LIB): ../../userland/lib/gcf/gcf.c ../../userland/lib/gcf/include/gcf.h
	$(MAKE) -C ../../userland/lib/gcf host

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BINARIES) *.a
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES)
	mkdir -p $(DESTDIR)
	mv $(BINARIES) $(DESTDIR)

install-headers:

-include $(OBJS:.o=.d)
