#
# Makefile for pluggable firewall modules.
#

SRC = pf.c ipset.c bdb.c
OBJ = $(SRC:.c=.o)
CC  = clang
CFLAGS = -g -O0 -Wall -pedantic -fpic -Wno-gnu-zero-variadic-macro-arguments

all: pf.so ipset.so fw_dummy.so bdb.so

pf.so: pf.o
	$(CC) $(CFLAGS) -shared -o pf.so pf.o

ipset.so: ipset.o
	$(CC) $(CFLAGS) -shared -o ipset.so ipset.o -lnetfilter_conntrack -lmnl -lcap -lipset

fw_dummy.so: fw_dummy.o
	$(CC) $(CFLAGS) -shared -o fw_dummy.so fw_dummy.o

bdb.so: bdb.o
	$(CC) $(CFLAGS) -shared -o bdb.so bdb.o -L/usr/local/BerkeleyDB.5.3/lib -ldb

.PHONY: clean

clean:
	rm -f $(OBJ) *.so
