#!/bin/bash

if df -h | grep -q "satablade"
then
	sudo umount /mnt/satablade
fi

disabled=`getConfigValue anitaSoft.config global disableSatablade`
if [ $disabled -eq 1 ] ; then
    echo "Satablade disk disabled"
    exit 0
fi


sudo chmod a-w /mnt/satablade
label=`getConfigString anitaSoft.config global satabladeName`
echo "Trying to mount $label on /mnt/satablade"
sudo mount -L $label -o defaults /mnt/satablade
if df -h | grep -q "satablade"
then
	sudo chmod a+wrx /mnt/satablade
fi
