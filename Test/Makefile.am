SUBDIRS = BUFR Dump Unit_Tests po

EXTRA_DIST = README

check_SCRIPTS = test_bufr_decode.sh test_bufr_reencode.sh \
	test_bufr_encode.sh test_mem.sh
check_PROGRAMS = test_mem test_tables test_section2 test_string_compression \
	test_qualifier_rtmd test_find test_find_quals test_zero

TESTS = $(check_SCRIPTS) $(check_PROGRAMS)

LDADD = @LTLIBINTL@ -L../API/Sources -lecbufr -lm

localedir = @datadir@/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = -g
CLEANFILES=*.gcda *.gcno
