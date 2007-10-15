#ifndef _MONITOR_H
#define _MONITOR_H

/******************************************************************************
 *
 * File Name:
 *
 *      Monitor.h
 *
 * Description:
 *
 *      Definitions for the the monitor application
 *
 * Revision History:
 *
 *      05-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "PciDevice.h"
#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define MONITOR_VERSION_MAJOR       0       // Version information
#define MONITOR_VERSION_MINOR       9
#define MONITOR_VERSION_REVISION    6

#define MONITOR_PROMPT              ">"     // The monitor prompt string

#define MAX_BUFFER_SZ               255
#define MAX_CMD_SZ                  30
#define MAX_CMD_HISTORY             30

#if defined(PLX_LINUX)
    #define INVALID_HANDLE_VALUE    (HANDLE)-1
#endif




/**********************************************
 *                 Globals
 **********************************************/
extern HANDLE             Gbl_hDevice;
extern DEVICE_NODE       *pDeviceSelected;
extern PCI_MEMORY         Gbl_PciBufferInfo;
extern PLX_DEVICE_OBJECT  Gbl_DeviceObj;




/*************************************
 *            Functions
 ************************************/
void
Monitor(
    void
    );

BOOLEAN
ProcessCommand(
    char *buffer
    );

void
PciSpacesMap(
    void
    );

void
PciSpacesUnmap(
    void
    );

void
GetCommonBufferProperties(
    void
    );


#endif
