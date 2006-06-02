#!/bin/bash

source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh
#cd ${ANITA_FLIGHT_SOFT_DIR}/programs
#getLogWindow.sh &
xterm -T Acqd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; Acqd ; read" &
xterm -T GPSd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; GPSd ; read" &
xterm -T Eventd -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; Eventd ; read" &
xterm -T Prioritizerd -e "source  /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; Prioritizerd ; read" &
xterm -T Archived -e "source  /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; Archived ; read" &
xterm -T top -e "top ; read" &
#xterm -T makeFakeTrigger -e "source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh; makeFakeTrigger ; read" &

