#
# Makefile for pluggable firewall modules.
#

SRC = pf.c netfilter.c bdb.c fw_dummy.c
OBJ = $(SRC:.c=.o)
CFLAGS += -fpic

all: pf.so netfilter.so fw_dummy.so bdb.so

pf.so: pf.o
	$(CC) $(CFLAGS) -shared -o pf.so pf.o

netfilter.so: netfilter.o
	$(CC) $(CFLAGS) -shared -o netfilter.so netfilter.o -L/usr/local/lib -lnetfilter_conntrack -lmnl -lcap -lipset -lnetfilter_log

fw_dummy.so: fw_dummy.o
	$(CC) $(CFLAGS) -shared -o fw_dummy.so fw_dummy.o

bdb.so: bdb.o
	$(CC) $(CFLAGS) -shared -o bdb.so bdb.o -L/usr/local/BerkeleyDB.5.3/lib -ldb

install: netfilter.so bdb.so pf.so
	install -d $(DESTDIR)$(libdir)/$(progname)
	install -m 750 netfilter.so $(DESTDIR)$(libdir)/$(progname)
	install -m 750 bdb.so $(DESTDIR)$(libdir)/$(progname)
	install -m 750 pf.so $(DESTDIR)$(libdir)/$(progname)

.PHONY: clean install

clean:
	rm -f $(OBJ) *.so
