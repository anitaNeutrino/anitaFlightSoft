#!/bin/sh

###First up try to mount drives
sleep 5
removeAllScsiDevices.sh
sleep 5
mountCurrentBlade.sh
sleep 2
mountCurrentUsbExt.sh
sleep 4
mountCurrentUsbInt.sh
sleep 4


#daemon -u anita -r Archived -n Archived
#daemon -u anita -r Acqd -n Acqd
#daemon -u anita -r Eventd -n Eventd
#daemon -u anita -r GPSd -n GPSd
#daemon -u anita -r Hkd -n Hkd
#daemon -u anita -r LOSd -n LOSd
#daemon -u anita -r Prioritizerd -n Prioritizerd
#daemon -u anita -r Monitord -n Monitord
#daemon -u anita -r Calibd -n Calibd
daemon -u anita -r SIPd -n SIPd
daemon -u anita -r Cmdd -n Cmdd