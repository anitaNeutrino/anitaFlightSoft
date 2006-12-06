#!/bin/bash
#
#  shell script to run crate_test program with display
#

cd /home/anita/plx_test/crate

# First, stop display program if it is running

test_r=`ps -e | grep radio_`
while [ "$test_r" != "" ] ; do
  echo ' Display is running as process ID '${test_r%pts/*}
  kill ${test_r%pts/*}
 # sleep 1
  test_r=`ps -e | grep crate_`
done

test_r=`ps -e | grep crate_t`
while [ "$test_r" != "" ] ; do
  echo ' Display is running as process ID '${test_r%pts/*}
  kill ${test_r%pts/*}
 # sleep 1
  test_r=`ps -e | grep crate_t`
done

# figure out data directory name and open it
n=0
D_DIR=./data/test_$n
while [ -d $D_DIR ] ; do
  let "n = n + 1"
  D_DIR=./data/test_$n
done

echo $D_DIR will be used as an output directory.
mkdir $D_DIR

#D_DIR=./data/`date +%b/%d`
#echo ${D_DIR} will be used for an output directory.
## make directory if non-exist.
#if [ ! -d $D_DIR ] ; then
#    D_DIR_M=./data/`date +%b`
#    if [ ! -d $D_DIR_M ] ; then
#        mkdir $D_DIR_M
#    fi
#    mkdir $D_DIR
#fi

# start programs with output directory D_DIR

xterm -e App/crate_test -w -d $D_DIR &

xterm -e ../display/radio_disp -t -l -d $D_DIR &
