#!/bin/bash
# test bash shell
#
i=0
#
while [ $i -lt 10 ] 
 do 
  let "i=i+1"
 done
DTEST=test_$i
echo test directory $DTEST
#
