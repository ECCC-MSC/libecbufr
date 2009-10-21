#!/bin/sh

DECODER=../Utilities/bufr_decoder
export BUFR_TABLES=../Tables/

test1file ()
{
	if [ -f ${OUT} ]; then
		if /usr/bin/diff -q ${OUT} ${TMPOUT} >/dev/null; then
			echo " (passed)"
			rm ${TMPOUT}
		else
			echo " (failed) ${TMPOUT} differs from ${OUT}"
			if [ $TEST_VERBOSE ]; then
				/usr/bin/diff -a ${OUT} ${TMPOUT}
			fi
			if [ ! $TEST_BATCH ]; then
				exit 1
			fi
		fi
	else
		echo "No previous ${OUT} to compare against"
		echo "Renaming ${TMPOUT} to ${OUT}"
		mv ${TMPOUT} ${OUT}
	fi
}

for i in BUFR/*.bufr
do
	BASE=`basename $i .bufr`
	echo -n "$i ..."
	OUT=BUFR/${BASE}.out
	TMPOUT=BUFR/${BASE}.out.tmp
	${DECODER} -inbufr $i -ltableb local_table_b -no_format > ${TMPOUT} 2>&1
   test1file
done

# redo the tests when not using local table b
# Don't think we need to test every file. will test this one only
i=BUFR/is_winide_BLDU.bufr
BASE=`basename $i .bufr`
OUT=BUFR/${BASE}.no_ltb.out
TMPOUT=BUFR/${BASE}.out.tmp
${DECODER} -inbufr $i -no_format > ${TMPOUT} 2>&1
test1file

exit 0
