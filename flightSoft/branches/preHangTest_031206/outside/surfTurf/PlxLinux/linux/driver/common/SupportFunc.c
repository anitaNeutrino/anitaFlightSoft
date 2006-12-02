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
 *      11-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/iobuf.h>
#include <linux/ioport.h>
#include "ApiFunctions.h"
#include "CommonApi.h"
#include "DriverDefs.h"
#include "PciSupport.h"
#include "SupportFunc.h"

#if defined(LINUX_24)
    #include <linux/highmem.h>
#endif




/*********************************************************************
*
* Function   :  PlxSynchronizedRegisterModify
*
* Description:  Modify a register synchronized with the ISR
*
**********************************************************************/
BOOLEAN
PlxSynchronizedRegisterModify(
    PLX_REG_DATA *pRegData
    )
{
    unsigned long flags;
    U32           RegValue;


    /*************************************************
     * This routine sychronizes modification of a
     * register with the ISR.  To do this, it uses
     * a special spinlock routine provided by the
     * kernel, which will temporarily disable interrupts.
     * This code should also work on SMP systems.
     ************************************************/

    // Disable interrupts and acquire lock
    spin_lock_irqsave(
        &(pRegData->pdx->Lock_Isr),
        flags
        );

    // Update the register
    RegValue =
        PLX_REG_READ(
            pRegData->pdx,
            pRegData->offset
            );

    RegValue |= pRegData->BitsToSet;
    RegValue &= ~(pRegData->BitsToClear);

    PLX_REG_WRITE(
        pRegData->pdx,
        pRegData->offset,
        RegValue
        );

    // Re-enable interrupts and release lock
    spin_unlock_irqrestore(
        &(pRegData->pdx->Lock_Isr),
        flags
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxSearchDevices
 *
 * Description:  Search for a specific device using device location parameters
 *
 ******************************************************************************/
VOID
PlxSearchDevices(
    DRIVER_OBJECT   *pdo,
    DEVICE_LOCATION *pDeviceCriteria,
    U32             *pSelection
    )
{
    U32               MatchFound;
    BOOLEAN           ContinueComparison;
    DEVICE_OBJECT    *fdo;
    DEVICE_EXTENSION *pdx;


    MatchFound = 0;
    fdo        = pGbl_DriverObject->DeviceObject;

    // Compare with items in device list
    while (fdo != NULL)
    {
        // Get the device extension
        pdx = fdo->DeviceExtension;

        ContinueComparison = TRUE;

        // Compare Serial Number
        if ((pDeviceCriteria->SerialNumber[0] != '\0') && ContinueComparison)
        {
            if (Plx_stricmp(
                    pdx->Device.SerialNumber,
                    pDeviceCriteria->SerialNumber
                    ) != 0)
           {
                ContinueComparison = FALSE;
           }
        }

        // Compare the Bus number
        if ((pDeviceCriteria->BusNumber != (U8)-1) && ContinueComparison)
        {
            if (pDeviceCriteria->BusNumber != pdx->Device.BusNumber)
                ContinueComparison = FALSE;
        }

        // Compare Slot Number
        if ((pDeviceCriteria->SlotNumber != (U8)-1) && ContinueComparison)
        {
            if (pDeviceCriteria->SlotNumber != pdx->Device.SlotNumber)
                ContinueComparison = FALSE;
        }

        // Compare Device ID
        if ((pDeviceCriteria->DeviceId != (U16)-1) && ContinueComparison)
        {
            if (pDeviceCriteria->DeviceId != pdx->Device.DeviceId)
                ContinueComparison = FALSE;
        }

        // Compare Vendor ID
        if ((pDeviceCriteria->VendorId != (U16)-1) && ContinueComparison)
        {
            if (pDeviceCriteria->VendorId != pdx->Device.VendorId)
                ContinueComparison = FALSE;
        }

        if (ContinueComparison == TRUE)
        {
            // Match found, check if it is the device we want
            if (MatchFound++ == *pSelection)
            {
                // Copy the device information
                memcpy(
                    pDeviceCriteria,
                    &pdx->Device,
                    sizeof(DEVICE_LOCATION)
                    );
            }
        }

        fdo = fdo->NextDevice;
    }

    // Return the number of matched devices
    *pSelection = MatchFound;
}




/******************************************************************************
 *
 * Function   :  PlxInterruptWaitObjectRemove_ByTag
 *
 * Description:  Find the interrupt wait object and remove it
 *
 ******************************************************************************/
VOID
PlxInterruptWaitObjectRemove_ByTag(
    DEVICE_EXTENSION *pdx,
    U32               Tag
    )
{
    unsigned long     flags;
    struct list_head *pList;
    INTR_WAIT_OBJECT *pElement;


    spin_lock_irqsave(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    pList = pdx->List_InterruptWait.next;

    // Find the object and remove it
    while (pList != &(pdx->List_InterruptWait))
    {
        // Get the wait object
        pElement =
            list_entry(
                pList,
                INTR_WAIT_OBJECT,
                list
                );

        // Check if the object address matches the Tag
        if ((U32)pElement == Tag)
        {
            // Remove the object from the list
            list_del(
                pList
                );

            spin_unlock_irqrestore(
                &(pdx->Lock_InterruptWaitList),
                flags
                );

            // Wake-up processes only if wait object is pending
            if (pElement->bPending == TRUE)
            {
                DebugPrintf((
                    "Canceling Interrupt wait object (0x%08x)\n",
                    (U32)pElement
                    ));

                // Wake-up any process waiting on the object
                wake_up_interruptible(
                    &(pElement->WaitQueue)
                    );
            }
            else
            {
                DebugPrintf((
                    "Removing Interrupt wait object (0x%08x)\n",
                    (U32)pElement
                    ));
            }

            // Release the object
            kfree(
                pElement
                );

            return;
        }

        // Jump to next item in the list
        pList = pList->next;
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_InterruptWaitList),
        flags
        );
}




/******************************************************************************
 *
 * Function   :  PlxInterruptWaitObjectRemoveAll_ByOwner
 *
 * Description:  Find and cancel all interrupt wait elements by owner
 *
 ******************************************************************************/
VOID
PlxInterruptWaitObjectRemoveAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
    unsigned long     flags;
    struct list_head *pList;
    struct list_head *pCurrent;
    INTR_WAIT_OBJECT *pElement;


    spin_lock_irqsave(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    pList = pdx->List_InterruptWait.next;

    // Find the wait object and remove it
    while (pList != &(pdx->List_InterruptWait))
    {
        // Get the wait object
        pElement =
            list_entry(
                pList,
                INTR_WAIT_OBJECT,
                list
                );

        // Save current object
        pCurrent = pList;

        // Jump to next item in the list
        pList = pList->next;

        // Check if the object owner matches
        if (pElement->pOwner == pOwner)
        {
            // Remove the object from the list
            list_del(
                pCurrent
                );

            // Wake-up processes only if wait object is pending
            if (pElement->bPending == TRUE)
            {
                DebugPrintf((
                    "Canceling Interrupt wait object (0x%08x)\n",
                    (U32)pElement
                    ));

                // Wake-up any process waiting on the object
                wake_up_interruptible(
                    &(pElement->WaitQueue)
                    );
            }
            else
            {
                DebugPrintf((
                    "Removing Interrupt wait object (0x%08x)\n",
                    (U32)pElement
                    ));
            }

            // Release the object
            kfree(
                pElement
                );
        }
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_InterruptWaitList),
        flags
        );
}




/******************************************************************************
 *
 * Function   :  PlxPciBarResourceMap
 *
 * Description:  Claim a PCI BAR resource and map it if a memory space
 *
 ******************************************************************************/
int
PlxPciBarResourceMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    )
{
    // Request and Map space
    if (pdx->PciBar[BarIndex].IsIoMapped)
    {
        if (check_region(
                pdx->PciBar[BarIndex].Physical.u.LowPart,
                pdx->PciBar[BarIndex].Size
                ) != 0)
        {
            ErrorPrintf(("ERROR - Unable to claim I/O port region, already in use\n"));
        }
        else
        {
            // Request I/O port region
            request_region(
                pdx->PciBar[BarIndex].Physical.u.LowPart,
                pdx->PciBar[BarIndex].Size,
                PLX_DRIVER_NAME
                );
        }

        // Do not map I/O space
        pdx->PciBar[BarIndex].pVa = NULL;
    }
    else
    {
        if (check_mem_region(
                pdx->PciBar[BarIndex].Physical.u.LowPart,
                pdx->PciBar[BarIndex].Size
                ) != 0)
        {
            ErrorPrintf(("ERROR - Unable to claim memory region, already in use\n"));
        }
        else
        {
            // Request memory region
            request_mem_region(
                pdx->PciBar[BarIndex].Physical.u.LowPart,
                pdx->PciBar[BarIndex].Size,
                PLX_DRIVER_NAME
                );

            // Get a kernel-mapped virtual address
            pdx->PciBar[BarIndex].pVa =
                ioremap(
                    pdx->PciBar[BarIndex].Physical.u.LowPart,
                    pdx->PciBar[BarIndex].Size
                    );
        }

        if (pdx->PciBar[BarIndex].pVa == NULL)
        {
            /*************************************************************
             * In some cases, a BAR resource mapping may fail due to
             * insufficient page table entries or other resource.  Since
             * the driver requires BAR0  for register access, we must try
             * to map at least the registers.  If this fails, then the
             * driver cannot load.
             *****************************************************************/
            if (BarIndex == 0)
            {
                // Attempt a minimum mapping
                pdx->PciBar[BarIndex].pVa =
                    ioremap(
                        pdx->PciBar[BarIndex].Physical.u.LowPart,
                        MIN_SIZE_MAPPING_BAR_0
                        );

                if (pdx->PciBar[BarIndex].pVa == NULL)
                {
                    return (-EACCES);
                }
                else
                {
                    pdx->PciBar[BarIndex].Size = MIN_SIZE_MAPPING_BAR_0;
                }
            }
            else
            {
                return (-ENOMEM);
            }
        }
    }

    return 0;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarResourcesUnmap
 *
 * Description:  Unmap all mapped PCI BAR memory for a device
 *
 ******************************************************************************/
VOID
PlxPciBarResourcesUnmap(
    DEVICE_EXTENSION *pdx
    )
{
    U8 i;


    // Go through all the BARS
    for (i = 0; i < PCI_NUM_BARS; i++)
    {
        // Unmap the space from Kernel space if previously mapped
        if (pdx->PciBar[i].Physical.u.LowPart != 0)
        {
            if (pdx->PciBar[i].IsIoMapped)
            {
                // Release I/O port region
                release_region(
                    pdx->PciBar[i].Physical.u.LowPart,
                    pdx->PciBar[i].Size
                    );
            }
            else
            {
                DebugPrintf(("Unmapping BAR %d...\n", i));

                // Unmap from kernel space
                if (pdx->PciBar[i].pVa != NULL)
                {
                    iounmap(
                        pdx->PciBar[i].pVa
                        );

                    pdx->PciBar[i].pVa = NULL;
                }

                // Release memory region
                release_mem_region(
                    pdx->PciBar[i].Physical.u.LowPart,
                    pdx->PciBar[i].Size
                    );
            }
        }
    }
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFreeAll_ByOwner
 *
 * Description:  Unmap & release all physical memory assigned to the specified owner
 *
 ******************************************************************************/
VOID
PlxPciPhysicalMemoryFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
    PCI_MEMORY        PciMem;
    struct list_head *pList;
    PLX_PHYSICAL_MEM *pMemObject;


    spin_lock(
        &(pdx->Lock_PhysicalMemList)
        );

    pList = pdx->List_PhysicalMem.next;

    // Traverse list to find the desired list objects
    while (pList != &(pdx->List_PhysicalMem))
    {
        // Get the object
        pMemObject =
            list_entry(
                pList,
                PLX_PHYSICAL_MEM,
                list
                );

        // Check if owner matches
        if (pMemObject->pOwner == pOwner)
        {
            // Copy memory information
            PciMem = pMemObject->PciMem;

            // Release list lock
            spin_unlock(
                &(pdx->Lock_PhysicalMemList)
                );

            // Release the memory & remove from list
            PlxPciPhysicalMemoryFree(
                pdx,
                &PciMem
                ); 

            spin_lock(
                &(pdx->Lock_PhysicalMemList)
                );

            // Restart parsing the list from the beginning
            pList = pdx->List_PhysicalMem.next;
        }
        else
        {
            // Jump to next item
            pList = pList->next;
        }
    }

    spin_unlock(
        &(pdx->Lock_PhysicalMemList)
        );
}




/******************************************************************************
 *
 * Function   :  PlxUserMappingRequestFreeAll_ByOwner
 *
 * Description:  Removes any pending mapping requests assigned to an owner
 *
 ******************************************************************************/
VOID
PlxUserMappingRequestFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
    struct list_head  *pList;
    MAP_PARAMS_OBJECT *pMapObject;


    spin_lock(
        &(pdx->Lock_MapParamsList)
        );

    // Find the mapping parameters from the call to mmap
    pList = pdx->List_MapParams.next;

    // Traverse list to find the desired list objects
    while (pList != &(pdx->List_MapParams))
    {
        // Get the object
        pMapObject =
            list_entry(
                pList,
                MAP_PARAMS_OBJECT,
                list
                );

        if (pMapObject->pOwner == pOwner)
        {
            // Remove the object from the list
            list_del(
                pList
                );

            DebugPrintf((
                "Removed request (0x%08x) to map to user VA 0x%08x (%d Kb)\n",
                (U32)pMapObject,
                (U32)pMapObject->vma.vm_start,
                (U32)((pMapObject->vma.vm_end - pMapObject->vma.vm_start) >> 10)
                ));

            // Release the object
            kfree(
                pMapObject
                );

            // Restart parsing the list from the beginning
            pList = pdx->List_MapParams.next;
        }
        else
        {
            // Jump to next item in the list
            pList = pList->next;
        }
    }

    spin_unlock(
        &(pdx->Lock_MapParamsList)
        );
}




