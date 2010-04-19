SUBDIRS = BUFR Dump Unit_Tests

EXTRA_DIST = README

check_SCRIPTS = test_bufr_decode.sh test_bufr_reencode.sh \
	test_bufr_encode.sh test_mem.sh
check_PROGRAMS = test_mem test_tables

TESTS = $(check_SCRIPTS) $(check_PROGRAMS)

test_mem_SOURCES = test_mem.c
test_tables_SOURCES = test_tables.c
LDADD = -L../API/Sources -lecbufr -lm
AM_CFLAGS = -g
CLEANFILES=*.gcda *.gcno