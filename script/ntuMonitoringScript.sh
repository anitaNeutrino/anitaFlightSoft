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
echo "ntuMonitorPid: $$:"
echo $$ > /tmp/ntuMonitorPid

while :
do
    rsync -av $NTU_IP:/tmp/lastTemps /tmp/lastNtuTemps > /dev/null
    rsync -av $NTU_IP:/tmp/lastDiskSpace /tmp/lastNtuDiskSpace > /dev/null
    sleep 30
done
