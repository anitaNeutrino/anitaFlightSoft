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
 *      Driver.c
 *
 * Description:
 *
 *      Contains driver entry and cleanup routines as well as device
 *      resource initialization.
 *
 * Revision History:
 *
 *      12-01-03 : PCI SDK v4.20
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include "CommonApi.h"
#include "Dispatch.h"
#include "Driver.h"
#include "PciSupport.h"
#include "PlxInterrupt.h"
#include "Plx_sysdep.h"
#include "SupportFunc.h"



/***********************************************
 *     List of IDs the driver will support
 *
 * Note: Static allocation for now.  Will move
 *       to some dynamic configuration later.
 **********************************************/
#if defined(PCI9050)
    char SupportedDevices[] = "905010b5 520110b5";
#elif defined(PCI9030)
    char SupportedDevices[] = "903010b5 300110b5 30c110b5";
#elif defined(PCI9080)
    char SupportedDevices[] = "908010b5 040110b5 086010b5";
#elif defined(PCI9054)
    char SupportedDevices[] = "905410b5 186010b5 c86010b5 540610b5";
#elif defined(PCI9056)
    char SupportedDevices[] = "905610b5 560110b5 56c210b5";
#elif defined(PCI9656)
    char SupportedDevices[] = "965610b5 960110b5 96c210b5";
#else
    #error Supported Device IDs not provided
#endif




/***********************************************
 *               Globals
 **********************************************/
DRIVER_OBJECT *pGbl_DriverObject;       // Pointer to the main driver object




/******************************************************************************
 *
 * Function   :  init_module
 *
 * Description:  Entry point for the driver
 *
 ******************************************************************************/
int
init_module(
    void
    )
{
    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("<========================================================>\n"));
    DebugPrintf((
        "PLX Driver v%d.%d%d - built on %s %s\n",
        DRIVER_VERSION_MAJOR, DRIVER_VERSION_MINOR,
        DRIVER_VERSION_REVISION, __DATE__, __TIME__
        ));
    DebugPrintf((
        "Supports Linux kernel version %s\n",
        UTS_RELEASE
        ));

    // Verify that PCI is present
    if (pci_present() == FALSE)
    {
        ErrorPrintf(("ERROR - PCI Subsystem not present\n"));
        return (-EIO);
    }

    // Allocate memory for the Driver Object
    pGbl_DriverObject =
        kmalloc(
            sizeof(DRIVER_OBJECT),
            GFP_KERNEL
            );

    if (pGbl_DriverObject == NULL)
    {
        ErrorPrintf(("ERROR - memory allocation for Driver Object failed\n"));
        return (-ENOMEM);
    }

    // Initialize driver object
    pGbl_DriverObject->DeviceObject             = NULL;
    pGbl_DriverObject->DriverType               = PLX_CHIP_TYPE;
    pGbl_DriverObject->SupportedIDs             = SupportedDevices;
    pGbl_DriverObject->DeviceCount              = 0;
    pGbl_DriverObject->CommonBuffer.PciMem.Size = 0;

    // Fill in the appropriate dispatch handlers
    pGbl_DriverObject->DispatchTable.llseek  = NULL;
    pGbl_DriverObject->DispatchTable.read    = Dispatch_read;
    pGbl_DriverObject->DispatchTable.write   = Dispatch_write;
    pGbl_DriverObject->DispatchTable.readdir = NULL;
    pGbl_DriverObject->DispatchTable.poll    = NULL;
    pGbl_DriverObject->DispatchTable.ioctl   = Dispatch_IoControl;
    pGbl_DriverObject->DispatchTable.mmap    = Dispatch_mmap;
    pGbl_DriverObject->DispatchTable.open    = Dispatch_open;
    pGbl_DriverObject->DispatchTable.flush   = NULL;
    pGbl_DriverObject->DispatchTable.release = Dispatch_release;
    pGbl_DriverObject->DispatchTable.fsync   = NULL;
    pGbl_DriverObject->DispatchTable.fasync  = NULL;
    pGbl_DriverObject->DispatchTable.lock    = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

    // Dispatch routines only in kernel 2.2
    pGbl_DriverObject->DispatchTable.check_media_change = NULL;
    pGbl_DriverObject->DispatchTable.revalidate         = NULL;

#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)

    // Dispatch routines only in kernel 2.4
    pGbl_DriverObject->DispatchTable.owner             = NULL;
    pGbl_DriverObject->DispatchTable.readv             = NULL;
    pGbl_DriverObject->DispatchTable.writev            = NULL;
    pGbl_DriverObject->DispatchTable.sendpage          = NULL;
    pGbl_DriverObject->DispatchTable.get_unmapped_area = NULL;

