#!/bin/bash
#
#  test script to test turf I/O 
#                 8-Dec-04  SM
#
LOG_F=test_`date +%d%b.%H%M`.log
echo "write to $LOG_F"
#
App/turf_test > $LOG_F &
# sleep 1
# start monitoring data by "tail".
tail -f $LOG_F
#
