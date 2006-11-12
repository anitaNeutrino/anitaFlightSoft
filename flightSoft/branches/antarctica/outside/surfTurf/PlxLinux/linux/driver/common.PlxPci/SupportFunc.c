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
 *      SupportFunc.c
 *
 * Description:
 *
 *      Additional support functions
 *
 * Revision History:
 *
 *      10-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/delay.h>
#include "PciSupport.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  Plx_sleep
 *
 * Description:  Function as a normal sleep. Parameter is in millisecond
 *
 ******************************************************************************/
VOID
Plx_sleep(
    U32 delay
    )
{
    mdelay(
        delay
        );
}




/******************************************************************************
 *
 * Function   :  PlxDeviceFind
 *
 * Description:  Search for a specific device using device key parameters
 *
 ******************************************************************************/
RETURN_CODE
PlxDeviceFind(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U8               *pDeviceNumber
    )
{
    U8                DeviceCount;
    U32               RegValue;
    BOOLEAN           bMatchId;
    BOOLEAN           bMatchLoc;
    PLX_DEVICE_NODE  *pNode;
    struct list_head *pEntry;


    DeviceCount = 0;

    pEntry = pdx->List_SupportedDevices.next;

    // Traverse list to find the desired list objects
    while (pEntry != &(pdx->List_SupportedDevices))
    {
        // Get the object
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        // Assume successful match
        bMatchLoc = TRUE;
        bMatchId  = TRUE;

        //
        // Compare device key information
        //

        // Compare Bus number
        if (pKey->bus != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->bus != pNode->Key.bus)
            {
                bMatchLoc = FALSE;
            }
        }

        // Compare Slot number
        if (pKey->slot != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->slot != pNode->Key.slot)
            {
                bMatchLoc = FALSE;
            }
        }

        // Compare Function number
        if (pKey->function != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->function != pNode->Key.function)
            {
                bMatchLoc = FALSE;
            }
        }

        //
        // Compare device ID information
        //

        // Get the Device/Vendor ID
        PLX_PCI_REG_READ(
            pNode,
            CFG_VENDOR_ID,
            &RegValue
            );

        pNode->Key.VendorId = (U16)(RegValue & 0xffff);
        pNode->Key.DeviceId = (U16)(RegValue >> 16);

        // Compare Vendor ID
        if (pKey->VendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->VendorId != pNode->Key.VendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Device ID
        if (pKey->DeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->DeviceId != pNode->Key.DeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Get the Subsystem Device/Vendor ID
        PLX_PCI_REG_READ(
            pNode,
            CFG_SUB_VENDOR_ID,
            &RegValue
            );

        pNode->Key.SubVendorId = (U16)(RegValue & 0xffff);
        pNode->Key.SubDeviceId = (U16)(RegValue >> 16);

        // Compare Subsystem Vendor ID
        if (pKey->SubVendorId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubVendorId != pNode->Key.SubVendorId)
            {
                bMatchId = FALSE;
            }
        }

        // Compare Subsystem Device ID
        if (pKey->SubDeviceId != (U16)PCI_FIELD_IGNORE)
        {
            if (pKey->SubDeviceId != pNode->Key.SubDeviceId)
            {
                bMatchId = FALSE;
            }
        }

        // Get the Revision ID
        PLX_PCI_REG_READ(
            pNode,
            CFG_REV_ID,
            &RegValue
            );

        pNode->Key.Revision = (U8)(RegValue & 0xFF);

        // Compare Revision
        if (pKey->Revision != (U8)PCI_FIELD_IGNORE)
        {
            if (pKey->Revision != pNode->Key.Revision)
            {
                bMatchId = FALSE;
            }
        }

        // Check if match on location and ID
        if (bMatchLoc && bMatchId)
        {
            // Match found, check if it is the desired device
            if (DeviceCount == *pDeviceNumber)
            {
                // Copy the device information
                *pKey = pNode->Key;

                return ApiSuccess;
            }

            // Increment device count
            DeviceCount++;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    // Return number of matched devices
    *pDeviceNumber = DeviceCount;

    return ApiInvalidDeviceInfo;
}




/******************************************************************************
 *
 * Function   :  PlxPciBusScan
 *
 * Description:  Scan the PCI Bus for PCI devices
 *
 ******************************************************************************/
U8
PlxPciBusScan(
    DRIVER_OBJECT *pDriverObject
    )
{
    U8                DeviceCount;
    struct pci_dev   *pPciDev;
    PLX_DEVICE_NODE  *pNode;
    DEVICE_EXTENSION *pdx;


    DebugPrintf(("Scanning PCI bus for devices...\n"));

    // Clear device count
    DeviceCount = 0;

    // Get device extension
    pdx = pDriverObject->DeviceObject->DeviceExtension;

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
            "Adding - %.4x %.4x  [Bus %02x  Slot %02x  Fn %02x]\n",
            pPciDev->device, pPciDev->vendor,
            pPciDev->bus->number, PCI_SLOT(pPciDev->devfn),
            PCI_FUNC(pPciDev->devfn)
            ));

        // Increment number of devices found
        DeviceCount++;

        // Allocate memory for new node
        pNode =
            kmalloc(
                sizeof(PLX_DEVICE_NODE),
                GFP_KERNEL
                );

        // Fill device structure with the PCI information.
        pNode->pPciDevice   = pPciDev;
        pNode->Key.bus      = pPciDev->bus->number;
        pNode->Key.slot     = PCI_SLOT(pPciDev->devfn);
        pNode->Key.function = PCI_FUNC(pPciDev->devfn);
        pNode->Key.VendorId = pPciDev->vendor;
        pNode->Key.DeviceId = pPciDev->device;

        // Set PLX chip version
        PlxChipTypeSet(
            pNode
            );

        // Add node to device list
        list_add_tail(
            &(pNode->ListEntry),
            &(pdx->List_SupportedDevices)
            );

        // Jump to next device
        pPciDev =
            Plx_pci_find_device(
                PCI_ANY_ID,
                PCI_ANY_ID,
                pPciDev
                );
    }

    DebugPrintf((
        "PciBusScan: %d device(s) found\n",
        DeviceCount
        ));

    return DeviceCount;
}




