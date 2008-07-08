#!/bin/bash


if [ -d /tmp/neobrick ];
then 
    echo "/tmp/neobrick exists"
else
    mkdir /tmp/neobrick
fi

cd /home/anita/flightSoft/testing

LOOP_COUNT=0;

while true ; do
    for file in `find /tmp/neobrick -name "*.da*"`; do
	echo $file
	./testNeobrick $file
    done     
    sleep 1
    let LOOP_COUNT=LOOP_COUNT+1
    echo "End of loop ${LOOP_COUNT}"
done