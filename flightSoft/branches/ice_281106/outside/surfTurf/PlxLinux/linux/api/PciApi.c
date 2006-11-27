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
 *     PlxApi.c
 *
 * Description:
 *
 *     This file contains all the PCI API functions.
 *
 * Revision:
 *
 *     09-01-03: PCI SDK v4.20
 *
 *****************************************************************************/


 /*****************************************************************************
  *
  *      Note on mappings to user virtual space & PLX software
  *     -------------------------------------------------------
  *
  * In Linux, the mapping of physically memory or PCI device memory,
  * involves use of the system call, mmap().  The mmap call, however,
  * provides only a limited amount of information to the driver when
  * the driver's mmap dispatch routine in invoked.
  *
  * For example, if attempting to map a PCI BAR space, there's nothing
  * provided in mmap to notify the driver which space to map, or even
  * what physical address to use for the mapping.
  *
  * To solve this and provide mappings for PCI spaces and physical memory,
  * the PLX API and driver use the custom "protocol" described below:
  *
  *   - The API call will determine the size of the area needed to be mapped.
  *
  *     This value depends upon the type of memory.  For PCI spaces, this
  *     involves getting the size of the space, plus dealing with
  *     non-page-aligned addresses.  For physical buffers, the buffer's
  *     physical address is provided by the PLX API call when either the
  *     buffer is allocated or requested (common buffer).
  *
  *   - mmap is called with the desired size.
  *
  *   - The driver will acknowledge the request and return no error, even
  *     though the actual mapping has not yet been performed.  Additionally,
  *     the driver will save the parameters of the mmap request in an
  *     internal list for later use.
  *
  *   - The API will now have a virtual address range to use for the mapping.
  *     The next step involves calling the driver with the information
  *     necessary to complete the request.  This information is dependent
  *     upon the type of memory to be mapped.
  *
  *   - The driver will then search its internal list to find the parameters
  *     from the previous call to mmap.  It will use these parameters along
  *     with the additional information provided by the API library to
  *     perform the desired mapping.
  *
  ****************************************************************************/


#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <asm/page.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "PciRegs.h"
#include "PlxApi.h"
#include "PlxIoctl.h"




/**********************************************
 *               Definitions
 *********************************************/
// API Version information
#define API_VERSION_MAJOR               4
#define API_VERSION_MINOR               2
#define API_VERSION_REVISION            0

#define MAX_DMA_TIMEOUT                 1000         // Max timeout in milliseconds for DMA transfer
#define DRIVER_PATH                     "/dev/plx/"  // Path to drivers


// List of PLX drivers to search for
static char *PlxDrivers[] = 
{
    "Pci9080",
    "Pci9054",
    "Pci9030",
    "Pci9050",
    "Pci9056",
    "Pci9656",
    "0"          // Must be last item to mark end of list
};


typedef struct _PCI_BAR_SPACE
{
    U32     va;                                  // Virtual address of space
    U32     size;                                // Size of mapped region
    U32     offset;                              // Actual starting offset from virtual base
    BOOLEAN bMapAttempted;                       // Flag to specify if mapping was attempted 
} PCI_BAR_SPACE;


typedef struct _DEVICE_NODE
{
    HANDLE               Handle;                 // Handle to the device driver
    DEVICE_LOCATION      Location;               // Device location
    PCI_BAR_SPACE        PciSpace[PCI_NUM_BARS]; // Virtual address mappings for the device
    PCI_MEMORY           PciMemory;              // Address of the common buffer
    struct _DEVICE_NODE *pNext;                  // Pointer to next device in the list
} DEVICE_NODE;




/**********************************************
 *           In-line Macros
 *********************************************/
#define LockGlobals()
#define UnlockGlobals()
#define DmaIndex(DmaChannel)         (DmaChannel - IopChannel2)




/**********************************************
 *          Portability Macros
 *********************************************/
#define CloseHandle(hdl)         close((int)(hdl))

#define IoMessage(hdl, cmd, pBuff, Size) \
    ioctl(          \
        (int)(hdl), \
        (cmd),      \
        pBuff       \
        )




/**********************************************
 *               Globals
 *********************************************/
static DEVICE_NODE *pApiDeviceList = NULL;   // Pointer to list of open devices and their resources




/**********************************************
 *       Private Function Prototypes
 *********************************************/
DEVICE_NODE *
DeviceListAdd(
    HANDLE           hDevice,
    DEVICE_LOCATION *pLocation
    );

BOOLEAN
DeviceListRemove(
    HANDLE hDevice
    );

DEVICE_NODE *
DeviceListFind(
    HANDLE hDevice
    );




/**********************************************
 *            Public Functions
 *********************************************/

/******************************************************************************
 *
 * Function   :  PlxSdkVersion
 *
 * Description:  Return the API Library version number
 *
 *****************************************************************************/