#endif

    // Mark the module owner (for 2.4 kernel)
    SET_MODULE_OWNER(
        &(pGbl_DriverObject->DispatchTable)
        );

    // Initialize spin locks
    spin_lock_init(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    // Register the driver with the OS
    pGbl_DriverObject->MajorID =
        register_chrdev(
            0,              // 0 = system chooses Major ID
            PLX_DRIVER_NAME,
            &(pGbl_DriverObject->DispatchTable)
            );

    DebugPrintf((
        "Registered driver (MajorID = %03d)\n",
        pGbl_DriverObject->MajorID
        ));

    // Scan the system for supported devices
    PciBusScan(
        pGbl_DriverObject
        );

    // Check if any devices were found
    if (pGbl_DriverObject->DeviceCount == 0)
    {
        ErrorPrintf(("ERROR - No supported devices found\n"));

        // Unload the driver
        cleanup_module();

        return (-ENODEV);
    }

    DebugPrintf((
        "Allocating common buffer...\n"
        ));

    // Set requested size
    pGbl_DriverObject->CommonBuffer.PciMem.Size = DEFAULT_SIZE_COMMON_BUFFER;

    // Allocate common buffer
    PlxPciPhysicalMemoryAllocate(
        NULL,                   // No device to assign buffer to
        &(pGbl_DriverObject->CommonBuffer.PciMem),
        TRUE,                   // Smaller buffer is ok
        pGbl_DriverObject       // Assign Driver object as owner
        );

    DebugPrintf((
        "...driver loaded\n"
        ));

    return 0;
}




/******************************************************************************
 *
 * Function   :  cleanup_module
 *
 * Description:  Unload the driver
 *
 ******************************************************************************/
void
cleanup_module(
    void
    )
{
    DEVICE_OBJECT *fdo;
    DEVICE_OBJECT *pNext;


    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Unloading Driver...\n"));

    // Get the device list
    fdo = pGbl_DriverObject->DeviceObject;

    // Remove all devices
    while (fdo != NULL)
    {
        // Store next device
        pNext = fdo->NextDevice;

        // Stop device and release its resources
        StopDevice(
            fdo
            );

        // Delete the device & remove from device list
        RemoveDevice(
            fdo
            );

        // Jump to next device object
        fdo = pNext;
    }

    // Release common buffer
    if (pGbl_DriverObject->CommonBuffer.PciMem.Size != 0)
    {
        DebugPrintf((
            "De-allocating Common Buffer at 0x%08x (%d Kb)...\n",
            pGbl_DriverObject->CommonBuffer.PciMem.PhysicalAddr,
            (pGbl_DriverObject->CommonBuffer.PciMem.Size >> 10)
            ));

        // Release the buffer
        Plx_pci_free_consistent_mem(
            &(pGbl_DriverObject->CommonBuffer)
            );
    }

    DebugPrintf((
        "De-register driver (MajorID = %03d)\n",
        pGbl_DriverObject->MajorID
        ));

    unregister_chrdev(
        pGbl_DriverObject->MajorID,
        PLX_DRIVER_NAME
        );

    // Release driver object
    kfree(
        pGbl_DriverObject
        );

    pGbl_DriverObject = NULL;

    DebugPrintf(("...Driver unloaded\n"));
}




/******************************************************************************
 *
 * Function   :  AddDevice
 *
 * Description:  Add a new device object to the driver
 *
 ******************************************************************************/
