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

.PHONY: clean

clean:
	rm $(TARGET) $(OBJ) $(SRC:.c=.d)

%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$

-include $(SRC:.c=.d)
