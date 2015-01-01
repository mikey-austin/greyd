bin_PROGRAMS = greyd greylogd greydb greyd-setup

greyd_LDFLAGS = -Wl,-E
greyd_LDADD = $(optional_ldadd)
greyd_SOURCES = main_greyd.c blacklist.c con.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c grey.c greydb.c greyd.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c

greylogd_LDFLAGS = -Wl,-E
greylogd_LDADD = $(optional_ldadd)
greylogd_SOURCES = main_greylogd.c blacklist.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c greydb.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c

greydb_LDFLAGS = -Wl,-E
greydb_LDADD = $(optional_ldadd)
greydb_SOURCES = main_greydb.c blacklist.c config_lexer.c config_parser.c config_section.c config_value.c failures.c greydb.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c sync.c utils.c mod.c

greyd_setup_LDFLAGS = -Wl,-E
greyd_setup_LDADD = $(optional_ldadd)
greyd_setup_SOURCES = main_greyd_setup.c blacklist.c con.c config_lexer.c config_parser.c config_section.c config_value.c failures.c firewall.c greydb.c greyd.c greyd_config.c hash.c ip.c lexer.c lexer_source.c list.c log.c queue.c utils.c spamd_lexer.c spamd_parser.c mod.c
