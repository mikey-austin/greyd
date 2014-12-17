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
	cp src/blacklist.c $(distdir)/src
	cp src/blacklist.h $(distdir)/src
	cp src/con.c $(distdir)/src
	cp src/config.c $(distdir)/src
	cp src/config.h $(distdir)/src
	cp src/config_lexer.c $(distdir)/src
	cp src/config_lexer.h $(distdir)/src
	cp src/config_parser.c $(distdir)/src
	cp src/config_parser.h $(distdir)/src
	cp src/config_section.c $(distdir)/src
	cp src/config_section.h $(distdir)/src
	cp src/config_value.c $(distdir)/src
	cp src/config_value.h $(distdir)/src
	cp src/con.h $(distdir)/src
	cp src/constants.h $(distdir)/src
	cp src/failures.c $(distdir)/src
	cp src/failures.h $(distdir)/src
	cp src/firewall.c $(distdir)/src
	cp src/firewall.h $(distdir)/src
	cp src/grey.c $(distdir)/src
	cp src/greydb.c $(distdir)/src
	cp src/greydb.h $(distdir)/src
	cp src/greyd.c $(distdir)/src
	cp src/greyd.h $(distdir)/src
	cp src/grey.h $(distdir)/src
	cp src/hash.c $(distdir)/src
	cp src/hash.h $(distdir)/src
	cp src/ip.c $(distdir)/src
	cp src/ip.h $(distdir)/src
	cp src/lexer.c $(distdir)/src
	cp src/lexer.h $(distdir)/src
	cp src/lexer_source.c $(distdir)/src
	cp src/lexer_source.h $(distdir)/src
	cp src/list.c $(distdir)/src
	cp src/list.h $(distdir)/src
	cp src/log.c $(distdir)/src
	cp src/log.h $(distdir)/src
	cp src/main_greydb.c $(distdir)/src
	cp src/main_greyd.c $(distdir)/src
	cp src/main_greyd_setup.c $(distdir)/src
	cp src/main_greylogd.c $(distdir)/src
	cp src/mod.c $(distdir)/src
	cp src/mod.h $(distdir)/src
	cp src/queue.c $(distdir)/src
	cp src/queue.h $(distdir)/src
	cp src/spamd_lexer.c $(distdir)/src
	cp src/spamd_lexer.h $(distdir)/src
	cp src/spamd_parser.c $(distdir)/src
	cp src/spamd_parser.h $(distdir)/src
	cp src/sync.c $(distdir)/src
	cp src/sync.h $(distdir)/src
	cp src/utils.c $(distdir)/src
	cp src/utils.h $(distdir)/src
	cp src/modules/Makefile $(distdir)/src/modules/
	cp src/modules/bdb.c $(distdir)/src/modules
	cp src/modules/fw_dummy.c $(distdir)/src/modules
	cp src/modules/netfilter.c $(distdir)/src/modules
	cp src/modules/pf.c $(distdir)/src/modules
	cp tests/Makefile $(distdir)/tests
	cp tests/test.c $(distdir)/tests
	cp tests/test.h $(distdir)/tests
	cp tests/run_tests.pl $(distdir)/tests
	cp tests/test_bdb.c $(distdir)/tests
	cp tests/test_blacklist.c $(distdir)/tests
	cp tests/test_con.c $(distdir)/tests
	cp tests/test_config.c $(distdir)/tests
	cp tests/test_config_lexer.c $(distdir)/tests
	cp tests/test_config_parser.c $(distdir)/tests
	cp tests/test_config_section.c $(distdir)/tests
	cp tests/test_config_value.c $(distdir)/tests
	cp tests/test_grey.c $(distdir)/tests
	cp tests/test_greyd_utils.c $(distdir)/tests
	cp tests/test_hash.c $(distdir)/tests
	cp tests/test_ip.c $(distdir)/tests
	cp tests/test_lexer_source.c $(distdir)/tests
	cp tests/test_list.c $(distdir)/tests
	cp tests/test_queue.c $(distdir)/tests
	cp tests/test_spamd_lexer.c $(distdir)/tests
	cp tests/test_spamd_parser.c $(distdir)/tests
	cp tests/test_test_framework.c $(distdir)/tests
	cp tests/data/config_lexer_test1.conf $(distdir)/tests/data
	cp tests/data/config_test1.conf $(distdir)/tests/data
	cp tests/data/config_test2.conf $(distdir)/tests/data
	cp tests/data/config_test3.conf $(distdir)/tests/data
	cp tests/data/lexer_source_1.conf $(distdir)/tests/data
	cp tests/data/lexer_source_2.conf.gz $(distdir)/tests/data

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
