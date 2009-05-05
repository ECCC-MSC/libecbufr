## Process this file with automake to produce Makefile.in

# automake <=1.9 doesn't have docdir
docdir=@datadir@/doc/@PACKAGE_NAME@
doc_DATA = AUTHORS ChangeLog COPYING INSTALL NEWS README TODO

EXTRA_DIST = reconf configure make_debpkg

SUBDIRS = API Examples Utilities Tables Test Docs

distdir=@PACKAGE_NAME@-@PACKAGE_VERSION@

pkg: all
	./make_debpkg @PACKAGE_NAME@ @PACKAGE_VERSION@
