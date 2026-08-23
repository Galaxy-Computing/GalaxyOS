DEFAULT_HOST!=../../../default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../../../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/galaxyos
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
PREFIX_BIN?=$(EXEC_PREFIX)/${HOSTARCH}

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -Iinclude

CFLAGS:=$(CFLAGS)
CPPFLAGS:=$(CPPFLAGS)

OBJS=\
main.o \

BINARIES=init.elf

.PHONY: all clean install install-headers
.SUFFIXES: .o .c .S

all: $(BINARIES)

init.elf: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BINARIES) *.a
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES)
	mkdir -p $(DESTDIR)$(PREFIX)
	cp $(BINARIES) $(DESTDIR)$(PREFIX)

install-headers:

-include $(OBJS:.o=.d)
