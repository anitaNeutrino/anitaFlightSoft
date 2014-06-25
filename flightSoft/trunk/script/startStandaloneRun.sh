#!/bin/bash
echo "Killing Programs"
killall Archived
killall Hkd
killall GPSd
killall Monitord
killall Cmdd
killall Calibd
killall Acqd
killall Prioritizerd
killall Eventd
killall LogWatchd


echo "Sleeping while files are written and zipped"
while [ 1 ]; do
 ps x > /tmp/psx
 if grep -q Archived /tmp/psx; then
     echo "Archived is still running"
     sleep 1
 else
     break;
 fi
done;

killall Neobrickd

sleep 2

ps x > /tmp/psx

rm -rf /tmp/neobrick/*
rm -rf /tmp/buffer/*
rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/prioritizerd

echo "Making new directories"
#CmdTest 4
#sleep 2



/home/anita/flightSoft/bin/startNewRun.sh
ls /mnt/satablade/current -l
mkdir /mnt/data/current/log
/home/anita/flightSoft/bin/simpleLog /mnt/data/current/log/simpleLog.txt
mkdir /mnt/satablade/current/log
cp /mnt/data/current/log/simpleLog.txt /mnt/satablade/current/log/simpleLog.txt
mkdir /mnt/satablade/current/config
cp /home/anita/flightSoft/config/*.config /mnt/satablade/current/config
mkdir /mnt/data/current/config
cp /home/anita/flightSoft/config/*.config /mnt/data/current/config


sleep 2

echo "Starting Programs"
#Cmdd > /tmp/cmdd.log 2>&1 </dev/null &
Archived > /tmp/archived.log 2>&1 </dev/null &
#Monitord > /tmp/monitord.log 2>&1 </dev/null&
#Hkd > /tmp/hkd.log 2>&1 </dev/null &
#GPSd > /tmp/gpsd.log 2>&1 </dev/null &
Calibd > /tmp/calibd.log 2>&1 </dev/null &
#LogWatchd > /tmp/logwatchd.log 2>&1 </dev/null &


#Just to make sure
#SIPd > /tmp/sipd 2>&1 </dev/null &
#LOSd > /tmp/losd 2>&1 </dev/null &
Calibd > /tmp/calibd.log 2>&1 </dev/null &
Eventd > /tmp/eventd.log 2>&1 </dev/null &
Prioritizerd > /tmp/prioritizerd.log 2>&1 </dev/null &


nice -n 10 Acqd 
