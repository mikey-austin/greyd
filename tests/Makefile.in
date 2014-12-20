#
# Makefile for greyd unit tests.
#

SRC			= test.c
TESTS		= $(wildcard test_*.c)
TEST_OBJ    = $(SRC:.c=.o) $(TESTS:.c=.o)
TARGETS		= $(TESTS:.c=.t)
LIBS        = -lz -ldl -lcrypto
CFLAGS		+= -I../src

all: $(TARGETS)

test_%.t: test_%.o $(TEST_OBJ)
	$(CC) $(CFLAGS) $(LIBS) -Wl,-E -o $@ $< $(SRC:.c=.o) $(shell find ../src -maxdepth 1 -name \*.o -and -not -name main_\*)

clean:
	rm -f $(TARGETS) $(TEST_OBJ)

test check: $(TARGETS)
	@echo
	@echo "******************************"
	@echo "** Preparing to run tests...**"
	@echo "******************************"
	@echo
	LD_LIBRARY_PATH=/usr/local/BerkeleyDB.5.3/lib perl run_tests.pl ./*.t

.PHONY: clean
