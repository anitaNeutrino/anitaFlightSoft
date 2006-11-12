#!/bin/bash

run=`cat /home/anita/flightSoft/lastRunNumber`
echo $run
let "run += 1"
echo $run

mkdir /mnt/bitmicro/run${run}
rm -f /mnt/bitmicro/current
if [ -d /mnt/bitmicro/current ] ; then
	echo "Moving /mnt/bitmicro/current"
	mv /mnt/bitmicro/current /mnt/bitmicro/current.run${run}
fi
ln -sf /mnt/bitmicro/run${run} /mnt/bitmicro/current

mkdir /mnt/puck/run${run}
rm -f /mnt/puck/current
if [ -d /mnt/puck/current ] ; then
	echo "Moving /mnt/puck/current"
	mv /mnt/puck/current/ /mnt/puck/current.run${run}
fi
ln -sf /mnt/puck/run${run} /mnt/puck/current

mkdir /mnt/blade/run${run}
rm -f /mnt/blade/current
if [ -d /mnt/blade/current ] ; then
	echo "Moving /mnt/blade/current"
	mv /mnt/blade/current /mnt/blade/current.run${run}
	    
fi
ln -sf /mnt/blade/run${run} /mnt/blade/current

mkdir /mnt/usbint/run${run}
rm -f /mnt/usbint/current
if [ -d /mnt/usbint/current ] ; then
	echo "Moving /mnt/usbint/current"	
	mv /mnt/usbint/current /mnt/usbint/current.run${run}

fi
ln -sf /mnt/usbint/run${run} /mnt/usbint/current

mkdir /mnt/usbext/run${run}
rm -f /mnt/usbext/current
if [ -d /mnt/usbext/current ] ; then
	echo "Moving /mnt/usbext/current"
	mv /mnt/usbext/current /mnt/usbext/current.run${run} 
fi
ln -sf /mnt/usbext/run${run} /mnt/usbext/current

echo "$run" > /home/anita/flightSoft/lastRunNumber