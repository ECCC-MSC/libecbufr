#!/bin/sh

export BUFR_TABLES=../Tables/

for i in BUFR/*.bufr
do
  ./test_mem $i
  if test $? -ne 0; then
      exit 1
  fi
done

exit 0
