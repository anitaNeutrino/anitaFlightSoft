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
 *      PciSupport.c
 *
 * Description:
 *
 *      This file contains the PCI access functions.
 *
 * Revision History:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "PciSupport.h"
#include "linux/wrapper.h"




/******************************************************************************
 *
 * Function   :  PlxPciRegisterRead
 *
 * Description:  Reads from a PCI register at the specified offset
 *
 ******************************************************************************/
RETURN_CODE
PlxPciRegisterRead(
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
    int             rc;
    U32             RegValue;
    struct pci_dev *pPciDevice;


    // Offset must on a 4-byte boundary
    if (offset & 0x3)
    {
        if (pValue != NULL)
            *pValue = (U32)-1;

        return ApiInvalidOffset;
    }

    // Locate the PCI device
    pPciDevice =
        pci_find_slot(
            bus,
            PCI_DEVFN(slot, function)
            );

    if (pPciDevice == NULL)
    {
        DebugPrintf((
            "ERROR - Device at bus %02d, slot %02d does not exist\n",
            bus, slot
            ));

        if (pValue != NULL)
            *pValue = (U32)-1;

        return ApiInvalidDeviceInfo;
    }

    rc =
        pci_read_config_dword(
            pPciDevice,
            offset,
            &RegValue
            );

    if (rc != 0)
    {
        DebugPrintf((
            "ERROR - Unable to read PCI register 0x%02x\n",
            offset
            ));

        if (pValue != NULL)
            *pValue = (U32)-1;

        return ApiConfigAccessFailed;
    }

    if (pValue != NULL)
        *pValue = RegValue;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciRegisterWrite
 *
 * Description:  Writes a value to a PCI register at the specified offset
 *
 ******************************************************************************/
RETURN_CODE
PlxPciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    int             rc;
    struct pci_dev *pPciDevice;


    // Offset must on a 4-byte boundary
    if (offset & 0x3)
    {
        return ApiInvalidOffset;
    }

    // Locate the PCI device
    pPciDevice =
        pci_find_slot(
            bus,
            PCI_DEVFN(slot, function)
            );

    if (pPciDevice == NULL)
    {
        DebugPrintf((
            "ERROR - Device at bus %02d, slot %02d does not exist\n",
            bus, slot
            ));

        return ApiInvalidDeviceInfo;
    }

    rc =
        pci_write_config_dword(
            pPciDevice,
            offset,
            value
            );

    if (rc != 0)
    {
        DebugPrintf((
            "ERROR - Unable to write to PCI register 0x%02x\n",
            offset
            ));

        return ApiConfigAccessFailed;
    }

    return ApiSuccess;
}






/**********************************************************************
 *
 * The following code is provided to support Linux kernel v2.2.  In
 * kernel v2.4, functions similar to the ones below are already
 * provided.  The code below allows for a common code base between
 * the various kernel versions.
 *
 *********************************************************************/

#if defined(LINUX_22)


/******************************************************************************
 *
 * Function   :  Plx_pci_find_device
 *
 * Description:  Locates a device matching the Vendor/Device ID
 *
 ******************************************************************************/
struct pci_dev*
Plx_pci_find_device(
    unsigned int          vendor,
    unsigned int          device,
    const struct pci_dev *from
    )
{
    BOOLEAN         bMatch;
    struct pci_dev *pPciDev;


    // If no previous device, start from head of list
    if (from == NULL)
        pPciDev = pci_devices;
    else
        pPciDev = from->next;

    while (pPciDev)
    {
        // Start with positive match
        bMatch = TRUE;

        // Compare Vendor ID
        if ((vendor != PCI_ANY_ID) && (vendor != pPciDev->vendor))
        {
            bMatch = FALSE;
        }

        // Compare Device ID
        if ((device != PCI_ANY_ID) && (device != pPciDev->device))
        {
            bMatch = FALSE;
        }

        // Check if device is found
        if (bMatch)
        {
            return pPciDev;
        }

        // Jump to next device
        pPciDev = pPciDev->next;
    }

    // Device not found
    return NULL;
}


#endif   // LINUX_22
