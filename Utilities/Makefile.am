bin_PROGRAMS = \
	bufr_encoder bufr_decoder bufr_filter bufr_bundle

exampledir = @docdir@/examples/
example_DATA=$(SOURCES)

EXTRA_DIST = bufr_encoder bufr_decoder bufr_filter bufr_bundle

SUBDIRS = po

bufr_encoder_SOURCES = \
	bufr_encoder.c

bufr_decoder_SOURCES = \
	bufr_decoder.c

bufr_filter_SOURCES = \
	bufr_filter.c

bufr_bundle_SOURCES = \
	bufr_bundle.c

INCLUDES = -I../API/Headers

LDADD = @LTLIBINTL@ -L../API/Sources -lecbufr -lm

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

CLEANFILES = $(bin_PROGRAMS) *.gcda *.gcno
