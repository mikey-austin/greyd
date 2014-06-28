#
# Makefile for greyd.
#

SRC		= $(wildcard *.c)
OBJ		= $(SRC:.c=.o)
TARGET 	= greyd
CC		= clang
CFLAGS	=

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

tests:
	cd tests && $(MAKE)

.PHONY: clean test

clean:
	rm $(TARGET) $(OBJ) $(SRC:.c=.d)

test: tests
	cd tests && $(MAKE) test
