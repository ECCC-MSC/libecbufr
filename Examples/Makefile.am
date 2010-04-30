# automake <=1.9 doesn't have docdir or we'd use that
exampledir = @datadir@/doc/${PACKAGE}/examples/
example_DATA=$(SOURCES) decode_sqlite.c
example_PROGRAMS = \
	bufr_merge decode_multi_dataset decode_multi_message decode_net \
	decode_random decode_simple dump_template encode_bogus encode_dumpfile \
	decode_cmc_table encode_freeform_tmpl encode_freeform_seq

EXTRA_DIST = $(SOURCES)
INCLUDES = -I../API/Headers -I/usr/include/@PACKAGE@@VERSION_MAJOR@
LDADD = -L../API/Sources -L/usr/lib/@PACKAGE@@VERSION_MAJOR@ -lecbufr -lm

# decode_sqlite_LDADD = -L../API/Sources -L/usr/lib/@PACKAGE@@VERSION_MAJOR@ \
#	-lecbufr -lsqlite3 -lm
