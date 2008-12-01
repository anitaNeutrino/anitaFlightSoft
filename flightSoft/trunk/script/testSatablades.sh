#!/bin/bash

#First up we get the current blade number
label=`getConfigString anitaSoft.config global satabladeName`
firstNum=${label#satablade}
#echo $firstNum
#firstNum=2

goodNum=0

#Now we loop over the current blade numbers and see whether the drive is there
for bladeNum in `seq $firstNum 8`; do 
    driveName=/dev/disk/by-label/satablade${bladeNum}
#    echo $driveName
    if [ -b $driveName ] ; then
	echo "$driveName exists"
	goodNum=$bladeNum
	break
    else
	echo "$driveName missing"
    fi
done
    
if [ "$goodNum" -eq "0" ]; then
    echo "All drives are missing"
    setConfigValue anitaSoft.config global disableSatablade 1
    #Now disable blades
else
    if [ "$goodNum" -eq "$firstNum" ]; then
	echo "First drive -- satablade${firstNum} is there"
    else
	echo "First drive -- satablade${firstNum} is missing, will use satablade${goodNum}"
	setConfigString anitaSoft.config global satabladeName satablade${goodNum}
    fi
fi