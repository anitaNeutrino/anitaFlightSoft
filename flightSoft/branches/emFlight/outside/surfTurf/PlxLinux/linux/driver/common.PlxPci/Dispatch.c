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
 *      11-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/module.h>
#include "ApiFunctions.h"
#include "Dispatch.h"
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
    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ==> OPEN_DEVICE\n"));

    if (MINOR(inode->i_rdev) != PLX_MNGMT_INTERFACE)
    {
        ErrorPrintf(("ERROR - Attempt to open non-existent device interface\n"));

        return (-ENODEV);
    }

    DebugPrintf(("Opening device interface...\n"));

    // Store device object for future calls
    filp->private_data = pGbl_DriverObject->DeviceObject;

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
    DebugPrintf_NoInfo(("\n"));
    DebugPrintf(("Received message ==> CLOSE_DEVICE\n"));

    if (MINOR(inode->i_rdev) != PLX_MNGMT_INTERFACE)
    {
        ErrorPrintf(("ERROR - Attempt to close non-existent device interface\n"));

        return (-ENODEV);
    }

    DebugPrintf(("Closing device interface...\n"));

    // Clear the device object from the private data
    filp->private_data = NULL;

    // Decrement driver use count for kernel
    MOD_DEC_USE_COUNT;

    DebugPrintf(("...device closed\n"));

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

    // Verify interface
    if (MINOR(inode->i_rdev) != PLX_MNGMT_INTERFACE)
    {
        ErrorPrintf((
            "ERROR - Attempt to send message to non-exist device interface\n"
            ));

        return (-EOPNOTSUPP);
    }

    // Get the device extension
    pdx = ((DEVICE_OBJECT*)(filp->private_data))->DeviceExtension;

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
         *    Miscellaneous functions
         **********************************/
        case PLX_IOCTL_PCI_DEVICE_FIND:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_DEVICE_FIND\n"));

            pIoBuffer->ReturnCode =
                PlxDeviceFind(
                    pdx,
                    &(pIoBuffer->u.MgmtData.u.Key),
                    (U8*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_DRIVER_VERSION:
            DebugPrintf_NoInfo(("PLX_IOCTL_DRIVER_VERSION\n"));

            pIoBuffer->u.MgmtData.value =
                    (DRIVER_VERSION_MAJOR    << 16) |
                    (DRIVER_VERSION_MINOR    <<  8) |
                    (DRIVER_VERSION_REVISION <<  0);
            break;

        case PLX_IOCTL_CHIP_TYPE_GET:
            DebugPrintf_NoInfo(("PLX_IOCTL_CHIP_TYPE_GET\n"));

            pIoBuffer->ReturnCode =
                PlxChipTypeGet(
                    pdx,
                    &(pIoBuffer->u.MgmtData.u.Key),
                    &(pIoBuffer->u.MiscData.data[0]),
                    (U8*)&(pIoBuffer->u.MiscData.data[1])
                    );
            break;

        case PLX_IOCTL_PCI_BOARD_RESET:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_BOARD_RESET\n"));

            PlxPciBoardReset(
                pdx
                );
            break;


        /*******************************************
         * PCI Register Access Functions
         ******************************************/
        case PLX_IOCTL_PCI_REGISTER_READ:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REGISTER_READ\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterRead(
                    pIoBuffer->u.MgmtData.u.Key.bus,
                    pIoBuffer->u.MgmtData.u.Key.slot,
                    pIoBuffer->u.MgmtData.u.Key.function,
                    pIoBuffer->u.MgmtData.offset,
                    &(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_PCI_REGISTER_WRITE:
            DebugPrintf_NoInfo(("PLX_IOCTL_PCI_REGISTER_WRITE\n"));

            pIoBuffer->ReturnCode =
                PlxPciRegisterWrite(
                    pIoBuffer->u.MgmtData.u.Key.bus,
                    pIoBuffer->u.MgmtData.u.Key.slot,
                    pIoBuffer->u.MgmtData.u.Key.function,
                    pIoBuffer->u.MgmtData.offset,
                    pIoBuffer->u.MgmtData.value
                    );
            break;


        /***********************************
         * Serial EEPROM Access Functions
         **********************************/
        case PLX_IOCTL_EEPROM_PRESENT:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_PRESENT\n"));

            pIoBuffer->ReturnCode =
                PlxEepromPresent(
                    pdx,
                    &(pIoBuffer->u.MgmtData.u.Key),
                    (BOOLEAN*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_EEPROM_READ_BY_OFFSET:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_READ_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxEepromReadByOffset(
                    pdx,
                    &(pIoBuffer->u.MgmtData.u.Key),
                    pIoBuffer->u.MgmtData.offset,
                    (U16*)&(pIoBuffer->u.MgmtData.value)
                    );
            break;

        case PLX_IOCTL_EEPROM_WRITE_BY_OFFSET:
            DebugPrintf_NoInfo(("PLX_IOCTL_EEPROM_WRITE_BY_OFFSET\n"));

            pIoBuffer->ReturnCode =
                PlxEepromWriteByOffset(
                    pdx,
                    &(pIoBuffer->u.MgmtData.u.Key),
                    pIoBuffer->u.MgmtData.offset,
                    (U16)pIoBuffer->u.MgmtData.value
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
