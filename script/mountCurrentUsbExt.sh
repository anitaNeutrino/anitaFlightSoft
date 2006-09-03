#!/bin/bash

sudo umount /mnt/usbext

disabled=`getConfigValue anitaSoft.config global disableUsbExt`
if [ $disabled -eq 1 ] ; then
    echo "Blade disk disabled"
    exit 0
fi

label=`getConfigString anitaSoft.config global usbExtName`
echo "Trying to mount $label on /mnt/usbext"
sudo mountUsbDriveByName.sh $label
