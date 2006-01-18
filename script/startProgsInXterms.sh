#!/bin/bash

source /home/rjn/flightSoft/script/anitaFlightSoftSetup.sh
#cd ${ANITA_FLIGHT_SOFT_DIR}/programs
getLogWindow.sh &
#xterm -T fakeCalibd -e "fakeCalibd ; read" &
#xterm -T firstAcqd -e "firstAcqd ; read" &
xterm -T fakeAcqd -e "source /home/rjn/flightSoft/script/anitaFlightSoftSetup.sh; fakeAcqd ; read" &
#xterm -T GPSd -e "GPSd ; read" &
xterm -T fakeGPSd -e "source /home/rjn/flightSoft/script/anitaFlightSoftSetup.sh; fakeGPSd ; read" &
#xterm -T Eventd -e "source /home/rjn/flightSoft/script/anitaFlightSoftSetup.sh; Eventd ; read" &
#xterm -T Prioritizerd -e "Prioritizerd ; read" &
xterm -T makeFakeTrigger -e "source /home/rjn/flightSoft/script/anitaFlightSoftSetup.sh; makeFakeTrigger ; read" &

