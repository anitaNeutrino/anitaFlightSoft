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

mkdir /mnt/satamini/run${run}
rm -f /mnt/satamini/current
if [ -d /mnt/satamini/current ] ; then
	echo "Moving /mnt/satamini/current"
	mv /mnt/satamini/current/ /mnt/satamini/current.run${run}
fi
ln -sf /mnt/satamini/run${run} /mnt/satamini/current

#mkdir /mnt/satablade/run${run}
#rm -f /mnt/satablade/current
#if [ -d /mnt/satablade/current ] ; then
#	echo "Moving /mnt/satablade/current"
#	mv /mnt/satablade/current /mnt/satablade/current.run${run}
	    
#fi
#ln -sf /mnt/satablade/run${run} /mnt/satablade/current

#mkdir /mnt/usbint/run${run}
#rm -f /mnt/usbint/current
#if [ -d /mnt/usbint/current ] ; then
#	echo "Moving /mnt/usbint/current"	
#	mv /mnt/usbint/current /mnt/usbint/current.run${run}

#fi
#ln -sf /mnt/usbint/run${run} /mnt/usbint/current

#mkdir /mnt/usbext/run${run}
#rm -f /mnt/usbext/current
#if [ -d /mnt/usbext/current ] ; then
#	echo "Moving /mnt/usbext/current"
#	mv /mnt/usbext/current /mnt/usbext/current.run${run} 
#fi
#ln -sf /mnt/usbext/run${run} /mnt/usbext/current

rm -rf /tmp/anita/acqd /tmp/anita/eventd /tmp/anita/gpsd /tmp/anita/prioritizerd /tmp/anita/calibd

echo "$run" > /mnt/data/numbers/lastRunNumber