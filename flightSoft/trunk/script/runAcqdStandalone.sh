#!/bin/bash

if [ $# -lt 1 ]
then
    echo "Usage `basename $0` <runName> <acqd options>" 
    exit 0
fi


n=0
HK_DIR=/mnt/blade4/hkData/$1
EVENT_DIR=/mnt/blade4/acqdData/$1

while [ -d $HK_DIR ] ; do
  let "n = n + 1"
  HK_DIR=/mnt/blade4/hkData/$1_$n
  EVENT_DIR=/mnt/blade4/acqdData/$1_$n
done

mkdir ${HK_DIR}
rm /mnt/data/anita
ln -sf ${HK_DIR} /mnt/data/anita
echo "Using Hk directory: $HK_DIR" 
echo "Using Event directory: $EVENT_DIR" 
echo "Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}"
Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}
#echo 0 > /home/rjn/flightSoft/lastEventNumber
#killAllProgs.sh
#xterm -T firstAcqd -e "firstAcqd ; read" &
#xterm -T GPSd -e "GPSd ; read" &
#xterm -T Eventd -e "Eventd ; read" &
#xterm -T Prioritizerd -e "Prioritizerd ; read" &
#xterm -T Archived -e "Archived ; read" &


