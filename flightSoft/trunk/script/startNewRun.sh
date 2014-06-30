#!/bin/bash

run=`cat /mnt/data/numbers/lastRunNumber`
echo $run
let "run += 1"
echo $run


mkdir /mnt/data/run${run}
rm -f /mnt/data/current
if [ -d /mnt/data/current ] ; then
    echo "Moving /mnt/data/current"
    mv /mnt/data/current/ /mnt/data/current.run${run}
fi
ln -sf /mnt/data/run${run} /mnt/data/current


disabledHelium2=`getConfigValue anitaSoft.config global disableHelium2`
if [ $disabledHelium2 -eq 1 ] ; then
    echo "Helium2 disk disabled"   
else 
    mkdir /mnt/helium2/run${run}
    rm -f /mnt/helium2/current
    if [ -d /mnt/helium2/current ] ; then
	echo "Moving /mnt/helium2/current"
	mv /mnt/helium2/current/ /mnt/helium2/current.run${run}
    fi
    ln -sf /mnt/helium2/run${run} /mnt/helium2/current
fi

disabledHelium1=`getConfigValue anitaSoft.config global disableHelium1`
if [ $disabledHelium1 -eq 1 ] ; then
    echo "Helium1 disk disabled"
else 
    mkdir /mnt/helium1/run${run}
    rm -f /mnt/helium1/current
    if [ -d /mnt/helium1/current ] ; then
	echo "Moving /mnt/helium1/current"
	mv /mnt/helium1/current /mnt/helium1/current.run${run}
	
    fi
    ln -sf /mnt/helium1/run${run} /mnt/helium1/current
fi


disabledUsb=`getConfigValue anitaSoft.config global disableUsb`
if [ $disabledUsb -eq 1 ] ; then
    echo "USB disk disabled"
else  
  #  echo "Need to fix this"
    mkdir /mnt/usbint/run${run}
    rm -f /mnt/usbint/current
    if [ -d /mnt/usbint/current ] ; then
	echo "Moving /mnt/usbint/current"
	mv /mnt/usbint/current /mnt/usbint/current.run${run} 
    fi
    ln -sf /mnt/usbint/run${run} /mnt/usbint/current
fi

rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/gpsd /tmp/anita/prioritizerd /tmp/anita/calibd

echo "$run" > /mnt/data/numbers/lastRunNumber
