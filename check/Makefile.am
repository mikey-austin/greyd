AUTOMAKE_OPTIONS = subdir-objects

check_PROGRAMS = test_blacklist.t test_con.t test_config.t test_config_lexer.t test_config_parser.t test_config_section.t test_config_value.t test_greyd_utils.t test_hash.t test_ip.t test_lexer_source.t test_list.t test_queue.t test_spamd_lexer.t test_spamd_parser.t test_test_framework.t test_utils.t test_trie.t benchmark_blacklist $(extra_test_programs)
TESTS = test_blacklist.t test_con.t test_config.t test_config_lexer.t test_config_parser.t test_config_section.t test_config_value.t test_greyd_utils.t test_hash.t test_ip.t test_lexer_source.t test_list.t test_queue.t test_spamd_lexer.t test_spamd_parser.t test_test_framework.t test_utils.t test_trie.t $(extra_tests)
EXTRA_PROGRAMS = test_grey.t test_grey_sqlite.t test_grey_bdb_sql.t test_db.t test_db_sqlite.t test_db_bdb_sql.t test_db_mysql.t test_grey_mysql.t test_db_postgresql.t test_grey_postgresql.t

TEST_EXTENSIONS = .t .sh
T_LOG_COMPILER = $(SH) ./test-wrapper
T_LOG_DRIVER = $(top_srcdir)/tap-driver.pl
SH_LOG_COMPILER = $(SH)
SH_LOG_DRIVER = $(top_srcdir)/tap-driver.pl
noinst_HEADERS = test.h

# Make a convenience library.
check_LTLIBRARIES = libgreyd_test.la
libgreyd_test_la_SOURCES = ../src/blacklist.c ../src/con.c ../src/config_lexer.c ../src/config_parser.c ../src/config_section.c ../src/config_value.c ../src/failures.c ../src/firewall.c ../src/grey.c ../src/greydb.c ../src/greyd.c ../src/greyd_config.c ../src/hash.c ../src/ip.c ../src/lexer.c ../src/lexer_source.c ../src/list.c ../src/log.c ../src/queue.c ../src/sync.c ../src/utils.c ../src/mod.c ../src/spamd_parser.c ../src/spamd_lexer.c ../src/trie.c

dist_data_DATA = data/lexer_source_2.conf.gz
data/lexer_source_2.conf.gz: data/lexer_source_1.conf
	gzip -c data/lexer_source_1.conf >data/lexer_source_2.conf.gz

CLEANFILES = data/lexer_source_2.conf.gz

test_ldflags = -Wl,-E
test_ldadd =  $(optional_ldadd) libgreyd_test.la
test_cflags = -I$(top_srcdir)/../src

test_blacklist_t_LDFLAGS = $(test_ldflags)
test_blacklist_t_LDADD = $(test_ldadd)
test_blacklist_t_CFLAGS = $(test_cflags)
test_blacklist_t_SOURCES = test_blacklist.c test.c

test_con_t_LDFLAGS = $(test_ldflags)
test_con_t_LDADD = $(test_ldadd)
test_con_t_CFLAGS = $(test_cflags)
test_con_t_SOURCES = test_con.c test.c

test_config_t_LDFLAGS = $(test_ldflags)
test_config_t_LDADD = $(test_ldadd)
test_config_t_CFLAGS = $(test_cflags)
test_config_t_SOURCES = test_config.c test.c

test_config_lexer_t_LDFLAGS = $(test_ldflags)
test_config_lexer_t_LDADD = $(test_ldadd)
test_config_lexer_t_CFLAGS = $(test_cflags)
test_config_lexer_t_SOURCES = test_config_lexer.c test.c

test_config_parser_t_LDFLAGS = $(test_ldflags)
test_config_parser_t_LDADD = $(test_ldadd)
test_config_parser_t_CFLAGS = $(test_cflags)
test_config_parser_t_SOURCES = test_config_parser.c test.c

test_config_section_t_LDFLAGS = $(test_ldflags)
test_config_section_t_LDADD = $(test_ldadd)
test_config_section_t_CFLAGS = $(test_cflags)
test_config_section_t_SOURCES = test_config_section.c test.c

test_config_value_t_LDFLAGS = $(test_ldflags)
test_config_value_t_LDADD = $(test_ldadd)
test_config_value_t_CFLAGS = $(test_cflags)
test_config_value_t_SOURCES = test_config_value.c test.c

test_greyd_utils_t_LDFLAGS = $(test_ldflags)
test_greyd_utils_t_LDADD = $(test_ldadd)
test_greyd_utils_t_CFLAGS = $(test_cflags)
test_greyd_utils_t_SOURCES = test_greyd_utils.c test.c

test_hash_t_LDFLAGS = $(test_ldflags)
test_hash_t_LDADD = $(test_ldadd)
test_hash_t_CFLAGS = $(test_cflags)
test_hash_t_SOURCES = test_hash.c test.c

