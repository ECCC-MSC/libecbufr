## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

# automake <=1.9 doesn't have docdir
docdir=@datadir@/doc/@PACKAGE_NAME@
doc_DATA = AUTHORS ChangeLog COPYING INSTALL NEWS README TODO ChangeLog.bzr

EXTRA_DIST = reconf configure make_debpkg

SUBDIRS = po API Examples Utilities Tables Bindings Test Docs @build_dir_cgi@

distdir=@PACKAGE_NAME@-@PACKAGE_VERSION@

if HAVE_PKGCONFIG
# pkg-config file makes it a lot easier to build against libecbufr
pcdir=@prefix@/lib/pkgconfig
pc_DATA = libecbufr.pc

ChangeLog.bzr:
	if test -x /usr/bin/bzr; then \
		/usr/bin/bzr log --long > $@ ; \
	else \
		echo "/usr/bin/bzr not installed" > $@ ; \
	fi

libecbufr.pc: Makefile
	echo 'prefix=@prefix@' > $@
	echo 'exec_prefix=@exec_prefix@' > $@
	echo 'libdir=@libdir@/@PACKAGE@@PACKAGE_VERSION@' >> $@
	echo 'includedir=@includedir@/@PACKAGE@@PACKAGE_VERSION@' >> $@
	echo >> $@
	echo 'Name: @PACKAGE@' >> $@
	echo 'Version: @PACKAGE_VERSION@' >> $@
	echo 'Description: Environment Canada BUFR Library' >> $@
	echo 'URL: https://launchpad.net/libecbufr' >> $@
	echo 'Libs: -L$${libdir} -lecbufr' >> $@
	echo 'Libs.private: -lm' >> $@
	echo 'Cflags: -I$${includedir}' >> $@

endif

# library is installed in a "non-standard" location, and we
# want the linker search path to pick it up. For older distros
# we'd have to modify /etc/ld.so.conf directly. This is cleaner.
install-data-local:
	if test -d /etc/ld.so.conf.d/; then \
		$(mkinstalldirs) $(DESTDIR)/etc/ld.so.conf.d/ ; \
		echo '@libdir@/@PACKAGE@@PACKAGE_VERSION@' > $(DESTDIR)/etc/ld.so.conf.d/@PACKAGE@.conf ; \
	fi

pkg: make_debsrcpkg all
	./make_debsrcpkg @PACKAGE_NAME@ @PACKAGE_VERSION@
	cd debpackage && dpkg-source -x @PACKAGE_NAME@_@PACKAGE_VERSION@.dsc
	cd debpackage/@PACKAGE_NAME@-@PACKAGE_VERSION@ && \
		dpkg-buildpackage -rfakeroot -b -us -uc -tc
	mv debpackage/*.deb .

