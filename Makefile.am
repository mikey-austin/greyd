SUBDIRS = drivers src check doc etc
AM_DISTCHECK_CONFIGURE_FLAGS = --disable-shared --with-bdb
ACLOCAL_AMFLAGS = -I m4

#
# Must have the following in your ~/.rpmmacros:
#  %_topdir %{echo $HOME}/rpmbuild
#
rpm: dist
	RPM_TOPDIR=`rpm --showrc |perl -n -e 'print if(s/.*_topdir\s+(.*)/$$1/)' |perl -lpe 's/%{[^}]+HOME}/$$ENV{HOME}/g'`  ; \
	cp $(srcdir)/packages/rpm/greyd.spec $$RPM_TOPDIR/SPECS ; \
	cp $(PACKAGE)-$(VERSION).tar.gz $$RPM_TOPDIR/SOURCES/ ; \
	rpmbuild -ba --clean --rmsource $$RPM_TOPDIR/SPECS/$(PACKAGE).spec ; \
	mv $$RPM_TOPDIR/RPMS/x86_64/$(PACKAGE)-*.rpm packages/rpm ; \
	mv $$RPM_TOPDIR/SRPMS/$(PACKAGE)-*.src.rpm packages/rpm
