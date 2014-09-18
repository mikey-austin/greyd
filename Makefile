#
# Makefile for greyd.
#

SRC		= $(wildcard *.c)
MAINS   = main_greyd_setup.o
OBJ		= $(SRC:.c=.o)
CC		= clang
CFLAGS	= -g -O0 -Wall -pedantic -Wno-gnu-zero-variadic-macro-arguments
TESTS   = tests

all: greyd-setup test

greyd-setup: $(OBJ)
	$(CC) $(CFLAGS) -o greyd-setup main_greyd_setup.o $(filter-out $(MAINS),$(OBJ))

#
# Generate the object file header dependencies.
#
%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$

-include $(SRC:.c=.d)

test: $(OBJ)
	cd $(TESTS) && $(MAKE) test

.PHONY: clean

clean:
	rm -f $(OBJ) $(MAINS) $(SRC:.c=.d) $(SRC:.c=.d).* greyd-setup
	cd $(TESTS) && $(MAKE) clean
