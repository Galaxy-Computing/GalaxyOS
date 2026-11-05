LDFLAGS?=
LIBS?=

CFLAGS:=-Og -g -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -Iinclude

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

conftool: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS)

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
