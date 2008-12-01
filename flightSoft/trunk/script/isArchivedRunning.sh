#/bin/bash


while [ 1 ]; do
 ps x > /tmp/psx
 if grep -q LOSd /tmp/psx; then
     echo "LOSd is still running"
     sleep 1
 else
     break;
 fi
done;