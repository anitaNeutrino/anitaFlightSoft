#!/bin/sh
source /home/anita/flightSoft/bin/anitaFlightSoftSetup.sh
export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

#X  2>&1 > /dev/null &
DISPLAY=:0 xhost +

daemon -u anita -r SIPd -n SIPd

#turnAllOff



#Start Cmdd
daemon -u anita -r Cmdd -n Cmdd

#Will have to change this to move 
/usr/sbin/runuser -u anita /home/anita/flightSoft/script/startNewRun.sh 
/usr/sbin/runuser -u anita /home/anita/flightSoft/script/createConfigAndLog.sh 
sleep 5
daemon -u anita -r Archived -n Archived
daemon -u anita  -r Eventd -n Eventd
daemon -u anita -r GPSd -n GPSd
daemon -u anita -r Hkd -n Hkd
daemon -u anita -r LOSd -n LOSd
daemon -u anita -r Prioritizerd -n Prioritizerd
daemon -u anita -r Monitord -n Monitord
daemon -u anita -r Calibd -n Calibd
daemon -u anita -r Playbackd -n Playbackd
sleep 5
/usr/sbin/runuser -u anita CmdTest 133 0 32
daemon -u anita -r Acqd -n Acqd
daemon -u anita -r runLengthLoop.sh -n runLengthLoop.sh
