#!/bin/sh

export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

turnAllOff
turnGPSOn

#Start Cmdd and SIPd
daemon -u anita -r SIPd -n SIPd
daemon -u anita -r Cmdd -n Cmdd


###First up try to mount drives
sleep 40
removeAllScsiDevices.sh
sleep 5
removeAllScsiDevices.sh
sleep 5
mountCurrentBlade.sh &
sleep 4
mountCurrentUsbExt.sh &
sleep 4
mountCurrentUsbInt.sh &
sleep 20


daemon -u anita -r Archived -n Archived
daemon -u anita -r Acqd -n Acqd
daemon -u anita -r Eventd -n Eventd
daemon -u anita -r GPSd -n GPSd
daemon -u anita -r Hkd -n Hkd
daemon -u anita -r LOSd -n LOSd
daemon -u anita -r Prioritizerd -n Prioritizerd
daemon -u anita -r Monitord -n Monitord
daemon -u anita -r Calibd -n Calibd

