#!/bin/bash

echo 0 > /tmp/usbExtMounted
export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

sudo umount /mnt/usbext

disabled=`getConfigValue anitaSoft.config global disableUsbExt`
if [ $disabled -eq 1 ] ; then
    echo "Blade disk disabled"
    exit 0
fi

label=`getConfigString anitaSoft.config global usbExtName`
echo "Trying to mount $label on /mnt/usbext"
sudo mountUsbDriveByName.sh $label
sudo chmod a+wrx /mnt/usbext
