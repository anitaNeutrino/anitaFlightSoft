#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage `basename $0` <drive label>"
    echo "Example `basename $0` usbext01"
    exit 0
fi

#DRIVE=$1
#SERIAL=$(grep $DRIVE /etc/usbLabels/driveNamesAndNumbers | awk '{print $2}')
#echo $SERIAL


MOUNT_POINT=/mnt/usbint
if echo $DRIVE | grep ext; then
#    echo "External Drive"
    MOUNT_POINT=/mnt/usbext
    
fi

#echo $MOUNT_POINT
umount $MOUNT_POINT > /dev/null 2>&1


#if [ -n "$SERIAL" ]
#then
#    echo "Serial Number is $SERIAL"

#    for USB in /proc/scsi/usb-storage-*/*;
#      do
#      if grep $SERIAL $USB; then
#      #echo $USB
#	  SCSI_HOSTNUM=`basename $USB`
#	  echo "Adding $USB to SCSI device list" 
#	  echo "scsi add-single-device $SCSI_HOSTNUM 0 0 0" > /proc/scsi/scsi      
#      fi 
#    done
#else
#    echo "Can't find serial number"
#fi

mount -L $DRIVE -o defaults $MOUNT_POINT
