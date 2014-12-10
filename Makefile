#
# Master makefile for greyd.
#

CC		= clang
CFLAGS	= -g -O0 -Wall -pedantic

all clean:
	$(MAKE) -C src/ $@
	$(MAKE) -C tests/ $@

check test: all
	$(MAKE) -C tests/ $@

.PHONY: all clean
