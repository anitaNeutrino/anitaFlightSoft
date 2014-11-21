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
daemon --stop -n LogWatchd
daemon --stop -n NTUd
#daemon --stop -n Openportd


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


sleep 2

ps x > /tmp/psx

rm -rf /tmp/neobrick/*
rm -rf /tmp/buffer/*
rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/prioritizerd

echo "Making new directories"
#CmdTest 4
#sleep 2



/home/anita/flightSoft/bin/startNewRun.sh
ls /mnt/helium1/current -l
mkdir /mnt/data/current/log
/home/anita/flightSoft/bin/simpleLog /mnt/data/current/log/simpleLog.txt
mkdir /mnt/helium1/current/log
cp /mnt/data/current/log/simpleLog.txt /mnt/helium1/current/log/simpleLog.txt
mkdir /mnt/helium1/current/config
cp /home/anita/flightSoft/config/*.config /mnt/helium1/current/config
mkdir /mnt/data/current/config
cp /home/anita/flightSoft/config/*.config /mnt/data/current/config


sleep 2

echo "Starting Programs"
nice daemon -r Cmdd -n Cmdd
daemon -r Archived -n Archived
nice daemon -r Hkd -n Hkd
nice daemon -r GPSd -n GPSd
nice daemon -r Monitord -n Monitord
nice daemon -r Calibd -n Calibd
nice daemon -r LogWatchd -n LogWatchd
daemon -r Prioritizerd -n Prioritizerd --env="DISPLAY=:0"
daemon -r Eventd -n Eventd
daemon -r LOSd -n LOSd
daemon -r SIPd -n SIPd
daemon -r Playbackd -n Playbackd
daemon -r NTUd -n NTUd
#daemon -r Openportd -n Openportd

#LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=/tmp/acqd.prof /home/anita/flightSoft/bin/Acqd
#taskset -c 3 Acqd 
Acqd
