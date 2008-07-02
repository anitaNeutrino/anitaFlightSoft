#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage `basename $0` <drive label>"
    echo "Example `basename $0` usbint3"
    exit 0
fi



if df -h | grep -q "usbint"
then
    sudo umount /mnt/usbint
fi

label=$1
echo "Trying to mount $label on /mnt/usbint"
sudo mount -L $label -o defaults /mnt/usbint
if df -h | grep -q "usbint"
then
    sudo chmod a+wrx /mnt/usbint
fi
