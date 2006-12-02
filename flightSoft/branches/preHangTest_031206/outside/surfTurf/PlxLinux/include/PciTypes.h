#ifndef __PCITYPES_H
#define __PCITYPES_H

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
 *      PciTypes.h
 *
 * Description:
 *
 *      This file defines the basic types available to the PCI code.
 *
 * Revision:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "PlxError.h"


#if defined(WDM_DRIVER)
    #include <wdm.h>            // WDM Driver types
#endif

#if defined(NT_DRIVER)
    #include <ntddk.h>          // NT Kernel Mode Driver types
#endif

#if defined(VXD_DRIVER)
    #include <basedef.h>        // Win9x/Me VxD Driver types
#endif

#if defined(_WIN32) && !defined(PLX_DRIVER)
    #include <wtypes.h>         // Windows application level types
#endif

#if defined(PLX_LINUX)
    #include <memory.h>         // To prevent application compile errors in Linux
#endif

#if defined(PLX_LINUX) || defined(LINUX_DRIVER)
    #include <linux/types.h>    // Linux types
#endif


#ifdef __cplusplus
extern "C" {
#endif



/******************************************
 *   Linux Application Level Definitions
 ******************************************/
#if defined(PLX_LINUX)
    typedef __s8             S8,  *PS8;
    typedef __u8             U8,  *PU8;
    typedef __s16            S16, *PS16;
    typedef __u16            U16, *PU16;
    typedef __s32            S32, *PS32;
    typedef __u32            U32, *PU32;
    typedef __s64            LONGLONG;
    typedef __u64            ULONGLONG;

    typedef void            *HANDLE;
    typedef int              PLX_DRIVER_HANDLE;     // Linux-specific driver handle
#endif



/******************************************
 *    Linux Kernel Level Definitions
 ******************************************/
#if defined(LINUX_DRIVER)
    typedef s8               S8,  *PS8;
    typedef u8               U8,  *PU8;
    typedef s16              S16, *PS16;
    typedef u16              U16, *PU16;
    typedef s32              S32, *PS32;
    typedef u32              U32, *PU32;
    typedef s64              LONGLONG;
    typedef u64              ULONGLONG;

    typedef void            *HANDLE;
    typedef int              PLX_DRIVER_HANDLE;     // Linux-specific driver handle
#endif



/******************************************
 *      Windows Type Definitions
 ******************************************/
#if defined(_WIN32)
    typedef signed char      S8,  *PS8;
    typedef unsigned char    U8,  *PU8;
    typedef signed short     S16, *PS16;
    typedef unsigned short   U16, *PU16;
    typedef signed long      S32, *PS32;
    typedef unsigned long    U32, *PU32;
    typedef signed _int64    LONGLONG;
    typedef unsigned _int64  ULONGLONG;

    typedef HANDLE           PLX_DRIVER_HANDLE;     // Windows-specific driver handle
#endif



/******************************************
 *    Volatile Basic Type Definitions
 ******************************************/
typedef volatile S8           VS8, *PVS8;
typedef volatile U8           VU8, *PVU8;
typedef volatile S16          VS16, *PVS16;
typedef volatile U16          VU16, *PVU16;
typedef volatile S32          VS32, *PVS32;
typedef volatile U32          VU32, *PVU32;
typedef volatile LONGLONG     VLONGLONG;
typedef volatile ULONGLONG    VULONGLONG;



/******************************************
 *      Miscellaneous definitions
 ******************************************/
#if !defined(VOID)
    typedef void              VOID, *PVOID;
#endif

typedef U8                    BOOLEAN;

#if !defined(_WIN32)
    typedef U8                BOOL;
#endif

#if !defined(NULL)
    #define NULL              ((VOID *) 0x0)
#endif

#if !defined(TRUE)
    #define TRUE              1
#endif

#if !defined(FALSE)
    #define FALSE             0
#endif



/******************************************
 *          64-bit Structures
 ******************************************/
typedef union _S64
{
    struct
    {
    #if defined(PLX_LITTLE_ENDIAN)
        U32  LowPart;
        S32  HighPart;
    #else
        S32  HighPart;
        U32  LowPart;
    #endif
    }u;

    LONGLONG QuadPart;
} S64;


typedef union _U64
{
    struct
    {
    #if defined(PLX_LITTLE_ENDIAN)
        U32  LowPart;
        U32  HighPart;
    #else
        U32  HighPart;
        U32  LowPart;
    #endif
    }u;

    ULONGLONG QuadPart;
} U64;


typedef union _VS64
{
    struct
    {
    #if defined(PLX_LITTLE_ENDIAN)
        VU32  LowPart;
        VS32  HighPart;
    #else
        VS32  HighPart;
        VU32  LowPart;
    #endif
    }u;

    VLONGLONG QuadPart;
} VS64;


typedef union _VU64
{
    struct
    {
    #if defined(PLX_LITTLE_ENDIAN)
        VU32  LowPart;
        VU32  HighPart;
    #else
        VU32  HighPart;
        VU32  LowPart;
    #endif
    }u;

    VULONGLONG QuadPart;
} VU64;



/******************************************
 * Definitions for 64-bit code compatibility
 ******************************************/
typedef U32                 ADDRESS;
typedef S32                 SDATA;
typedef U32                 UDATA;



/******************************************
 * PCI SDK Defined Structures
 ******************************************/

// Device Location Structure
typedef struct _DEVICE_LOCATION
{
    U8  BusNumber;
    U8  SlotNumber;
    U16 DeviceId;
    U16 VendorId;
    U8  SerialNumber[12];
} DEVICE_LOCATION;

// Virtual Addresses Structure
typedef struct _VIRTUAL_ADDRESSES
{
    U32 Va0;
    U32 Va1;
    U32 Va2;
    U32 Va3;
    U32 Va4;
    U32 Va5;
    U32 VaRom;
} VIRTUAL_ADDRESSES;

// PCI Memory Structure
typedef struct _PCI_MEMORY
{
    U32 UserAddr;
    U32 PhysicalAddr;
    U32 Size;
} PCI_MEMORY;

// PCI Device Key Identifier
typedef struct _PLX_DEVICE_KEY
{
    U32 IsValidTag;                  // Magic number to determine validity
    U8  bus;                         // Physical device location
    U8  slot;
    U8  function;
    U16 VendorId;                    // Device Identifier
    U16 DeviceId;
    U16 SubVendorId;
    U16 SubDeviceId;
    U8  Revision;
    U8  ApiIndex;                    // Index used internally by the API
    U8  DeviceNumber;                // Number used internally by the device driver
} PLX_DEVICE_KEY;

// PLX Device Object Structure
typedef struct _PLX_DEVICE_OBJECT
{
    U32                IsValidTag;   // Magic number to determine validity
    PLX_DEVICE_KEY     Key;          // Device location key identifier
    PLX_DRIVER_HANDLE  hDevice;      // Handle to driver
} PLX_DEVICE_OBJECT;



#ifdef __cplusplus
}
#endif

#endif
