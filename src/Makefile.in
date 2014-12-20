#
# Makefile for greyd.
#

SRC = $(wildcard *.c)
MAINS_SRC = $(wildcard main_*.c)
MAINS = $(MAINS_SRC:.c=.o)
OBJ = $(SRC:.c=.o)
LIBS = -lz -ldl -lcrypto
TESTS = tests

all: greyd greyd-setup greydb greylogd modules

greyd-setup: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greyd-setup main_greyd_setup.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greydb: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greydb main_greydb.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greyd: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greyd main_greyd.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

greylogd: $(OBJ)
	$(CC) $(CFLAGS) -Wl,-E -o greylogd main_greylogd.o $(filter-out $(MAINS),$(OBJ)) $(LIBS)

modules:
	$(MAKE) -C modules

install:
	install -d $(DESTDIR)$(sbindir)
	install -m 0750 greyd $(DESTDIR)$(sbindir)
	install -m 0750 greylogd $(DESTDIR)$(sbindir)
	install -m 0750 greydb $(DESTDIR)$(sbindir)
	install -m 0750 greyd-setup $(DESTDIR)$(sbindir)
	$(MAKE) -C modules/ $@

#
# Generate the object file header dependencies.
#
%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$

-include $(SRC:.c=.d)

.PHONY: clean modules install

clean:
	rm -f $(OBJ) $(MAINS) $(SRC:.c=.d) $(SRC:.c=.d).* greyd-setup greydb greyd greylogd
	$(MAKE) -C modules $@
