#!/bin/bash

DATE=`head -1 $1`
UNIXTIME=`date -d"$DATE" +%s`
USAGE=`grep "Disk usage" $1 | awk {'print $4'}`
AVAIL=`grep "Disk usage" $1 | awk {'print $7'}`
TOTAL=`grep "Disk usage" $1 | awk {'print $10'}`
TEMP=`grep "Temperature" $1 | awk {'print $2'}`
PRESS=`grep "Pressure" $1 | awk {'print $2'}`
echo $UNIXTIME $USAGE $AVAIL $TOTAL $TEMP $PRESS
