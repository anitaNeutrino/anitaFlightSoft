#!/bin/bash


disabled=`getConfigValue anitaSoft.config global disableNtu`
if [ $disabled -eq 1 ] ; then
    echo "Ntu disk disabled"
    exit 0
fi


sudo chmod a-w /mnt/ntu
label=`getConfigString anitaSoft.config global ntuName`
echo "Trying to mount $label on /mnt/ntu"
