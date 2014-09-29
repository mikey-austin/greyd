#
# Makefile for pluggable firewall modules.
#

SRC = pf.c ipset.c
OBJ = $(SRC:.c=.o)
CC  = clang
CFLAGS = -g -O0 -Wall -pedantic -fpic

all: pf.so ipset.so

pf.so: pf.o
	$(CC) $(CFLAGS) -shared -o pf.so pf.o

ipset.so: ipset.o
	$(CC) $(CFLAGS) -shared -o ipset.so ipset.o

.PHONY: clean

clean:
	rm -f $(OBJ) pf.so ipset.so
