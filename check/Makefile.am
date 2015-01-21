AUTOMAKE_OPTIONS = subdir-objects

check_PROGRAMS = test_bdb test_blacklist test_con test_config test_config_lexer test_config_parser test_config_section test_config_value test_grey test_greyd_utils test_hash test_ip test_lexer_source test_list test_queue test_spamd_lexer test_spamd_parser test_test_framework test_utils
noinst_HEADERS = test.h

greyd_source = ../src/blacklist.c ../src/con.c ../src/config_lexer.c ../src/config_parser.c ../src/config_section.c ../src/config_value.c ../src/failures.c ../src/firewall.c ../src/grey.c ../src/greydb.c ../src/greyd.c ../src/greyd_config.c ../src/hash.c ../src/ip.c ../src/lexer.c ../src/lexer_source.c ../src/list.c ../src/log.c ../src/queue.c ../src/sync.c ../src/utils.c ../src/mod.c ../src/spamd_parser.c ../src/spamd_lexer.c

dist_data_DATA = data/lexer_source_2.conf.gz
data/lexer_source_2.conf.gz: data/lexer_source_1.conf
	gzip -c data/lexer_source_1.conf >data/lexer_source_2.conf.gz

CLEANFILES = data/lexer_source_2.conf.gz

test_ldflags = -Wl,-E
test_ldadd =  $(optional_ldadd)
test_cflags = -I$(top_srcdir)/../src

test_bdb_CFLAGS = $(test_cflags)
test_bdb_LDFLAGS = $(test_ldflags)
test_bdb_LDADD = $(test_ldadd)
test_bdb_SOURCES = test_bdb.c test.c $(greyd_source)

test_blacklist_LDFLAGS = $(test_ldflags)
test_blacklist_LDADD = $(test_ldadd)
test_blacklist_CFLAGS = $(test_cflags)
test_blacklist_SOURCES = test_blacklist.c test.c $(greyd_source)

test_con_LDFLAGS = $(test_ldflags)
test_con_LDADD = $(test_ldadd)
test_con_CFLAGS = $(test_cflags)
test_con_SOURCES = test_con.c test.c $(greyd_source)

test_config_LDFLAGS = $(test_ldflags)
test_config_LDADD = $(test_ldadd)
test_config_CFLAGS = $(test_cflags)
test_config_SOURCES = test_config.c test.c $(greyd_source)

test_config_lexer_LDFLAGS = $(test_ldflags)
test_config_lexer_LDADD = $(test_ldadd)
test_config_lexer_CFLAGS = $(test_cflags)
test_config_lexer_SOURCES = test_config_lexer.c test.c $(greyd_source)

test_config_parser_LDFLAGS = $(test_ldflags)
test_config_parser_LDADD = $(test_ldadd)
test_config_parser_CFLAGS = $(test_cflags)
test_config_parser_SOURCES = test_config_parser.c test.c $(greyd_source)

test_config_section_LDFLAGS = $(test_ldflags)
test_config_section_LDADD = $(test_ldadd)
test_config_section_CFLAGS = $(test_cflags)
test_config_section_SOURCES = test_config_section.c test.c $(greyd_source)

test_config_value_LDFLAGS = $(test_ldflags)
test_config_value_LDADD = $(test_ldadd)
test_config_value_CFLAGS = $(test_cflags)
test_config_value_SOURCES = test_config_value.c test.c $(greyd_source)

test_grey_LDFLAGS = $(test_ldflags)
test_grey_LDADD = -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_bdb.la
test_grey_CFLAGS = $(test_cflags)
test_grey_SOURCES = test_grey.c test.c $(greyd_source)

test_greyd_utils_LDFLAGS = $(test_ldflags)
test_greyd_utils_LDADD = $(test_ldadd)
test_greyd_utils_CFLAGS = $(test_cflags)
test_greyd_utils_SOURCES = test_greyd_utils.c test.c $(greyd_source)

test_hash_LDFLAGS = $(test_ldflags)
test_hash_LDADD = $(test_ldadd)
test_hash_CFLAGS = $(test_cflags)
test_hash_SOURCES = test_hash.c test.c $(greyd_source)

test_ip_LDFLAGS = $(test_ldflags)
test_ip_LDADD = $(test_ldadd)
test_ip_CFLAGS = $(test_cflags)
test_ip_SOURCES = test_ip.c test.c $(greyd_source)

test_lexer_source_LDFLAGS = $(test_ldflags)
test_lexer_source_LDADD = $(test_ldadd)
test_lexer_source_CFLAGS = $(test_cflags)
test_lexer_source_SOURCES = test_lexer_source.c test.c $(greyd_source)

test_list_LDFLAGS = $(test_ldflags)
test_list_LDADD = $(test_ldadd)
test_list_CFLAGS = $(test_cflags)
test_list_SOURCES = test_list.c test.c $(greyd_source)

test_queue_LDFLAGS = $(test_ldflags)
test_queue_LDADD = $(test_ldadd)
test_queue_CFLAGS = $(test_cflags)
test_queue_SOURCES = test_queue.c test.c $(greyd_source)

test_spamd_lexer_LDFLAGS = $(test_ldflags)
test_spamd_lexer_LDADD = $(test_ldadd)
test_spamd_lexer_CFLAGS = $(test_cflags)
test_spamd_lexer_SOURCES = test_spamd_lexer.c test.c $(greyd_source)

test_spamd_parser_LDFLAGS = $(test_ldflags)
test_spamd_parser_LDADD = $(test_ldadd)
test_spamd_parser_CFLAGS = $(test_cflags)
test_spamd_parser_SOURCES = test_spamd_parser.c test.c $(greyd_source)

test_test_framework_LDFLAGS = $(test_ldflags)
test_test_framework_LDADD = $(test_ldadd)
test_test_framework_CFLAGS = $(test_cflags)
test_test_framework_SOURCES = test_test_framework.c test.c $(greyd_source)

test_utils_LDFLAGS = $(test_ldflags)
test_utils_LDADD = $(test_ldadd)
test_utils_CFLAGS = $(test_cflags)
test_utils_SOURCES = test_utils.c test.c $(greyd_source)

LOG_DRIVER = $(top_srcdir)/tap-driver.pl

TESTS = test_bdb test_blacklist test_con test_config test_config_lexer test_config_parser test_config_section test_config_value test_grey test_greyd_utils test_hash test_ip test_lexer_source test_list test_queue test_spamd_lexer test_spamd_parser test_test_framework test_utils
