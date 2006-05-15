#!/bin/bash

#source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh
xterm -T fakePackets -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; fakePackets ; read" & 
xterm -T Archived -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; Archived ; read" &
xterm -T LOSd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; LOSd ; read" & 