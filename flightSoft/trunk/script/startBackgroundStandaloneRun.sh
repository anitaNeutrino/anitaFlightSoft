#!/bin/bash
echo "Killing Programs"
daemon --stop -n Archived
daemon --stop -n Hkd
daemon --stop -n GPSd
daemon --stop -n Monitord
daemon --stop -n Cmdd
daemon --stop -n Calibd
daemon --stop -n Acqd
daemon --stop -n Prioritizerd
daemon --stop -n Eventd

echo "Sleeping while files are written and zipped"
sleep 5
rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/prioritizerd

echo "Making new directories"
#CmdTest 4
#sleep 2
/home/anita/flightSoft/bin/startNewRun.sh
ls /mnt/satamini/current -l
mkdir /mnt/data/current/log
/home/anita/flightSoft/bin/simpleLog /mnt/data/current/log/simpleLog.txt
mkdir /mnt/satamini/current/log
cp /mnt/data/current/log/simpleLog.txt /mnt/satamini/current/log/simpleLog.txt
mkdir /mnt/data/current/config
mkdir /mnt/satamini/current/config
cp /home/anita/flightSoft/config/*.config /mnt/satamini/current/config
cp /home/anita/flightSoft/config/*.config /mnt/data/current/config

sleep 2
echo "Starting Programs"
daemon -r Cmdd -n Cmdd
daemon -r Archived -n Archived
daemon -r Hkd -n Hkd
daemon -r GPSd -n GPSd
daemon -r Monitord -n Monitord
daemon -r Calibd -n Calibd

#Just to make sure
daemon -r Prioritizerd -n Prioritizerd 
daemon -r Eventd -n Eventd
#daemon -r LOSd -n LOSd
daemon -r SIPd -n SIPd

sleep 2

daemon -r Acqd -n Acqd
