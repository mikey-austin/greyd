EXTRA_LTLIBRARIES = greyd_spf.la
pkglib_LTLIBRARIES = $(optional_plugins)
noinst_LTLIBRARIES = greyd_plugin_dummy.la

greyd_plugin_dummy_la_SOURCES = plugin_dummy.c
greyd_plugin_dummy_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_spf_la_SOURCES = spf.c
greyd_spf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'
