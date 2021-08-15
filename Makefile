CC:=gcc
SRCFILES:= main.c nl80211.c provider.c
CFLAGS:=$(shell pkg-config --cflags json-c)
LDFLAGS:=$(shell pkg-config --libs json-c)

ifdef LIBNL_TINY
CFLAGS += $(shell pkg-config --cflags libnl-tiny) -DLIBNL_TINY -D_GNU_SOURCE
LDFLAGS += $(shell pkg-config --libs libnl-tiny)
endif

ifndef LIBNL_TINY
CFLAGS += $(shell pkg-config --cflags libnl-3.0)
LDFLAGS += $(shell pkg-config --libs libnl-3.0) $(shell pkg-config --libs libnl-genl-3.0)
endif

ifdef UCLIENT
CFLAGS += -lubox -luclient
SRCFILES += uclient.c
else
LDFLAGS += $(shell pkg-config --libs libcurl)
SRCFILES += curl.c
endif


all: clocate

clocate:
	$(CC) $(CFLAGS) $(LDFLAGS) -o clocate $(SRCFILES)

clean:
	rm -rf clocate clocated

.PHONY: clean all
