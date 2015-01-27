AM_CPPFLAGS = -DPFCTL_PATH='"$(PFCTL)"'

EXTRA_LTLIBRARIES = greyd_bdb.la greyd_netfilter.la greyd_pf.la greyd_npf.la
pkglib_LTLIBRARIES = $(optional_drivers)
noinst_LTLIBRARIES = greyd_fw_dummy.la

greyd_fw_dummy_la_SOURCES = fw_dummy.c
greyd_fw_dummy_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_bdb_la_SOURCES = bdb.c
greyd_bdb_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_netfilter_la_SOURCES = netfilter.c
greyd_netfilter_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_pf_la_SOURCES = pf.c
greyd_pf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_npf_la_SOURCES = npf.c
greyd_npf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'
