#
# Makefile for greyd unit tests.
#

SRC			= test.c
TESTS		= $(wildcard test_*.c)
OBJ			= $(SRC:.c=.o) $(TESTS:.c=.o)
TARGET		= $(TESTS:.c=.t)
WRAPPERS	= $(TESTS:.c=.pl)
CC			= clang
CFLAGS		= 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(WRAPPERS): $(TARGET)
	echo "system(\"$(shell pwd)/$(TARGET)\")" >$(WRAPPERS)

.PHONY: clean test

clean:
	rm -f $(TARGET) $(OBJ) $(WRAPPERS)

test: $(WRAPPERS)
	perl -MTest::Harness -e 'runtests(@ARGV)' $(WRAPPERS)