/******************************************************************************
 *
 * Function   :  PlxDmaChannelCleanup
 *
 * Description:  Called by the Cleanup routine to close any open channels
 *
 ******************************************************************************/
VOID
PlxDmaChannelCleanup(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
#if defined(DMA_SUPPORT)
    U8          i;
    DMA_CHANNEL channel;


    // Added to avoid compiler warning
    channel = PrimaryPciChannel0;

    for (i = 0; i < NUMBER_OF_DMA_CHANNELS; i++)
    {
        // Check if terminating application is the owner
        if (pdx->DmaInfo[i].pOwner == pOwner)
        {
            DebugPrintf(("Closing DMA channel %d\n", i));

            switch (i)
            {
                case 0:
                    channel = PrimaryPciChannel0;
                    break;

                case 1:
                    channel = PrimaryPciChannel1;
                    break;

                case 2:
                    channel = IopChannel2;
                    break;
            }

            switch (pdx->DmaInfo[i].state)
            {
                case DmaStateClosed:
                    // DMA closed already, do nothing
                    break;

                case DmaStateBlock:
                    PlxDmaBlockChannelClose(
                        pdx,
                        channel,
                        FALSE
                        );
                    break;

                case DmaStateSgl:
                    PlxDmaSglChannelClose(
                        pdx,
                        channel,
                        FALSE
                        );
                    break;

                default:
                    // Unknown or unsupported state
                    break;
            }
        }
    }
#endif  // DMA_SUPPORT
}




