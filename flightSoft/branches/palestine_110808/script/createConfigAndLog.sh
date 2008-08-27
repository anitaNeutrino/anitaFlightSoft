#!/bin/bash


echo "User: Cmdd" > /tmp/simpleLog
echo "Run Description:" >> /tmp/simpleLog
echo "   Started by Cmdd" >> /tmp/simpleLog


if [ -d /mnt/data/current ]; then
    mkdir /mnt/data/current/log
    cp /tmp/simpleLog /mnt/data/current/log/simpleLog.txt
fi

if [ -d /mnt/satamini/current ]; then
    mkdir /mnt/satamini/current/log
    cp /tmp/simpleLog /mnt/satamini/current/log
    mkdir /mnt/satamini/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/satamini/current/config
fi    

if [ -d /mnt/satablade/current ]; then
    mkdir /mnt/satablade/current/log
    cp /tmp/simpleLog /mnt/satablade/current/log
    mkdir /mnt/satablade/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/satablade/current/config
fi    


if [ -d /mnt/usbint/current ]; then
    mkdir /mnt/usbint/current/log
    cp /tmp/simpleLog /mnt/usbint/current/log
    mkdir /mnt/usbint/current/config
    cp /home/anita/flightSoft/config/*.config /mnt/usbint/current/config
fi    



