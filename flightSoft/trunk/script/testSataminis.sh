#!/bin/bash

#First up we get the current mini number
label=`getConfigString anitaSoft.config global sataminiName`
firstNum=${label#satamini}
#echo $firstNum
#firstNum=2

goodNum=0

#Now we loop over the current mini numbers and see whether the drive is there
for miniNum in `seq $firstNum 8`; do 
    driveName=/dev/disk/by-label/satamini${miniNum}
#    echo $driveName
    if [ -b $driveName ] ; then
	echo "$driveName exists"
	goodNum=$miniNum
	break
    else
	echo "$driveName missing"
    fi
done
    
if [ "$goodNum" -eq "0" ]; then
    echo "All drives are missing"
    setConfigValue anitaSoft.config global disableSatamini 1
    #Now disable minis
else
    if [ "$goodNum" -eq "$firstNum" ]; then
	echo "First drive -- satamini${firstNum} is there"
    else
	echo "First drive -- satamini${firstNum} is missing, will use satamini${goodNum}"
	setConfigString anitaSoft.config global sataminiName satamini${goodNum}
    fi
fi