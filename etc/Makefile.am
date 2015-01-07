confdir = $(sysconfdir)/$(PACKAGE)/
conf_DATA = greyd.conf
dist_data_DATA = greyd.conf.in

greyd.conf: greyd.conf.in
	$(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]localstatedir[@],$(localstatedir),g' \
		-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		<$(srcdir)/greyd.conf.in >greyd.conf

CLEANFILES = greyd.conf
