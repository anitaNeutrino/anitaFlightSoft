#!/bin/bash

n=1
THE_DIR=/mnt/blade4/slacData/theRuns/run$n

while [ -d $THE_DIR ] ; do
  let "n = n + 1"
  THE_DIR=/mnt/blade4/slacData/theRuns/run$n
done

mkdir ${THE_DIR}
sudo umount /mnt/data
sudo mount --bind ${THE_DIR} /mnt/data
makeAnitaDirs.sh
#rm /mnt/data/anita
#ln -sf ${THE_DIR} /mnt/data/anita
echo "Using directory: $THE_DIR" 
#eacho "Using Event directory: $EVENT_DIR" 
#echo "Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}"
#Acqd -w -d ${EVENT_DIR} $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14}
#echo 0 > /home/rjn/flightSoft/lastEventNumber
#killAllProgs.sh
#xterm -T firstAcqd -e "firstAcqd ; read" &
#xterm -T GPSd -e "GPSd ; read" &
#xterm -T Eventd -e "Eventd ; read" &
#xterm -T Prioritizerd -e "Prioritizerd ; read" &
#xterm -T Archived -e "Archived ; read" &


