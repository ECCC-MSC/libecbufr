if CHECK_ACTIVATED
check_PROGRAMS = check_libecbufr

TESTS = $(check_PROGRAMS)

check_libecbufr_SOURCES = check_libecbufr.c check_str_util.c check_bufr_io.c check_array.c check_libecburf.h
AM_CFLAGS = -g -I${PWD}
LDADD = -L../../API/Sources -lecbufr -lm -lcheck
endif
