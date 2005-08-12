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


#include <linux/sched.h>
#include <linux/tqueue.h>
#include <asm/io.h>
#include "Plx.h"
#include "PlxTypes.h"
#include "PlxChip.h"
#include "Plx_sysdep.h"




/**********************************************
 *               Definitions
 *********************************************/
#define PLX_MNGMT_INTERFACE                 0xff  // Minor number of Management interface
#define PLX_MAX_NAME_LENGTH                 0x20  // Max length of registered device name
#define PCI_MAX_BUSES_TO_SCAN               10    // Maximum number of PCI buses to scan for valid devices (NT only)
#define MIN_WORKING_POWER_STATE	            D2    // Minimum state required for local register access

// Driver Version information
#define DRIVER_VERSION_MAJOR                4
#define DRIVER_VERSION_MINOR                2
#define DRIVER_VERSION_REVISION             0

// Used for build of SGL descriptors
#define SGL_DESC_IDX_PCI_ADDR               0
#define SGL_DESC_IDX_LOC_ADDR               1
#define SGL_DESC_IDX_COUNT                  2
#define SGL_DESC_IDX_NEXT_DESC              3


/***********************************************************
 * The following definition is required for drivers
 * requiring DMA functionality.  This includes allocation
 * of a DMA buffer and DMA support functions.
 **********************************************************/
#if defined(PCI9080) || defined(PCI9054)
    #define DMA_SUPPORT
#elif defined(PCI9056)  || defined(PCI9656)
    #define DMA_SUPPORT
#elif defined(PCI9050)  || defined(PCI9030)
    // No DMA support
#else
    #error DMA support not set properly for driver
#endif




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



// Macros for PLX chip register access
#define PLX_REG_READ(pdx, offset)                  readl(((U32)((pdx)->PciBar[0].pVa)) + (offset))
#define PLX_REG_WRITE(pdx, offset, value)          writel((value), ((U32)((pdx)->PciBar[0].pVa)) + (offset))



// Macro for code portability
#define RtlZeroMemory(pDest, ByteCount)            memset((pDest), 0, (ByteCount))



// PCI Interrupt Event Structure
typedef struct _INTR_WAIT_OBJECT
{
    struct list_head   list;
    VOID              *pOwner;
    U32                NotifyOnInterrupt;
    BOOLEAN            bPending;
    wait_queue_head_t  WaitQueue;
} INTR_WAIT_OBJECT;


// Information about contiguous, page-locked buffers
typedef struct _PLX_PHYSICAL_MEM
{
    struct list_head  list;
    VOID             *pOwner;
    VOID             *pKernelVa;
    U32               order;
    PCI_MEMORY        PciMem;
} PLX_PHYSICAL_MEM;


// User mapping request parameters
typedef struct _MAP_PARAMS_OBJECT
{
    struct list_head       list;
    VOID                  *pOwner;
    struct vm_area_struct  vma;
} MAP_PARAMS_OBJECT;


// PCI BAR Space information 
typedef struct _PCI_BAR_INFO
{
    U32     *pVa;                        // BAR Kernel Virtual Address
    U64      Physical;                   // BAR Physical Address
    U32      Size;                       // BAR size
    BOOLEAN  IsIoMapped;                 // Memory or I/O mapped?
} PCI_BAR_INFO;


// DMA channel information 
typedef struct _DMA_CHANNEL_INFO
{
    DMA_STATE          state;                  // DMA Channel open state
    VOID              *pOwner;                 // Object that requested to open the channel
    BOOLEAN            bPending;               // Flag to note if a DMA is pending
    BOOLEAN            bLocalAddrConstant;     // Flag to keep track if local address remains constant
    U32                NumIoBuff;              // Number of I/O buffers allocated for buffer
    struct kiobuf    **pIoVector;              // I/O vector containing I/O buffer pointers
    U32                BufferSize;             // Total size of the user buffer
    int                direction;              // The direction of the transfer
    PLX_PHYSICAL_MEM   pSgl;                   // Address of the current SGL descriptor list
} DMA_CHANNEL_INFO;


// Argument for interrupt synchronized register access functions
typedef struct _PLX_REG_DATA
{
    struct _DEVICE_EXTENSION *pdx;
    U16                       offset;
    U32                       BitsToSet;
    U32                       BitsToClear;
} PLX_REG_DATA;


