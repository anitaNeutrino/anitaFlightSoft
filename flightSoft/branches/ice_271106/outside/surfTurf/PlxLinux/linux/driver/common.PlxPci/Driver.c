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
 *      Initializes the driver and claims system resources for the device.
 *
 * Revision History:
 *
 *      11-01-03 : PCI SDK v4.20
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include "Dispatch.h"
#include "Driver.h"
#include "PciSupport.h"
#include "Plx_sysdep.h"
#include "SupportFunc.h"




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
    int status;


    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("<========================================================>\n"));
    DebugPrintf((
        "PLX PCI Service Driver v%d.%d%d - built on %s %s\n",
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
    pGbl_DriverObject->DeviceObject = NULL;
    pGbl_DriverObject->DeviceCount  = 0;

    // Fill in the appropriate dispatch handlers
    pGbl_DriverObject->DispatchTable.llseek  = NULL;
    pGbl_DriverObject->DispatchTable.read    = NULL;
    pGbl_DriverObject->DispatchTable.write   = NULL;
    pGbl_DriverObject->DispatchTable.readdir = NULL;
    pGbl_DriverObject->DispatchTable.poll    = NULL;
    pGbl_DriverObject->DispatchTable.ioctl   = Dispatch_IoControl;
    pGbl_DriverObject->DispatchTable.mmap    = NULL;
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

    // Add a single device object
    status =
        AddDevice(
            pGbl_DriverObject,
            NULL
            );

    if (status != 0)
    {
        DebugPrintf(("...Driver unloaded\n"));
        return (-ENODEV);
    }

    // Scan the PCI bus to build list of devices
    pGbl_DriverObject->DeviceCount =
        PlxPciBusScan(
            pGbl_DriverObject
            );

    // Check if any devices were found
    if (pGbl_DriverObject->DeviceCount == 0)
    {
        ErrorPrintf(("ERROR - No PCI devices found\n"));

        // Unload the driver
        cleanup_module();

        return (-ENODEV);
    }

    DebugPrintf((
        "...Driver loaded\n"
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

        // Delete the device & remove from device list
        RemoveDevice(
            fdo
            );

        // Jump to next device object
        fdo = pNext;
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
    fdo->DriverObject    = pDriverObject;        // Save parent driver object
    fdo->NextDevice      = NULL;
    fdo->DeviceExtension = &(fdo->DeviceInfo);


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

    // Initialize device list
    INIT_LIST_HEAD(
        &(pdx->List_SupportedDevices)
        );


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
        PLX_DRIVER_NAME
        ));

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
    struct list_head *pListItem;
    DEVICE_OBJECT    *pDevice;
    PLX_DEVICE_NODE  *pNode;
    DEVICE_EXTENSION *pdx;


    pdx = fdo->DeviceExtension;

    DebugPrintf(("Deleting supported devices list...\n"));

    // Free supported device list
    while (!list_empty(
                &(pdx->List_SupportedDevices)
                ))
    {
        // Get next list item
        pListItem = pdx->List_SupportedDevices.next;

        // Get the device node
        pNode =
            list_entry(
                pListItem,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Remove node from list
        list_del(
            pListItem
            );

        // Release the object
        kfree(
            pNode
            );
    }

    DebugPrintf((
        "Removing device (%s)...\n",
        PLX_DRIVER_NAME
        ));

    // Acquire Device List lock
    spin_lock(
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    // Get device list head
    pDevice = pGbl_DriverObject->DeviceObject;

    if (pDevice == NULL)
    {
        // Release Device List lock
        spin_unlock(
            &(pGbl_DriverObject->Lock_DeviceList)
            );

        ErrorPrintf(("ERROR - Unable to remove device, device list is empty\n"));
        return (-ENODEV);
    }

    if (pDevice == fdo)
    {
        // Remove device from first in list
        pGbl_DriverObject->DeviceObject = fdo->NextDevice;
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
                    &(pGbl_DriverObject->Lock_DeviceList)
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
        &(pGbl_DriverObject->Lock_DeviceList)
        );

    DebugPrintf((
        "Deleting device object (0x%08x)...\n",
        (U32)fdo
        ));

    // Delete the device object
    kfree(
        fdo
        );

    return 0;
}