#if defined(DMA_SUPPORT)

/******************************************************************************
 *
 * Function   :  PlxBlockDmaTransferComplete
 *
 * Description:  Perform any necessary cleanup after a Block DMA transfer
 *
 * Note       :  This function is designed to perform cleanup during the
 *               ISR DPC function.
 *
 ******************************************************************************/
VOID
PlxBlockDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    )
{
    if (pdx->DmaInfo[DmaIndex].bPending == FALSE)
    {
        DebugPrintf(("No pending block DMA to complete\n"));
        return;
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Clear the DMA pending flag
    pdx->DmaInfo[DmaIndex].bPending = FALSE;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    // Restore chip state
    PlxChipRestoreAfterDma(
        pdx,
        DmaIndex
        );
}




/******************************************************************************
 *
 * Function   :  PlxSglDmaTransferComplete
 *
 * Description:  Perform any necessary cleanup after an SGL DMA transfer
 *
 * Note       :  This function is designed to perform cleanup during the
 *               ISR DPC function.
 *
 ******************************************************************************/
VOID
PlxSglDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    )
{
    U8  i;
    U32 offset;
    U32 BusAddr;
    U32 PageNum;
    U32 BlockSize;
    U32 BytesRemaining;

    
    if (pdx->DmaInfo[DmaIndex].bPending == FALSE)
    {
        DebugPrintf(("No pending SGL DMA to complete\n"));
        return;
    }

    DebugPrintf((
        "Releasing resources used for SGL DMA transfer...\n"
        ));

    // Release memory used for SGL descriptors
    if (pdx->DmaInfo[DmaIndex].pSgl.pKernelVa != NULL)
    {
        Plx_pci_free_consistent_mem(
            &pdx->DmaInfo[DmaIndex].pSgl
            );

        pdx->DmaInfo[DmaIndex].pSgl.pKernelVa = NULL;
    }

    // Get original buffer size
    BytesRemaining = pdx->DmaInfo[DmaIndex].BufferSize;

    // Unmap previous mappings
    i = 0;
    while (i < pdx->DmaInfo[DmaIndex].NumIoBuff)
    {
        // Set offset for initial page
        offset = pdx->DmaInfo[DmaIndex].pIoVector[i]->offset;

        // Unmap the individual pages
        PageNum = 0;
        while (PageNum < pdx->DmaInfo[DmaIndex].pIoVector[i]->nr_pages)
        {
            // Calculate transfer size
            if (BytesRemaining > (PAGE_SIZE - offset))
            {
                BlockSize = PAGE_SIZE - offset;
            }
            else
            {
                BlockSize = BytesRemaining;
            }

            // Get bus address for page
            BusAddr =
                Plx_page_to_bus(
                    pdx->DmaInfo[DmaIndex].pIoVector[i]->maplist[PageNum]
                    );

            // Add offset
            BusAddr += offset;

            Plx_pci_unmap_page(
                pdx->pPciDevice,
                BusAddr,
                BlockSize,
                pdx->DmaInfo[DmaIndex].direction
                );

            // Clear offset
            offset = 0;

            // Adjust bytes remaining
            BytesRemaining -= BlockSize;

            // Go to next page
            PageNum++;
        }

        // Unmap the I/O buffer
        unmap_kiobuf(
            pdx->DmaInfo[DmaIndex].pIoVector[i]
            );

        i++;
    }

    // Unlock pages from memory
    unlock_kiovec(
        pdx->DmaInfo[DmaIndex].NumIoBuff,
        pdx->DmaInfo[DmaIndex].pIoVector
        );

    // Release I/O Vectors
    free_kiovec(
        pdx->DmaInfo[DmaIndex].NumIoBuff,
        pdx->DmaInfo[DmaIndex].pIoVector
        );

    // Release I/O buffer pointer memory
    kfree(
        pdx->DmaInfo[DmaIndex].pIoVector
        );

    // Restore chip state
    PlxChipRestoreAfterDma(
        pdx,
        DmaIndex
        );

    // Clear the DMA pending flag
    pdx->DmaInfo[DmaIndex].bPending = FALSE;
}




