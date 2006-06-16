#!/bin/bash

#source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh
xterm -T fakePackets -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; fakePackets 10000; read" & 
xterm -T Archived -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; Archived ; read" &
xterm -T Prioritizerd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; Prioritizerd ; read" &
xterm -T LOSd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh ; LOSd ; read" & 