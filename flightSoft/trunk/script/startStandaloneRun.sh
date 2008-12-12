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

daemon --stop -n Neobrickd

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
daemon -r Cmdd -n Cmdd
nice -n 20 daemon -r Archived -n Archived
daemon -r Hkd -n Hkd
daemon -r GPSd -n GPSd
daemon -r Monitord -n Monitord
daemon -r Calibd -n Calibd
daemon -r LogWatchd -n LogWatchd

#Just to make sure
daemon -r Prioritizerd -n Prioritizerd 
daemon -r Eventd -n Eventd
daemon -r LOSd -n LOSd
daemon -r SIPd -n SIPd
daemon -r Neobrickd -n Neobrickd
daemon -r Playbackd -n Playbackd
sleep 2

Acqd 
