#ifndef _PCI_DEVICE_H
#define _PCI_DEVICE_H

/******************************************************************************
 *
 * File Name:
 *
 *      PciDevice.h
 *
 * Description:
 *
 *      Definitions for PCI functions
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/

#define MATCH_BASE_EXACT          ((U16)1 << 15)
#define MATCH_BASE_GENERIC        ((U16)1 << 12)
#define MATCH_BASE                MATCH_BASE_EXACT | MATCH_BASE_GENERIC

#define MATCH_SUBCLASS_EXACT      ((U16)1 << 11)
#define MATCH_SUBCLASS_GENERIC    ((U16)1 <<  8)
#define MATCH_SUBCLASS            MATCH_SUBCLASS_EXACT | MATCH_SUBCLASS_GENERIC

#define MATCH_INTERFACE_EXACT     ((U16)1 << 7)
#define MATCH_INTERFACE_GENERIC   ((U16)1 << 4)
#define MATCH_INTERFACE           MATCH_INTERFACE_EXACT | MATCH_INTERFACE_GENERIC

// Function number passed to driver in upper 3-bits of slot
#define PCI_DEVFN(slot, fn)       ((char)((((char)(fn)) << 5) | ((char)(slot) & 0x1f)))


typedef enum _PCI_ACCESS_METHOD
{
    PCI_ACCESS_UNKNOWN,
    PCI_ACCESS_DRIVER,
    PCI_ACCESS_SERVICE
} PCI_ACCESS_METHOD;


typedef struct _DEVICE_NODE
{
    PLX_DEVICE_KEY       Key;
    DEVICE_LOCATION      Location;                      // Device location information
    PCI_ACCESS_METHOD    PciMethod;                     // Which API to access device?
    U32                  PlxChipType;                   // PLX Chip type
    U8                   PlxRevision;                   // PLX Chip revision
    U8                   PciHeaderType;                 // Type of PCI Config registers
    U32                  PciClass;                      // PCI Device Class codes
    BOOLEAN              bSelected;                     // Flag to specify if device is selected
    U32                  Va_PciBar[6];                  // Virtual addresses of PCI BAR spaces
    struct _DEVICE_NODE *pNext;                         // Pointer to next node in device list
} DEVICE_NODE;




/*************************************
 *            Globals
 ************************************/
extern DEVICE_NODE *pDeviceList;




/*************************************
 *            Functions
 ************************************/
void
DeviceListCreate(
    void
    );

void
DeviceListDisplay(
    void
    );

void
DeviceListFree(
    void
    );

DEVICE_NODE *
DeviceNodeAdd(
    U8 bus,
    U8 slot,
    U8 function
    );

BOOL
DeviceNodeExist(
    U8 bus,
    U8 slot,
    U8 function
    );

void
DevicePropertiesFill(
    DEVICE_NODE *pNode
    );

DEVICE_NODE*
GetDevice(
    U8      index,
    BOOLEAN bPlxOnly
    );

char*
Device_GetClassString(
    U32 PciClass
    );

RETURN_CODE
PlxCm_DeviceOpen(
    DEVICE_NODE *pNode
    );

RETURN_CODE
PlxCm_DeviceClose(
    DEVICE_NODE *pNode
    );

RETURN_CODE
PlxCm_PciRegisterRead(
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    );

RETURN_CODE
PlxCm_PciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    );

BOOLEAN
PlxCm_EepromPresent(
    RETURN_CODE *pReturnCode
    );

RETURN_CODE
PlxCm_EepromReadByOffset(
    U16  offset,
    U32 *pValue
    );

RETURN_CODE
PlxCm_EepromWriteByOffset(
    U16 offset,
    U32 value
    );



#endif
