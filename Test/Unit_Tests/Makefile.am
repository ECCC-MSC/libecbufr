if CHECK_ACTIVATED
check_PROGRAMS = check_libecbufr

TESTS = $(check_PROGRAMS)

check_libecbufr_SOURCES = check_libecbufr.c check_bufr_util.c check_bufr_io.c check_bufr_array.c check_libecburf.h
AM_CFLAGS = -g -I${PWD} -std=c99
LDADD = -L../../API/Sources -lecbufr -lm -lcheck
endif

CLEANFILES=*.gcda *.gcno
