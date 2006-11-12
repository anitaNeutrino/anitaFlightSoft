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
 *      Dispatch.c
 *
 * Description:
 *
 *      This file routes incoming I/O Request packets
 *
 * Revision History:
 *
 *      10-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "ApiFunctions.h"
#include "CommonApi.h"
#include "Dispatch.h"
#include "Driver.h"
#include "PciSupport.h"
#include "PlxIoctl.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  Dispatch_open
 *
 * Description:  Handle open() which allows applications to create a
 *               connection to the driver
 *
 ******************************************************************************/
int
Dispatch_open(
    struct inode *inode,
    struct file  *filp
    )
{
    int            rc;
    U8             i;
    DEVICE_OBJECT *fdo;


    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ==> OPEN_DEVICE\n"));

    if (MINOR(inode->i_rdev) == PLX_MNGMT_INTERFACE)
    {
        DebugPrintf(("Opening Management interface...\n"));

        // Store the driver object in the private data
        filp->private_data = (void *)pGbl_DriverObject;
    }
    else
    {
        // Select desired device from device list
        i   = MINOR(inode->i_rdev);
        fdo = pGbl_DriverObject->DeviceObject;

        while (i-- && fdo != NULL)
           fdo = fdo->NextDevice;

        if (fdo == NULL)
        {
            ErrorPrintf(("WARNING - Attempt to open non-existent device\n"));
            return (-ENODEV);
        }

        DebugPrintf((
            "Opening device (%s)...\n",
            fdo->DeviceExtension->Device.SerialNumber
            ));

        // Acquire Open lock
        spin_lock(
            &(fdo->Lock_DeviceOpen)
            );

        // Attempt to start the device
        rc =
            StartDevice(
                fdo
                );

        if (rc != 0)
        {
            // Release Open lock
            spin_unlock(
                &(fdo->Lock_DeviceOpen)
                );

            return rc;
        }

        // Increment open count for this device
        fdo->OpenCount++;

        // Release Open lock
        spin_unlock(
            &(fdo->Lock_DeviceOpen)
            );

        // Store device object for future calls
        filp->private_data = (void *)fdo;
    }

    // Increment driver use count for kernel
    MOD_INC_USE_COUNT;

    DebugPrintf(("...device opened\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_release
 *
 * Description:  Handle close() call, which closes the connection between the
 *               application and drivers.
 *
 ******************************************************************************/
int
Dispatch_release(
    struct inode *inode,
    struct file  *filp
    )
{
    DEVICE_OBJECT *fdo;


    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ==> CLOSE_DEVICE\n"));

    if (MINOR(inode->i_rdev) == PLX_MNGMT_INTERFACE)
    {
        DebugPrintf(("Closing Management interface...\n"));

        // Clear the driver object from the private data
        filp->private_data = NULL;
    }
    else
    {
        // Get the device object
        fdo = (DEVICE_OBJECT *)(filp->private_data);

        DebugPrintf((
            "Closing device (%s)...\n",
            fdo->DeviceExtension->Device.SerialNumber
            ));

        // Remove any interrupt wait elements owned by the process
        PlxInterruptWaitObjectRemoveAll_ByOwner(
            fdo->DeviceExtension,
            (VOID*)filp
            );

        // Close DMA channels owned by the process
        PlxDmaChannelCleanup(
            fdo->DeviceExtension,
            (VOID*)filp
            );

        // Remove any pending mapping requests initiated by process
        PlxUserMappingRequestFreeAll_ByOwner(
            fdo->DeviceExtension,
            (VOID*)filp
            );

        // Release any physical memory allocated by process
        PlxPciPhysicalMemoryFreeAll_ByOwner(
            fdo->DeviceExtension,
            (VOID*)filp
            );

        // Decrement open count for this device
        spin_lock(
            &(fdo->Lock_DeviceOpen)
            );

        fdo->OpenCount--;

        // Stop the device if no longer used
        if (fdo->OpenCount == 0)
        {
            StopDevice(
                fdo
                );
        }

        spin_unlock(
            &(fdo->Lock_DeviceOpen)
            );
    }

    // Decrement driver use count for kernel
    MOD_DEC_USE_COUNT;

    DebugPrintf(("...device closed\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_read
 *
 * Description:  Handle read() call, which is not supported by this driver
 *
 ******************************************************************************/
ssize_t
Dispatch_read(
    struct file *filp,
    char        *buff,
    size_t       count,
    loff_t      *offp
    )
{
    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ===> READ  (Not supported)\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_write
 *
 * Description:  Handle write() call, which is not supported by this driver
 *
 ******************************************************************************/
ssize_t
Dispatch_write(
    struct file *filp,
    const char  *buff,
    size_t       count,
    loff_t      *offp
    )
{
    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ===> WRITE  (Not supported)\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_mmap
 *
 * Description:  Maps a PCI space into user virtual space
 *
 ******************************************************************************/
int
Dispatch_mmap(
    struct file           *filp,
    struct vm_area_struct *vma
    )
{
    DEVICE_EXTENSION  *pdx;
    MAP_PARAMS_OBJECT *pObject;


    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ===> MMAP\n"));

    pObject =
        kmalloc(
            sizeof(MAP_PARAMS_OBJECT),
            GFP_KERNEL
            );

    if (pObject == NULL)
    {
        DebugPrintf((
            "ERROR - memory allocation for map object failed\n"
            ));

        return -ENOMEM;
    }

    // Save the mapping parameters for later use
    pObject->vma = *vma;

    // Record the owner
    pObject->pOwner = (VOID*)filp;

    pdx = ((DEVICE_OBJECT*)(filp->private_data))->DeviceExtension;

    // Add to list of map objects
    spin_lock(
        &(pdx->Lock_MapParamsList)
        );

    list_add_tail(
        &(pObject->list),
        &(pdx->List_MapParams)
        );

    spin_unlock(
        &(pdx->Lock_MapParamsList)
        );

    DebugPrintf((
        "Added map object (0x%08x) to list\n",
        (U32)pObject
        ));

    DebugPrintf(("...Completed message\n"));

    return 0;
}




/******************************************************************************
 *
 * Function   :  Dispatch_IoControl
 *
 * Description:  Processes the IOCTL messages sent to this device
 *
 ******************************************************************************/
int 
Dispatch_IoControl(
    struct inode  *inode,
    struct file   *filp,
    unsigned int   cmd,
    unsigned long  args
    )
{
    int                status;
    IOCTLDATA          IoBuffer;
    IOCTLDATA         *pIoBuffer;
    DEVICE_EXTENSION  *pdx;


    DebugPrintf_NoInfo(("\n"));

    // Get the device extension
    if (MINOR(inode->i_rdev) == PLX_MNGMT_INTERFACE)
    {
        // Management interface node only supports some IOCTLS
        switch (cmd)
        {
            case PLX_IOCTL_DRIVER_VERSION:
            case PLX_IOCTL_PCI_DEVICE_FIND:
            case PLX_IOCTL_PCI_REGISTER_READ:
            case PLX_IOCTL_PCI_REGISTER_WRITE:
            case PLX_IOCTL_PCI_REG_READ_UNSUPPORTED:
            case PLX_IOCTL_PCI_REG_WRITE_UNSUPPORTED:
                pdx = NULL;
                break;

            default:
                ErrorPrintf((
                    "ERROR: Management interface doesn't support PLX_IOCTL_Xxx (%02d)\n",
                    _IOC_NR(cmd)
                    ));
                return (-EOPNOTSUPP);
        }
    }
    else
    {
        pdx = ((DEVICE_OBJECT*)(filp->private_data))->DeviceExtension;
    }

    // Copy the I/O Control message from user space
    status =
        copy_from_user(
            &IoBuffer,
            (IOCTLDATA*)args,
            sizeof(IOCTLDATA)
            );

    if (status != 0)
    {
        ErrorPrintf((
            "ERROR - Unable to copy user I/O message data\n"
            ));

        return (-EFAULT);
    }

    pIoBuffer = &IoBuffer;

    DebugPrintf(("Received PLX message ===> "));

    switch (cmd)
    {
        /***********************************
         * PLX device management functions
         **********************************/
        case PLX_IOCTL_DEVICE_INIT:
            DebugPrintf_NoInfo(("PLX_IOCTL_DEVICE_INIT\n"));

            // Send the device location data back to the API
            pIoBuffer->u.MgmtData.u.Device = pdx->Device;
            break;

        case PLX_IOCTL_PCI_DEVICE_FIND:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_DEVICE_FIND\n"));

            // Search through list of devices
            PlxSearchDevices(
                (DRIVER_OBJECT*)filp->private_data,
                &(pIoBuffer->u.MgmtData.u.Device),
                &(pIoBuffer->u.MgmtData.value)
                );
            break;

        case PLX_IOCTL_DRIVER_VERSION:
            DebugPrintf_NoInfo(("PLX_IOCTL_DRIVER_VERSION\n"));

            pIoBuffer->u.MgmtData.value  = (DRIVER_VERSION_MAJOR    << 16);
            pIoBuffer->u.MgmtData.value |= (DRIVER_VERSION_MINOR    <<  8);
            pIoBuffer->u.MgmtData.value |= (DRIVER_VERSION_REVISION <<  0);
            break;

        case PLX_IOCTL_CHIP_TYPE_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_CHIP_TYPE_GET\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode = ApiSuccess;

                PlxChipTypeGet(
                    pdx,
                    &(pIoBuffer->u.MiscData.data[0]),
                    (U8*)&(pIoBuffer->u.MiscData.data[1])
                    );
            }
            break;

        case PLX_IOCTL_PCI_BOARD_RESET:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BOARD_RESET\n"));

            if (pdx->Power <= MIN_WORKING_POWER_STATE)
            {
                PlxPciBoardReset(
                    pdx
                    );
            }
            break;

        case PLX_IOCTL_PCI_BAR_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BAR_GET\n"));

            pIoBuffer->u.MgmtData.value =
                         pdx->PciBar[pIoBuffer->u.MgmtData.offset].Physical.u.LowPart;
            pIoBuffer->u.MgmtData.u.bFlag =
                         pdx->PciBar[pIoBuffer->u.MgmtData.offset].IsIoMapped;
            break;

        case PLX_IOCTL_PCI_BAR_RANGE_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BAR_RANGE_GET\n"));

            pIoBuffer->u.MgmtData.value =
                         pdx->PciBar[pIoBuffer->u.MgmtData.offset].Size;
            break;

        case PLX_IOCTL_PCI_BAR_MAP:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BAR_MAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciBarMap(
                    pdx,
                    (U8)(pIoBuffer->u.MgmtData.offset),
                    &(pIoBuffer->u.MgmtData.value),
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_PCI_BAR_UNMAP:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BAR_UNMAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciBarUnmap(
                    pdx,
                    (VOID*)(pIoBuffer->u.MgmtData.value),
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_ALLOCATE:
            DebugPrintf_NoInfo(("PLX_IOCTL_PHYSICAL_MEM_ALLOCATE\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryAllocate(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.PciMemory),
                    (BOOLEAN)(pIoBuffer->u.MiscData.data[0]),
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_FREE:
            DebugPrintf_NoInfo(("PLX_IOCTL_PHYSICAL_MEM_FREE\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryFree(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.PciMemory)
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_MAP:
            DebugPrintf_NoInfo(("PLX_IOCTL_PHYSICAL_MEM_MAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryMap(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.PciMemory),
                    (BOOLEAN)pIoBuffer->u.MiscData.data[0],
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_PHYSICAL_MEM_UNMAP:
            DebugPrintf_NoInfo(("PLX_IOCTL_PHYSICAL_MEM_UNMAP\n"));

            pIoBuffer->ReturnCode =
                PlxPciPhysicalMemoryUnmap(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.PciMemory),
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_COMMON_BUFFER_PROPERTIES:
            DebugPrintf_NoInfo(("PLX_IOCTL_COMMON_BUFFER_PROPERTIES\n"));

           // Return buffer information
           pIoBuffer->u.MiscData.u.PciMemory.PhysicalAddr =
                         pGbl_DriverObject->CommonBuffer.PciMem.PhysicalAddr;
           pIoBuffer->u.MiscData.u.PciMemory.Size =
                         pGbl_DriverObject->CommonBuffer.PciMem.Size;
           break;


        /*******************************************
         * PCI Register Access Functions
         ******************************************/
        case PLX_IOCTL_PCI_REGISTER_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REGISTER_READ\n"));

            pIoBuffer->u.MgmtData.value =
                PlxPciRegisterRead(
                    pdx,
                    pIoBuffer->u.MgmtData.u.Device.BusNumber,
                    pIoBuffer->u.MgmtData.u.Device.SlotNumber,
                    pIoBuffer->u.MgmtData.offset,
                    &(pIoBuffer->ReturnCode)
                    );
            break;

        case PLX_IOCTL_PCI_REGISTER_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterWrite(
                    pdx,
                    pIoBuffer->u.MgmtData.u.Device.BusNumber,
                    pIoBuffer->u.MgmtData.u.Device.SlotNumber,
                    pIoBuffer->u.MgmtData.offset,
                    pIoBuffer->u.MgmtData.value
                    );
            break;

        case PLX_IOCTL_PCI_REG_READ_UNSUPPORTED:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REG_READ_UNSUPPORTED\n"));

            pIoBuffer->u.MgmtData.value =
                PlxPciRegisterRead_Unsupported(
                    pdx,
                    pIoBuffer->u.MgmtData.u.Device.BusNumber,
                    pIoBuffer->u.MgmtData.u.Device.SlotNumber,
                    pIoBuffer->u.MgmtData.offset,
                    &(pIoBuffer->ReturnCode)
                    );
            break;

        case PLX_IOCTL_PCI_REG_WRITE_UNSUPPORTED:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REG_WRITE_UNSUPPORTED\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterWrite_Unsupported(
                    pdx,
                    pIoBuffer->u.MgmtData.u.Device.BusNumber,
                    pIoBuffer->u.MgmtData.u.Device.SlotNumber,
                    pIoBuffer->u.MgmtData.offset,
                    pIoBuffer->u.MgmtData.value
                    );
            break;


        /***********************************
         * Local Register Access Functions
         **********************************/
        case PLX_IOCTL_REGISTER_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_REGISTER_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode       = ApiPowerDown;
                pIoBuffer->u.MgmtData.value = (U32)-1;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterRead(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_REGISTER_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_REGISTER_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterWrite(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;


        /********************************
         * Interrupt Support Functions
         *******************************/
        case PLX_IOCTL_INTR_ENABLE:
            DebugPrintf_NoInfo(("PLX_IOCTL_INTR_ENABLE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciIntrEnable(
                        pdx,
                        &(pIoBuffer->u.MiscData.u.IntrInfo)
                        );
            }
            break;

        case PLX_IOCTL_INTR_DISABLE:
            DebugPrintf_NoInfo(("PLX_IOCTL_INTR_DISABLE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciIntrDisable(
                        pdx,
                        &(pIoBuffer->u.MiscData.u.IntrInfo)
                        );
            }
            break;

        case PLX_IOCTL_INTR_STATUS_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_INTR_STATUS_GET\n"));

            pIoBuffer->ReturnCode =
                PlxPciIntrStatusGet(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.IntrInfo)
                    );
            break;

        case PLX_IOCTL_INTR_ATTACH:
            DebugPrintf_NoInfo(("PLX_IOCTL_INTR_ATTACH\n"));

            pIoBuffer->ReturnCode =
                PlxPciIntrAttach(
                    pdx,
                    &(pIoBuffer->u.MiscData.u.IntrInfo),
                    &(pIoBuffer->u.MiscData.data[0]),
                    (VOID*)filp
                    );
            break;

        case PLX_IOCTL_INTR_WAIT:
            DebugPrintf_NoInfo(("PLX_IOCTL_INTR_WAIT\n"));

            pIoBuffer->ReturnCode =
                PlxPciIntrWait(
                    pdx,
                    pIoBuffer->u.MiscData.data[0],
                    pIoBuffer->u.MiscData.data[1]
                    );
            break;


        /*********************************
         * Bus Memory and I/O Functions
         ********************************/
        case PLX_IOCTL_BUS_IOP_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_BUS_IOP_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciBusMemTransfer(
                        pdx,
                        pIoBuffer->u.BusIopData.IopSpace,
                        pIoBuffer->u.BusIopData.Address,
                        pIoBuffer->u.BusIopData.bRemap,
                        (VOID*)pIoBuffer->u.BusIopData.Buffer,
                        pIoBuffer->u.BusIopData.TransferSize,
                        pIoBuffer->u.BusIopData.AccessType,
                        TRUE           // Specify read operation
                        );
            }
            break;

        case PLX_IOCTL_BUS_IOP_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_BUS_IOP_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciBusMemTransfer(
                        pdx,
                        pIoBuffer->u.BusIopData.IopSpace,
                        pIoBuffer->u.BusIopData.Address,
                        pIoBuffer->u.BusIopData.bRemap,
                        (VOID*)pIoBuffer->u.BusIopData.Buffer,
                        pIoBuffer->u.BusIopData.TransferSize,
                        pIoBuffer->u.BusIopData.AccessType,
                        FALSE          // Specify write operation
                        );
            }
            break;

        case PLX_IOCTL_IO_PORT_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_IO_PORT_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciIoPortTransfer(
                        pIoBuffer->u.BusIopData.Address,
                        pIoBuffer->u.BusIopData.AccessType,
                        (VOID*)&(pIoBuffer->u.BusIopData.Buffer),
                        TRUE           // Specify read operation
                        );
            }
            break;

        case PLX_IOCTL_IO_PORT_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_IO_PORT_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxPciIoPortTransfer(
                        pIoBuffer->u.BusIopData.Address,
                        pIoBuffer->u.BusIopData.AccessType,
                        (VOID*)&(pIoBuffer->u.BusIopData.Buffer),
                        FALSE          // Specify write operation
                        );
            }
            break;


        /*******************************
         * Power Management Functions
         ******************************/
        case PLX_IOCTL_POWER_LEVEL_SET:
            DebugPrintf_NoInfo(("PLX_IOCTL_POWER_LEVEL_SET\n"));

            pIoBuffer->ReturnCode =
                PlxPciPowerLevelSet(
                    pdx,
                    (PLX_POWER_LEVEL)(pIoBuffer->u.MgmtData.value)
                    );

            if (pIoBuffer->ReturnCode == ApiSuccess)
                pdx->Power = (PLX_POWER_LEVEL)(pIoBuffer->u.MgmtData.value);

            break;

        case PLX_IOCTL_POWER_LEVEL_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_POWER_LEVEL_GET\n"));

            pIoBuffer->ReturnCode =
                PlxPciPowerLevelGet(
                    pdx,
                    (PLX_POWER_LEVEL*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_PM_ID_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_PM_ID_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciPmIdRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_PM_NCP_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_PM_NCP_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciPmNcpRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;


        /***********************************
         *     Hot Swap Functions
         **********************************/
        case PLX_IOCTL_HS_ID_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_HS_ID_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciHotSwapIdRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_HS_NCP_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_HS_NCP_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciHotSwapNcpRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_HS_STATUS:
            DebugPrintf_NoInfo(("PLX_IOCTL_HS_STATUS\n"));

            pIoBuffer->ReturnCode =
                PlxPciHotSwapStatus(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;


        /***********************************
         *         VPD Functions
         **********************************/
        case PLX_IOCTL_VPD_ID_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_VPD_ID_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciVpdIdRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_VPD_NCP_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_VPD_NCP_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciVpdNcpRead(
                    pdx,
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_VPD_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_VPD_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciVpdRead(
                    pdx,
                    pIoBuffer->u.MgmtData.offset,
                    &(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_VPD_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_VPD_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxPciVpdWrite(
                    pdx,
                    pIoBuffer->u.MgmtData.offset,
                    pIoBuffer->u.MgmtData.value
                    );
            break;


        /***********************************
         * Serial EEPROM Access Functions
         **********************************/
        case PLX_IOCTL_EEPROM_PRESENT:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_PRESENT\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxEepromPresent(
                        pdx,
                        &(pIoBuffer->u.MgmtData.u.bFlag)
                        );
            }
            break;

        case PLX_IOCTL_EEPROM_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxEepromAccess(
                        pdx,
                        pIoBuffer->u.MiscData.u.EepromType,
                        (VOID*)(pIoBuffer->u.MiscData.data[0]),
                        pIoBuffer->u.MiscData.data[1],
                        TRUE           // Specify read operation
                        );
            }
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_READ_BY_OFFSET\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxEepromReadByOffset(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_EEPROM_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxEepromAccess(
                        pdx,
                        pIoBuffer->u.MiscData.u.EepromType,
                        (VOID*)(pIoBuffer->u.MiscData.data[0]),
                        pIoBuffer->u.MiscData.data[1],
                        FALSE          // Specify write operation
                        );
            }
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxEepromWriteByOffset(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;


        /**************************************************
         * Mailbox and Doorbell Register Access Functions
         **************************************************/
        case PLX_IOCTL_MAILBOX_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_MAILBOX_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode       = ApiPowerDown;
                pIoBuffer->u.MgmtData.value = (U32)-1;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterMailboxRead(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_MAILBOX_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_MAILBOX_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterMailboxWrite(
                        pdx,
                        pIoBuffer->u.MgmtData.offset,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;

        case PLX_IOCTL_DOORBELL_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_DOORBELL_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode       = ApiPowerDown;
                pIoBuffer->u.MgmtData.value = (U32)-1;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterDoorbellRead(
                        pdx,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_DOORBELL_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_DOORBELL_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxRegisterDoorbellWrite(
                        pdx,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;


        /****************************
         * Messaging Unit Functions
         ****************************/
        case PLX_IOCTL_MU_INBOUND_PORT_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_MU_INBOUND_PORT_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxMuInboundPortRead(
                        pdx,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_MU_INBOUND_PORT_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_MU_INBOUND_PORT_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxMuInboundPortWrite(
                        pdx,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;

        case PLX_IOCTL_MU_OUTBOUND_PORT_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_MU_OUTBOUND_PORT_READ\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxMuOutboundPortRead(
                        pdx,
                        &(pIoBuffer->u.MgmtData.value)
                        );
            }
            break;

        case PLX_IOCTL_MU_OUTBOUND_PORT_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_MU_OUTBOUND_PORT_WRITE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxMuOutboundPortWrite(
                        pdx,
                        pIoBuffer->u.MgmtData.value
                        );
            }
            break;


        /***********************************
         *    DMA Management Functions
         **********************************/
        case PLX_IOCTL_DMA_CONTROL:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_CONTROL\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaControl(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        pIoBuffer->u.DmaData.u.command
                        );
            }
            break;

        case PLX_IOCTL_DMA_STATUS:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_STATUS\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaStatus(
                        pdx,
                        pIoBuffer->u.DmaData.channel
                        );
            }
            break;


        /***********************
         * Block DMA Functions
         ***********************/
        case PLX_IOCTL_DMA_BLOCK_CHANNEL_OPEN:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_BLOCK_CHANNEL_OPEN\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaBlockChannelOpen(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        &(pIoBuffer->u.DmaData.u.desc),
                        (VOID*)filp
                        );
            }
            break;

        case PLX_IOCTL_DMA_BLOCK_TRANSFER:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_BLOCK_TRANSFER\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaBlockTransfer(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        &(pIoBuffer->u.DmaData.u.TxParams),
                        NULL
                        );
            }
            break;

        case PLX_IOCTL_DMA_BLOCK_CHANNEL_CLOSE:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_BLOCK_CHANNEL_CLOSE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaBlockChannelClose(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        TRUE
                        );
            }
            break;


        /**********************
         * SGL DMA Functions
         *********************/
        case PLX_IOCTL_DMA_SGL_OPEN:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_SGL_OPEN\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaSglChannelOpen(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        &(pIoBuffer->u.DmaData.u.desc),
                        (VOID*)filp
                        );
            }
            break;

        case PLX_IOCTL_DMA_SGL_TRANSFER:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_SGL_TRANSFER\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaSglTransfer(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        &(pIoBuffer->u.DmaData.u.TxParams),
                        NULL
                        );
            }
            break;

        case PLX_IOCTL_DMA_SGL_CLOSE:
            DebugPrintf_NoInfo(("PLX_IOCTL_DMA_SGL_CLOSE\n"));

            if (pdx->Power > MIN_WORKING_POWER_STATE)
            {
                pIoBuffer->ReturnCode = ApiPowerDown;
            }
            else
            {
                pIoBuffer->ReturnCode =
                    PlxDmaSglChannelClose(
                        pdx,
                        pIoBuffer->u.DmaData.channel,
                        TRUE
                        );
            }
            break;


        /******************************
         *  Miscellaneous functions
         *****************************/
        case PLX_IOCTL_ABORTADDR_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_ABORTADDR_READ\n"));

            pIoBuffer->ReturnCode =
                PlxAbortAddrRead(
                    pdx,
                    &(pIoBuffer->u.MgmtData.value)
                    );
            break;


        /*****************************
         *   Unsupported Messages
         ****************************/
        default:
            DebugPrintf_NoInfo((
                "Unsupported PLX_IOCTL_Xxx (%02d)\n",
                _IOC_NR(cmd)
                ));

            pIoBuffer->ReturnCode = ApiUnsupportedFunction;
            break;
    }

    DebugPrintf(("...Completed message\n"));

    status =
        copy_to_user(
            (IOCTLDATA*)args,
            pIoBuffer,
            sizeof(IOCTLDATA)
            );

    return status;
}
