#/bin/bash

while true; 
do
COUNTER=0
for file in /tmp/anita/telem/event/pri0/link/h*;
do 
  endPart=${file#*_}
  priDir=${file%/link/*}
  headFile=${priDir}/${file##*/}
  bodyFile=${priDir}/ev_${endPart}    
  rm $file
  rm $headFile
  rm $bodyFile 
  let COUNTER=COUNTER+1
done
echo "Deleted ${COUNTER} event telem files"
sleep 1
for file in /tmp/anita/telem/head/link/h*;
do 
  priDir=${file%/link/*}
  headFile=${priDir}/${file##*/}
  rm $file
  rm $headFile
done
done