sysconf_DATA = greyd.conf
dist_data_DATA = greyd.conf.in

greyd.conf: greyd.conf.in
	$(SED) -e 's,[@]PACKAGE_NAME[@],$(progname),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]localstatedir[@],$(localstatedir),g' \
		-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		< $< >$@

CLEANFILES = greyd.conf
