SUBDIRS = drivers src check doc etc
AM_DISTCHECK_CONFIGURE_FLAGS = --disable-shared --with-bdb
ACLOCAL_AMFLAGS = -I m4

#
# Must have the following in your ~/.rpmmacros:
#  %_topdir /your/home/dir/rpmbuild
#
rpm: dist
	$(eval RPM_TOPDIR := $(shell rpm --eval '%{_topdir}'))
	cp $(srcdir)/packages/rpm/greyd.spec $(RPM_TOPDIR)/SPECS
	cp $(PACKAGE)-$(VERSION).tar.gz $(RPM_TOPDIR)/SOURCES/
	rpmbuild -ba -v --clean --rmsource $(RPM_TOPDIR)/SPECS/$(PACKAGE).spec
	mv $(RPM_TOPDIR)/RPMS/x86_64/$(PACKAGE)-*.rpm packages/rpm
	mv $(RPM_TOPDIR)/SRPMS/$(PACKAGE)-*.src.rpm packages/rpm
