#!/usr/bin/env python
"""
This script should run on the flight computer, regularly ping the NTU computer and if
it gets no response, will send a command to toggle the relays to the NTU computer and SSD drives 
off then on.

Apparently the best way to do this is power off the computer first (12V) then the SSDs (5V)
then power on the SSDs then the computer.

The flight computer should push to the ntu with rsync.
"""

from os import system
from time import sleep
from datetime import datetime

def main():

    ntuIPaddress = '192.168.20.211' # The IP address to ping
    logFileName = 'checkNtu.log' # Timestamp of when restart was done

    # Values copied from flightSoft/common/include/anitaCommand.h
    CMD_TURN_NTU_SSD_5V_ON = 158
    CMD_TURN_NTU_SSD_5V_OFF = 159
    CMD_TURN_NTU_SSD_12V_ON = 160
    CMD_TURN_NTU_SSD_12V_OFF = 161

    while(1):

        # Ping ntu and get return value, this will happen a lot so keep the command line clear 
        response = system('ping -c 1 ' + ntuIPaddress + ' > /dev/null')

        # Ping should return 0 if the package was successful
        if response is not 0:
        
            # Turn computer power off, it's probably already off
            print 'Turning off NTU SSD 12V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_OFF))

            # Turn disk off
            print 'Turning off NTU SSD 5V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_OFF))
            sleep(10)

            # Turn disk on
            print 'Turning on NTU SSD 5V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_ON))
            sleep(10)

            #Turn computer back on
            print 'Turning on NTU SSD 12V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_ON))

            # Reboot is now effectively issued, log it
            appendLogFile(logFileName)

            # Then wait a long time for the computer to load
            # Is this long enough, too long?
            sleep(12*60)

        else:

            # Wait 5 seconds
            sleep(5)


def appendLogFile(logFileName):
    
    # Open the log file in append mode
    with file(logFileName, 'a') as logFile:
        logFile.write(str(datetime.now()) + '\n')

if __name__ == '__main__':
    main()
