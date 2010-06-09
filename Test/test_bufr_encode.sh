#!/bin/sh

# encode specific template/dump combinations and check them against the
# BUFR examples

ENCODER=../Utilities/bufr_encoder
export BUFR_TABLES=../Tables/
unset AFSISIO

for i in Dump/*.template
do
	TMPL=$i
	BASE=`basename $i .template`
	DUMP="Dump/${BASE}.dump"

	# [ -f ${DUMP} ] || continue

	echo -n "$TMPL ..."

	BUFR=Dump/${BASE}.bufr
	TMPBUFR=Dump/${BASE}.bufr.tmp

	${ENCODER} -nolocal -template ${TMPL} -datafile ${DUMP} -ltableb ./local_table_b \
				-ltabled local_table_d -outbufr ${TMPBUFR}
	if [ -f ${BUFR} ]; then
		if /usr/bin/diff -qb ${BUFR} ${TMPBUFR} >/dev/null; then
			echo " (passed)"
			rm ${TMPBUFR}
		else
			echo " (failed) ${TMPBUFR} differs from ${BUFR}"
			if [ ! $TEST_BATCH ]; then
				exit 1
			fi
		fi
	else
		echo "No previous ${BUFR} to compare against"
		echo "Renaming ${TMPBUFR} to ${BUFR}"
		mv ${TMPBUFR} ${BUFR}
	fi
done

exit 0
