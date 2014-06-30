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

if [ -d /mnt/helium2/current ]; then
    mkdir /mnt/helium2/current/log
    cp /tmp/simpleLog.txt /mnt/helium2/current/log
    mkdir /mnt/helium2/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/helium2/current/config
fi    

if [ -d /mnt/helium1/current ]; then
    mkdir /mnt/helium1/current/log
    cp /tmp/simpleLog.txt /mnt/helium1/current/log
    mkdir /mnt/helium1/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/helium1/current/config
fi    




