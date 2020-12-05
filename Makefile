CC:=gcc
CFLAGS:=$(shell pkg-config --cflags json-c)
LDFLAGS:=$(shell pkg-config --libs json-c) $(shell pkg-config --libs libcurl)

ifdef LIBNL_TINY
CFLAGS += $(shell pkg-config --cflags libnl-tiny) -DLIBNL_TINY -D_GNU_SOURCE
LDFLAGS += $(shell pkg-config --libs libnl-tiny)
endif

ifndef LIBNL_TINY
CFLAGS += $(shell pkg-config --cflags libnl-3.0)
LDFLAGS += $(shell pkg-config --libs libnl-3.0) $(shell pkg-config --libs libnl-genl-3.0)
endif

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o clocate main.c nl80211.c mozilla.c google.c provider.c