// All relevant information about the device
typedef struct _DEVICE_EXTENSION
{
    struct _DEVICE_OBJECT *pDeviceObject;           // Pointer to parent device object
    struct pci_dev        *pPciDevice;              // Pointer to OS-supplied PCI device information
    PLX_POWER_LEVEL        Power;                   // Power state
    DEVICE_LOCATION        Device;                  // Device information
    PCI_BAR_INFO           PciBar[PCI_NUM_BARS];    // PCI BARs information

    spinlock_t             Lock_HwAccess;           // Spinlock used for register access

    U32                    InterruptSource;         // Source(s) of last interrupt
    U32                    IntrDoorbellValue;       // Last Doorbell interrupt value
    spinlock_t             Lock_Isr;                // Spinlock used to sync with ISR
    struct tq_struct       Task_DpcForIsr;          // Task queue used by ISR to queue DPC

    struct list_head       List_InterruptWait;      // List of registered interrupt wait events
    spinlock_t             Lock_InterruptWaitList;  // Spinlock for interrupt wait list

    struct list_head       List_PhysicalMem;        // List of user-allocated physical memory
    spinlock_t             Lock_PhysicalMemList;    // Spinlock for physical memory list

    struct list_head       List_MapParams;          // Saves mapping parameters from calls to mmap
    spinlock_t             Lock_MapParamsList;      // Spinlock for map parameters list

#if defined(DMA_SUPPORT)                            // DMA channel information
    DMA_CHANNEL_INFO       DmaInfo[NUMBER_OF_DMA_CHANNELS];
    spinlock_t             Lock_DmaChannel;         // Spinlock for DMA channel access
#endif

} DEVICE_EXTENSION; 


// Main driver object
typedef struct _DRIVER_OBJECT
{
    struct _DEVICE_OBJECT  *DeviceObject;     // Pointer to first device in list
    U8                      DeviceCount;      // Number of devices in list
    spinlock_t              Lock_DeviceList;  // Spinlock for device list
    int                     MajorID;          // The OS-assigned driver Major ID
    U16                     DriverType;       // Type of driver (9054, 9656, etc)
    PLX_PHYSICAL_MEM        CommonBuffer;     // Contiguous memory to be shared by all processes
    char                   *SupportedIDs;     // Dev/Ven IDs of devices supported
    struct file_operations  DispatchTable;    // Driver dispatch table
} DRIVER_OBJECT;


// The device object
typedef struct _DEVICE_OBJECT
{
    struct _DEVICE_OBJECT *NextDevice;       // Pointer to next device in list
    DRIVER_OBJECT         *DriverObject;     // Pointer to parent driver object
    U8                     OpenCount;        // Count of active open descriptors to the device
    spinlock_t             Lock_DeviceOpen;  // Spinlock for opening/closing the device
    BOOLEAN                Flag_started;     // Keeps track whether device has been started
    BOOLEAN                Flag_Interrupt;   // Keeps track whether device has claimed an IRQ
    DEVICE_EXTENSION      *DeviceExtension;              // Pointer to device information
    DEVICE_EXTENSION       DeviceInfo;       // Device information
} DEVICE_OBJECT;




/**********************************************
 *               Globals
 *********************************************/
extern DRIVER_OBJECT *pGbl_DriverObject;       // Pointer to the main driver object




/**********************************************
 *               Functions
 *********************************************/
BOOLEAN
PlxChipPciInterruptEnable(
    DEVICE_EXTENSION *pdx
    );

BOOLEAN
PlxChipPciInterruptDisable(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxPciBoardReset(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxChipSetInterruptNotifyFlags(
    PLX_INTR         *pPlxIntr,
    INTR_WAIT_OBJECT *pWaitObject
    );

VOID
PlxChipGetSpace(
    DEVICE_EXTENSION *pdx,
    IOP_SPACE         IopSpace,
    U8               *BarIndex,
    U16              *Offset_RegRemap
    );

VOID
PlxChipRestoreAfterDma(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    );

VOID
PlxChipPostCommonBufferProperties(
    DEVICE_EXTENSION *pdx,
    U32               PhysicalAddress,
    U32               Size
    );

BOOLEAN
PlxIsPowerLevelSupported(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL   PowerLevel
    );

BOOLEAN
PlxIsNewCapabilityEnabled(
    DEVICE_EXTENSION *pdx,
    U8                CapabilityToVerify
    );

U32
PlxHiddenRegisterRead(
    DEVICE_EXTENSION *pdx,
    U16               offset
    );




#endif