int
AddDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pPciDev
    )
{
    U8                i;
    DEVICE_OBJECT    *fdo;
    DEVICE_OBJECT    *pDevice;
    DEVICE_EXTENSION *pdx;


    // Allocate memory for the device object
    fdo =
        kmalloc(
            sizeof(DEVICE_OBJECT),
            GFP_KERNEL
            );

    if (fdo == NULL)
    {
        ErrorPrintf(("ERROR - memory allocation for device object failed\n"));
        return (-ENOMEM);
    }

    // Initialize device object
    fdo->DriverObject    = pDriverObject;       // Save parent driver object
    fdo->OpenCount       = 0;
    fdo->Flag_started    = FALSE;
    fdo->Flag_Interrupt  = FALSE;
    fdo->NextDevice      = NULL;
    fdo->DeviceExtension = &(fdo->DeviceInfo);

    // Initialize spinlocks
    spin_lock_init(
        &(fdo->Lock_DeviceOpen)
        );


    //
    // Initialize the device extension
    //

    pdx = fdo->DeviceExtension;

    // Clear device extension
    memset(
        pdx,
        0,
        sizeof(DEVICE_EXTENSION)
        );

    // Save the parent device object
    pdx->pDeviceObject = fdo;

    // Save the OS-supplied PCI object
    pdx->pPciDevice = pPciDev;

    // Set initial power state
    pdx->Power = D0;

    // Store device location information
    pdx->Device.BusNumber  = pPciDev->bus->number;
    pdx->Device.SlotNumber = PCI_SLOT(pPciDev->devfn);
    pdx->Device.DeviceId   = pPciDev->device;
    pdx->Device.VendorId   = pPciDev->vendor;

    // Build Serial number
    sprintf(
        pdx->Device.SerialNumber,
        PLX_DRIVER_NAME "-%d",
        pDriverObject->DeviceCount
        );

    // Initialize task element for ISR DPC queueing
    Plx_tq_struct_reset_list(
        pdx->Task_DpcForIsr
        );

    pdx->Task_DpcForIsr.sync    = 0;
    pdx->Task_DpcForIsr.routine = DpcForIsr;     // DPC routine
    pdx->Task_DpcForIsr.data    = pdx;           // Parameter to DPC routine

    // Initialize spinlocks
    spin_lock_init(
        &(pdx->Lock_HwAccess)
        );

    spin_lock_init(
        &(pdx->Lock_Isr)
        );

    // Initialize interrupt wait list
    INIT_LIST_HEAD(
        &(pdx->List_InterruptWait)
        );

    spin_lock_init(
        &(pdx->Lock_InterruptWaitList)
        );

    // Initialize map parameters list
    INIT_LIST_HEAD(
        &(pdx->List_MapParams)
        );

    spin_lock_init(
        &(pdx->Lock_MapParamsList)
        );

    // Initialize physical memories list
    INIT_LIST_HEAD(
        &(pdx->List_PhysicalMem)
        );

    spin_lock_init(
        &(pdx->Lock_PhysicalMemList)
        );

    // Initialize PCI BAR variables
    for (i = 0 ; i < PCI_NUM_BARS; i++)
    {
        pdx->PciBar[i].pVa               = NULL;
        pdx->PciBar[i].Physical.QuadPart = (U32)NULL;
        pdx->PciBar[i].Size              = 0;
    }

#if defined(DMA_SUPPORT)
    // Initialize DMA management variables
    for (i = 0 ; i < NUMBER_OF_DMA_CHANNELS; i++)
    { 
        pdx->DmaInfo[i].state              = DmaStateClosed;
        pdx->DmaInfo[i].bPending           = FALSE;
        pdx->DmaInfo[i].bLocalAddrConstant = FALSE;
    }

    // Initialize spinlock
    spin_lock_init(
        &(pdx->Lock_DmaChannel)
        );
#endif  // DMA_SUPPORT


    //
    // Add to driver device list
    //

    // Acquire Device List lock
    spin_lock(
        &(pDriverObject->Lock_DeviceList)
        );

    // Get device list head
    pDevice = pDriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Add device as first in list
        pDriverObject->DeviceObject = fdo;
    }
    else
    {
        // Go to end of list
        while (pDevice->NextDevice != NULL)
            pDevice = pDevice->NextDevice;

        // Add device to end of list
        pDevice->NextDevice = fdo;
    }

    // Increment device count
    pDriverObject->DeviceCount++;

    // Release Device List lock
    spin_unlock(
        &(pDriverObject->Lock_DeviceList)
        );

    DebugPrintf((
        "Created Device (%s)...\n",
        pdx->Device.SerialNumber
        ));

    // Enable the device
    if (pci_enable_device(
            pPciDev
            ) != 0)
    {
        DebugPrintf(("ERROR - PCI device enable failed\n"));
    }

    return 0;
}




/******************************************************************************
 *
 * Function   :  RemoveDevice
 *
 * Description:  Remove a device object from device list and delete it
 *
 ******************************************************************************/