/******************************************************************************
 *
 * Function   :  PlxLockBufferAndBuildSgl
 *
 * Description:  Lock a user buffer and build an SGL for it
 *
 ******************************************************************************/
VOID*
PlxLockBufferAndBuildSgl(
    DEVICE_EXTENSION     *pdx,
    U8                    DmaIndex,
    DMA_TRANSFER_ELEMENT *pDma
    )
{
    int rc;
    int rw;
    U8  i;
    U32 VaSgl;
    U32 BusSgl;
    U32 offset;
    U32 UserVa;
    U32 PageNum;
    U32 BusAddr;
    U32 BlockSize;
    U32 LocalAddr;
    U32 TotalPages;
    U32 BusSglOriginal;
    U32 BytesRemaining;


    DebugPrintf((
        "Building SGL (UserAddr=%08x, LocAddr=%08x, %d bytes)\n",
        pDma->u.UserVa,
        pDma->LocalAddr,
        pDma->TransferCount
        ));

    // Calculate the number of I/O vectors needed
    offset                           = pDma->u.UserVa & (PAGE_SIZE - 1);
    BytesRemaining                   = pDma->TransferCount;
    pdx->DmaInfo[DmaIndex].NumIoBuff = 0;
    while (BytesRemaining != 0)
    {
        // Add an I/O buffer
        pdx->DmaInfo[DmaIndex].NumIoBuff++;

        if (BytesRemaining <= (KIO_MAX_ATOMIC_IO << 10) - offset)
        {
            BytesRemaining = 0;
        }
        else
        {
            BytesRemaining -= (KIO_MAX_ATOMIC_IO << 10) - offset;
        }

        // Clear offset
        offset = 0;
    }

    // Allocate memory for I/O buffer pointers
    pdx->DmaInfo[DmaIndex].pIoVector =
        kmalloc(
            pdx->DmaInfo[DmaIndex].NumIoBuff * sizeof(struct kiobuf *),
            GFP_KERNEL
            );

    if (pdx->DmaInfo[DmaIndex].pIoVector == NULL)
    {
        DebugPrintf(("ERROR - Unable to allocate I/O buffer pointers\n"));
        return NULL;
    }

    // Allocate I/O vector
    rc =
        alloc_kiovec(
            pdx->DmaInfo[DmaIndex].NumIoBuff,
            pdx->DmaInfo[DmaIndex].pIoVector
            );

    if (rc != 0)
    {
        DebugPrintf(("ERROR - Unable to allocate I/O vector\n"));

        kfree(
            pdx->DmaInfo[DmaIndex].pIoVector
            );

        return NULL;
    }

    // Determine transfer direction
    if (pDma->LocalToPciDma)
        rw = READ;
    else
        rw = WRITE;

    // Map user buffer
    i              = 0;
    UserVa         = pDma->u.UserVa;
    BytesRemaining = pDma->TransferCount;
    TotalPages     = 0;
    offset         = pDma->u.UserVa & (PAGE_SIZE - 1);

    while (BytesRemaining != 0)
    {
        // Determine size of current block
        if (BytesRemaining <= (KIO_MAX_ATOMIC_IO << 10) - offset)
        {
            BlockSize = BytesRemaining;
        }
        else
        {
            BlockSize = (KIO_MAX_ATOMIC_IO << 10) - offset;
        }

        // Map up to limit of I/O buffer
        rc =
            map_user_kiobuf(
                rw,
                pdx->DmaInfo[DmaIndex].pIoVector[i],
                UserVa,
                BlockSize
                );

        if (rc != 0)
        {
            DebugPrintf(("ERROR - Unable to map user buffer\n"));

            // Unmap previous mappings
            while (i != 0)
            {
                unmap_kiobuf(
                    pdx->DmaInfo[DmaIndex].pIoVector[i]
                    );

                i--;
            }

            free_kiovec(
                pdx->DmaInfo[DmaIndex].NumIoBuff,
                pdx->DmaInfo[DmaIndex].pIoVector
                );

            kfree(
                pdx->DmaInfo[DmaIndex].pIoVector
                );

            return NULL;
        }

        // Update total number of pages
        TotalPages += pdx->DmaInfo[DmaIndex].pIoVector[i]->nr_pages;

        // Switch to next I/O buffer
        i++;

        // Clear offset
        offset = 0;

        // Adjust to next base address
        UserVa += BlockSize;

        // Adjust bytes remaining
        BytesRemaining -= BlockSize;
    }

    // Lock the pages into memory
    rc =
        lock_kiovec(
            pdx->DmaInfo[DmaIndex].NumIoBuff,
            pdx->DmaInfo[DmaIndex].pIoVector,
            1            // Wait for pages to lock
            );

    if (rc != 0)
    {
        DebugPrintf(("ERROR - Unable to lock pages into memory\n"));
    }


    /******************************************************
     * Build SGL descriptors
     *
     * The following code will build the SGL descriptors
     * in PCI memory.  There will be one descriptor for
     * each page of memory since the pages are scattered
     * throughout physical memory.
     *****************************************************/

    /*************************************************************
     * Calculate memory needed for SGL descriptors
     *
     * Mem needed = #pages * size of descriptor
     *
     * Additionally, a value of 16 (10h) is added to provide a
     * buffer to allow space to round up to the next 16-byte
     * boundary, which is a requirement of the hardware.
     ************************************************************/
    pdx->DmaInfo[DmaIndex].pSgl.PciMem.Size = (TotalPages * (4 * sizeof(U32))) + 16;

    // Allocate memory for SGL descriptors
    VaSgl =
        (U32)Plx_pci_alloc_consistent_mem(
                 &pdx->DmaInfo[DmaIndex].pSgl
                 );

    if (VaSgl == (U32)NULL)
    {
        DebugPrintf((
            "ERROR - Unable to allocate %d bytes for %d SGL descriptors\n",
            pdx->DmaInfo[DmaIndex].pSgl.PciMem.Size, TotalPages
            ));

        return NULL;
    }

    // Prepare for build of SGL
    LocalAddr = pDma->LocalAddr;

    // Make sure address is aligned on 16-byte boundary
    VaSgl = (VaSgl + 0xF) & ~(0xF);

    // Get physical address of SGL descriptors
    BusSgl =
        virt_to_bus(
            (void *)VaSgl
            );

    // Save start of SGL descriptors
    BusSglOriginal = BusSgl;

    // Store total buffer size
    pdx->DmaInfo[DmaIndex].BufferSize = pDma->TransferCount;

    // Store DMA transfer direction
    if (pDma->LocalToPciDma)
        pdx->DmaInfo[DmaIndex].direction = PCI_DMA_FROMDEVICE;
    else
        pdx->DmaInfo[DmaIndex].direction = PCI_DMA_TODEVICE;

    // Set total bytes
    BytesRemaining = pDma->TransferCount;

    i = 0;
    while (i < pdx->DmaInfo[DmaIndex].NumIoBuff)
    {
        // Set offset for initial page
        offset = pdx->DmaInfo[DmaIndex].pIoVector[i]->offset;

        PageNum = 0;
        while (PageNum < pdx->DmaInfo[DmaIndex].pIoVector[i]->nr_pages)
        {
            // Calculate transfer size
            if (BytesRemaining > (PAGE_SIZE - offset))
            {
                BlockSize = PAGE_SIZE - offset;
            }
            else
            {
                BlockSize = BytesRemaining;
            }

            // Get bus address of buffer
            BusAddr =
                Plx_pci_map_page(
                    pdx->pPciDevice,
                    pdx->DmaInfo[DmaIndex].pIoVector[i]->maplist[PageNum],
                    offset,
                    BlockSize,
                    pdx->DmaInfo[DmaIndex].direction
                    );

            // Write PCI address in descriptor
            *(((U32*)VaSgl) + SGL_DESC_IDX_PCI_ADDR) = BusAddr;

            // Write Local address in descriptor
            *(((U32*)VaSgl) + SGL_DESC_IDX_LOC_ADDR) = LocalAddr;

            // Write transfer count in descriptor
            *(((U32*)VaSgl) + SGL_DESC_IDX_COUNT) = BlockSize;

            // Adjust byte count
            BytesRemaining -= BlockSize;

            if (BytesRemaining == 0)
            {
                // Set the last descriptor
                *(((U32*)VaSgl) + SGL_DESC_IDX_NEXT_DESC) =
                                (pDma->LocalToPciDma << 3) | (1 << 1) | (1 << 0);
            }
            else
            {
                // Calculate address of next descriptor
                BusSgl += (4 * sizeof(U32));

                // Write next descriptor address
                *(((U32*)VaSgl) + SGL_DESC_IDX_NEXT_DESC) =
                                 BusSgl | (pDma->LocalToPciDma << 3) | (1 << 0);

                // Adjust Local address
                if (pdx->DmaInfo[DmaIndex].bLocalAddrConstant == FALSE)
                    LocalAddr += BlockSize;

                // Adjust virtual address of next descriptor
                VaSgl += (4 * sizeof(U32));

                // Clear offset
                offset = 0;
            }

            // Go to next page
            PageNum++;
        }

        i++;
    }

    return (VOID*)BusSglOriginal;
}

