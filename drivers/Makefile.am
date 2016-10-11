AM_CPPFLAGS = -DPFCTL_PATH='"$(PFCTL)"'

EXTRA_LTLIBRARIES = greyd_bdb.la greyd_sqlite.la greyd_bdb_sql.la greyd_netfilter.la greyd_pf.la greyd_npf.la greyd_mysql.la greyd_postgresql.la
pkglib_LTLIBRARIES = $(optional_drivers)
noinst_LTLIBRARIES = greyd_fw_dummy.la

greyd_fw_dummy_la_SOURCES = fw_dummy.c
greyd_fw_dummy_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_bdb_la_SOURCES = bdb.c
greyd_bdb_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_sqlite_la_SOURCES = sqlite.c
greyd_sqlite_la_LIBADD  = -lsqlite3
greyd_sqlite_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_bdb_sql_la_SOURCES = sqlite.c
greyd_bdb_sql_la_CFLAGS  = -DBUILD_DB_SQL
greyd_bdb_sql_la_LIBADD  = -ldb_sql
greyd_bdb_sql_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_mysql_la_SOURCES = mysql.c
greyd_mysql_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_postgresql_la_SOURCES = postgresql.c
greyd_postgresql_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_netfilter_la_SOURCES = netfilter.c
greyd_netfilter_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_pf_la_SOURCES = pf.c
greyd_pf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_npf_la_SOURCES = npf.c
greyd_npf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'
