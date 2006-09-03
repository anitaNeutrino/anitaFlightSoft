#!/bin/bash

sudo umount /mnt/usbext
label=`getConfigString anitaSoft.config global usbExtName`
sudo mountUsbDriveByName.sh $label
