DESTDIR?=
PREFIX_BIN?=/galaxyos/$(HOSTARCH)
EXEC_PREFIX?=$(PREFIX_BIN)
INCLUDEDIR?=$(PREFIX_BIN)/include
LIBDIR?=$(PREFIX_BIN)

SUBDIRS := lib bin
SUBDIRS_INSTALL := $(addsuffix -install, $(SUBDIRS))
SUBDIRS_INSTALL_HEADERS := $(addsuffix -install-headers, $(SUBDIRS))
SUBDIRS_CLEAN := $(addsuffix -clean, $(SUBDIRS))

.PHONY: all install install-headers clean $(SUBDIRS) $(SUBDIRS_INSTALL) $(SUBDIRS_INSTALL_HEADERS) $(SUBDIRS_CLEAN)

all: $(SUBDIRS)
install: $(SUBDIRS_INSTALL)
install-headers: $(SUBDIRS_INSTALL_HEADERS)
clean: $(SUBDIRS_CLEAN)

$(SUBDIRS): %:
		$(MAKE) -C $@

$(SUBDIRS_INSTALL): %:
		$(MAKE) install -C $(patsubst %-install,%,$@)

$(SUBDIRS_INSTALL_HEADERS): %:
		$(MAKE) install-headers -C $(patsubst %-install-headers,%,$@)

$(SUBDIRS_CLEAN): %:
		$(MAKE) clean -C $(patsubst %-clean,%,$@)