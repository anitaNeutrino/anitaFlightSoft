/*******************************************************************************
 * Copyright (c) 2003 PLX Technology, Inc.
 * 
 * PLX Technology Inc. licenses this software under specific terms and
 * conditions.  Use of any of the software or derviatives thereof in any
 * product without a PLX Technology chip is strictly prohibited. 
 * 
 * PLX Technology, Inc. provides this software AS IS, WITHANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHLIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 * 
 * NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  NO EVENT SHALL PLX'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO PLX FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 *****************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *     PciDrvApi.c
 *
 * Description:
 *
 *     This file contains all the PLX PCI Driver API functions.
 *
 * Revision:
 *
 *     10-01-03: PCI SDK v4.20
 *
 *****************************************************************************/


#include "PciDrvApi.h"
#include "Plx.h"
#include "PlxIoctl.h"




/**********************************************
 *               Definitions
 *********************************************/
// API Version information
#define API_VERSION_MAJOR               4
#define API_VERSION_MINOR               2
#define API_VERSION_REVISION            0

// Device object validity codes
#define PLX_TAG_VALID                   0x5F504C58         // "_PLX" in Hex
#define PLX_TAG_INVALID                 0x564F4944         // "VOID" in Hex

#if defined(_WIN32)
    #define PLX_PCI_DRIVER_NAME         "\\\\.\\PlxPci"    // Registered link name of driver
#else
    #define PLX_PCI_DRIVER_NAME         "/dev/plx/PlxPci"  // Registered link name of driver
#endif




/**********************************************
 *                 Macros
 *********************************************/
#define ObjectValidate(pObj)        ((pObj)->IsValidTag = PLX_TAG_VALID)
#define ObjectInvalidate(pObj)      ((pObj)->IsValidTag = PLX_TAG_INVALID)
#define IsObjectValid(pObj)         ((pObj)->IsValidTag == PLX_TAG_VALID)

#if defined(_WIN32)
    #define Driver_Disconnect       CloseHandle

    #define IoMessage(hdl, cmd, pBuff, Size) \
        DeviceIoControl(    \
            (hdl),          \
            (cmd),          \
            (pBuff),        \
            (Size),         \
            (pBuff),        \
            (Size),         \
            &BytesReturned, \
            NULL            \
            )
#else
    #define Driver_Disconnect       close

    #define IoMessage(hdl, cmd, pBuff, Size) \
        ioctl(          \
            (int)(hdl), \
            (cmd),      \
            pBuff       \
            )
#endif




/**********************************************
 *                Globals
 *********************************************/
#if defined(_WIN32)
    U32 BytesReturned;          // Required by DeviceIoControl when lpOverlapped is NULL
#endif




/**********************************************
 *       Private Function Prototypes
 *********************************************/
static BOOLEAN
Driver_Connect(
    PLX_DRIVER_HANDLE *pHandle
    );




#if defined(_WIN32)
/*****************************************************************************
 *
 * Function   :  DllMain
 *
 * Description:  DllMain is called by Windows each time a process or
 *               thread attaches or detaches to/from this DLL.
 *
 *****************************************************************************/
