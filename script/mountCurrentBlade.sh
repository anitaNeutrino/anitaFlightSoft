#!/bin/bash

sudo umount /mnt/blade
label=`getConfigString anitaSoft.config global bladeName`
echo "Trying to mount $label on /mnt/blade"
sudo mount -L $label -o defaults,sync /mnt/blade
