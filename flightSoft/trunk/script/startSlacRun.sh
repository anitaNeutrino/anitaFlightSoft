#!/bin/bash
source ~/flightSoft/script/anitaFlightSoftSetup.sh
n=1
THE_DIR=/mnt/blade4/slacData/theRuns/run$n

while [ -d $THE_DIR ] ; do
  let "n = n + 1"
  THE_DIR=/mnt/blade4/slacData/theRuns/run$n
done
echo "Killing old progs"
CmdTest 131 246 1

mkdir ${THE_DIR}
sudo umount /mnt/data >> /dev/null 2>&1
sudo mount --bind ${THE_DIR} /mnt/data

mkdir ${THE_DIR}/config
cp -r ~/flightSoft/config ${THE_DIR}/config

echo "Using directory: $THE_DIR" 

echo "Starting new progs"
CmdTest 133 246 1

sleep 2

echo "Checking"
ps x | grep "Archived{1}"
qps x | grep "Cmdd"
ps x | grep "Eventd"
ps x | grep "GPSd"
ps x | grep "Hkd"
ps x | grep "LOSd"
ps x | grep "Prioritizerd"
ps x | grep "SIPd"







#rm /mnt/data/anita
#ln -sf ${THE_DIR} /mnt/data/anita
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