RETURN_CODE
PlxSdkVersion(
    U8 *VersionMajor,
    U8 *VersionMinor,
    U8 *VersionRevision
    )
{
    if ((VersionMajor    == NULL) ||
        (VersionMinor    == NULL) ||
        (VersionRevision == NULL))
    {
        return ApiNullParam;
    }

    *VersionMajor    = API_VERSION_MAJOR;
    *VersionMinor    = API_VERSION_MINOR;
    *VersionRevision = API_VERSION_REVISION;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxDriverVersion
 *
 * Description:  Return the driver version number
 *
 *****************************************************************************/
RETURN_CODE
PlxDriverVersion(
    HANDLE  hDevice,
    U8     *VersionMajor,
    U8     *VersionMinor,
    U8     *VersionRevision
    )
{
    IOCTLDATA IoMsg;


    if ((VersionMajor    == NULL) ||
        (VersionMinor    == NULL) ||
        (VersionRevision == NULL))
    {
        return ApiNullParam;
    }

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_DRIVER_VERSION,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *VersionMajor    = (U8)((IoMsg.u.MgmtData.value >> 16) & 0xFF);
    *VersionMinor    = (U8)((IoMsg.u.MgmtData.value >>  8) & 0xFF);
    *VersionRevision = (U8)((IoMsg.u.MgmtData.value >>  0) & 0xFF);

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxChipTypeGet
 *
 * Description:  Get the PLX Chip type and revision
 *
 *****************************************************************************/
RETURN_CODE
PlxChipTypeGet(
    HANDLE  hDevice,
    U32    *pChipType,
    U8     *pRevision
    )
{
    IOCTLDATA IoMsg;


    if ((pChipType == NULL) ||
        (pRevision == NULL))
    {
        return ApiNullParam;
    }

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMessage(
        hDevice,
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
 * Function   :  PlxPciBoardReset
 *
 * Description:  Resets a PCI board using the software reset feature.
 *
 *****************************************************************************/
VOID
PlxPciBoardReset(
    HANDLE hDevice
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_BOARD_RESET,
        &IoMsg,
        0
        );
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryAllocate
 *
 * Description:  Allocate a physically contigous page-locked buffer
 *
 *****************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryAllocate(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo,
    BOOLEAN     bSmallerOk
    )
{
    IOCTLDATA IoMsg;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.data[0]     = (U32)bSmallerOk;
    IoMsg.u.MiscData.u.PciMemory = *pMemoryInfo;

    IoMessage(
        hDevice,
        PLX_IOCTL_PHYSICAL_MEM_ALLOCATE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Copy buffer information
    *pMemoryInfo = IoMsg.u.MiscData.u.PciMemory;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFree
 *
 * Description:  Free a previously allocated physically contigous buffer
 *
 *****************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryFree(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo
    )
{
    IOCTLDATA IoMsg;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    // Unmap the buffer if it was previously mapped to user space
    if (pMemoryInfo->UserAddr != (U32)NULL)
    {
        PlxPciPhysicalMemoryUnmap(
            hDevice,
            pMemoryInfo
            );
    }

    IoMsg.u.MiscData.u.PciMemory = *pMemoryInfo;

    IoMessage(
        hDevice,
        PLX_IOCTL_PHYSICAL_MEM_FREE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Clear buffer information
    pMemoryInfo->PhysicalAddr = (U32)NULL;
    pMemoryInfo->Size         = 0;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryMap
 *
 * Description:  Maps a page-locked buffer into user virtual space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryMap(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo
    )
{
    IOCTLDATA IoMsg;


    /****************************************************
     * Please refer to the note at the top if this file
     * regarding mappings to user virtual space.
     ***************************************************/

    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Set default return value
    pMemoryInfo->UserAddr = (U32)NULL;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    // Verify buffer object
    if ((pMemoryInfo->PhysicalAddr == (U32)NULL) ||
        (pMemoryInfo->Size         == 0))
    {
        return ApiInvalidData;
    }

    // Get a valid virtual address
    pMemoryInfo->UserAddr =
        (U32)mmap(
                 0,
                 pMemoryInfo->Size,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 (int)hDevice,
                 0
                 );

    if (pMemoryInfo->UserAddr == (U32)MAP_FAILED)
    {
        pMemoryInfo->UserAddr = (U32)NULL;

        return ApiInsufficientResources;
    }

    // Send message to the driver to perform mapping
    IoMsg.u.MiscData.data[0]     = (U32)FALSE;   // Specify non-device memory
    IoMsg.u.MiscData.u.PciMemory = *pMemoryInfo;

    IoMessage(
        hDevice,
        PLX_IOCTL_PHYSICAL_MEM_MAP,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (IoMsg.ReturnCode != ApiSuccess)
    {
        // Unmap the region
        munmap(
            (void *)pMemoryInfo->UserAddr,
            pMemoryInfo->Size
            );

        // Clear address
        pMemoryInfo->UserAddr = (U32)NULL;

        return IoMsg.ReturnCode;
    }

    // Get virtual address
    pMemoryInfo->UserAddr = IoMsg.u.MiscData.u.PciMemory.UserAddr;

    return ApiSuccess;
}





/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryUnmap
 *
 * Description:  Unmaps a page-locked buffer from user virtual space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryUnmap(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo
    )
{
    int rc;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    // Verify virtual address
    if (pMemoryInfo->UserAddr == (U32)NULL)
    {
        return ApiInvalidAddress;
    }

    // Unmap buffer from virtual space
    rc =
        munmap(
            (void *)pMemoryInfo->UserAddr,
            pMemoryInfo->Size
            );

    if (rc != 0)
    {
        return ApiInvalidAddress;
    }

    // Clear buffer address
    pMemoryInfo->UserAddr = (U32)NULL;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxCommonBufferGet
 *
 * Description:  Retrieves the common buffer information for a given device
 *
 *****************************************************************************/
RETURN_CODE
PlxPciCommonBufferGet(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo
    )
{
    RETURN_CODE rc;


    // Get buffer properties
    rc =
        PlxPciCommonBufferProperties(
            hDevice,
            pMemoryInfo
            );

    if (rc != ApiSuccess)
        return rc;

    // Map buffer into user virtual space
    rc =
        PlxPciCommonBufferMap(
            hDevice,
            &(pMemoryInfo->UserAddr)
            );

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxCommonBufferProperties
 *
 * Description:  Retrieves the common buffer properties for a given device
 *
 *****************************************************************************/
RETURN_CODE
PlxPciCommonBufferProperties(
    HANDLE      hDevice,
    PCI_MEMORY *pMemoryInfo
    )
{
    IOCTLDATA    IoMsg;
    DEVICE_NODE *pDevice;


    if (pMemoryInfo == NULL)
        return ApiNullParam;

    // Verify the handle
    pDevice =
        DeviceListFind(
            hDevice
            );

    if (pDevice == NULL)
        return ApiInvalidHandle;

    LockGlobals();

    // If we already have the information, just copy it
    if (pDevice->PciMemory.PhysicalAddr != (U32)NULL)
    {
        *pMemoryInfo = pDevice->PciMemory;

        UnlockGlobals();

        return ApiSuccess;
    }

    UnlockGlobals();

    IoMessage(
        hDevice,
        PLX_IOCTL_COMMON_BUFFER_PROPERTIES,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pMemoryInfo = IoMsg.u.MiscData.u.PciMemory;

    LockGlobals();

    // Save the data for any future calls
    pDevice->PciMemory.PhysicalAddr = pMemoryInfo->PhysicalAddr;
    pDevice->PciMemory.Size         = pMemoryInfo->Size;

    UnlockGlobals();

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxCommonBufferMap
 *
 * Description:  Maps the common buffer into user virtual space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciCommonBufferMap(
    HANDLE  hDevice,
    VOID   *pVa
    )
{
    PCI_MEMORY   MemInfo;
    RETURN_CODE  rc;
    DEVICE_NODE *pDevice;


    /****************************************************
     * Please refer to the note at the top if this file
     * regarding mappings to user virtual space.
     ***************************************************/

    if (pVa == NULL)
        return ApiNullParam;

    // Set default return value
    *(U32*)pVa = (U32)NULL;

    // Verify the handle
    pDevice =
        DeviceListFind(
            hDevice
            );

    if (pDevice == NULL)
        return ApiInvalidHandle;


    // If buffer was previously mapped, just copy it
    if (pDevice->PciMemory.UserAddr != (U32)NULL)
    {
        *(U32*)pVa = pDevice->PciMemory.UserAddr;

        return ApiSuccess;
    }

    // Check for valid common buffer info
    if (pDevice->PciMemory.PhysicalAddr == (U32)NULL)
    {
        // Get buffer properties
        rc =
            PlxPciCommonBufferProperties(
                hDevice,
                &MemInfo
                );

        if (rc != ApiSuccess)
            return rc;
    }
    else
    {
        // Copy buffer properties
        MemInfo = pDevice->PciMemory;
    }

    rc =
        PlxPciPhysicalMemoryMap(
            hDevice,
            &MemInfo
            );

    if (rc != ApiSuccess)
    {
        *(U32*)pVa = (U32)NULL;

        return rc;
    }

    // Pass virtual address
    *(U32*)pVa = MemInfo.UserAddr;

    // Save the data for any future calls
    LockGlobals();

    pDevice->PciMemory.UserAddr = MemInfo.UserAddr;

    UnlockGlobals();

    return ApiSuccess;
}





/******************************************************************************
 *
 * Function   :  PlxCommonBufferUnmap
 *
 * Description:  Unmaps the common buffer from user virtual space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciCommonBufferUnmap(
    HANDLE  hDevice,
    VOID   *pVa
    )
{
    PCI_MEMORY   MemInfo;
    RETURN_CODE  rc;
    DEVICE_NODE *pDevice;


    if (pVa == NULL)
        return ApiNullParam;

    if (*(U32*)pVa == (U32)NULL)
        return ApiInvalidAddress;

    // Verify the handle
    pDevice =
        DeviceListFind(
            hDevice
            );

    if (pDevice == NULL)
        return ApiInvalidHandle;

    // Verify virtual address
    if (pDevice->PciMemory.UserAddr != *(U32*)pVa)
    {
        return ApiInvalidAddress;
    }

    // Copy buffer properties
    MemInfo = pDevice->PciMemory;

    rc =
        PlxPciPhysicalMemoryUnmap(
            hDevice,
            &MemInfo
            );

    if (rc == ApiSuccess)
    {
        // Clear internal data
        LockGlobals();

        pDevice->PciMemory.UserAddr = (U32)NULL;

        UnlockGlobals();

        // Clear buffer address
        *(U32*)pVa = (U32)NULL;
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxPciDeviceOpen
 *
 * Description:  This obtains a handle to a PLX device.  The first device that
 *               matches the criteria in the first argument will be selected.
 *
 *****************************************************************************/
RETURN_CODE
PlxPciDeviceOpen(
    DEVICE_LOCATION *pDevice,
    HANDLE          *pHandle
    )
{
    U8           VerMajor;
    U8           VerMinor;
    U8           VerRevision;
    U32          i;
    char         DriverName[25];
    IOCTLDATA    IoMsg;
    RETURN_CODE  rc;


    if ((pDevice == NULL) || (pHandle == NULL))
        return ApiNullParam;

    // Get the Serial number of the device, if not provided
    if (pDevice->SerialNumber[0] == '\0')
    {
        i = 0;

        // Search for the device matching the criteria
        rc =
            PlxPciDeviceFind(
                pDevice,
                &i
                );

        if (rc != ApiSuccess)
        {
            *pHandle = NULL;
            return rc;
        }
    }

    // If provided, the SerialNumber is sufficient to open a device
    sprintf(
        DriverName,
        DRIVER_PATH "%s",
        pDevice->SerialNumber
        );

    // Open the device
    *pHandle =
        (HANDLE *)open(
                      DriverName,
                      O_RDWR,
                      0666
                      );

    if (*pHandle < 0)
    {
        *pHandle = NULL;
        return ApiInvalidDeviceInfo;
    }

    // Add the handle to the device list.
    if (DeviceListAdd(
            *pHandle,
            pDevice
            ) == NULL)
    {
        CloseHandle(
            *pHandle
            );

        *pHandle = NULL;

        return ApiInsufficientResources;
    }

    // Verify the driver version
    PlxDriverVersion(
        *pHandle,
        &VerMajor,
        &VerMinor,
        &VerRevision
        );

    // Make sure the driver matches the DLL
    if ((VerMajor    != API_VERSION_MAJOR) ||
        (VerMinor    != API_VERSION_MINOR) ||
        (VerRevision != API_VERSION_REVISION))
    {
        // Versions don't match, return
        DeviceListRemove(
            *pHandle
            );

        // Close the handle
        CloseHandle(
            *pHandle
            );

        *pHandle = NULL;

        return ApiInvalidDriverVersion;
    }

    // Get device data from driver
    IoMessage(
        *pHandle,
        PLX_IOCTL_DEVICE_INIT,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Copy device information 
    *pDevice = IoMsg.u.MgmtData.u.Device;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciDeviceClose
 *
 * Description:  This closes a previously opened handle to a PCI device.  
 *
 *****************************************************************************/
RETURN_CODE
PlxPciDeviceClose(
    HANDLE hDevice
    )
{
    if (DeviceListRemove(
            hDevice
            ) == FALSE)
    {
        return ApiInvalidDeviceInfo;
    }

    // Close the handle
    CloseHandle(
        hDevice
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciDeviceFind
 *
 * Description:  This will search the driver's device extensions to find the 
 *               device that matches the input data.
 *
 *****************************************************************************/
RETURN_CODE
PlxPciDeviceFind(
    DEVICE_LOCATION *pDevice,
    U32             *pRequestLimit
    )
{
    U32       i;
    U32       totalDevFound;
    char      DriverName[25];
    HANDLE    hDevice;
    BOOLEAN   driverOpened;
    IOCTLDATA IoMsg;


    // Check for null pointers
    if (pDevice == NULL || pRequestLimit == NULL)
    {
        return ApiNullParam;
    }

    totalDevFound = 0;
    driverOpened  = FALSE;

    i = 0;

    // Scan through present drivers for matches
    while (PlxDrivers[i][0] != '0')
    {
        sprintf(
            DriverName,
            DRIVER_PATH "%s",
            PlxDrivers[i]
            );

        // Increment to next driver
        i++;

        // Open the driver's management interface
        hDevice =
            (HANDLE)open(
                        DriverName,
                        O_RDWR,
                        0666
                        );

        if (hDevice == (HANDLE)-1)
            continue;

        // Driver is open.  Find any devices that match

        driverOpened = TRUE;


        if (*pRequestLimit != FIND_AMOUNT_MATCHED)
            IoMsg.u.MgmtData.value = *pRequestLimit - totalDevFound;
        else
            IoMsg.u.MgmtData.value = FIND_AMOUNT_MATCHED;

        // Copy search criteria
        IoMsg.u.MgmtData.u.Device = *pDevice;

        IoMessage(
            hDevice,
            PLX_IOCTL_PCI_DEVICE_FIND,
            &IoMsg,
            sizeof(IOCTLDATA)
            );

        totalDevFound += IoMsg.u.MgmtData.value;

        if (*pRequestLimit != FIND_AMOUNT_MATCHED)
        {
            // Check to see if we have reached the specified device
            if (*pRequestLimit < totalDevFound)
            {
                *pDevice = IoMsg.u.MgmtData.u.Device;

                CloseHandle(
                    hDevice
                    );

                return ApiSuccess;
            }
        }

        CloseHandle(
            hDevice
            );
    }

    if (driverOpened == FALSE)
    {
        if (*pRequestLimit == FIND_AMOUNT_MATCHED)
        {
            *pRequestLimit = 0;
        }

        return ApiNoActiveDriver;
    }

    if (*pRequestLimit == FIND_AMOUNT_MATCHED)
    {
        *pRequestLimit = totalDevFound;
        if (totalDevFound == 0)
        {
            return ApiInvalidDeviceInfo;
        }
        else
        {
            return ApiSuccess;
        }
    }

    return ApiInvalidDeviceInfo;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarGet
 *
 * Description:  Returns the PCI BAR address of a PCI space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciBarGet(
    HANDLE   hDevice,
    U8       BarIndex,
    U32     *pPciBar,
    BOOLEAN *pFlag_IsIoSpace
    )
{
    IOCTLDATA IoMsg;


    if (pPciBar == NULL)
        return ApiNullParam;

    // Set default value
    *pPciBar = 0;

    // Verify BAR number
    switch (BarIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            break;

        default:
            return ApiInvalidIndex;
    }

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = BarIndex;

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_BAR_GET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pPciBar = IoMsg.u.MgmtData.value;

    if (pFlag_IsIoSpace != NULL)
        *pFlag_IsIoSpace = IoMsg.u.MgmtData.u.bFlag;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarRangeGet
 *
 * Description:  Returns the size of a PCI BAR space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciBarRangeGet(
    HANDLE  hDevice, 
    U8      BarIndex,
    U32    *pData
    )
{
    IOCTLDATA IoMsg;


    if (pData == NULL)
        return ApiNullParam;

    // Verify BAR number
    switch (BarIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            break;

        default:
            return ApiInvalidIndex;
    }

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = BarIndex;

    IoMessage(
        hDevice,
        PLX_IOCTL_PCI_BAR_RANGE_GET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pData = IoMsg.u.MgmtData.value;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarMap
 *
 * Description:  Maps a PCI BAR space into user virtual space.
 *
 *****************************************************************************/
RETURN_CODE
PlxPciBarMap(
    HANDLE  hDevice,
    U8      BarIndex,
    VOID   *pVa
    )
{
    U32            PciAddr;
    BOOLEAN        IsIoSpace;
    IOCTLDATA      IoMsg;
    RETURN_CODE    rc;
    DEVICE_NODE   *pDevice;
    PCI_BAR_SPACE  PciSpace;


    /****************************************************
     * Please refer to the note at the top if this file
     * regarding mappings to user virtual space.
     ***************************************************/

    if (pVa == NULL)
        return ApiNullParam;

    // Set default return value
    *(U32*)pVa = (U32)NULL;

    // Verify BAR number
    switch (BarIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            break;

        default:
            return ApiInvalidIndex;
    }

    // Verify the handle
    pDevice =
        DeviceListFind(
            hDevice
            );

    if (pDevice == NULL)
    {
        return ApiInvalidHandle;
    }

    // Check if mapping has already been performed
    if (pDevice->PciSpace[BarIndex].va != (U32)NULL)
    {
        *(U32*)pVa = pDevice->PciSpace[BarIndex].va;

        return ApiSuccess;
    }

    // Do not re-attempt failed mappings
    if (pDevice->PciSpace[BarIndex].bMapAttempted)
    {
        return ApiFailed;
    }

    // Flag map attempt
    pDevice->PciSpace[BarIndex].bMapAttempted = TRUE;

    // Get the size of the space
    rc =
        PlxPciBarRangeGet(
            hDevice, 
            BarIndex,
            &(PciSpace.size)
            );

    if ((rc != ApiSuccess) || (PciSpace.size == 0))
    {
        return ApiInvalidPciSpace;
    }

    // Calculate PCI BAR offset since mappings only happen at PAGE boundaries
    PlxPciBarGet(
        hDevice,
        BarIndex,
        &PciAddr,
        &IsIoSpace
        );

    if (IsIoSpace || (PciAddr == 0))
    {
        return ApiInvalidPciSpace;
    }

    // Calculate starting offset from page boundary
    PciSpace.offset = PciAddr & ((1 << PAGE_SHIFT) - 1);

    // Add offset to requested size
    PciSpace.size += PciSpace.offset;

    // Get a valid virtual address
    PciSpace.va =
        (U32)mmap(
                 0,
                 PciSpace.size,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 (int)hDevice,
                 0
                 );

    if (PciSpace.va == (U32)MAP_FAILED)
    {
        *(U32*)pVa = (U32)NULL;
        return ApiInsufficientResources;
    }

    // Send message to the driver to perform mapping
    IoMsg.u.MiscData.data[0]                  = (U32)TRUE;   // Specify device memory
    IoMsg.u.MiscData.u.PciMemory.PhysicalAddr = PciAddr & ~((1 << PAGE_SHIFT) - 1);
    IoMsg.u.MiscData.u.PciMemory.UserAddr     = PciSpace.va;
    IoMsg.u.MiscData.u.PciMemory.Size         = PciSpace.size;

    IoMessage(
        hDevice,
        PLX_IOCTL_PHYSICAL_MEM_MAP,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (IoMsg.ReturnCode != ApiSuccess)
    {
        // Unmap the region
        munmap(
            (void *)PciSpace.va,
            PciSpace.size
            );

        return IoMsg.ReturnCode;
    }

    // Calculate final virtual address
    *(U32*)pVa = PciSpace.va + PciSpace.offset;

    LockGlobals();

    // Copy values for later retrieval
    pDevice->PciSpace[BarIndex] = PciSpace;

    UnlockGlobals();

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarUnmap
 *
 * Description:  Unmaps a PCI BAR space from user virtual space
 *
 *****************************************************************************/
RETURN_CODE
PlxPciBarUnmap(
    HANDLE  hDevice,
    VOID   *pVa
    )
{
    int          rc;
    U8           BarIndex;
    U32          BarVa;
    DEVICE_NODE *pDevice;


    if (pVa == NULL)
        return ApiNullParam;

    if (*(U32*)pVa == (U32)NULL)
    {
        return ApiInvalidAddress;
    }

    // Verify the handle
    pDevice =
        DeviceListFind(
            hDevice
            );

    if (pDevice == NULL)
    {
        return ApiInvalidHandle;
    }

    LockGlobals();

    // Search for actual mapped address
    for (BarIndex = 0; BarIndex < PCI_NUM_BARS; BarIndex++)
    {
        // Calculate address provided to application
        BarVa = pDevice->PciSpace[BarIndex].va +
                pDevice->PciSpace[BarIndex].offset;

        // Unmap the space if match found
        if (*(U32*)pVa == BarVa)
        {
            rc =
                munmap(
                    (void *)pDevice->PciSpace[BarIndex].va,
                    pDevice->PciSpace[BarIndex].size
                    );

            // Clear internal data
            pDevice->PciSpace[BarIndex].va            = (U32)NULL;
            pDevice->PciSpace[BarIndex].size          = 0;
            pDevice->PciSpace[BarIndex].offset        = 0;
            pDevice->PciSpace[BarIndex].bMapAttempted = FALSE;

            UnlockGlobals();

            if (rc != 0)
            {
                return ApiInvalidAddress;
            }

            return ApiSuccess;
        }
    }

    return ApiInvalidAddress;
}




/******************************************************************************
 *
 * Function   :  PlxPciConfigRegisterRead
 *
 * Description:  Read a PCI configuration register
 *
 *****************************************************************************/
U32
PlxPciConfigRegisterRead(
    U8           bus,
    U8           slot,
    U16          offset,
    RETURN_CODE *pReturnCode
    )
{
    U8        i;
    U8        DriverName[25];
    HANDLE    hDevice;
    IOCTLDATA IoMsg;


    // Open any driver management interface for PCI register access
    i = 0;

    // Find a loaded driver
    while (PlxDrivers[i][0] != '0')
    {
        sprintf(
            DriverName,
            DRIVER_PATH "%s",
            PlxDrivers[i]
            );

        // Increment to next driver
        i++;

        // Open the driver's management interface
        hDevice =
            (HANDLE)open(
                        DriverName,
                        O_RDWR,
                        0666
                        );

        if (hDevice != (HANDLE)-1)
        {
            // A driver was found, perform PCI register access

            IoMsg.u.MgmtData.offset              = offset;
            IoMsg.u.MgmtData.u.Device.BusNumber  = bus;
            IoMsg.u.MgmtData.u.Device.SlotNumber = slot;

            IoMessage(
                hDevice,
                PLX_IOCTL_PCI_REGISTER_READ,
                &IoMsg,
                sizeof(IOCTLDATA)
                );

            if (pReturnCode != NULL)
                *pReturnCode = IoMsg.ReturnCode;

            CloseHandle(
                hDevice
                );

            return IoMsg.u.MgmtData.value;
        }
    }

    if (pReturnCode != NULL)
        *pReturnCode = ApiNoActiveDriver;

    return (U32)-1;
}




/******************************************************************************
 *
 * Function   :  PlxPciConfigRegisterWrite
 *
 * Description:  Writes to a PCI configuration register
 *
 *****************************************************************************/
RETURN_CODE
PlxPciConfigRegisterWrite(
    U8   bus,
    U8   slot,
    U16  offset,
    U32 *pData
    )
{
    U8        i;
    U8        DriverName[25];
    HANDLE    hDevice;
    IOCTLDATA IoMsg;


    if (pData == NULL)
        return ApiNullParam;

    // Open any driver management interface for PCI register access
    i = 0;

    // Find a loaded driver
    while (PlxDrivers[i][0] != '0')
    {
        sprintf(
            DriverName,
            DRIVER_PATH "%s",
            PlxDrivers[i]
            );

        // Increment to next driver
        i++;

        // Open the driver's management interface
        hDevice =
            (HANDLE)open(
                        DriverName,
                        O_RDWR,
                        0666
                        );

        if (hDevice != (HANDLE)-1)
        {
            // A driver was found, perform PCI register access

            IoMsg.u.MgmtData.u.Device.BusNumber  = bus;
            IoMsg.u.MgmtData.u.Device.SlotNumber = slot;
            IoMsg.u.MgmtData.offset              = offset;
            IoMsg.u.MgmtData.value               = *pData;

            IoMessage(
                hDevice,
                PLX_IOCTL_PCI_REGISTER_WRITE,
                &IoMsg,
                sizeof(IOCTLDATA)
                );

            CloseHandle(
                hDevice
                );

            return IoMsg.ReturnCode;
        }
    }

    return ApiNoActiveDriver;
}




/******************************************************************************
 *
 * Function   :  PlxPciRegisterRead_Unsupported
 *
 * Description:  Unsupported function to read a PCI register of any device
 *
 *****************************************************************************/
U32
PlxPciRegisterRead_Unsupported(
    U8           bus,
    U8           slot,
    U16          offset,
    RETURN_CODE *pReturnCode
    )
{
    U8        i;
    U8        DriverName[25];
    HANDLE    hDevice;
    IOCTLDATA IoMsg;


    /*****************************************************
     * Note: The access of PCI registers of a device in
     *       an arbitrary bus and slot may not be supported
     *       in true Plug 'n' Play Operating Systems.
     *
     *       In PLX PnP drivers, an unsupported function
     *       is provided which accesses PCI registers of
     *       any device in an arbitrary bus or slot.
     *
     *       **** WARNING **** WARNING **** WARNING ****
     *       This function may not be provided in future
     *       SDK releases.  There is also no guaratee that
     *       it will succeed in all systems.
     ****************************************************/


    // Open any driver management interface for PCI register access
    i = 0;

    // Find a loaded driver
    while (PlxDrivers[i][0] != '0')
    {
        sprintf(
            DriverName,
            DRIVER_PATH "%s",
            PlxDrivers[i]
            );

        // Increment to next driver
        i++;

        // Open the driver's management interface
        hDevice =
            (HANDLE)open(
                        DriverName,
                        O_RDWR,
                        0666
                        );

        if (hDevice != (HANDLE)-1)
        {
            // A driver was found, perform PCI register access

            IoMsg.u.MgmtData.offset              = offset;
            IoMsg.u.MgmtData.u.Device.BusNumber  = bus;
            IoMsg.u.MgmtData.u.Device.SlotNumber = slot;

            IoMessage(
                hDevice,
                PLX_IOCTL_PCI_REG_READ_UNSUPPORTED,
                &IoMsg,
                sizeof(IOCTLDATA)
                );

            if (pReturnCode != NULL)
                *pReturnCode = IoMsg.ReturnCode;

            CloseHandle(
                hDevice
                );

            return IoMsg.u.MgmtData.value;
        }
    }

    if (pReturnCode != NULL)
        *pReturnCode = ApiNoActiveDriver;

    return (U32)-1;
}




/******************************************************************************
 *
 * Function   :  PlxPciRegisterWrite_Unsupported
 *
 * Description:  Unsupported function to write a PCI register of any device
 *
 *****************************************************************************/
RETURN_CODE
PlxPciRegisterWrite_Unsupported(
    U8   bus,
    U8   slot,
    U16  offset,
    U32  value
    )
{
    U8        i;
    U8        DriverName[25];
    HANDLE    hDevice;
    IOCTLDATA IoMsg;


    /*****************************************************
     * Note: The access of PCI registers of a device in
     *       an arbitrary bus and slot may not be supported
     *       in true Plug 'n' Play Operating Systems.
     *
     *       In PLX PnP drivers, an unsupported function
     *       is provided which accesses PCI registers of
     *       any device in an arbitrary bus or slot.
     *
     *       **** WARNING **** WARNING **** WARNING ****
     *       This function may not be provided in future
     *       SDK releases.  There is also no guaratee that
     *       it will succeed in all systems.
     ****************************************************/

    // Open any driver management interface for PCI register access
    i = 0;

    // Find a loaded driver
    while (PlxDrivers[i][0] != '0')
    {
        sprintf(
            DriverName,
            DRIVER_PATH "%s",
            PlxDrivers[i]
            );

        // Increment to next driver
        i++;

        // Open the driver's management interface
        hDevice =
            (HANDLE)open(
                        DriverName,
                        O_RDWR,
                        0666
                        );

        if (hDevice != (HANDLE)-1)
        {
            // A driver was found, perform PCI register access

            IoMsg.u.MgmtData.u.Device.BusNumber  = bus;
            IoMsg.u.MgmtData.u.Device.SlotNumber = slot;
            IoMsg.u.MgmtData.offset              = offset;
            IoMsg.u.MgmtData.value               = value;

            IoMessage(
                hDevice,
                PLX_IOCTL_PCI_REG_WRITE_UNSUPPORTED,
                &IoMsg,
                sizeof(IOCTLDATA)
                );

            CloseHandle(
                hDevice
                );

            return IoMsg.ReturnCode;
        }
    }

    return ApiNoActiveDriver;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterRead
 *
 * Description:  This function reads a register from a opened device
 *
 *****************************************************************************/
U32
PlxRegisterRead(
    HANDLE       hDevice,
    U16          offset,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U32)-1;
    }

    // Copy data to buffer
    IoMsg.u.MgmtData.offset = offset;

    IoMessage(
        hDevice,
        PLX_IOCTL_REGISTER_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
       *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite
 *
 * Description:  This will write to a register of an opened device.
 *
 *****************************************************************************/
RETURN_CODE
PlxRegisterWrite(
    HANDLE hDevice,
    U16    offset,
    U32    value
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = offset;
    IoMsg.u.MgmtData.value  = value;

    IoMessage(
        hDevice,
        PLX_IOCTL_REGISTER_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterMailboxRead
 *
 * Description:  This will read the value at a given Mailbox register.
 *
 *****************************************************************************/
U32
PlxRegisterMailboxRead(
    HANDLE       hDevice,
    MAILBOX_ID   MailboxId,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U32)-1;
    }

    IoMsg.u.MgmtData.offset = (U16)MailboxId;

    IoMessage(
        hDevice,
        PLX_IOCTL_MAILBOX_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterMailboxWrite
 *
 * Description:  This will write a value to a given Mailbox register.
 *
 *****************************************************************************/
RETURN_CODE
PlxRegisterMailboxWrite(
    HANDLE     hDevice,
    MAILBOX_ID MailboxId,
    U32        value
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = (U16)MailboxId;
    IoMsg.u.MgmtData.value  = value;
    
    IoMessage(
        hDevice,
        PLX_IOCTL_MAILBOX_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterDoorbellRead
 *
 * Description:  This function returns the value of the last doorbell interrupt.
 *
 *****************************************************************************/
U32
PlxRegisterDoorbellRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U32)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_DOORBELL_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterDoorbellSet
 *
 * Description:  This writes a value to the PCI to Local Doorbell Register.
 *
 *****************************************************************************/
RETURN_CODE
PlxRegisterDoorbellSet(
    HANDLE hDevice,
    U32    value
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.value = value;

    IoMessage(
        hDevice,
        PLX_IOCTL_DOORBELL_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIntrEnable
 *
 * Description:  Enable specific interrupts of the PLX chip
 *
 *****************************************************************************/
RETURN_CODE
PlxIntrEnable(
    HANDLE    hDevice,
    PLX_INTR *pPlxIntr
    )
{
    IOCTLDATA IoMsg;


    // Check for null pointers
    if (pPlxIntr == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.u.IntrInfo = *pPlxIntr;

    IoMessage(
        hDevice,
        PLX_IOCTL_INTR_ENABLE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIntrDisable
 *
 * Description:  Disable specific interrupts of the PLX chip
 *
 *****************************************************************************/
RETURN_CODE
PlxIntrDisable(
    HANDLE    hDevice,
    PLX_INTR *pPlxIntr
    )
{
    IOCTLDATA IoMsg;


    // Check for null pointers
    if (pPlxIntr == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.u.IntrInfo = *pPlxIntr;

    IoMessage(
        hDevice,
        PLX_IOCTL_INTR_DISABLE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIntrAttach
 *
 * Description:  Registers an event for later interrupt notification
 *
 *****************************************************************************/
RETURN_CODE
PlxIntrAttach(
    HANDLE    hDevice,
    PLX_INTR  intrTypes,
    HANDLE   *pEventHdl
    )
{
    IOCTLDATA IoMsg;


    if (pEventHdl == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.u.IntrInfo = intrTypes;

    IoMessage(
        hDevice,
        PLX_IOCTL_INTR_ATTACH,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Return the event handle
    *pEventHdl = (HANDLE)IoMsg.u.MiscData.data[0];

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIntrWait
 *
 * Description:  Waits for an attached interrupt event
 *
 *****************************************************************************/
RETURN_CODE
PlxIntrWait(
    HANDLE hDevice,
    HANDLE hEvent,
    U32    Timeout_ms
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.data[0] = (U32)hEvent;
    IoMsg.u.MiscData.data[1] = Timeout_ms;

    // Send message to driver
    IoMessage(
        hDevice,
        PLX_IOCTL_INTR_WAIT,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIntrStatusGet
 *
 * Description:  This returns the status of the last interrupt events.
 *
 *****************************************************************************/
RETURN_CODE
PlxIntrStatusGet(
    HANDLE    hDevice,
    PLX_INTR *pPlxIntr
    )
{
    IOCTLDATA IoMsg;


    if (pPlxIntr == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_INTR_STATUS_GET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pPlxIntr = IoMsg.u.MiscData.u.IntrInfo;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPciAbortAddrRead
 *
 * Description:  Read PCI Abort address
 *
 *****************************************************************************/
U32
PlxPciAbortAddrRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U32)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_ABORTADDR_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxBusIopRead
 *
 * Description:  Reads data from a specified PCI BAR space
 *
 *****************************************************************************/
RETURN_CODE
PlxBusIopRead(
    HANDLE       hDevice,
    IOP_SPACE    IopSpace,
    U32          address,
    BOOLEAN      bRemap,
    VOID        *pBuffer,
    U32          ByteCount,
    ACCESS_TYPE  AccessType
    )
{
    IOCTLDATA IoMsg;


    if (pBuffer == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.BusIopData.IopSpace     = IopSpace;
    IoMsg.u.BusIopData.Address      = address;
    IoMsg.u.BusIopData.bRemap       = bRemap;
    IoMsg.u.BusIopData.TransferSize = ByteCount;
    IoMsg.u.BusIopData.AccessType   = AccessType;
    IoMsg.u.BusIopData.Buffer       = (U32)pBuffer;

    IoMessage(
        hDevice,
        PLX_IOCTL_BUS_IOP_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxBusIopWrite
 *
 * Description:  Writes data to a specified PCI BAR space
 *
 *****************************************************************************/
RETURN_CODE
PlxBusIopWrite(
    HANDLE       hDevice,
    IOP_SPACE    IopSpace,
    U32          address,
    BOOLEAN      bRemap,
    VOID        *pBuffer,
    U32          ByteCount,
    ACCESS_TYPE  AccessType
    )
{
    IOCTLDATA IoMsg;


    if (pBuffer == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.BusIopData.IopSpace     = IopSpace;
    IoMsg.u.BusIopData.Address      = address;
    IoMsg.u.BusIopData.bRemap       = bRemap;
    IoMsg.u.BusIopData.TransferSize = ByteCount;
    IoMsg.u.BusIopData.AccessType   = AccessType;
    IoMsg.u.BusIopData.Buffer       = (U32)pBuffer;

    IoMessage(
        hDevice,
        PLX_IOCTL_BUS_IOP_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIoPortRead
 *
 * Description:  Reads a single data from a specified I/O port
 *
 *****************************************************************************/
RETURN_CODE
PlxIoPortRead(
    HANDLE       hDevice,
    U32          IoPort,
    ACCESS_TYPE  AccessType,
    VOID        *pValue
    )
{
    IOCTLDATA IoMsg;


    if (pValue == NULL)
       return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.BusIopData.Address    = IoPort;
    IoMsg.u.BusIopData.AccessType = AccessType;

    IoMessage(
        hDevice,
        PLX_IOCTL_IO_PORT_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Copy the data
    switch (AccessType)
    {
        case BitSize8:
            *(U8*)pValue = (U8)(IoMsg.u.BusIopData.Buffer);
            break;

        case BitSize16:
            *(U16*)pValue = (U16)(IoMsg.u.BusIopData.Buffer);
            break;

        case BitSize32:
            *(U32*)pValue = (U32)(IoMsg.u.BusIopData.Buffer);
            break;

        default:
            return ApiInvalidAccessType;
    }

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxIoPortWrite
 *
 * Description:  Writes a single data to a specified I/O port
 *
 *****************************************************************************/
RETURN_CODE
PlxIoPortWrite(
    HANDLE       hDevice,
    U32          IoPort,
    ACCESS_TYPE  AccessType,
    VOID        *pValue
    )
{
    IOCTLDATA IoMsg;


    if (pValue == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    switch (AccessType)
    {
        case BitSize8:
            (U8)(IoMsg.u.BusIopData.Buffer) = *(U8*)pValue;
            break;

        case BitSize16:
            (U16)(IoMsg.u.BusIopData.Buffer) = *(U16*)pValue;
            break;

        case BitSize32:
            (U32)(IoMsg.u.BusIopData.Buffer) = *(U32*)pValue;
            break;

        default:
            return ApiInvalidAccessType;
    }

    IoMsg.u.BusIopData.Address    = IoPort;
    IoMsg.u.BusIopData.AccessType = AccessType;

    IoMessage(
        hDevice,
        PLX_IOCTL_IO_PORT_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxSerialEepromPresent
 *
 * Description:  Returns TRUE if a serial EEPROM device is detected.
 *
 * Note       :  A blank EEPROM may return FALSE depending upon PLX chip type.
 *
 *****************************************************************************/
BOOLEAN
PlxSerialEepromPresent(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return FALSE;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_EEPROM_PRESENT,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.u.bFlag;
}




/******************************************************************************
 *
 * Function   :  PlxSerialEepromRead
 *
 * Description:  Read the configuration EEPROM of a PCI device
 *
 *****************************************************************************/
RETURN_CODE
PlxSerialEepromRead(
    HANDLE       hDevice,
    EEPROM_TYPE  EepromType,
    U32         *pBuffer,
    U32          size
    )
{
    IOCTLDATA IoMsg;


    if (pBuffer == NULL)
        return ApiNullParam;

    if (size == 0)
        return ApiInvalidSize;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.data[0]      = (U32)pBuffer;
    IoMsg.u.MiscData.data[1]      = size;
    IoMsg.u.MiscData.u.EepromType = EepromType;

    IoMessage(
        hDevice,
        PLX_IOCTL_EEPROM_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxSerialEepromReadByOffset
 *
 * Description:  Read a value from a specified offset of the EEPROM
 *
 *****************************************************************************/
RETURN_CODE
PlxSerialEepromReadByOffset(
    HANDLE  hDevice,
    U16     offset,
    U32    *pValue
    )
{
    IOCTLDATA IoMsg;


    if (pValue == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = offset;

    IoMessage(
        hDevice,
        PLX_IOCTL_EEPROM_READ_BY_OFFSET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pValue = IoMsg.u.MgmtData.value;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxSerialEepromWrite
 *
 * Description:  Write to the configuration EEPROM of a PCI device
 *
 *****************************************************************************/
RETURN_CODE
PlxSerialEepromWrite(
    HANDLE       hDevice,
    EEPROM_TYPE  EepromType,
    U32         *pBuffer,
    U32          size
    )
{
    IOCTLDATA IoMsg;


    if (pBuffer == NULL)
        return ApiNullParam;

    if (size == 0)
        return ApiInvalidSize;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MiscData.data[0]      = (U32)pBuffer;
    IoMsg.u.MiscData.data[1]      = size;
    IoMsg.u.MiscData.u.EepromType = EepromType;

    IoMessage(
        hDevice,
        PLX_IOCTL_EEPROM_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxSerialEepromWriteByOffset
 *
 * Description:  Write a value to a specified offset of the EEPROM
 *
 *****************************************************************************/
RETURN_CODE
PlxSerialEepromWriteByOffset(
    HANDLE  hDevice,
    U16     offset,
    U32     value
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = offset;
    IoMsg.u.MgmtData.value  = value;

    IoMessage(
        hDevice,
        PLX_IOCTL_EEPROM_WRITE_BY_OFFSET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaControl
 *
 * Description:  Perform a Command on a specified DMA channel.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaControl(
    HANDLE      hDevice,
    DMA_CHANNEL channel,
    DMA_COMMAND command
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel   = channel;
    IoMsg.u.DmaData.u.command = command;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_CONTROL,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaStatus
 *
 * Description:  Read the status of a DMA transfer.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaStatus(
    HANDLE      hDevice,
    DMA_CHANNEL channel
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel = channel;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_STATUS,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockChannelOpen
 *
 * Description:  Open a channel to perform DMA Block transfers.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaBlockChannelOpen(
    HANDLE            hDevice,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc
    )
{
    IOCTLDATA IoMsg;


    if (pDesc == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel = channel;
    IoMsg.u.DmaData.u.desc  = *pDesc;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_BLOCK_CHANNEL_OPEN,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockTransfer
 *
 * Description:  Start a Block DMA transfer
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaBlockTransfer(
    HANDLE                hDevice,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *pDmaData,
    BOOLEAN               returnImmediate
    )
{
    HANDLE      hEvent;
    PLX_INTR    PlxIntr;
    IOCTLDATA   IoMsg;
    RETURN_CODE rc;


    if (pDmaData == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    // Added to avoid compiler warning
    hEvent = (HANDLE)-1;

    if (returnImmediate == FALSE)
    {
        // Clear interrupt fields
        memset(
            &PlxIntr,
            0,
            sizeof(PLX_INTR)
            );

        switch (channel)
        {
            case PrimaryPciChannel0:
                PlxIntr.PciDmaChannel0 = 1;
                break;

            case PrimaryPciChannel1:
                PlxIntr.PciDmaChannel1 = 1;
                break;

            case PrimaryPciChannel2:
                PlxIntr.PciDmaChannel2 = 1;
                break;

            case PrimaryPciChannel3:
                PlxIntr.DmaChannel3 = 1;
                break;

            default:
                // Added to avoid compiler warning
                break;
        }

        // Register to wait for DMA interrupt
        PlxIntrAttach(
            hDevice,
            PlxIntr,
            &hEvent
            );
    }

    IoMsg.u.DmaData.channel    = channel;
    IoMsg.u.DmaData.u.TxParams = *pDmaData;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_BLOCK_TRANSFER,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Wait for completion if requested
    if ((IoMsg.ReturnCode == ApiSuccess) &&
        (returnImmediate  == FALSE))
    {
        rc =
            PlxIntrWait(
                hDevice,
                hEvent,
                MAX_DMA_TIMEOUT
                );

        switch (rc)
        {
            case ApiSuccess:
                // DMA transfer completed
                return ApiSuccess;

            case ApiWaitTimeout:
                return ApiPciTimeout;

            case ApiWaitCanceled:
                return ApiFailed;

            default:
                // Added to avoid compiler warning
                break;
        }
    }

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockChannelClose
 *
 * Description:  Close a previously opened DMA channel.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaBlockChannelClose(
    HANDLE      hDevice,
    DMA_CHANNEL channel
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel = channel;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_BLOCK_CHANNEL_CLOSE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglChannelOpen
 *
 * Description:  Open a channel to perform sgl transfers.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaSglChannelOpen(
    HANDLE            hDevice,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc
    )
{
    IOCTLDATA IoMsg;


    if (pDesc == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel = channel;
    IoMsg.u.DmaData.u.desc  = *pDesc;    

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_SGL_OPEN,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglTransfer
 *
 * Description:  Start an SGL DMA transfer
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaSglTransfer(
    HANDLE                hDevice,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *pDmaData,
    BOOLEAN               returnImmediate
    )
{
    HANDLE      hEvent;
    PLX_INTR    PlxIntr;
    IOCTLDATA   IoMsg;
    RETURN_CODE rc;


    if (pDmaData == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    // Added to avoid compiler warning
    hEvent = (HANDLE)-1;

    if (returnImmediate == FALSE)
    {
        // Clear interrupt fields
        memset(
            &PlxIntr,
            0,
            sizeof(PLX_INTR)
            );

        switch (channel)
        {
            case PrimaryPciChannel0:
                PlxIntr.PciDmaChannel0 = 1;
                break;

            case PrimaryPciChannel1:
                PlxIntr.PciDmaChannel1 = 1;
                break;

            case PrimaryPciChannel2:
                PlxIntr.PciDmaChannel2 = 1;
                break;

            case PrimaryPciChannel3:
                PlxIntr.DmaChannel3 = 1;
                break;

            default:
                // Added to avoid compiler warning
                break;
        }

        // Register to wait for DMA interrupt
        PlxIntrAttach(
            hDevice,
            PlxIntr,
            &hEvent
            );
    }

    IoMsg.u.DmaData.channel    = channel;
    IoMsg.u.DmaData.u.TxParams = *pDmaData;
    IoMsg.ReturnCode           = ApiSuccess;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_SGL_TRANSFER,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    // Wait for completion if requested
    if ((IoMsg.ReturnCode == ApiSuccess) &&
        (returnImmediate  == FALSE))
    {
        rc =
            PlxIntrWait(
                hDevice,
                hEvent,
                MAX_DMA_TIMEOUT
                );

        switch (rc)
        {
            case ApiSuccess:
                // DMA transfer completed
                return ApiSuccess;

            case ApiWaitTimeout:
                return ApiPciTimeout;

            case ApiWaitCanceled:
                return ApiFailed;

            default:
                // Added to avoid compiler warning
                break;
        }
    }

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglChannelClose
 *
 * Description:  Close a previously opened DMA channel.
 *
 *****************************************************************************/
RETURN_CODE
PlxDmaSglChannelClose(
    HANDLE      hDevice,
    DMA_CHANNEL channel
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.DmaData.channel = channel;

    IoMessage(
        hDevice,
        PLX_IOCTL_DMA_SGL_CLOSE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxMuInboundPortRead
 *
 * Description:  Get a Message Frame. 
 *
 *****************************************************************************/
RETURN_CODE
PlxMuInboundPortRead(
    HANDLE  hDevice,
    U32    *pFrame
    )
{
    IOCTLDATA IoMsg;


    if (pFrame == NULL)
        return ApiNullParam;
    
    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_MU_INBOUND_PORT_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pFrame = IoMsg.u.MgmtData.value;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxMuInboundPortWrite
 *
 * Description:  Write a message frame to the Inbound Port.
 *
 *****************************************************************************/
RETURN_CODE
PlxMuInboundPortWrite(
    HANDLE  hDevice,
    U32    *pFrame
    )
{
    IOCTLDATA IoMsg;


    if (pFrame == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.value = *pFrame;

    IoMessage(
        hDevice,
        PLX_IOCTL_MU_INBOUND_PORT_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxMuOutboundPortRead
 *
 * Description:  Retrieve a posted message frame.
 *
 *****************************************************************************/
RETURN_CODE
PlxMuOutboundPortRead(
    HANDLE  hDevice,
    U32    *pFrame
    )
{
    IOCTLDATA IoMsg;


    if (pFrame == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_MU_OUTBOUND_PORT_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    *pFrame = IoMsg.u.MgmtData.value;

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxMuOutboundPortWrite
 *
 * Description:  Post a message to the Outbound Port.
 *
 *****************************************************************************/
RETURN_CODE
PlxMuOutboundPortWrite(
    HANDLE  hDevice,
    U32    *pFrame
    )
{
    IOCTLDATA IoMsg;


    if (pFrame == NULL)
        return ApiNullParam;

    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.value = *pFrame;

    IoMessage(
        hDevice,
        PLX_IOCTL_MU_OUTBOUND_PORT_WRITE,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPowerLevelGet
 *
 * Description:  Retrieve the Power Level from a PCI device
 *
 *****************************************************************************/
PLX_POWER_LEVEL
PlxPowerLevelGet(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL) 
            *pReturnCode = ApiInvalidHandle;
        return (PLX_POWER_LEVEL)(U32)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_POWER_LEVEL_GET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL) 
        *pReturnCode = IoMsg.ReturnCode;

    return (PLX_POWER_LEVEL)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxPowerLevelSet
 *
 * Description:  Set the power level of a PCI device.
 *
 *****************************************************************************/
RETURN_CODE
PlxPowerLevelSet(
    HANDLE          hDevice,
    PLX_POWER_LEVEL PowerLevel
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.value = (U32)PowerLevel;

    IoMessage(
        hDevice,
        PLX_IOCTL_POWER_LEVEL_SET,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    return IoMsg.ReturnCode;
}




/******************************************************************************
 *
 * Function   :  PlxPmIdRead
 *
 * Description:  Read the Power Management ID value.
 *
 *****************************************************************************/
U8
PlxPmIdRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_PM_ID_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxPmNcpRead
 *
 * Description:  Read the PowerManagement Next Capablilities Pointer value
 *
 *****************************************************************************/
U8
PlxPmNcpRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_PM_NCP_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxHotSwapIdRead
 *
 * Description:  Read the Hot Swap ID value.
 *
 *****************************************************************************/
U8
PlxHotSwapIdRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_HS_ID_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxHotSwapNcpRead
 *
 * Description:  Read the HotSwap Next Capablilities Pointer value
 *
 *****************************************************************************/
U8
PlxHotSwapNcpRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_HS_NCP_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxHotSwapStatus
 *
 * Description:  Report the status of the HotSwap Register.
 *
 *****************************************************************************/
U8
PlxHotSwapStatus(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_HS_STATUS,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxVpdIdRead
 *
 * Description:  Read the Vital Product Data ID value.
 *
 *****************************************************************************/
U8
PlxVpdIdRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_VPD_ID_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxVpdNcpRead
 *
 * Description:  Read the Vital Product Data Next Capablilities Pointer
 *
 *****************************************************************************/
U8
PlxVpdNcpRead(
    HANDLE       hDevice,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U8)-1;
    }

    IoMessage(
        hDevice,
        PLX_IOCTL_VPD_NCP_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return (U8)(IoMsg.u.MgmtData.value);
}




/******************************************************************************
 *
 * Function   :  PlxVpdRead
 *
 * Description:  Read the Vpd Data
 *
 *****************************************************************************/
U32
PlxVpdRead(
    HANDLE       hDevice,
    U16          offset,
    RETURN_CODE *pReturnCode
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidHandle;
        return (U32)-1;
    }

    IoMsg.u.MgmtData.offset = offset;

    IoMessage(
        hDevice,
        PLX_IOCTL_VPD_READ,
        &IoMsg,
        sizeof(IOCTLDATA)
        );

    if (pReturnCode != NULL)
        *pReturnCode = IoMsg.ReturnCode;

    return IoMsg.u.MgmtData.value;
}




/******************************************************************************
 *
 * Function   :  PlxVpdWrite
 *
 * Description:  Write to the VPD location
 *
 *****************************************************************************/
RETURN_CODE
PlxVpdWrite(
    HANDLE hDevice,
    U16    offset,
    U32    value
    )
{
    IOCTLDATA IoMsg;


    // Verify the handle
    if (DeviceListFind(
            hDevice
            ) == NULL)
    {
        return ApiInvalidHandle;
    }

    IoMsg.u.MgmtData.offset = offset;
    IoMsg.u.MgmtData.value  = value;

    IoMessage(
        hDevice,
        PLX_IOCTL_VPD_WRITE,
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

/**********************************************
*          Device List Functions
**********************************************/

/******************************************************************************
 *
 * Function   :  DeviceListAdd
 *
 * Description:  Add a new node to the list with the provided handle
 *
 * Returns    :  Pointer    - to newly added node
 *               NULL       - if error 
 *
 *****************************************************************************/
DEVICE_NODE *
DeviceListAdd(
    HANDLE           hDevice,
    DEVICE_LOCATION *pLocation
    )
{
    U8           i;
    DEVICE_NODE *pCurrentDevice;
    DEVICE_NODE *pPreviousDevice;


    LockGlobals();

    pPreviousDevice = pApiDeviceList;

    if (pApiDeviceList != NULL)
    {
        while (pPreviousDevice->pNext != NULL)
        {
            pPreviousDevice = pPreviousDevice->pNext;
        }
    }

    // Allocate a new node
    pCurrentDevice = (DEVICE_NODE *)malloc(sizeof(DEVICE_NODE));

    if (pCurrentDevice == NULL)
    {
        UnlockGlobals();
        return NULL;
    }

    // Fill in new data
    pCurrentDevice->Handle = hDevice;
    pCurrentDevice->pNext  = NULL;

    pCurrentDevice->Location = *pLocation;

    pCurrentDevice->PciMemory.UserAddr     = (U32)NULL;
    pCurrentDevice->PciMemory.PhysicalAddr = (U32)NULL;
    pCurrentDevice->PciMemory.Size         = 0;

    for (i = 0; i < PCI_NUM_BARS; i++)
    {
        pCurrentDevice->PciSpace[i].va            = (U32)NULL;
        pCurrentDevice->PciSpace[i].size          = 0;
        pCurrentDevice->PciSpace[i].offset        = 0;
        pCurrentDevice->PciSpace[i].bMapAttempted = FALSE;
    }

    // Adjust the pointers
    if (pApiDeviceList == NULL)
    {
        pApiDeviceList = pCurrentDevice;
    }
    else
    {
        pPreviousDevice->pNext = pCurrentDevice;
    }

    // Release access to globals
    UnlockGlobals();

    return pCurrentDevice;
}




/******************************************************************************
 *
 * Function   :  DeviceListRemove
 *
 * Description:  Remove the node in the list associated with a handle
 *
 * Returns    :  TRUE    - if sucessful
 *               FALSE   - if unable to remove/find node
 *
 *****************************************************************************/
BOOLEAN
DeviceListRemove(
    HANDLE hDevice
    )
{
    U8           i;
    BOOLEAN      DeviceFound;
    DEVICE_NODE *pCurrentDevice;
    DEVICE_NODE *pPreviousDevice;


    pCurrentDevice =
        DeviceListFind(
            hDevice
            );

    if (pCurrentDevice == NULL)
        return FALSE;

    // Unmap any leftover virtual addresses
    for (i = 0; i < PCI_NUM_BARS; i++)
    {
        PlxPciBarUnmap(
            hDevice,
            &(pCurrentDevice->PciSpace[i].va)
            );
    }

    // Unmap the common buffer
    if (pCurrentDevice->PciMemory.UserAddr != (U32)NULL)
    {
        PlxPciCommonBufferUnmap(
            hDevice,
            &(pCurrentDevice->PciMemory)
            );
    }

    LockGlobals();

    // Traverse device list to remove device
    pCurrentDevice = pApiDeviceList;

    // Added to prevent compiler warning
    pPreviousDevice = pCurrentDevice;

    // Search for the node in the Device List
    DeviceFound = FALSE;
    do
    {
        if (pCurrentDevice->Handle == hDevice)
            DeviceFound = TRUE;
        else
        {
            // Check for end of list
            if (pCurrentDevice->pNext == NULL)
            {
                UnlockGlobals();
                return FALSE;
            }

            pPreviousDevice = pCurrentDevice;
            pCurrentDevice  = pCurrentDevice->pNext;
        }
    }
    while (DeviceFound == FALSE);

    // Remove device from the list
    if (pApiDeviceList == pCurrentDevice)
    {
        pApiDeviceList = pCurrentDevice->pNext;
    }
    else
    {
        pPreviousDevice->pNext = pCurrentDevice->pNext;
    }

    // Release access to globals
    UnlockGlobals();

    // Release memory
    free(
        pCurrentDevice
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  DeviceListFind
 *
 * Description:  Locate the node in the list associated with a handle
 *
 * Returns    :  Pointer   - if sucessful
 *               NULL      - if unable to find node
 *
 *****************************************************************************/
DEVICE_NODE *
DeviceListFind(
    HANDLE hDevice
    )
{
    DEVICE_NODE *pCurrentDevice;


    LockGlobals();

    pCurrentDevice = pApiDeviceList;

    // Search for the node in the Device List
    while (pCurrentDevice != NULL)
    {
        if (pCurrentDevice->Handle == hDevice)
        {
            UnlockGlobals();
            return pCurrentDevice;
        }

        pCurrentDevice = pCurrentDevice->pNext;
    }

    UnlockGlobals();

    return NULL;
}
