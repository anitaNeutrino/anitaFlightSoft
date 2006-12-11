#!/bin/sh

export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

turnAllOff
turnGPSOn

sudo -u anita cp /home/anita/flightSoft/config/defaults/Acqd.config.0 /home/anita/flightSoft/config/Acqd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Archived.config.0 /home/anita/flightSoft/config/Archived.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Calibd.config.0 /home/anita/flightSoft/config/Calibd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Eventd.config.0 /home/anita/flightSoft/config/Eventd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/GPSd.config.0 /home/anita/flightSoft/config/GPSd.config
#sudo -u anita cp /home/anita/flightSoft/config/defaults/LOSd.config.0 /home/anita/flightSoft/config/LOSd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Monitord.config.0 /home/anita/flightSoft/config/Monitord.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Playbackd.config.0 /home/anita/flightSoft/config/Playbackd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/Prioritizerd.config.0 /home/anita/flightSoft/config/Prioritizerd.config
sudo -u anita cp /home/anita/flightSoft/config/defaults/SIPd.config.0 /home/anita/flightSoft/config/SIPd.config

#Start Cmdd and SIPd
nice -n -20 daemon -u anita -r SIPd -n SIPd
nice -n -20 daemon -u anita -r Cmdd -n Cmdd

#rm -rf /mnt/usbext/*
#rm -rf /mnt/usbint/*
###First up try to mount drives
sleep 40
removeAllScsiDevices.sh
sleep 5
removeAllScsiDevices.sh
sleep 5
mountPuck.sh &
sleep 4
mountCurrentBlade.sh &
sleep 4
mountCurrentUsbExt.sh &
sleep 10
mountCurrentUsbInt.sh &
sleep 40
sudo -u anita /home/anita/flightSoft/bin/startNewRun.sh
#sudo -u anita CmdTest 4
sleep 10

daemon -u anita -r Archived -n Archived
daemon -u anita -r Acqd -n Acqd
daemon -u anita -r Eventd -n Eventd
daemon -u anita -r GPSd -n GPSd
daemon -u anita -r Hkd -n Hkd
daemon -u anita -r LOSd -n LOSd
nice -n -5 daemon -u anita -r Prioritizerd -n Prioritizerd
daemon -u anita -r Monitord -n Monitord
daemon -u anita -r Calibd -n Calibd
daemon -u anita -r Playbackd -n Playbackd

