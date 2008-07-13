#!/bin/bash


if [ -d /tmp/neobrick ];
then 
    echo "/tmp/neobrick exists"
else
    mkdir /tmp/neobrick
fi

cd /home/anita/flightSoft/testing

LOOP_COUNT=0;

if [ -d /tmp/anita ];
then 
    echo "/tmp/anita exists"
else
    mkdir /tmp/anita
fi

if [ -d /tmp/anita/neobrickd ];
then 
    echo "/tmp/anita/neobrickd exists"
else
    mkdir /tmp/anita/neobrickd
fi

if [ -d /tmp/anita/neobrickd/link ];
then 
    echo "/tmp/anita/neobrickd/link exists"
else
    mkdir /tmp/anita/neobrickd/link
fi

while true ; do
    for file in `find /tmp/neobrick -name "*.da*"`; do
	echo $file
	ln -sf $file /tmp/anita/neobrickd/link
    done     
    sleep 1
    let LOOP_COUNT=LOOP_COUNT+1
    echo "End of loop ${LOOP_COUNT}"
done