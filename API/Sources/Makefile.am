## Process this file with automake to produce Makefile.in

libecbufrdir=@libdir@/@PACKAGE@@VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_MICRO@

libecbufr_LTLIBRARIES = libecbufr.la

libecbufr_la_SOURCES=\
		bufr_array.c bufr_util.c bufr_linklist.c \
		bufr_af.c bufr_meta.c bufr_value.c bufr_desc.c \
		bufr_sequence.c bufr_tables.c bufr_io.c bufr_sio.c \
		bufr_dataset.c bufr_ddo.c bufr_template.c \
		bufr_message.c bufr_ieee754.c cmc_tables.c bufr_api.c bufr_local.c

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

CLEANFILES=*.gcda *.gcno
