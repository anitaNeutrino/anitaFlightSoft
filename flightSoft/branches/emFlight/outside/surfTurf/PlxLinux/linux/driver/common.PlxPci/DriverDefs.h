#ifndef __DRIVER_DEFS_H
#define __DRIVER_DEFS_H

/*******************************************************************************
 * Copyright (c) 2003 PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this software under specific terms and
 * conditions.  Use of any of the software or derviatives thereof in any
 * product without a PLX Technology chip is strictly prohibited.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL PLX'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO PLX FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      DriverDefs.h
 *
 * Description:
 *
 *      Common definitions used in the driver.
 *
 * Revision History:
 *
 *      11-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "Plx.h"
#include "PlxTypes.h"
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_DRIVER_NAME                     "PlxPci"
#define PLX_MNGMT_INTERFACE                 0xff          // Minor number of Management interface
#define PLX_MAX_NAME_LENGTH                 0x20          // Max length of registered device name
#define PCI_MAX_BUSES_TO_SCAN               10            // Maximum number of PCI buses to scan for valid devices (NT only)
#define EEPROM_TEST_OFFSET                  0x50          // Offset to use for EEPROM present check

// Driver Version information
#define DRIVER_VERSION_MAJOR                4
#define DRIVER_VERSION_MINOR                2
#define DRIVER_VERSION_REVISION             0




/********************************************************
 * Macros to support Kernel-level logging in Debug builds
 *
 * NOTE:  The use of the ## operator is GNU-specific to
 *        support variable-length arguments in a macro.
 *******************************************************/
#if defined(PLX_DEBUG)
    #define ErrorPrintf(arg)                _Error_Print_Macro        arg
    #define DebugPrintf(arg)                _Debug_Print_Macro        arg
    #define DebugPrintf_NoInfo(arg)         _Debug_Print_NoInfo_Macro arg
#else
    #define ErrorPrintf(arg)                _Error_Print_Macro arg
    #define DebugPrintf(arg)
    #define DebugPrintf_NoInfo(arg)
#endif

#define _Error_Print_Macro(fmt, args...)         printk(KERN_WARNING PLX_DRIVER_NAME ": " fmt, ## args)
#define _Debug_Print_Macro(fmt, args...)         printk(KERN_DEBUG   PLX_DRIVER_NAME ": " fmt, ## args)
#define _Debug_Print_NoInfo_Macro(fmt, args...)  printk(KERN_DEBUG   "\b\b\b   \b\b\b" fmt, ## args)



// Macros for Memory access to/from user-space addresses
#define DEV_MEM_TO_USER_8( VaUser, VaDev, count)    Plx_dev_mem_to_user_8( (void*)(VaUser), (void*)(VaDev), (count))
#define DEV_MEM_TO_USER_16(VaUser, VaDev, count)    Plx_dev_mem_to_user_16((void*)(VaUser), (void*)(VaDev), (count))
#define DEV_MEM_TO_USER_32(VaUser, VaDev, count)    Plx_dev_mem_to_user_32((void*)(VaUser), (void*)(VaDev), (count))
#define USER_TO_DEV_MEM_8( VaDev, VaUser, count)    Plx_user_to_dev_mem_8( (void*)(VaDev), (void*)(VaUser), (count))
#define USER_TO_DEV_MEM_16(VaDev, VaUser, count)    Plx_user_to_dev_mem_16((void*)(VaDev), (void*)(VaUser), (count))
#define USER_TO_DEV_MEM_32(VaDev, VaUser, count)    Plx_user_to_dev_mem_32((void*)(VaDev), (void*)(VaUser), (count))



// Macros for I/O port access (single-unit access)
#define IO_PORT_READ_8(port)                        inb((port))
#define IO_PORT_READ_16(port)                       inw((port))
#define IO_PORT_READ_32(port)                       inl((port))
#define IO_PORT_WRITE_8(port, val)                  outb((val), (port))
#define IO_PORT_WRITE_16(port, val)                 outw((val), (port))
#define IO_PORT_WRITE_32(port, val)                 outl((val), (port))

// Macros for I/O port access (multi-unit access)
#define IO_PORT_READ_8_BUFFER(port, buf, len)       insb((port), (buf), (len))
#define IO_PORT_READ_16_BUFFER(port, buf, len)      insw((port), (buf), (len))
#define IO_PORT_READ_32_BUFFER(port, buf, len)      insl((port), (buf), (len))
#define IO_PORT_WRITE_8_BUFFER(port, val, len)      outsb((port), (buf), (len))
#define IO_PORT_WRITE_16_BUFFER(port, val, len)     outsw((port), (buf), (len))
#define IO_PORT_WRITE_32_BUFFER(port, val, len)     outsl((port), (buf), (len))



// Macro for code portability
#define RtlZeroMemory(pDest, ByteCount)            memset((pDest), 0, (ByteCount))
#define RtlCopyMemory(pDest, pSrc, ByteCount)      memcpy((pDest), (pSrc), (ByteCount))


// Device node information
typedef struct _PLX_DEVICE_NODE
{
    struct list_head  ListEntry;
    struct pci_dev   *pPciDevice;      // Pointer to OS-supplied PCI device information
    U32               PlxChipType;     // PLX chip type
    U8                Revision;        // PLX chip revision
    PLX_DEVICE_KEY    Key;             // Device location & identification
} PLX_DEVICE_NODE;


// All relevant information about the device
typedef struct _DEVICE_EXTENSION
{
    struct _DEVICE_OBJECT *pDeviceObject;          // Device object this extension belongs to
    struct list_head       List_SupportedDevices;  // List of detected supported devices
} DEVICE_EXTENSION; 


// Main driver object
typedef struct _DRIVER_OBJECT
{
    struct _DEVICE_OBJECT  *DeviceObject;     // Pointer to first device in list
    U8                      DeviceCount;      // Number of devices in list
    spinlock_t              Lock_DeviceList;  // Spinlock for device list
    int                     MajorID;          // The OS-assigned driver Major ID
    struct file_operations  DispatchTable;    // Driver dispatch table
} DRIVER_OBJECT;


// The device object
typedef struct _DEVICE_OBJECT
{
    struct _DEVICE_OBJECT *NextDevice;       // Pointer to next device in list
    DRIVER_OBJECT         *DriverObject;     // Pointer to parent driver object
    DEVICE_EXTENSION      *DeviceExtension;  // Pointer to device information
    DEVICE_EXTENSION       DeviceInfo;       // Device information
} DEVICE_OBJECT;




/**********************************************
 *               Globals
 *********************************************/
extern DRIVER_OBJECT *pGbl_DriverObject;       // Pointer to the main driver object




#endif