BOOLEAN WINAPI
DllMain(
    HANDLE hInst, 
    U32    ReasonForCall,
    LPVOID lpReserved
    )
{
    // Added to prevent compiler warning
    if (hInst == INVALID_HANDLE_VALUE)
    {
    }

    if (lpReserved == NULL)
    {
    }

    switch (ReasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            // No need to do anything when threads are attached
            break;

        case DLL_THREAD_DETACH:
            // No need to do anything when threads are detached
            break;

        default:
            break;
    }

    return TRUE;
}
#endif




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceOpen
 *
 * Description:  Selects a PCI device
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_DeviceOpen(
    PLX_DEVICE_KEY    *pKey,
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8          VerMajor;
    U8          VerMinor;
    U8          VerRevision;
    RETURN_CODE rc;


    if ((pDevice == NULL) || (pKey == NULL))
        return ApiNullParam;

    // Mark object as invalid
    ObjectInvalidate(pDevice);

    // Copy key information
    pDevice->Key = *pKey;

    // Fill in any missing key information
    if (!IsObjectValid(pKey))
    {
        rc =
            PlxPci_DeviceFind(
                &(pDevice->Key),
                0               // First matching device
                );

        if (rc != ApiSuccess)
        {
            return rc;
        }
    }

    // Connect to driver
    if (Driver_Connect(
            &pDevice->hDevice
            ) == FALSE)
    {
        return ApiInvalidDeviceInfo;
    }

    // Mark object as valid
    ObjectValidate(pDevice);

    // Verify the driver version
    PlxPci_DriverVersion(
        pDevice,
        &VerMajor,
        &VerMinor,
        &VerRevision
        );

    // Make sure the driver matches the DLL
    if ((VerMajor    != API_VERSION_MAJOR) ||
        (VerMinor    != API_VERSION_MINOR) ||
        (VerRevision != API_VERSION_REVISION))
    {
        // Close the handle
        Driver_Disconnect(
            pDevice->hDevice
            );

        // Mark object as invalid
        ObjectInvalidate(pDevice);

        return ApiInvalidDriverVersion;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceClose
 *
 * Description:  This closes a previously opened device
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_DeviceClose(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    if (pDevice == NULL)
        return ApiNullParam;

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    // Close the handle
    Driver_Disconnect(
        pDevice->hDevice
        );

    // Mark object as invalid
    ObjectInvalidate(pDevice);

    // Mark object key as invalid
    ObjectInvalidate(&(pDevice->Key));

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceFind
 *
 * Description:  Locates a specific device or returns total number matched
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_DeviceFind(
    PLX_DEVICE_KEY *pKey,
    U8              DeviceNumber
    )
{
    IOCTLDATA         IoMsg;
    PLX_DRIVER_HANDLE hDevice;


    // Check for null pointers
    if (pKey == NULL)
    {
        return ApiNullParam;
    }

    // Connect to driver
    if (Driver_Connect(
            &hDevice
            ) == FALSE)
    {
        return ApiNoActiveDriver;
    }

    // Copy search criteria
    IoMsg.u.MgmtData.u.Key = *pKey;
    IoMsg.u.MgmtData.value = DeviceNumber;

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_DEVICE_FIND,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Release driver connection
    Driver_Disconnect(
        hDevice
        );

    // Copy match result if successful
    if (IoMsg.ReturnCode == ApiSuccess)
    {
        // Copy device key information
        *pKey = IoMsg.u.MgmtData.u.Key;

        // Validate key
        ObjectValidate(
            pKey
            );
    }

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_ApiVersion
 *
 * Description:  Return the API Library version number
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_ApiVersion(
    U8 *pVersionMajor,
    U8 *pVersionMinor,
    U8 *pVersionRevision
    )
{
    if ((pVersionMajor    == NULL) ||
        (pVersionMinor    == NULL) ||
        (pVersionRevision == NULL))
    {
        return ApiNullParam;
    }

    *pVersionMajor    = API_VERSION_MAJOR;
    *pVersionMinor    = API_VERSION_MINOR;
    *pVersionRevision = API_VERSION_REVISION;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DriverVersion
 *
 * Description:  Return the driver version number
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_DriverVersion(
    PLX_DEVICE_OBJECT *pDevice,
    U8                *pVersionMajor,
    U8                *pVersionMinor,
    U8                *pVersionRevision
    )
{
    IOCTLDATA IoMsg;


    if ((pVersionMajor    == NULL) ||
        (pVersionMinor    == NULL) ||
        (pVersionRevision == NULL))
    {
        return ApiNullParam;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_DRIVER_VERSION,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pVersionMajor    = (U8)((IoMsg.u.MgmtData.value >> 16) & 0xFF);
    *pVersionMinor    = (U8)((IoMsg.u.MgmtData.value >>  8) & 0xFF);
    *pVersionRevision = (U8)((IoMsg.u.MgmtData.value >>  0) & 0xFF);

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_ChipTypeGet
 *
 * Description:  Get the PLX Chip type and revision
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_ChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice,
    U32               *pChipType,
    U8                *pRevision
    )
{
    IOCTLDATA IoMsg;


    if ((pChipType == NULL) ||
        (pRevision == NULL))
    {
        return ApiNullParam;
    }

    // Verify device object
    if (!IsObjectValid(pDevice))
        return ApiInvalidDeviceInfo;

    IoMsg.u.MgmtData.u.Key = pDevice->Key;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_CHIP_TYPE_GET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pChipType = IoMsg.u.MiscData.data[0];
    *pRevision = (U8)(IoMsg.u.MiscData.data[1]);

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPci_DeviceReset
 *
 * Description:  Resets a PCI board using the software reset feature.
 *
 *****************************************************************************/
VOID
PlxPci_DeviceReset(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    // Verify device object
    if (!IsObjectValid(pDevice))
        return;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_PCI_BOARD_RESET,
        NULL,
        0
        );
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterRead
 *
 * Description:  This is used to read a PCI configuration register
 *
 *****************************************************************************/
U32
PlxPci_PciRegisterRead(
    U8           bus,
    U8           slot,
    U8           function,
    U16          offset,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA         IoMsg;
    PLX_DRIVER_HANDLE hDevice;


    // Connect to driver
    if (Driver_Connect(
            &hDevice
            ) == FALSE)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiNoActiveDriver;

        return (U32)-1;
    }

    IoMsg.u.MgmtData.u.Key.bus      = bus;
    IoMsg.u.MgmtData.u.Key.slot     = slot;
    IoMsg.u.MgmtData.u.Key.function = function;
    IoMsg.u.MgmtData.offset         = offset;

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_REGISTER_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    Driver_Disconnect(
        hDevice
        );

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxPci_PciRegisterWrite
 *
 * Description:  This is used to write to a PCI configuration register
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_PciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    IOCTLDATA         IoMsg;
    PLX_DRIVER_HANDLE hDevice;


    // Connect to driver
    if (Driver_Connect(
            &hDevice
            ) == FALSE)
    {
        return ApiNoActiveDriver;
    }

    IoMsg.u.MgmtData.u.Key.bus      = bus;
    IoMsg.u.MgmtData.u.Key.slot     = slot;
    IoMsg.u.MgmtData.u.Key.function = function;
    IoMsg.u.MgmtData.offset         = offset;
    IoMsg.u.MgmtData.value          = value;

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_REGISTER_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    Driver_Disconnect(
        hDevice
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromPresent
 *
 * Description:  Returns TRUE if a serial EEPROM device is detected.
 *
 * Note       :  A blank EEPROM may return FALSE depending upon PLX chip type.
 *
 *****************************************************************************/
BOOLEAN
PlxPci_EepromPresent(
    PLX_DEVICE_OBJECT *pDevice,
    RETURN_CODE       *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the device
    if (!IsObjectValid(pDevice))
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidDeviceInfo;
        return FALSE;
    }

    IoMsg.u.MgmtData.u.Key = pDevice->Key;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_EEPROM_PRESENT,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (BOOLEAN)IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromReadByOffset
 *
 * Description:  Read a value from a specified offset of the EEPROM
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_EepromReadByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U16               *pValue
    )
{
    IOCTLDATA IoMsg;


    if (pValue == NULL)
        return ApiNullParam;

    // Verify the device
    if (!IsObjectValid(pDevice))
    {
        return ApiInvalidDeviceInfo;
    }

    // Provide necessary information
    IoMsg.u.MgmtData.u.Key  = pDevice->Key;
    IoMsg.u.MgmtData.offset = offset;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (IoMsg.ReturnCode == ApiSuccess)
    {
        *pValue = (U16)IoMsg.u.MgmtData.value;
    }
    else
    {
        *pValue = (U16)-1;
    }

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPci_EepromWriteByOffset
 *
 * Description:  Write a value to a specified offset of the EEPROM
 *
 *****************************************************************************/
RETURN_CODE
PlxPci_EepromWriteByOffset(
    PLX_DEVICE_OBJECT *pDevice,
    U16                offset,
    U16                value
    )
{
    IOCTLDATA IoMsg;


    // Verify the device
    if (!IsObjectValid(pDevice))
    {
        return ApiInvalidDeviceInfo;
    }

    // Provide necessary information
    IoMsg.u.MgmtData.u.Key  = pDevice->Key;
    IoMsg.u.MgmtData.offset = offset;
    IoMsg.u.MgmtData.value  = value;

    IoMessage(
        pDevice->hDevice,
        PLX_IOCTL_EEPROM_WRITE_BY_OFFSET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/***********************************************************
*
*                  PRIVATE FUNCTIONS
*
***********************************************************/


/******************************************************************************
 *
 * Function   :  Driver_Connect
 *
 * Description:  Attempts to connect a registered driver by link name
 *
 * Returns    :  TRUE   - Driver was found and connected to
 *               FALSE  - Driver not found
 *
 *****************************************************************************/
BOOLEAN
Driver_Connect(
    PLX_DRIVER_HANDLE *pHandle
    )
{
#if defined(_WIN32)

    char          DriverName[30];
    OSVERSIONINFO VerInfo;


    // Set size of structure
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // Get OS version information
    if (GetVersionEx(
            &VerInfo
            ) == FALSE)
    {
        return FALSE;
    }

    // Build driver name
    strcpy(
        DriverName,
        PLX_PCI_DRIVER_NAME
        );

    // If Win9x class OS, add ".VxD" extension for PCI service
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        strcat(
            DriverName,
            ".vxd"
            );
    }

    // Open the device
    *pHandle =
        CreateFile(
            DriverName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_DELETE_ON_CLOSE,
            NULL
            );

    if (*pHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

#else

    // Open the device
    *pHandle =
        open(
            PLX_PCI_DRIVER_NAME,
            O_RDWR,
            0666
            );

    if (*pHandle < 0)
    {
        return FALSE;
    }

#endif

    return TRUE;
}
