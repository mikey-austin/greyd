#
# Makefile for greyd.
#

SRC		= $(wildcard *.c)
MAINS   = main_greyd.o main_greyd_setup.o main_greydb.o main_greylogd.o
OBJ		= $(SRC:.c=.o)
LIBS    = -lz -ldl
TESTS   = tests

all: greyd greyd-setup greydb greylogd modules

greyd-setup: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greyd-setup main_greyd_setup.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greydb: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greydb main_greydb.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greyd: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greyd main_greyd.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greylogd: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greylogd main_greylogd.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

#
# Generate the object file header dependencies.
#
%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$

-include $(SRC:.c=.d)

modules: $(OBJ)
	cd modules && $(MAKE)

.PHONY: clean

clean:
	rm -f $(OBJ) $(MAINS) $(SRC:.c=.d) $(SRC:.c=.d).* greyd-setup greydb greyd greylogd
	$(MAKE) -C modules/ clean
