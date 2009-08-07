#!/bin/sh

DECODER=../Utilities/bufr_decoder
export BUFR_TABLES=../Tables/

for i in BUFR/*.bufr
do
	BASE=`basename $i .bufr`
	echo -n "$i ..."
	OUT=BUFR/${BASE}.out
	TMPOUT=BUFR/${BASE}.out.tmp
	${DECODER} -inbufr $i -ltableb local_table_b > ${TMPOUT} 2>&1
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
done

exit 0
