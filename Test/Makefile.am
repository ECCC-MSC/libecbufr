SUBDIRS = BUFR Dump Unit_Tests po

EXTRA_DIST = README

check_SCRIPTS = test_bufr_decode.sh test_bufr_reencode.sh \
	test_bufr_encode.sh test_mem.sh
check_PROGRAMS = test_mem test_tables test_section2 test_string_compression

TESTS = $(check_SCRIPTS) $(check_PROGRAMS)

test_mem_SOURCES = test_mem.c
test_tables_SOURCES = test_tables.c
test_section2_SOURCES = test_section2.c
test_string_compression_SOURCES = test_string_compression.c
LDADD = @LTLIBINTL@ -L../API/Sources -lecbufr -lm

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = -g
CLEANFILES=*.gcda *.gcno