int
RemoveDevice(
    DEVICE_OBJECT *fdo
    )
{
    DEVICE_OBJECT    *pDevice;
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    DebugPrintf((
        "Removing device (%s)...\n",
        pdx->Device.SerialNumber
        ));

    // Acquire Device List lock
    spin_lock(
        &(fdo->DriverObject->Lock_DeviceList)
        );

    // Get device list head
    pDevice = fdo->DriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Release Device List lock
        spin_unlock(
            &(fdo->DriverObject->Lock_DeviceList)
            );

        ErrorPrintf(("ERROR - Unable to remove device, device list is empty\n"));
        return (-ENODEV);
    }

    if (pDevice == fdo)
    {
        // Remove device from first in list
        fdo->DriverObject->DeviceObject = fdo->NextDevice;
    }
    else
    {
        // Scan list for the device
        while (pDevice->NextDevice != fdo)
        {
            pDevice = pDevice->NextDevice;

            if (pDevice == NULL)
            {
                // Release Device List lock
                spin_unlock(
                    &(fdo->DriverObject->Lock_DeviceList)
                    );

                ErrorPrintf((
                    "ERROR - Device object (0x%08x) not found in device list\n",
                    (U32)fdo
                    ));

                return (-ENODEV);
            }
        }

        // Remove device from list
        pDevice->NextDevice = fdo->NextDevice;
    }

    // Decrement device count
    pGbl_DriverObject->DeviceCount--;

    // Release Device List lock
    spin_unlock(
        &(fdo->DriverObject->Lock_DeviceList)
        );

    DebugPrintf((
        "Deleting device object (0x%08x)...\n",
        (U32)fdo
        ));

    // Release device object
    kfree(
        fdo
        );

    return 0;
}




/******************************************************************************
 *
 * Function   :  StartDevice
 *
 * Description:  Start a device
 *
 ******************************************************************************/
