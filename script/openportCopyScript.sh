#!/bin/bash

#TDRSS1 via openport
OPENPORT_USER=anita
OPENPORT_DEST_IP=67.239.76.175
OPENPORT_DEST_DIR=/data/anita/openport/

#TDRSS1 via CSBF network
#OPENPORT_USER=anita
#OPENPORT_DEST_IP=192.168.20.225
#OPENPORT_DEST_DIR=/data/anita/openport

echo "openportCopyPid: $$"
echo $$ > /tmp/openportCopyPid


while [ 1 ]; do
    shouldBreak=0
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
#		    if test $? -gt 128; then
#			shouldBreak=1; 
#			break;
#		    fi		    
		done
	    fi
#	    if [ "$shouldBreak" -eq 1 ]; then
#		break;
#	    fi
	    if [ $COUNT -lt $COUNT_RUNS ]; then
		echo "Could remove dir"
		rmdir $rundir
	    fi
	done
    else 
	sleep 10;
#	test $? -gt 128 && break;
    fi

#    if [ "$shouldBreak" -eq 1 ]; then
#	break;
#    fi
done
