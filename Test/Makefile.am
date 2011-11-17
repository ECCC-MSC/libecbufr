SUBDIRS = BUFR Dump Unit_Tests po

EXTRA_DIST = README

check_SCRIPTS = test_bufr_decode.sh test_bufr_reencode.sh \
	test_bufr_encode.sh test_mem.sh
check_PROGRAMS = test_mem test_tables test_section2 test_string_compression \
	test_qualifier_rtmd test_find

TESTS = $(check_SCRIPTS) $(check_PROGRAMS)

test_mem_SOURCES = test_mem.c
test_tables_SOURCES = test_tables.c
test_section2_SOURCES = test_section2.c
test_string_compression_SOURCES = test_string_compression.c
test_qualifier_rtmd_SOURCES = test_qualifier_rtmd.c
test_find_SOURCES = test_find.c
LDADD = @LTLIBINTL@ -L../API/Sources -lecbufr -lm

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = -g
CLEANFILES=*.gcda *.gcno
