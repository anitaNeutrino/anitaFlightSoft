#!/bin/sh
source /home/anita/flightSoft/bin/anitaFlightSoftSetup.sh
export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

X  2>&1 > /dev/null &
DISPLAY=:0 xhost +

daemon -u anita -r SIPd -n SIPd

#turnAllOff


### Do this so that the RTL's can restart themselves 
chmod a+w -R /dev/bus/usb/


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
daemon -u anita -r Openportd -n Openportd
daemon -u anita -r Prioritizerd -n Prioritizerd
daemon -u anita -r Monitord -n Monitord
daemon -u anita -r Calibd -n Calibd
daemon -u anita -r Playbackd -n Playbackd
daemon -u anita -r LogWatchd -n LogWatchd
daemon -u anita -r RTLd -n RTLd
daemon -u anita -r Tuffd -n Tuffd
daemon -u anita -r checkNtu.py -n checkNtu.py
sleep 2
/usr/sbin/runuser -u anita CmdTest 133 0 32
daemon -u anita -r Acqd -n Acqd

