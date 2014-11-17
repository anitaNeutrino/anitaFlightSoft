#!/bin/bash

RUN_START_TIME=`stat -c %Y /mnt/data/numbers/lastRunNumber`
NOW_TIME=`date +%s`
MAX_RUN_LENGTH=`getConfigValue anitaSoft.config global maxRunLength`

if [ -f "/tmp/anita/pid/cmdd.pid" ]; then
    if grep Cmdd /proc/`cat /tmp/anita/pid/cmdd.pid`/comm; then 
	echo "Cmdd running";
    else
	echo "Cmdd not running";
	daemon -r Cmdd -n Cmdd
    fi
fi


let COMP_TIME=RUN_START_TIME+MAX_RUN_LENGTH
let DIFF_TIME=COMP_TIME-NOW_TIME

#echo $RUN_START_TIME
#echo $NOW_TIME
#echo $COMP_TIME

echo $DIFF_TIME > /tmp/checkedRun

if [ $DIFF_TIME -lt 0 ]; then
    echo "Time for a new run"
    source /home/anita/flightSoft/script/anitaFlightSoftSetup.sh
    CmdTest 003
else  
    echo "This run still has $DIFF_TIME seconds to run"
fi


    
