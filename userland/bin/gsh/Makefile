DEFAULT_HOST!=../../../default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../../../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?="-L=/galaxyos/${HOSTARCH}"
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

BINARIES=gsh.elf

.PHONY: all clean install install-headers
.SUFFIXES: .o .c .S

all: $(BINARIES)

gsh.elf: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) -l:glcore.a

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BINARIES) *.a
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES)
	mkdir -p $(DESTDIR)$(PREFIX_BIN)
	cp $(BINARIES) $(DESTDIR)$(PREFIX_BIN)

install-headers:

-include $(OBJS:.o=.d)
