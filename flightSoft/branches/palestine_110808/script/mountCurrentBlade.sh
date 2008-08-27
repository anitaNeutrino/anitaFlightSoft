#!/bin/bash

if df -h | grep -q "blade"
then
    sudo umount /mnt/blade
fi

disabled=`getConfigValue anitaSoft.config global disableBlade`
if [ $disabled -eq 1 ] ; then
    echo "Blade disk disabled"
    exit 0
fi


sudo chmod a-w /mnt/blade
label=`getConfigString anitaSoft.config global bladeName`
echo "Trying to mount $label on /mnt/blade"
sudo mount -L $label -o defaults /mnt/blade
if df -h | grep -q "blade"
then
	sudo chmod a+wrx /mnt/blade
fi
