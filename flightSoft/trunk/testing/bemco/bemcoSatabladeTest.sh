#!/bin/bash
#Script to test writing to satablades in the bemco test.

cd /home/anita/flightSoft/testing


BASE_LOG_DIR=/home/anita/bemcoLog/satablade
RUN=1;
while true;
do
    LOG_DIR=${BASE_LOG_DIR}/run${RUN}
    if [ -d $LOG_DIR ]; 
    then
	echo -n ""
	#Run exists
    else
	#New Run
	break
    fi
    let RUN=RUN+1
done

mkdir ${LOG_DIR}
echo "Using log dir $LOG_DIR"

TEST_DIR=/mnt/satablade
REF_DIR=/tmp/satablade

if [ -d ${TEST_DIR} ] ;
then 
    echo -n ""
else 
   echo "${TEST_DIR} doesn't exist.... giving up"
   exit 0
fi

if [ -d ${REF_DIR} ] ;
then 
    echo -n ""
else 
    mkdir ${REF_DIR}
fi


NUM_EVENTS=20000 
NUM_LOOPS=20000
let LAST_FILE=${NUM_EVENTS}-100

WRITE_LOG=${LOG_DIR}/writeLog.txt
WRITE_LOG_TIME=${LOG_DIR}/writeLogTime.txt
SYNC_LOG=${LOG_DIR}/syncLog.txt
SYNC_LOG_TIME=${LOG_DIR}/syncLogTime.txt
DIFF_LOG=${LOG_DIR}/diffLog.txt
RM_LOG=${LOG_DIR}/rmLog.txt
MAIN_LOG=${LOG_DIR}/mainLog.txt

let END_VAL=${NUM_LOOPS}-1
echo $END_VAL
for loop in `seq 0 $END_VAL`; do 
    echo "Starting Loop $loop"
    echo "Starting Loop $loop" >> $MAIN_LOG
    for bladeNum in `seq 1 8`; do
	bladeName=satablade${bladeNum}
	echo "Starting with blade $bladeName"
	echo "Starting with blade $bladeName" >> $MAIN_LOG
	mountSatabladeByName.sh $bladeName
	df -h >> $MAIN_LOG
	
	startTime=`date --utc +%s`
	/usr/bin/time -f "${loop} %e %S %U %I %O" -a -o ${WRITE_LOG_TIME}  ./testDiskWritingWithCheck $REF_DIR $TEST_DIR $NUM_EVENTS 2>&1 >> ${WRITE_LOG}
	endTestTime=`date --utc +%s`
	/usr/bin/time -f "${loop} %e %S %U %I %O" -a -o ${SYNC_LOG_TIME} sync 2>&1 >> ${SYNC_LOG}
	endSyncTime=`date --utc  +%s`
	for evNum in `seq 0 100 $LAST_FILE`; do
	    testFile=${TEST_DIR}/test_${evNum}.dat
	    refFile=${REF_DIR}/test_${evNum}.dat
	    diff $testFile $refFile 2>&1 >> ${DIFF_LOG}
	    rm $testFile $refFile 2>&1 >> ${RM_LOG}
	done
	/usr/bin/time -f "${loop} %e %S %U %I %O" -a -o ${SYNC_LOG_TIME} sync 2>&1 >> ${SYNC_LOG}
	endCheckTime=`date --utc  +%s`
	echo "Loop ${loop} $startTime $endTestTime $endSyncTime $endCheckTime" >> $MAIN_LOG
	let elapsedTime=endCheckTime-startTime
	echo "Ending loop ${loop} after $elapsedTime seconds"
    done
done


   
