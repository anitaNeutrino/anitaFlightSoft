#!/bin/bash

sudo umount /mnt/usbint
label=`getConfigString anitaSoft.config global usbIntName`
sudo mountUsbDriveByName.sh $label
