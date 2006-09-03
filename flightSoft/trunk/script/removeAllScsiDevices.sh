#!/bin/bash
###############################################################################
### Evil script that removes all the SCSI devices, will be run once at boot
### then we will only implement one at a time.
### rjn@hep.ucl.ac.uk August 2006
###############################################################################

for USB in /proc/scsi/usb-storage-*/*;
do
  SCSI_HOSTNUM=`basename $USB`
  echo "Removing $USB from SCSI device list" 
  echo "scsi remove-single-device $SCSI_HOSTNUM 0 0 0" > /proc/scsi/scsi
done