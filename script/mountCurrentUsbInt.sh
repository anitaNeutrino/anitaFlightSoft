#!/bin/bash

sudo umount /mnt/usbint

disabled=`getConfigValue anitaSoft.config global disableUsbInt`
if [ $disabled -eq 1 ] ; then
    echo "Blade disk disabled"
    exit 0
fi

label=`getConfigString anitaSoft.config global usbIntName`

echo "Trying to mount $label on /mnt/usbint"
sudo mountUsbDriveByName.sh $label
