#!/bin/bash

run=`cat /mnt/data/numbers/lastRunNumber`
echo $run
let "run += 1"
echo $run

mkdir /mnt/pmcdata/run${run}
rm -f /mnt/pmcdata/current
if [ -d /mnt/pmcdata/current ] ; then
	echo "Moving /mnt/pmcdata/current"
	mv /mnt/pmcdata/current /mnt/pmcdata/current.run${run}
fi
ln -sf /mnt/pmcdata/run${run} /mnt/pmcdata/current


disabledMini=`getConfigValue anitaSoft.config global disableSatamini`
if [ $disabledMini -eq 1 ] ; then
    echo "Satamini disk disabled"   
else 
    mkdir /mnt/satamini/run${run}
    rm -f /mnt/satamini/current
    if [ -d /mnt/satamini/current ] ; then
	echo "Moving /mnt/satamini/current"
	mv /mnt/satamini/current/ /mnt/satamini/current.run${run}
    fi
    ln -sf /mnt/satamini/run${run} /mnt/satamini/current
fi

disabledBlade=`getConfigValue anitaSoft.config global disableSatablade`
if [ $disabledBlade -eq 1 ] ; then
    echo "Satamini disk disabled"
else 
    mkdir /mnt/satablade/run${run}
    rm -f /mnt/satablade/current
    if [ -d /mnt/satablade/current ] ; then
	echo "Moving /mnt/satablade/current"
	mv /mnt/satablade/current /mnt/satablade/current.run${run}
	
    fi
    ln -sf /mnt/satablade/run${run} /mnt/satablade/current
fi


disabledUsb=`getConfigValue anitaSoft.config global disableUsb`
if [ $disabledUsb -eq 1 ] ; then
    echo "USB disk disabled"
else  
    echo "Need to fix this"
#mkdir /mnt/usbext/run${run}
#rm -f /mnt/usbext/current
#if [ -d /mnt/usbext/current ] ; then
#	echo "Moving /mnt/usbext/current"
#	mv /mnt/usbext/current /mnt/usbext/current.run${run} 
#fi
#ln -sf /mnt/usbext/run${run} /mnt/usbext/current
fi

rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/gpsd /tmp/anita/prioritizerd /tmp/anita/calibd

echo "$run" > /mnt/data/numbers/lastRunNumber