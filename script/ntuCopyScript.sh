#!/bin/bash
source /home/anita/.bash_profile
source /home/anita/flightSoft/bin/anitaFlightSoftSetup.sh
export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft
export PATH=/bin:/usr/bin:/usr/local/bin
## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}

NTU_IP="[fe80::2e0:4bff:fe40:8074%p4p1]"


disabled=`getConfigValue anitaSoft.config global disableNtu`
if [ $disabled -eq 1 ] ; then
    echo "Ntu disk disabled"
    exit 0
fi


ntuName=`getConfigString anitaSoft.config global ntuName`
#ntuCopyDir=`getConfigString anitaSoft.config global ntuCopyDir`
ntuCopyDir=/tmp/ntu
echo "ntuName: $ntuName"
echo "ntuCopyDir: $ntuCopyDir:"
echo "ntuCopyPid: $$"
echo $$ > /tmp/ntuCopyPid

while :
do
    rsync -av $NTU_IP:~/lastCopiedRun /tmp/lastCopiedRun > /dev/null

    #This first part just makes sure that any straggler files are copied correctly   
    LAST_RUN=`cat /mnt/data/numbers/lastRunNumber`
    START_RUN=`cat /tmp/lastCopiedRun`
    PENULTIMATE_RUN=`expr $LAST_RUN - 1`
    for run in `seq $START_RUN $PENULTIMATE_RUN`; do
	if [ -d "${ntuCopyDir}/run$run" ]; then
	    rsync -av --remove-source-files ${ntuCopyDir}/run$run anita@${NTU_IP}:${ntuName}/  > /tmp/getRawData.log
	    if  test `cat /tmp/getRawData.log  | wc -l` -gt 04 ; then
		echo "Copied Run $run"
		echo  $run > /tmp/lastCopiedRun
		rsync -av /tmp/lastCopiedRun  $NTU_IP:~/lastCopiedRun
	    fi
	    rm -rf ${ntuCopyDir}/run$run
	fi
    done

    #Now try and copy only .gz files for current run
    rsync -avP --remove-source-files --exclude '*.dat' --exclude '*.new' --exclude '*.temp' ${ntuCopyDir}/run${LAST_RUN} anita@${NTU_IP}:${ntuName}/ > /tmp/copyCurrentRun.log  
    #sleep 5
done