#endif  // DMA_SUPPORT




/******************************************************************************
 *
 * Function   :  IsSupportedDevice
 *
 * Description:  Determine if device is supported by the driver
 *
 ******************************************************************************/
BOOLEAN
IsSupportedDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pDev
    )
{
    U8  i;
    U32 DevVenID;


    // Check for empty list
    if (pDriverObject->SupportedIDs[0] == '\0')
        return FALSE;

    i = 0;

    // Compare device with list of supported IDs
    do
    {
        DevVenID = 0;

        // Get next ID
        while (pDriverObject->SupportedIDs[i] != '\0' &&
               pDriverObject->SupportedIDs[i] != ' ')
        {
            // Shift in next character
            DevVenID <<= 4;

            if ((pDriverObject->SupportedIDs[i] >= 'A') &&
                (pDriverObject->SupportedIDs[i] <= 'F'))
            {
                DevVenID |= (pDriverObject->SupportedIDs[i] - 'A' + 0xA);
            }
            else if ((pDriverObject->SupportedIDs[i] >= 'a') &&
                     (pDriverObject->SupportedIDs[i] <= 'f'))
            {
                DevVenID |= (pDriverObject->SupportedIDs[i] - 'a' + 0xA);
            }
            else
            {
                DevVenID |= (pDriverObject->SupportedIDs[i] - '0');
            }

            // Jump to next character
            i++;
        }

        // Compare ID with PCI device
        if ( (pDev->vendor == (DevVenID & 0xffff)) &&
             (pDev->device == (DevVenID >> 16)) )
        {
            // Device IDs match
            return TRUE;
        }

        // Skip over space character
        if (pDriverObject->SupportedIDs[i] == ' ')
            i++;
    }
    while (DevVenID != 0);

    // No match
    return FALSE;
}




