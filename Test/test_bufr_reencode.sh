#!/bin/sh

# Decode BUFR samples to dump/template files, then re-encode, then
# decode. This doesn't check the output against older versions which
# makes it more of a smoke test. Smoke tests are good, too.

DECODER=../Utilities/bufr_decoder
ENCODER=../Utilities/bufr_encoder
export BUFR_TABLES=../Tables/

for i in BUFR/*.bufr
do
	BASE=`basename $i .bufr`
	echo -n "$i ..."

	TMPDUMP="${TMPDIR-/tmp}/${BASE}-tmp.dump"
	TMPTMPL="${TMPDIR-/tmp}/${BASE}-tmp.template"
	TMPBUFR="${TMPDIR-/tmp}/${BASE}-tmp.bufr"
	TMPOUT="${TMPDIR-/tmp}/${BASE}-tmp.out"

	# decode into the dump and template. If the decode fails, don't
	# worry. We're assuming that the sample was already run past the
	# decoder test.
	${DECODER} -inbufr $i -dump -output ${TMPDUMP} -otemplate ${TMPTMPL} \
		>/dev/null 2>&1
	if [ -s ${TMPDUMP} ] && [ -s ${TMPTMPL} ]; then
		echo "decoded ... "
		${ENCODER} -template ${TMPTMPL} -outbufr ${TMPBUFR} -nolocal \
			-datafile ${TMPDUMP} >/dev/null 2>&1
		if [ -s ${TMPBUFR} ]; then
			echo -n "       encoded ${TMPBUFR} ... "
			rm ${TMPTMPL} ${TMPDUMP}

			${DECODER} -inbufr $i > ${TMPOUT} 2>&1
			if [ -s ${TMPOUT} ]; then
				echo "decoded"
				rm ${TMPBUFR} ${TMPOUT}
			else
				echo 'decode failed!'
				exit 1
			fi
		else
			echo 'encode failed!'
			exit 1
		fi
	else
		echo "decode failed"
	fi
done

exit 0
