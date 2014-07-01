#
# Makefile for greyd.
#

SRC		= $(wildcard *.c)
OBJ		= $(SRC:.c=.o)
TARGET 	= greyd
CC		= clang
CFLAGS	= -g -O0
TESTS   = tests

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

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
	rm -f $(TARGET) $(OBJ) $(SRC:.c=.d) $(SRC:.c=.d).*
	cd $(TESTS) && $(MAKE) clean
