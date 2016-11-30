#!/bin/bash
if [ "$1" = "" ]
then
   echo "usage: `basename $0` <run no> <which helium" 1>&2
   exit 1
fi

if [ "$2" = "" ]
then
   echo "usage: `basename $0` <run no> <which helium>" 1>&2
   exit 1
fi



RUN=$1
HELIUM=$2
RAW_RUN_DIR=/mnt/helium${HELIUM}/run${RUN}
HEAD_FILE_LIST=/tmp/headList${RUN}.txt
find ${RAW_RUN_DIR}/event -name "hd*.gz" | sort -n > ${HEAD_FILE_LIST}

