#!/usr/bin/env python
"""
This script should run on the flight computer, regularly ping the NTU computer and if
it gets no response, will send a command to toggle the relays to the NTU computer and SSD drives 
off then on.

Apparently the best way to do this is power off the computer first (12V) then the SSDs (5V)
then power on the SSDs then the computer.

The flight computer should push to the ntu with rsync.
"""

from sys import argv
from os import system
from time import sleep
from time import time
from datetime import datetime

def main(args):

    # You can now call this with a force option, which will issue restart commands to calibd
    forceFlag = False
    testMode = False
    firstTime = True

    for arg in args:
        if arg == '-f' or arg == '--force':
            forceFlag = True
        if arg == '-t' or arg == '--test':
            testMode = True

    ntuIPaddress="fe80::2e0:4bff:fe40:8074%p4p1"
    #ntuIPaddress = '192.168.1.5' # The IP address to ping
    logFileName = '/home/anita/ntuCheckLog/checkNtu.log' # Timestamp of when restart was done
    #logFileName = 'checkNtu.log' # Timestamp of when restart was done

    # How long we should give the ntu to boot before toggling relays again
    maxWaitTimeMinutes = 15
    maxWaitTimeSeconds = maxWaitTimeMinutes*60
    lastRebootTime = time()
    if testMode:
        lastRebootTime = 0
    notHeardBackSinceReboot = True

    appendLogFile(logFileName, 'Check NTU script started! Will wait at least '
                  + str(maxWaitTimeMinutes) 
                  + ' minutes before togging relays if NTU is unresponsive.' )

    # Values copied from flightSoft/common/include/anitaCommand.h
    CMD_TURN_NTU_SSD_5V_ON = 158
    CMD_TURN_NTU_SSD_5V_OFF = 159
    CMD_TURN_NTU_SSD_12V_ON = 160
    CMD_TURN_NTU_SSD_12V_OFF = 161
    
    while True:

        # Ping ntu and get return value, this will happen a lot so keep the command line clear 
        # Options mean we wait 5 seconds between pings
        
        response = None
        if testMode == False:
            response = system('ping6 -i 6 -c 1 ' + ntuIPaddress + ' > /dev/null')
        else:
            response = system('ping6 -i 6 -c 1 ' + ntuIPaddress)

        # Ping should return 0 if the package was successful
        if response is not 0 and time() - lastRebootTime > maxWaitTimeSeconds:

            print 'Unable to raise response from ntu at ' + ntuIPaddress
            print 'Will toggle ntu lines to enable a restart'
        
            # Turn computer power off, it's probably already off
            print 'Turning off NTU SSD 12V'
            if testMode == False:
                system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_OFF))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Turn disk off
            print 'Turning off NTU SSD 5V'
            if testMode == False:
                system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_OFF))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Turn disk on
            print 'Turning on NTU SSD 5V'
            if testMode == False:
                system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_ON))
            sleep(2)
            if forceFlag:
                restartCalibd()

            #Turn computer back on
            print 'Turning on NTU SSD 12V'
            if testMode == False:
                system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_ON))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Reboot is now effectively issued, log it
            message = 'No response from NTU to ping. Issuing reboot command. '
            message += 'Will wait until NTU responds or ' + str(maxWaitTimeMinutes) + ' minutes.'

            if forceFlag:
                message += ' Warning! Manually restarting Calibd!'

            if testMode == True:
                message = 'TEST! ' + message
            print message

            appendLogFile(logFileName, message)

            lastRebootTime = time()
            notHeardBackSinceReboot = True

        elif notHeardBackSinceReboot == True and response == 0:
            # We got a response, log it!
            message = ''
            if firstTime == True:
                message = 'NTU responded to ping for the first time since checkNtu.py script started'
                firstTime == False
            else:
                message = 'NTU responded to ping for the first time since reboot command issued'
            appendLogFile(logFileName, message)
            notHeardBackSinceReboot = False
        else:
            pass
        sleep(5)

def restartCalibd():
    """
    Issues a respawn command to Calibd, this function is only called if 
    the checkNtu.py script is called with -f or --force.
    I don't feel good about myself, but you asked for this...
    """

    # Copied from flightSoft/common/include/anitaCommands.h
    CMD_RESPAWN_PROGS = 132
    CALIBD_ID_MASK = 0x004

    print 'checkNtu.py started with --force option! Manually respawning Calibd!'
    system('CmdTest ' + str(CMD_RESPAWN_PROGS) + ' ' + str(CALIBD_ID_MASK))
    sleep(2)
    return 0


def appendLogFile(logFileName, message):
    
    # Open the log file in append mode
    with file(logFileName, 'a') as logFile:
        logFile.write(str(datetime.now()) + '\t' + message + '\n')

if __name__ == '__main__':
    main(argv)