int
StartDevice(
    DEVICE_OBJECT *fdo
    )
{
    int               rc;
    U8                i;
    U8                ResourceCount;
    DEVICE_EXTENSION *pdx;


    if (fdo->Flag_started == TRUE)
        return 0;

    pdx           = fdo->DeviceExtension;
    ResourceCount = 0;

    for (i = 0; i < PCI_NUM_BARS; ++i)
    {
        // Verify the address is valid
        if (pci_resource_start(
                pdx->pPciDevice,
                i
                ) == 0)
        {
            continue;
        }

        DebugPrintf(("   Resource %02d\n", ResourceCount));

        // Increment resource count
        ResourceCount++;

        // Get PCI physical address
        pdx->PciBar[i].Physical.QuadPart =
            pci_resource_start(
                pdx->pPciDevice,
                i
                );

        // Determine resource type
        if (pci_resource_flags(
                pdx->pPciDevice,
                i
                ) & IORESOURCE_IO)
        {
            DebugPrintf(("     Type     : I/O Port\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Physical.QuadPart &= ~(0x3);
            pdx->PciBar[i].IsIoMapped         = TRUE;
        }
        else
        {
            DebugPrintf(("     Type     : Memory Space\n"));

            // Make sure flags are cleared properly
            pdx->PciBar[i].Physical.QuadPart &= ~(0xf);
            pdx->PciBar[i].IsIoMapped         = FALSE;
        }

        DebugPrintf((
            "     Address  : %08x\n",
            pdx->PciBar[i].Physical.u.LowPart
            ));

        // Get the size
        pdx->PciBar[i].Size =
            Plx_pci_resource_len(
                pdx,
                i
                );

        if (pdx->PciBar[i].Size >= (1 << 10))
        {
            DebugPrintf((
                "     Size     : %08x  (%d Kb)\n",
                pdx->PciBar[i].Size, pdx->PciBar[i].Size >> 10
                ));
        }
        else
        {
            DebugPrintf((
                "     Size     : %08x  (%d bytes)\n",
                pdx->PciBar[i].Size, pdx->PciBar[i].Size
                ));
        }

        // Claim and map the resource
        rc =
            PlxPciBarResourceMap(
                pdx,
                i
                );

        if (rc == 0)
        {
            if (pdx->PciBar[i].IsIoMapped == FALSE)
            {
                DebugPrintf((
                    "     Kernel VA: %08x\n",
                    (U32)pdx->PciBar[i].pVa
                    ));
            }
        }
        else
        {
            if (pdx->PciBar[i].IsIoMapped == FALSE)
            {
                ErrorPrintf((
                    "     Kernel VA: ERROR - Unable to map space to Kernel VA\n"
                    ));
            }

            // PCI BAR 0 is required for register access
            if (i == 0)
            {
                ErrorPrintf(("ERROR - BAR 0 mapping is required for register access\n"));
                return rc;
            }
        }
    }

    // Disable the PCI interrupt
    PlxChipPciInterruptDisable(
        pdx
        );

    // Install the ISR if available
    if (pdx->pPciDevice->irq != 0)
    {
        DebugPrintf(("   Resource %02d\n", ResourceCount));
        DebugPrintf(("     Type     : Interrupt\n"));
        DebugPrintf(("     IRQ      : %02d [0x%02x]\n",
                     pdx->pPciDevice->irq, pdx->pPciDevice->irq));

        // Increment resource count
        ResourceCount++;

        rc =
            request_irq(
                pdx->pPciDevice->irq,    // The device IRQ
                OnInterrupt,             // Interrupt handler
                SA_SHIRQ,                // Flags, support interrupt sharing
                PLX_DRIVER_NAME,         // The driver name
                pdx                      // Parameter to the ISR
                );

        if (rc != 0)
        {
            ErrorPrintf(("ERROR - Unable to claim interrupt resource\n"));
        }
        else
        {
            DebugPrintf(("Connected to interrupt vector\n"));

            // Flag that the interrupt resource was claimed
            fdo->Flag_Interrupt = TRUE;

            // Enable the PCI Interrupt
            PlxChipPciInterruptEnable(
                pdx
                );
        }
    }
    else
    {
        DebugPrintf(("Device is not using an interrupt resource\n"));
    }

    // Relay Common buffer properties to device
    PlxChipPostCommonBufferProperties(
        pdx,
        pGbl_DriverObject->CommonBuffer.PciMem.PhysicalAddr,
        pGbl_DriverObject->CommonBuffer.PciMem.Size
        );

    // Update device started flag
    fdo->Flag_started = TRUE;

    return 0;
}




/******************************************************************************
 *
 * Function   :  StopDevice
 *
 * Description:  Stop a device
 *
 ******************************************************************************/
VOID
StopDevice(
    DEVICE_OBJECT *fdo
    )
{
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    // Only stop devices which have been started
    if (fdo->Flag_started == FALSE)
    {
        return;
    }

    DebugPrintf(("Releasing device resources...\n"));

    if (fdo->Flag_Interrupt == TRUE)
    {
        // Disable the PCI interrupt
        PlxChipPciInterruptDisable(
            pdx
            );

        DebugPrintf((
            "Removing interrupt handler (IRQ = %02d [0x%02x])...\n",
            pdx->pPciDevice->irq, pdx->pPciDevice->irq
            ));

        // Release IRQ
        free_irq(
            pdx->pPciDevice->irq,
            pdx
            );

        // Flag that the interrupt resource was released
        fdo->Flag_Interrupt = FALSE;
    }

    // Remove relayed Common buffer properties
    PlxChipPostCommonBufferProperties(
        pdx,
        0,
        0
        );

    // Unmap I/O regions from kernel space (No local register access after this)
    PlxPciBarResourcesUnmap(
        pdx
        );

    // Mark that device is not started
    fdo->Flag_started = FALSE;
}




/******************************************************************************
 *
 * Function   :  PciBusScan
 *
 * Description:  Scan OS-generated PCI device list to search for supported devices
 *
 ******************************************************************************/
int
PciBusScan(
    DRIVER_OBJECT *pDriverObject
    )
{
    int             status;
    int             DeviceCount;
    struct pci_dev *pPciDev;


    DebugPrintf(("Scanning PCI bus for supported devices...\n"));

    // Clear device count
    DeviceCount = 0;

    // Get OS-generated list of PCI devices
    pPciDev =
        Plx_pci_find_device(
            PCI_ANY_ID,
            PCI_ANY_ID,
            NULL
            );

    while (pPciDev)
    {
        DebugPrintf((
            "Scanning - %.4x %.4x  [Bus %.2x  Slot %.2x]\n",
            pPciDev->device, pPciDev->vendor,
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn)
            ));

        // Check if device is supported
        if (IsSupportedDevice(
                pDriverObject,
                pPciDev
                ))
        {
            DebugPrintf(("Supported device found, adding device...\n"));

            // Add the device to the device list
            status =
                AddDevice(
                    pDriverObject,
                    pPciDev
                    );

            if (status == 0)
            {
                // Increment number of devices found
                DeviceCount++;
            }
        }

        // Jump to next device
        pPciDev =
            Plx_pci_find_device(
                PCI_ANY_ID,
                PCI_ANY_ID,
                pPciDev
                );
    }

    DebugPrintf((
        "PciBusScan: %d supported device(s) found\n",
        DeviceCount
        ));

    return DeviceCount;
}
