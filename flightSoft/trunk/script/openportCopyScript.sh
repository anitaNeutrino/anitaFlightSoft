#!/bin/bash

OPENPORT_USER=radio
OPENPORT_DEST_IP=192.168.1.2
OPENPORT_DEST_DIR=/storage/antarctica14/telem/openport/

echo "openportCopyPid: $$"
echo $$ > /tmp/openportCopyPid

while [ 1 ]; do
    if test `ls /tmp/openport | wc -l` -gt 0 ; then
	cd /tmp/openport
	COUNT_RUNS=`ls /tmp/openport | wc -l`
	echo "There are $COUNT_RUNS runs waiting"
	COUNT=0
	for rundir in [0-9]*; do
	    let COUNT=COUNT+1
	    echo $rundir
	    if test `ls /tmp/openport/$rundir | wc -l` -gt 0 ; then
		for file in $rundir/*; do
		    rsync -avP --remove-source-files --bwlimit=100  $file  ${OPENPORT_USER}@${OPENPORT_DEST_IP}:${OPENPORT_DEST_DIR}/$rundir/ > /tmp/openportCopy.log  
		done
	    fi
	    if [ $COUNT -lt $COUNT_RUNS ]; then
		echo "Could remove dir"
		rmdir $rundir
	    fi
	done
    else 
	sleep 10;
    fi
done
