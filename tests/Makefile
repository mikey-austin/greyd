#
# Makefile for greyd unit tests.
#

SRC			= test.c
TESTS		= $(wildcard test_*.c)
TEST_OBJ    = $(SRC:.c=.o) $(TESTS:.c=.o)
TARGETS		= $(TESTS:.c=.t)
WRAPPERS	= $(TESTS:.c=.pl)
CC			= clang
CFLAGS		= -g -O0

all: $(TARGETS)

test_%.t: test_%.o $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(SRC:.c=.o) $(shell find .. -maxdepth 1 -name \*.o -and -not -name main.\*)
	echo "system(\"valgrind -q --leak-check=full --error-exitcode=1 --tool=memcheck $(shell pwd)/$@\")" >test_$*.pl

.PHONY: clean test

clean:
	rm -f $(TARGETS) $(TEST_OBJ) $(WRAPPERS)

test: $(TARGETS)
	@echo
	@echo "******************************"
	@echo "** Preparing to run tests...**"
	@echo "******************************"
	@echo
	HARNESS_COLOR=1 HARNESS_VERBOSE=1 perl -MTest::Harness -e 'runtests(@ARGV)' *.pl
