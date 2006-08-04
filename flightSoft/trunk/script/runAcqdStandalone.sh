#!/bin/bash

if [ $# -lt 1 ]
then
    echo "Usage `basename $0` <runName> <acqd options>" 
    exit 0
fi


DATA_DIR=/mnt/blade3/hawaiiData

n=0
HK_DIR=${DATA_DIR}/hkData/$1
EVENT_DIR=${DATA_DIR}/acqdData/$1

while [ -d $HK_DIR ] ; do
  let "n = n + 1"
  HK_DIR=${DATA_DIR}/hkData/$1_$n
  EVENT_DIR=${DATA_DIR}/acqdData/$1_$n
done

mkdir ${HK_DIR}


echo "Using Hk directory: $HK_DIR" 
echo "Using Event directory: $EVENT_DIR" 
echo "Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}"
Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}



