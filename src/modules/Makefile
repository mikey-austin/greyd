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
	$(CC) $(CFLAGS) -shared -o netfilter.so netfilter.o -lnetfilter_conntrack -lmnl -lcap -lipset -lnetfilter_log

fw_dummy.so: fw_dummy.o
	$(CC) $(CFLAGS) -shared -o fw_dummy.so fw_dummy.o

bdb.so: bdb.o
	$(CC) $(CFLAGS) -shared -o bdb.so bdb.o -L/usr/local/BerkeleyDB.5.3/lib -ldb

.PHONY: clean

clean:
	rm -f $(OBJ) *.so
