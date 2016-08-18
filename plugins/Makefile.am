EXTRA_LTLIBRARIES = greyd_spf.la greyd_guile.la
pkglib_LTLIBRARIES = $(optional_plugins)
noinst_LTLIBRARIES = greyd_plugin_dummy.la

greyd_plugin_dummy_la_SOURCES = plugin_dummy.c
greyd_plugin_dummy_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_spf_la_SOURCES = spf.c
greyd_spf_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'

greyd_guile_la_SOURCES = guile.c
greyd_guile_la_LDFLAGS = -module -avoid-version -rpath '$(libdir)'