/******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_8
 *
 * Description:  Copy data from device to a user-mode buffer, 8-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_8(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    )
{
    U8 value;


    while (count)
    {
        // Get next value from device
        value =
            readb(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            (U8*)VaUser
            );

        // Increment pointers
        ((U8*)VaDev)++;
        ((U8*)VaUser)++;

        // Decrement count
        count -= sizeof(U8);
    }
}




/******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_16
 *
 * Description:  Copy data from device to a user-mode buffer, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_16(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    )
{
    U16 value;


    while (count)
    {
        // Get next value from device
        value =
            readw(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            (U16*)VaUser
            );

        // Increment pointers
        ((U16*)VaDev)++;
        ((U16*)VaUser)++;

        // Decrement count
        count -= sizeof(U16);
    }
}




/******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_32
 *
 * Description:  Copy data from device to a user-mode buffer, 32-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_32(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    )
{
    U32 value;


    while (count)
    {
        // Get next value from device
        value =
            readl(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            (U32*)VaUser
            );

        // Increment pointers
        ((U32*)VaDev)++;
        ((U32*)VaUser)++;

        // Decrement count
        count -= sizeof(U32);
    }
}




/******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_8
 *
 * Description:  Copy data from a user-mode buffer to device, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_8(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    )
{
    U8 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            (U8*)VaUser
            );

        // Write value to device
        writeb(
            value,
            VaDev
            );

        // Increment pointers
        ((U8*)VaDev)++;
        ((U8*)VaUser)++;

        // Decrement count
        count -= sizeof(U8);
    }
}




/******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_16
 *
 * Description:  Copy data from a user-mode buffer to device, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_16(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    )
{
    U16 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            (U16*)VaUser
            );

        // Write value to device
        writew(
            value,
            VaDev
            );

        // Increment pointers
        ((U16*)VaDev)++;
        ((U16*)VaUser)++;

        // Decrement count
        count -= sizeof(U16);
    }
}




/******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_32
 *
 * Description:  Copy data from a user-mode buffer to device, 32-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_32(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    )
{
    U32 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            (U32*)VaUser
            );

        // Write value to device
        writel(
            value,
            VaDev
            );

        // Increment pointers
        ((U32*)VaDev)++;
        ((U32*)VaUser)++;

        // Decrement count
        count -= sizeof(U32);
    }
}




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
 * Function   :  Plx_stricmp
 *
 * Description:  Performs a lowercase comparison of 2 strings.  Needed because
 *               kernel doesn't export a similar string function.
 *
 ******************************************************************************/
int
Plx_stricmp(
    const char *string1,
    const char *string2
    )
{
    char c1;
    char c2;


    do
    {
        // Convert characters to lowercase
        c1 = tolower(*string1);
        c2 = tolower(*string2);

        // Compare characters
        if (c1 < c2)
            return -1;

        if (c1 > c2)
            return 1;

        // Move to next character
        string1++;
        string2++;
    }
    while (c1 != '\0');

    // Strings are equal
    return 0;
}