/******************************************************************************
 *
 * Function   :  GetDeviceNodeFromKey
 *
 * Description:  Traverse device list and return the node associated by key
 *
 ******************************************************************************/
PLX_DEVICE_NODE *
GetDeviceNodeFromKey(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey
    )
{
    struct list_head *pEntry;
    PLX_DEVICE_NODE  *pNode;


    pEntry = pdx->List_SupportedDevices.next;

    // Traverse list to find the desired object
    while (pEntry != &(pdx->List_SupportedDevices))
    {
        // Get the object
        pNode =
            list_entry(
                pEntry,
                PLX_DEVICE_NODE,
                ListEntry
                );

        if ((pNode->Key.bus      == pKey->bus) &&
            (pNode->Key.slot     == pKey->slot) &&
            (pNode->Key.function == pKey->function))
        {
            return pNode;
        }

        // Jump to next entry
        pEntry = pEntry->next;
    }

    return NULL;
}





/******************************************************************************
 *
 * Function   :  PlxChipTypeSet
 *
 * Description:  Attempts to determine PLX chip type and revision
 *
 ******************************************************************************/
BOOLEAN
PlxChipTypeSet(
    PLX_DEVICE_NODE *pNode
    )
{
    U32 RegValue;
    U32 PciRevision;


    // Get PCI Class code/Revision ID register
    PLX_PCI_REG_READ(
        pNode,
        CFG_REV_ID,
        &RegValue
        );

    PciRevision = RegValue & 0xFF;

    // Determine device by ID
    switch (((U32)pNode->Key.DeviceId << 16) | pNode->Key.VendorId)
    {
        case 0x905010b5:        // 9050 Device
        case 0x520110b5:        // PLX 9052 RDK
            pNode->PlxChipType = 0x9050;
            if (PciRevision == 0x2)
                pNode->Revision = 2;
            else
                pNode->Revision = 1;
            break;

        case 0x903010b5:        // 9030 Device
        case 0x300110b5:        // PLX 9030 RDK
        case 0x30c110b5:        // PLX 9030 RDK - cPCI
            pNode->PlxChipType = 0x9030;
            pNode->Revision    = 1;
            break;

        case 0x908010b5:        // 9080 Device
        case 0x040110b5:        // PLX 9080-401B RDK
        case 0x086010b5:        // PLX 9080-860 RDK
            pNode->PlxChipType = 0x9080;
            pNode->Revision    = 3;
            break;

        case 0x905410b5:        // 9054 Device
        case 0x540610b5:        // PLX 9054 RDK-LITE
        case 0x186010b5:        // PLX 9054-860 RDK
        case 0xc86010b5:        // PLX 9054-860 RDK - cPCI
            pNode->PlxChipType = 0x9054;
            pNode->Revision    = 0;
            break;

        case 0x905610b5:        // 9056 Device
        case 0x560110b5:        // PLX 9056 RDK-LITE
        case 0x56c210b5:        // PLX 9056-860 RDK
            pNode->PlxChipType = 0x9056;
            pNode->Revision    = 0;
            break;

        case 0x965610b5:        // 9656 Device
        case 0x960110b5:        // PLX 9656 RDK-LITE
        case 0x96c210b5:        // PLX 9656-860 RDK
            pNode->PlxChipType = 0x9656;
            pNode->Revision    = 0;
            break;

        case 0x00213388:        // 6140/6152/6254(NT) Device
            if (PciRevision == 4)
            {
                pNode->PlxChipType = 0x6254;
                pNode->Revision    = 0xBB;
            }
            else
            {
                // Get 6152 VPD register
                PLX_PCI_REG_READ(
                    pNode,
                    0xA0,
                    &RegValue
                    );

                if ((RegValue & 0xF) == CAP_ID_VPD)
                {
                    pNode->PlxChipType = 0x6152;

                    switch (PciRevision)
                    {
                        case 0x13:
                            pNode->Revision = 0xBA;
                            break;

                        case 0x14:
                            pNode->Revision = 0xCA;
                            break;

                        case 0x15:
                            pNode->Revision = 0xCC;
                            break;

                        case 0x16:
                            pNode->Revision = 0xDA;
                            break;

                        default:
                            pNode->Revision = 0x0;
                            break;
                    }
                }
                else
                {
                    pNode->PlxChipType = 0x6140;
                    if (PciRevision == 0x12)
                    {
                        pNode->Revision = 0xAA;
                    }
                    else
                    {
                        // Revsion 0x13 only other
                        pNode->Revision = 0xDA;
                    }
                }
            }
            break;

        case 0x00223388:        // 6150 Device
            pNode->PlxChipType = 0x6150;
 
            if (PciRevision == 0x4)
                pNode->Revision = 0xBB;
            else
                pNode->Revision = 0x0;
            break;

        case 0x00263388:        // 6154 Device
            pNode->PlxChipType = 0x6154;

            if (PciRevision == 0x4)
                pNode->Revision = 0xBB;
            else
                pNode->Revision = 0x0;
            break;

        case 0x00203388:        // 6254 Device
            pNode->PlxChipType = 0x6254;

            if (PciRevision == 0x4)
                pNode->Revision = 0xBB;
            else
                pNode->Revision = 0x0;
            break;

        case 0x00303388:        // 6520 Device
            pNode->PlxChipType = 0x6520;

            if (PciRevision == 0x2)
                pNode->Revision = 0xBB;
            else
                pNode->Revision = 0x0;
            break;

        case 0x00283388:        // 6540 Device - Transparent mode
        case 0x00293388:        // 6540 Device - Non-transparent mode
            pNode->PlxChipType = 0x6540;

            if (PciRevision == 0x2)
                pNode->Revision = 0xBB;
            else
                pNode->Revision = 0x0;
            break;

        default:
            pNode->PlxChipType = 0;
            pNode->Revision    = 0;
            return FALSE;
    }

    return TRUE;
}
