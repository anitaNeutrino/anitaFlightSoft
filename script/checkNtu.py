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
from datetime import datetime

def main(args):

    # You can now call this with a force option, which will issue restart commands to calibd
    forceFlag = False
    for arg in args:
        if arg == '-f' or arg == '--force':
            forceFlag = True

    ntuIPaddress = '192.168.1.5' # The IP address to ping
    logFileName = 'checkNtu.log' # Timestamp of when restart was done

    # How long we should give the ntu to boot before toggling relays again
    maxSleepTimeMinutes = 15

    # Values copied from flightSoft/common/include/anitaCommand.h
    CMD_TURN_NTU_SSD_5V_ON = 158
    CMD_TURN_NTU_SSD_5V_OFF = 159
    CMD_TURN_NTU_SSD_12V_ON = 160
    CMD_TURN_NTU_SSD_12V_OFF = 161
    
    while True:

        # Ping ntu and get return value, this will happen a lot so keep the command line clear 
        # Options mean we wait 5 seconds between pings
        response = system('ping -i 6 -c 1 -w 5' + ntuIPaddress + ' > /dev/null')

        # Ping should return 0 if the package was successful
        if response is not 0:

            ntuDown = True

            print 'Unable to raise response from ntu at ' + ntuIPaddress
            print 'Will toggle ntu lines to enable a restart'
        
            # Turn computer power off, it's probably already off
            print 'Turning off NTU SSD 12V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_OFF))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Turn disk off
            print 'Turning off NTU SSD 5V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_OFF))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Turn disk on
            print 'Turning on NTU SSD 5V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_5V_ON))
            sleep(2)
            if forceFlag:
                restartCalibd()

            #Turn computer back on
            print 'Turning on NTU SSD 12V'
            system('CmdTest ' + str(CMD_TURN_NTU_SSD_12V_ON))
            sleep(2)
            if forceFlag:
                restartCalibd()

            # Reboot is now effectively issued, log it
            message = 'Toggled relays to issue reboot'
            if forceFlag:
                message += ' with force!'
            appendLogFile(logFileName, message)

            # Then wait a long time for the computer to load
            # Is this long enough, too long?
            sleep(maxSleepTimeMinutes*60)

        else: # Got response

            # Wait 5 seconds
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
