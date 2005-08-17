#!/bin/bash


rm -rf /tmp/anita
echo 0 > /home/rjn/flightSoft/lastEventNumber
killAllProgs.sh
xterm -T firstAcqd -e "firstAcqd ; read" &
xterm -T GPSd -e "GPSd ; read" &
xterm -T Eventd -e "Eventd ; read" &
xterm -T Prioritizerd -e "Prioritizerd ; read" &
xterm -T Archived -e "Archived ; read" &
