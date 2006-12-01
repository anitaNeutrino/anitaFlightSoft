#!/bin/bash

sudo umount /mnt/blade

disabled=`getConfigValue anitaSoft.config global disableBlade`
if [ $disabled -eq 1 ] ; then
    echo "Blade disk disabled"
    exit 0
fi


sudo chmod a-w /mnt/blade
label=`getConfigString anitaSoft.config global bladeName`
echo "Trying to mount $label on /mnt/blade"
sudo mount -L $label -o defaults,sync /mnt/blade
if df -h | grep -q "blade"
then
	sudo chmod a+wrx /mnt/blade
fi
