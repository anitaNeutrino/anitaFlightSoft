#!/bin/bash


echo "User: Cmdd" > /tmp/simpleLog.txt
echo "Run Description:" >> /tmp/simpleLog.txt
echo "   Started by Cmdd" >> /tmp/simpleLog.txt
echo "   Uptime: " >> /tmp/simpleLog.txt
cat /proc/uptime >> /tmp/simpleLog.txt
echo "   Disks: " >> /tmp/simpleLog.txt
df -h >> /tmp/simpleLog.txt

if [ -d /mnt/data/current ]; then
    mkdir /mnt/data/current/log
    cp /tmp/simpleLog.txt /mnt/data/current/log/simpleLog.txt
fi

if [ -d /mnt/satamini/current ]; then
    mkdir /mnt/satamini/current/log
    cp /tmp/simpleLog.txt /mnt/satamini/current/log
    mkdir /mnt/satamini/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/satamini/current/config
fi    

if [ -d /mnt/satablade/current ]; then
    mkdir /mnt/satablade/current/log
    cp /tmp/simpleLog.txt /mnt/satablade/current/log
    mkdir /mnt/satablade/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/satablade/current/config
fi    




