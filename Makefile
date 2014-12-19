#
# Master makefile for greyd.
#

progname = greyd
package = $(progname)
version = 0.1.0
tarname = $(package)
distdir = $(tarname)-$(version)
prefix = /usr/local
exec_prefix = $(prefix)
sbindir = $(exec_prefix)/sbin
libdir = $(exec_prefix)/lib
sysconfdir = $(prefix)/etc
export progname
export package
export prefix
export exec_prefix
export sbindir
export libdir
export sysconfdir

CC = clang
export CC

CFLAGS = -Wall -Wunused -Wstrict-prototypes
export CFLAGS

all clean:
	$(MAKE) -C src/ $@
	$(MAKE) -C tests/ $@

check test: all
	$(MAKE) -C tests/ $@

install:
	install -d $(DESTDIR)$(sysconfdir)/$(progname)
	install -m 0640 sample-greyd.conf $(DESTDIR)$(sysconfdir)/$(progname)/greyd.conf
	$(MAKE) -C src/ $@

dist: $(distdir).tar.gz

$(distdir).tar.gz: $(distdir)
	tar chof - $(distdir) |gzip -9c >$@
	rm -rf $(distdir)

$(distdir): FORCE
	mkdir -p $(distdir)/src
	mkdir -p $(distdir)/src/modules
	mkdir -p $(distdir)/tests
	mkdir -p $(distdir)/tests/data
	cp Makefile $(distdir)
	cp src/Makefile $(distdir)/src
	cp src/*.[ch] $(distdir)/src
	cp src/modules/Makefile $(distdir)/src/modules/
	cp src/modules/*.c $(distdir)/src/modules
	cp tests/Makefile $(distdir)/tests
	cp tests/*.[ch] $(distdir)/tests
	cp tests/run_tests.pl $(distdir)/tests
	cp tests/data/*.conf $(distdir)/tests/data
	cp tests/data/*.gz $(distdir)/tests/data

distcheck: $(distdir).tar.gz
	gunzip -c $(distdir).tar.gz |tar xvf -
	cd $(distdir) && $(MAKE) all
	cd $(distdir) && $(MAKE) check
	cd $(distdir) && $(MAKE) clean
	rm -rf $(distdir)
	@echo
	@echo "** Package $(distdir).tar.gz is good to go"

FORCE:
	-rm $(distdir).tar.gz >/dev/null 2>&1
	-rm $(distdir) >/dev/null 2>&1

.PHONY: FORCE all clean dist distcheck
