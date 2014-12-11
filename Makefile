#
# Master makefile for greyd.
#

CC		= clang
export CC
CFLAGS	= -g -O0 -Wall -pedantic
export CFLAGS

all clean:
	$(MAKE) -C src/ $@
	$(MAKE) -C tests/ $@

check test: all
	$(MAKE) -C tests/ $@

.PHONY: all clean
