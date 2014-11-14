#!/bin/bash

OPENPORT_USER=radio
OPENPORT_DEST_IP=192.168.1.2
OPENPORT_DEST_DIR=/anitaStorage/antarctica14/telem/openport


while [ 1 ]; do
    if test `ls /tmp/openport/* | wc -l` -gt 0 ; then
	
	for file in /tmp/openport/*; do
	    rsync -avP --remove-source-files  $file  ${OPENPORT_USER}@${OPENPORT_DEST_IP}:${OPENPORT_DEST_DIR}/ > /tmp/openportCopy.log  
	done
    else 
	sleep 10;
    fi
done
