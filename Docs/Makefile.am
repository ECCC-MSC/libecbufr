DOXFILES = index.dox LibraryCodeStructure.jpg BUFR\ Data\ Structure.jpg
EXTRA_DIST = libecbufr-dox-en.conf libecbufr-dox-fr.conf libecbufr-dox.conf \
	README $(DOXFILES)

docdir=@datadir@/doc/@PACKAGE@

DOXOPTS = echo -e "PROJECT_NAME=@PACKAGE@\nPROJECT_NUMBER=@PACKAGE_VERSION@\n"

DOCDIRS = rtf-en rtf-fr html-en html-fr rtf html

all: $(DOCDIRS)

clean:
	rm -rf $(DOCDIRS) libecbufr-dox-fr.conf libecbufr-dox.conf

install-data-local: $(DOCDIRS)
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	cp -fR $(DOCDIRS) $(DESTDIR)$(docdir)

html-en: libecbufr-dox-en.conf $(DOXFILES)
	(cat $<; $(DOXOPTS)) | $(DOXYGEN) -

html-fr: libecbufr-dox-fr.conf $(DOXFILES)
	(cat $<; $(DOXOPTS)) | $(DOXYGEN) -

html: libecbufr-dox.conf $(DOXFILES)
	(cat $<; $(DOXOPTS)) | $(DOXYGEN) -

rtf-en: html-en
	(cat $<; $(DOXOPTS)) | $(DOXYGEN) -

rtf-fr: html-fr
	(cat $<; $(DOXOPTS)) | $(DOXYGEN) -

rtf: html

# rather than trying to keep two configs in sync, derive the French from
# the English.
libecbufr-dox-fr.conf: libecbufr-dox-en.conf
	sed -e 's/= English/= French/' -e 's/-en/-fr/' < $< > $@

# rather than trying to keep multiple configs in sync, derive from
# the English.
libecbufr-dox.conf: libecbufr-dox-en.conf
	sed -e 's/ENABLED_SECTIONS       = English/ENABLED_SECTIONS       = English French/' -e 's/-en//' < $< > $@
