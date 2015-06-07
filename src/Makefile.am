AM_CPPFLAGS = -DDEFAULT_CONFIG='"$(DEFAULT_CONFIG)"' -DGREYD_PIDFILE='"$(GREYD_PIDFILE)"' -DGREYLOGD_PIDFILE='"$(GREYLOGD_PIDFILE)"'

sbin_PROGRAMS = greyd greylogd greydb greyd-setup
noinst_HEADERS = blacklist.h con.h config_lexer.h config_parser.h config_section.h config_value.h failures.h firewall.h grey.h greydb.h greyd.h greyd_config.h hash.h ip.h lexer.h lexer_source.h list.h log.h queue.h sync.h utils.h mod.h spamd_parser.h spamd_lexer.h constants.h trie.h sha1.h

greyd_LDFLAGS = -Wl,-E
greyd_LDADD = $(optional_ldadd)
greyd_SOURCES = main_greyd.c blacklist.c con.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c grey.c greydb.c greyd.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c trie.c sha1.c

greylogd_LDFLAGS = -Wl,-E
greylogd_LDADD = $(optional_ldadd)
greylogd_SOURCES = main_greylogd.c blacklist.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c greydb.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c trie.c sha1.c

greydb_LDFLAGS = -Wl,-E
greydb_LDADD = $(optional_ldadd)
greydb_SOURCES = main_greydb.c blacklist.c config_lexer.c config_parser.c config_section.c config_value.c failures.c greydb.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c trie.c sha1.c

greyd_setup_LDFLAGS = -Wl,-E
greyd_setup_LDADD = $(optional_ldadd)
greyd_setup_SOURCES = main_greyd_setup.c blacklist.c con.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c greydb.c greyd.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c utils.c spamd_lexer.c spamd_parser.c mod.c trie.c
