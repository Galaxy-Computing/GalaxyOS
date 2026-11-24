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
LIBDIR?=$(EXEC_PREFIX)/${HOSTARCH}

HOST_CC?=gcc
HOST_CFLAGS?=-Og -g -Wall -Wextra

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -Iinclude

OBJS=\
gcf.o \

HOST_OBJS=\
gcf-host.o \

BINARIES=gcf.a

.PHONY: all host clean install install-headers install-libs
.SUFFIXES: .o .c .S

all: $(BINARIES)

host: gcf-host.a

gcf.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

gcf-host.a: $(HOST_OBJS)
	ar rcs $@ $(HOST_OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

gcf-host.o: gcf.c include/gcf.h
	$(HOST_CC) -MD -c $< -o $@ -std=gnu11 $(HOST_CFLAGS) -Iinclude

clean:
	rm -f $(BINARIES) gcf-host.a *.a
	rm -f $(OBJS) $(HOST_OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) $(HOST_OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs: $(BINARIES)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d) $(HOST_OBJS:.o=.d)
