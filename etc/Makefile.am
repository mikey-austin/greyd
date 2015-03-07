confdir = $(sysconfdir)/$(PACKAGE)/
conf_DATA = greyd.conf greyd.redhat-init greylogd.redhat-init
dist_data_DATA = greyd.conf.in greyd.redhat-init.in greylogd.redhat-init.in

greyd.conf: greyd.conf.in
	$(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
		-e 's,[@]GREYD_PIDFILE[@],$(GREYD_PIDFILE),g' \
		-e 's,[@]GREYLOGD_PIDFILE[@],$(GREYLOGD_PIDFILE),g' \
		-e 's,[@]DEFAULT_CONFIG[@],$(DEFAULT_CONFIG),g' \
		-e 's,[@]CURL[@],$(CURL),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]sbindir[@],$(sbindir),g' \
		-e 's,[@]localstatedir[@],$(localstatedir),g' \
		-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		<$(srcdir)/greyd.conf.in >greyd.conf

greyd.redhat-init: greyd.redhat-init.in
	$(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
		-e 's,[@]GREYD_PIDFILE[@],$(GREYD_PIDFILE),g' \
		-e 's,[@]GREYLOGD_PIDFILE[@],$(GREYLOGD_PIDFILE),g' \
		-e 's,[@]DEFAULT_CONFIG[@],$(DEFAULT_CONFIG),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]sbindir[@],$(sbindir),g' \
		-e 's,[@]localstatedir[@],$(localstatedir),g' \
		-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		<$(srcdir)/greyd.redhat-init.in >greyd.redhat-init

greylogd.redhat-init: greylogd.redhat-init.in
	$(SED) -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
		-e 's,[@]GREYD_PIDFILE[@],$(GREYD_PIDFILE),g' \
		-e 's,[@]GREYLOGD_PIDFILE[@],$(GREYLOGD_PIDFILE),g' \
		-e 's,[@]DEFAULT_CONFIG[@],$(DEFAULT_CONFIG),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]sbindir[@],$(sbindir),g' \
		-e 's,[@]localstatedir[@],$(localstatedir),g' \
		-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
		<$(srcdir)/greylogd.redhat-init.in >greylogd.redhat-init

CLEANFILES = greyd.conf greyd.redhat-init greylogd.redhat-init