test_ip_t_LDFLAGS = $(test_ldflags)
test_ip_t_LDADD = $(test_ldadd)
test_ip_t_CFLAGS = $(test_cflags)
test_ip_t_SOURCES = test_ip.c test.c

test_lexer_source_t_LDFLAGS = $(test_ldflags)
test_lexer_source_t_LDADD = $(test_ldadd)
test_lexer_source_t_CFLAGS = $(test_cflags)
test_lexer_source_t_SOURCES = test_lexer_source.c test.c

test_list_t_LDFLAGS = $(test_ldflags)
test_list_t_LDADD = $(test_ldadd)
test_list_t_CFLAGS = $(test_cflags)
test_list_t_SOURCES = test_list.c test.c

test_queue_t_LDFLAGS = $(test_ldflags)
test_queue_t_LDADD = $(test_ldadd)
test_queue_t_CFLAGS = $(test_cflags)
test_queue_t_SOURCES = test_queue.c test.c

test_spamd_lexer_t_LDFLAGS = $(test_ldflags)
test_spamd_lexer_t_LDADD = $(test_ldadd)
test_spamd_lexer_t_CFLAGS = $(test_cflags)
test_spamd_lexer_t_SOURCES = test_spamd_lexer.c test.c

test_spamd_parser_t_LDFLAGS = $(test_ldflags)
test_spamd_parser_t_LDADD = $(test_ldadd)
test_spamd_parser_t_CFLAGS = $(test_cflags)
test_spamd_parser_t_SOURCES = test_spamd_parser.c test.c

test_test_framework_t_LDFLAGS = $(test_ldflags)
test_test_framework_t_LDADD = $(test_ldadd)
test_test_framework_t_CFLAGS = $(test_cflags)
test_test_framework_t_SOURCES = test_test_framework.c test.c

test_utils_t_LDFLAGS = $(test_ldflags)
test_utils_t_LDADD = $(test_ldadd)
test_utils_t_CFLAGS = $(test_cflags)
test_utils_t_SOURCES = test_utils.c test.c

test_trie_t_LDFLAGS = $(test_ldflags)
test_trie_t_LDADD = $(test_ldadd)
test_trie_t_CFLAGS = $(test_cflags)
test_trie_t_SOURCES = test_trie.c test.c

test_db_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_bdb.so"'
test_db_t_LDFLAGS = $(test_ldflags)
test_db_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_bdb.la
test_db_t_SOURCES = test_db.c test.c

test_db_sqlite_t_LDFLAGS = $(test_ldflags)
test_db_sqlite_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_sqlite.la
test_db_sqlite_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_sqlite.so"'
test_db_sqlite_t_SOURCES = test_db.c test.c

test_db_bdb_sql_t_LDFLAGS = $(test_ldflags)
test_db_bdb_sql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_bdb_sql.la
test_db_bdb_sql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_bdb_sql.so"'
test_db_bdb_sql_t_SOURCES = test_db.c test.c

test_db_mysql_t_LDFLAGS = $(test_ldflags)
test_db_mysql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_mysql.la
test_db_mysql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_mysql.so"'
test_db_mysql_t_SOURCES = test_db.c test.c

test_db_postgresql_t_LDFLAGS = $(test_ldflags)
test_db_postgresql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_postgresql.la
test_db_postgresql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_postgresql.so"'
test_db_postgresql_t_SOURCES = test_db.c test.c

test_grey_t_LDFLAGS = $(test_ldflags)
test_grey_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_bdb.la
test_grey_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_bdb.so"'
test_grey_t_SOURCES = test_grey.c test.c

test_grey_sqlite_t_LDFLAGS = $(test_ldflags)
test_grey_sqlite_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_sqlite.la
test_grey_sqlite_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_sqlite.so"'
test_grey_sqlite_t_SOURCES = test_grey.c test.c

test_grey_bdb_sql_t_LDFLAGS = $(test_ldflags)
test_grey_bdb_sql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_bdb_sql.la
test_grey_bdb_sql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_bdb_sql.so"'
test_grey_bdb_sql_t_SOURCES = test_grey.c test.c

test_grey_mysql_t_LDFLAGS = $(test_ldflags)
test_grey_mysql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_mysql.la
test_grey_mysql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_mysql.so"'
test_grey_mysql_t_SOURCES = test_grey.c test.c

#test_grey_postregsql_t_LDFLAGS = $(test_ldflags)
#test_grey_postregsql_t_LDADD = libgreyd_test.la -dlopen ../drivers/greyd_fw_dummy.la -dlopen ../drivers/greyd_postgresql.la
#test_grey_postregsql_t_CFLAGS = $(test_cflags) -D'DB_DRIVER="greyd_postgresql.so"'
#test_grey_postregsql_t_SOURCES = test_grey.c test.c

benchmark_blacklist_LDFLAGS = $(test_ldflags)
benchmark_blacklist_LDADD = $(test_ldadd)
benchmark_blacklist_CFLAGS = $(test_cflags)
benchmark_blacklist_SOURCES = benchmark_blacklist.c
