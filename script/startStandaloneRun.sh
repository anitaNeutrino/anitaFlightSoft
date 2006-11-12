#!/bin/bash
echo "Killing Programs"
daemon --stop -n Archived
daemon --stop -n Hkd
daemon --stop -n GPSd
daemon --stop -n Monitord
daemon --stop -n Cmdd
daemon --stop -n Calibd
daemon --stop -n Acqd
echo "Sleeping while files are written and zipped"
sleep 10

echo "Making new directories"
/home/anita/flightSoft/bin/startNewRun.sh
mkdir /mnt/data/current/log
/home/anita/flightSoft/bin/simpleLog /mnt/data/current/log/simpleLog.txt

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
daemon -r LOSd -n LOSd
daemon -r SIPd -n SIPd

Acqd 
