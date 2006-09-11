#!/bin/bash

run=`cat /home/anita/flightSoft/lastRunNumber`
echo $run
let "run += 1"
echo $run

mkdir /mnt/bitmicro/run${run}
rm -f /mnt/bitmicro/current
ln -sf /mnt/bitmicro/run${run} /mnt/bitmicro/current

mkdir /mnt/puck/run${run}
rm -f /mnt/puck/current
ln -sf /mnt/puck/run${run} /mnt/puck/current

mkdir /mnt/blade/run${run}
rm -f /mnt/blade/current
ln -sf /mnt/blade/run${run} /mnt/blade/current

mkdir /mnt/usbint/run${run}
rm -f /mnt/usbint/current
ln -sf /mnt/usbint/run${run} /mnt/usbint/current

mkdir /mnt/usbext/run${run}
rm -f /mnt/usbext/current
ln -sf /mnt/usbext/run${run} /mnt/usbext/current

echo "$run" > /home/anita/flightSoft/lastRunNumber