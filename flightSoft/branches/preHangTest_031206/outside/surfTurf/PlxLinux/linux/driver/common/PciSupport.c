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
 *      05-01-03 : PCI SDK v4.10
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
U32
PlxPciRegisterRead(
    DEVICE_EXTENSION *pdx,
    U8                bus,
    U8                slot,
    U16               offset,
    RETURN_CODE      *pReturnCode
    )
{
    int             rc;
    U8              function;
    U32             RegValue;
    struct pci_dev *pPciDevice;


    // Offset must on a 4-byte boundary
    if (offset & 0x3)
    {
        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidOffset;

        return (U32)-1;
    }

    // Function number is in upper 3-bits of slot
    function = slot >> 5;

    // Clear upper 3-bits
    slot &= 0x1F;

    // Locate PCI device 
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

        if (pReturnCode != NULL)
            *pReturnCode = ApiInvalidDeviceInfo;

        return (U32)-1;
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

        if (pReturnCode != NULL)
            *pReturnCode = ApiConfigAccessFailed;

        return (U32)-1;
    }

    if (pReturnCode != NULL)
        *pReturnCode = ApiSuccess;

    return RegValue;
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
    DEVICE_EXTENSION *pdx,
    U8                bus,
    U8                slot,
    U16               offset,
    U32               value
    )
{
    int             rc;
    U8              function;
    struct pci_dev *pPciDevice;


    // Offset must on a 4-byte boundary
    if (offset & 0x3)
    {
        return ApiInvalidOffset;
    }

    // Function number is in upper 3-bits of slot
    function = slot >> 5;

    // Clear upper 3-bits
    slot &= 0x1F;

    // Locate PCI device 
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




/******************************************************************************
 *
 * Function   :  Plx_pci_alloc_consistent_mem
 *
 * Arguments  :  PLX_PHYSICAL_MEM *  - A pointer to a structure which will hold
 *                                     information about the allocated memory
 *
 * Description:  The function allocates a contiguous block of system memory and
 *               marks it as reserved. Marking the memory as reserved makes it
 *               possible to map it to a user virtual address with remap_page_range.
 *
 ******************************************************************************/
void*
Plx_pci_alloc_consistent_mem(
    PLX_PHYSICAL_MEM *pMemObject
    )
{
    int size;
    int pages;
    U32 virt_addr;


    // Align size to next page boundary
    size =
        PAGE_ALIGN(
            pMemObject->PciMem.Size
            );

    // Get number of pages to allocate
    pages = size >> PAGE_SHIFT;

    // Get the log2(N) to get the order
    pMemObject->order = 0;
    while (pages > (1 << pMemObject->order))
    {
        (pMemObject->order)++;
    }

    // Attempt to allocate contiguous memory
    pMemObject->pKernelVa =
        (VOID *)__get_free_pages(
                    GFP_KERNEL,
                    pMemObject->order
                    );

    if (pMemObject->pKernelVa == NULL)
    {
        return NULL;
    }

    // Tag all pages as reserved
    for (virt_addr = (U32)pMemObject->pKernelVa;
         virt_addr < (U32)pMemObject->pKernelVa + size; 
         virt_addr += PAGE_SIZE)
    {
#if defined(LINUX_22)
        mem_map_reserve(
            MAP_NR(virt_addr)
            );
#else
        mem_map_reserve(
            virt_to_page(virt_addr)
            );
#endif
    }

    // Get bus address of buffer
    pMemObject->PciMem.PhysicalAddr =
        virt_to_bus(
            pMemObject->pKernelVa
            );

    // Return the kernel virtual address of the allocated block
    return pMemObject->pKernelVa;
}




/******************************************************************************
 *
 * Function   :  Plx_pci_free_consistent_mem
 *
 * Description:  Free a previously allocated physically contiguous buffer
 *
 ******************************************************************************/
void
Plx_pci_free_consistent_mem(
    PLX_PHYSICAL_MEM *pMemObject
    )
{
    U32 virt_addr;


    // Remove reservation tag for all pages
    for (virt_addr = (U32)pMemObject->pKernelVa; 
         virt_addr < (U32)pMemObject->pKernelVa + PAGE_ALIGN(pMemObject->PciMem.Size); 
         virt_addr += PAGE_SIZE)
    {
#if defined(LINUX_22)
        mem_map_unreserve(
            MAP_NR(virt_addr)
            );
#else
        mem_map_unreserve(
            virt_to_page(virt_addr)
            );
#endif
    }

    // Release the buffer
    free_pages(
        (unsigned long)(pMemObject->pKernelVa),
        pMemObject->order
        );
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




/******************************************************************************
 *
 * Function   :  Plx_pci_resource_len
 *
 * Description:  Returns the size of a PCI BAR region
 *
 ******************************************************************************/
U32
Plx_pci_resource_len(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    )
{
    U16 offset;
    U32 size;
    U32 RegValue;


    // Calculate BAR offset
    offset = (BarIndex * sizeof(U32)) + CFG_BAR0;

    // Get current value
    PLX_PCI_REG_READ(
        pdx,
        offset,
        &RegValue
        );

    // Query space for size
    PLX_PCI_REG_WRITE(
        pdx,
        offset,
        0xffffffff
        );

    // Get size
    PLX_PCI_REG_READ(
        pdx,
        offset,
        &size
        );

    // Restore register value
    PLX_PCI_REG_WRITE(
        pdx,
        offset,
        RegValue
        );

    // Clear non-mask bits
    if (size & (1 << 0))
    {
        // I/O address mask is [31:2]
        size &= ~((1 << 1) | (1 << 0));
    }
    else
    {
        // Memory address mask is [31:4]
        size &= ~((1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
    }

    // Calculate size
    size = (~size) + 1;

    return size;
}


#endif   // LINUX_22
