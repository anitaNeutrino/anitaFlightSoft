#!/bin/bash

cd ${ANITA_FLIGHT_SOFT_DIR}/programs
getLogWindow.sh &
xterm -T fakeCalibd -e "./fakeCalibd ; read" &
xterm -T fakeAcqd -e "./fakeAcqd ; read" &
xterm -T fakeGPSd -e "./fakeGPSd ; read" &
xterm -T Eventd -e "./Eventd ; read" &
xterm -T Prioritizerd -e "./Prioritizerd ; read" &
xterm -T makeFakeTrigger -e "./makeFakeTrigger ; read" &

