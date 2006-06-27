#!/bin/sh

daemon -u anita -r Archived -n Archived
daemon -u anita -r Acqd -n Acqd
daemon -u anita -r Eventd -n Eventd
daemon -u anita -r GPSd -n GPSd
daemon -u anita -r Hkd -n Hkd
daemon -u anita -r LOSd -n LOSd
daemon -u anita -r Prioritizerd -n Prioritizerd
daemon -u anita -r Monitord -n Monitord
daemon -u anita -r Calibd -n Calibd
daemon -u anita -r SIPd -n SIPd
daemon -u anita -r Cmdd -n Cmdd