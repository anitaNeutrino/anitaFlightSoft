#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage `basename $0` <drive label>"
    echo "Example `basename $0` satablade3"
    exit 0
fi



if df -h | grep -q "satablade"
then
    sudo umount /mnt/satablade
fi

label=$1
echo "Trying to mount $label on /mnt/satablade"
sudo mount -L $label -o defaults /mnt/satablade
if df -h | grep -q "satablade"
then
	sudo chmod a+wrx /mnt/satablade
fi
