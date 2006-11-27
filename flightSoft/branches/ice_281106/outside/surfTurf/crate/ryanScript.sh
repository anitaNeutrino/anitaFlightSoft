#!/bin/bash

test_r=`ps -e | grep crate_test`
while [ "$test_r" != "" ] ; do
  echo ' Display is running as process ID '${test_r%pts/*}
  kill ${test_r%pts/*}
 # sleep 1
  test_r=`ps -e | grep crate_t`
done

# figure out data directory name and open it
n=138
D_DIR=/tmp/tvTest$n
while [ -d $D_DIR ] ; do
  let "n = n + 1"
  D_DIR=/tmp/tvTest$n
done

echo $D_DIR will be used as an output directory.
mkdir $D_DIR
App/crate_test -v -v -w -d $D_DIR -n 4000

