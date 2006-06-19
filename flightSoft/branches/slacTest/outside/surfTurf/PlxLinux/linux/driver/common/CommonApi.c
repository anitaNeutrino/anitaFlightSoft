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
 *      CommonApi.c
 *
 * Description:
 *
 *      API functions common to all PLX chips
 *
 * Revision History:
 *
 *      10-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "ApiFunctions.h"
#include "CommonApi.h"
#include "DriverDefs.h"
#include "PciSupport.h"
#include "SupportFunc.h"




/*********************************************************************
 *
 * Function   :  PlxChipTypeGet
 *
 * Description:  Gets the PLX chip type and revision
 *
 *********************************************************************/
VOID
PlxChipTypeGet(
    DEVICE_EXTENSION *pdx,
    U32              *pChipType,
    U8               *pRevision
    )
{
    U32 RegValue;
    U32 RegValue_Original;


    /*********************************************************
     * The algorithm for determining chip type is as follows:
     *
     * - Read local register offset 0x4
     * - Write the register back with the upper byte set to 0xf
     * - Read reg and check upper byte.
     * - If still 0, then chip is 9030/9050/9052
     * - If set to 0xf, then chip is 9080/9054
     *
     * For 9080/9054/9656/9056:
     *   - Hard Coded ID determines chip type
     *   - Revision ID register determines revision
     *      - 9054 requires some additional tests
     *
     * For 9030/9050/9052:
     *   - If VPD lower byte is 0x3, chip is 9030
     *   - If not, revision register determines 9050 or 9052
     *
     ********************************************************/

    // Read register offset 0x4
    RegValue_Original =
        PLX_REG_READ(
            pdx,
            0x04
            );

    // Set upper byte to 0xff
    PLX_REG_WRITE(
        pdx,
        0x4,
        RegValue_Original | (0xFF << 24)
        );

    // Read register offset 0x4
    RegValue =
        PLX_REG_READ(
            pdx,
            0x04
            );

    // Restore register
    PLX_REG_WRITE(
        pdx,
        0x4,
        RegValue_Original
        );

    // If bits 24-27 are not written, a register access problem exists
    if ((RegValue >> 24) == 0)
    {
        // Unknown chip type
        *pChipType = 0x0;
        *pRevision = 0x0;
        return;
    }

    // Check if upper bits were set
    if ((RegValue >> 28) == 0xf)
    {
        // Check Hard-coded ID
        RegValue =
            PLX_REG_READ(
                pdx,
                0x70               // PERM_VENDOR_ID
                );

        // Check for 9080
        if (RegValue == 0x908010b5)
        {
            *pChipType = 0x9080;
            RegValue =
                PLX_REG_READ(
                    pdx,
                    0x74           // REVISION_ID
                    );

            *pRevision = (U8)RegValue;

            return;
        }

        // Check for 9054
        if (RegValue == 0x905410b5)
        {
            *pChipType = 0x9054;

            // Check the revision register
            RegValue =
                PLX_REG_READ(
                    pdx,
                    0x74           // REVISION_ID
                    );

            if (RegValue != 0xA)
            {
                if (RegValue == 0xC)
                {
                    // Set value for AC revision
                    *pRevision = 0xAC;
                }
                else
                {
                    *pRevision = (U8)RegValue;
                }

                return;
            }

            PLX_PCI_REG_READ(
                pdx,
                CFG_REV_ID,
                &RegValue
                );

            if ((RegValue & 0xf) == 0xb)
                *pRevision = 0xAB;
            else
                *pRevision = 0xAA;

            return;
        }

        // Check for 9056
        if (RegValue == 0x905610b5)
        {
            *pChipType = 0x9056;

            RegValue =
                PLX_REG_READ(
                    pdx,
                    0x74           // REVISION_ID
                    );

            *pRevision = (U8)RegValue;

            return;
        }

        // Check for 9656
        if (RegValue == 0x965610b5)
        {
            *pChipType = 0x9656;

            RegValue =
                PLX_REG_READ(
                    pdx,
                    0x74           // REVISION_ID
                    );

            *pRevision = (U8)RegValue;

            return;
        }
    }
    else
    {
        // Read PCI next capabilites register
        PLX_PCI_REG_READ(
            pdx,
            0x34,                  // NEXT_CAP_PTR
            &RegValue
            );

        if (RegValue != 0x0)
        {
            *pChipType = 0x9030;
            *pRevision = 1;
            return;
        }

        // 9050/52 is final possibility
        *pChipType = 0x9050;

        // Read 9050/9052 PCI revision
        PLX_PCI_REG_READ(
            pdx,
            CFG_REV_ID,
            &RegValue
            );

        if ((RegValue & 0xF) == 0x2)
        {
            *pRevision = 2;
        }
        else
        {
            *pRevision = 1;
        }

        return;
    }

    *pChipType = 0x0;
    *pRevision = 0x0;
}




/******************************************************************************
 *
 * Function   :  PlxPciIntrAttach
 *
 * Description:  Registers a wait object for notification on interrupt(s)
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIntrAttach(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr,
    U32              *pTag,
    VOID             *pOwner
    )
{
    unsigned long     flags;
    INTR_WAIT_OBJECT *pWaitObject;


    pWaitObject =
        kmalloc(
            sizeof(INTR_WAIT_OBJECT),
            GFP_KERNEL
            );

    if (pWaitObject == NULL)
    {
        DebugPrintf((
            "ERROR - memory allocation for interrupt wait object failed\n"
            ));

        *pTag = (U32)NULL;

        return ApiInsufficientResources;
    }

    // Use object address as Tag
    *pTag = (U32)pWaitObject;

    // Record the owner
    pWaitObject->pOwner = pOwner;

    // Mark the object as pending
    pWaitObject->bPending = TRUE;

    // Initialize wait queue
    init_waitqueue_head(
        &(pWaitObject->WaitQueue)
        );

    // Set interrupt notification flags
    PlxChipSetInterruptNotifyFlags(
        pPlxIntr,
        pWaitObject
        );

    // Add to list of waiting objects
    spin_lock_irqsave(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    list_add_tail(
        &(pWaitObject->list),
        &(pdx->List_InterruptWait)
        );

    spin_unlock_irqrestore(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    DebugPrintf((
        "Attached interrupt wait object (0x%08x)\n",
        (U32)pWaitObject
        ));

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciIntrWait
 *
 * Description:  Put the process to sleep until wake-up event occurs or timeout
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIntrWait(
    DEVICE_EXTENSION *pdx,
    U32               Tag,
    U32               Timeout_ms
    )
{
    int               Wait_rc;
    RETURN_CODE       rc;
    unsigned long     flags;
    struct list_head *pList;
    INTR_WAIT_OBJECT *pElement;


    // Find the wait object in the list
    spin_lock_irqsave(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    pList = pdx->List_InterruptWait.next;

    // Find the wait object and wait for wake-up event
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
            spin_unlock_irqrestore(
                &(pdx->Lock_InterruptWaitList),
                flags
                );

            DebugPrintf((
                "Waiting for Interrupt wait object (0x%08x) to wake-up\n",
                (U32)pElement
                ));

            /*********************************************************
             * Convert milliseconds to jiffies.  The following
             * formula is used:
             *
             *                      ms * HZ
             *           jiffies = ---------
             *                       1,000
             *
             *
             *  where:  HZ      = System-defined clock ticks per second
             *          ms      = Timeout in milliseconds
             *          jiffies = Number of HZ's per second
             *
             ********************************************************/

            Timeout_ms = (Timeout_ms * HZ) / 1000;

            // Wait for interrupt event
            Wait_rc =
                Plx_wait_event_interruptible_timeout(
                    pElement->WaitQueue,
                    (pElement->bPending == FALSE),
                    Timeout_ms
                    );

            if (Wait_rc == 0)
            {
                // Condition met, no longer pending
                DebugPrintf(("Interrupt wait object awakened\n"));
                rc = ApiSuccess;
            }
            else if (Wait_rc > 0)
            {
                // Timeout reached
                DebugPrintf(("Timeout waiting for interrupt\n"));
                return ApiWaitTimeout;
            }
            else
            {
                // Interrupted by a signal
                DebugPrintf(("Interrupt wait object interrupted by signal\n"));
                rc = ApiWaitCanceled;
            }

            // The event occured, remove from list
            PlxInterruptWaitObjectRemove_ByTag(
                pdx,
                Tag
                );

            return rc;
        }

        // Jump to next item in the list
        pList = pList->next;
    }

    spin_unlock_irqrestore(
        &(pdx->Lock_InterruptWaitList),
        flags
        );

    DebugPrintf((
        "Interrupt wait object (0x%08x) not found or previously canceled\n",
        Tag
        ));

    // Object not found at this point
    return ApiWaitCanceled;
}




/******************************************************************************
 *
 * Function   :  PlxPciBusMemTransfer
 *
 * Description:  Reads/Writes device memory to/from a buffer
 *
 ******************************************************************************/
RETURN_CODE
PlxPciBusMemTransfer(
    DEVICE_EXTENSION *pdx,
    IOP_SPACE         IopSpace,
    U32               LocalAddress,
    BOOLEAN           bRemap,
    VOID             *pBuffer,
    U32               ByteCount,
    ACCESS_TYPE       AccessType,
    BOOLEAN           bReadOperation
    )
{
    U8  BarIndex;
    U16 Offset_RegRemap;
    U32 SpaceVa;
    U32 RegValue;
    U32 SpaceRange;
    U32 SpaceOffset;
    U32 RemapOriginal;
    U32 BytesToTransfer;


    // Added to prevent compiler warnings
    RemapOriginal = 0;

    // Verify data alignment
    switch (AccessType)
    {
        case BitSize8:
            break;

        case BitSize16:
            if (LocalAddress & 0x1)
            {
                DebugPrintf(("ERROR - Local address not aligned\n"));
                return ApiInvalidAddress;
            }

            if (ByteCount & 0x1)
            {
                DebugPrintf(("ERROR - Byte count not aligned\n"));
                return ApiInvalidSize;
            }
            break;

        case BitSize32:
            if (LocalAddress & 0x3)
            {
                DebugPrintf(("ERROR - Local address not aligned\n"));
                return ApiInvalidAddress;
            }

            if (ByteCount & 0x3)
            {
                DebugPrintf(("ERROR - Byte count not aligned\n"));
                return ApiInvalidSize;
            }
            break;

        default:
            DebugPrintf(("ERROR - Invalid access type\n"));
            return ApiInvalidAccessType;
    }

    // Get and Verify the Local Space
    PlxChipGetSpace(
        pdx,
        IopSpace,
        &BarIndex,
        &Offset_RegRemap
        );

    if (BarIndex == (U8)-1)
    {
        return ApiInvalidIopSpace;
    }

    // Only memory spaces are supported by this function
    if (pdx->PciBar[BarIndex].IsIoMapped)
    {
        DebugPrintf(("ERROR - I/O spaces not supported by this function\n"));
        return ApiInvalidIopSpace;
    }

    // Get kernel virtual address for the space
    SpaceVa = (U32)pdx->PciBar[BarIndex].pVa;

    if (SpaceVa == (U32)NULL)
    {
        DebugPrintf((
            "ERROR - Invalid kernel VA (0x%08x) for PCI BAR\n",
            SpaceVa
            ));

        return ApiInvalidAddress;
    }

    // Save the remap register
    if (bRemap)
    {
        RemapOriginal =
            PLX_REG_READ(
                pdx,
                Offset_RegRemap
                );
    }
    else
    {
        // Make sure requested area doesn't exceed our local space window boundary
        if ((LocalAddress + ByteCount) > pdx->PciBar[BarIndex].Size)
        {
            DebugPrintf(("ERROR - requested area exceeds space range\n"));
            return ApiInvalidSize;
        }
    }

    // Get the range of the space
    SpaceRange = ~(pdx->PciBar[BarIndex].Size - 1);

    // Transfer data in blocks
    while (ByteCount != 0)
    {
        // Adjust remap if necessary
        if (bRemap)
        {
            // Clear upper bits of remap
            RegValue = RemapOriginal & ~SpaceRange;

            // Adjust window to local address
            RegValue |= LocalAddress & SpaceRange;

            PLX_REG_WRITE(
                pdx,
                Offset_RegRemap,
                RegValue
                );
        }

        // Get current offset into space
        SpaceOffset = LocalAddress & (~SpaceRange);

        // Calculate bytes to transfer for next block
        if (ByteCount <= (((~SpaceRange) + 1) - SpaceOffset))
        {
            BytesToTransfer = ByteCount;
        }
        else
        {
            BytesToTransfer = ((~SpaceRange) + 1) - SpaceOffset;
        }

        // Make sure user buffer is accessible for next block
        if (bReadOperation)
        {
            if (access_ok(
                    VERIFY_WRITE,
                    pBuffer,
                    BytesToTransfer
                    ) == FALSE)
            {
                DebugPrintf(("ERROR - User buffer not accessible\n"));
                return ApiInsufficientResources;
            }
        }
        else
        {
            if (access_ok(
                    VERIFY_READ,
                    pBuffer,
                    BytesToTransfer
                    ) == FALSE)
            {
                DebugPrintf(("ERROR - User buffer not accessible\n"));
                return ApiInsufficientResources;
            }
        }

        if (bReadOperation)
        {
            // Copy block to user buffer
            switch (AccessType)
            {
                case BitSize8:
                    DEV_MEM_TO_USER_8(
                        pBuffer,
                        (SpaceVa + SpaceOffset),
                        BytesToTransfer
                        );
                    break;

                case BitSize16:
                    DEV_MEM_TO_USER_16(
                        pBuffer,
                        (SpaceVa + SpaceOffset),
                        BytesToTransfer
                        );
                    break;

                case BitSize32:
                    DEV_MEM_TO_USER_32(
                        pBuffer,
                        (SpaceVa + SpaceOffset),
                        BytesToTransfer
                        );
                    break;

                case BitSize64:
                    // 64-bit not implemented yet
                    break;
            }
        }
        else
        {
            // Copy user buffer to device memory
            switch (AccessType)
            {
                case BitSize8:
                    USER_TO_DEV_MEM_8(
                        (SpaceVa + SpaceOffset),
                        pBuffer,
                        BytesToTransfer
                        );
                    break;

                case BitSize16:
                    USER_TO_DEV_MEM_16(
                        (SpaceVa + SpaceOffset),
                        pBuffer,
                        BytesToTransfer
                        );
                    break;

                case BitSize32:
                    USER_TO_DEV_MEM_32(
                        (SpaceVa + SpaceOffset),
                        pBuffer,
                        BytesToTransfer
                        );
                    break;

                case BitSize64:
                    // 64-bit not implemented yet
                    break;
            }
        }

        // Adjust for next block access
        (U32)pBuffer += BytesToTransfer;
        LocalAddress += BytesToTransfer;
        ByteCount    -= BytesToTransfer;
    }

    // Restore the remap register
    if (bRemap)
    {
        PLX_REG_WRITE(
            pdx,
            Offset_RegRemap,
            RemapOriginal
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciIoPortTransfer
 *
 * Description:  Read or Write from/to an I/O port
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIoPortTransfer(
    U32          IoPort,
    ACCESS_TYPE  AccessType,
    VOID        *pValue,
    BOOLEAN      bReadOperation
    )
{
    if (pValue == NULL)
        return ApiNullParam;

    if (bReadOperation)
    {
        switch (AccessType)
        {
            case BitSize8:
                *(U8*)pValue =
                          IO_PORT_READ_8(
                              IoPort
                              );
                break;

            case BitSize16:
                *(U16*)pValue =
                           IO_PORT_READ_16(
                               IoPort
                               );
                break;

            case BitSize32:
                *(U32*)pValue =
                           IO_PORT_READ_32(
                               IoPort
                               );
                break;

            default:
                return ApiInvalidAccessType;
        }
    }
    else
    {
        switch (AccessType)
        {
            case BitSize8:
                IO_PORT_WRITE_8(
                    IoPort,
                    *(U8*)pValue
                    );
                break;

            case BitSize16:
                IO_PORT_WRITE_16(
                    IoPort,
                    *(U16*)pValue
                    );
                break;

            case BitSize32:
                IO_PORT_WRITE_32(
                    IoPort,
                    *(U32*)pValue
                    );
                break;

            default:
                return ApiInvalidAccessType;
        }
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxEepromAccess
 *
 * Description:  Access the Serial EEPROM
 *
 ******************************************************************************/
RETURN_CODE
PlxEepromAccess(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    VOID             *pBuffer,
    U32               ByteCount,
    BOOLEAN           bReadOperation
    )
{
    U16         offset;
    U32         EepromValue;
    BOOLEAN     bFlag;
    RETURN_CODE rc;


    // Verify count is aligned on 4-byte boundary
    if (ByteCount & 0x3)
    {
        DebugPrintf(("ERROR - size not aligned on 4-byte boundary\n"));
        return ApiInvalidSize;
    }

    switch (EepromType)
    {
        case Eeprom93CS46:
            if (ByteCount > 0x80)
                return ApiInvalidSize;
            break;

        case Eeprom93CS56:
            if (ByteCount > 0x100)
                return ApiInvalidSize;
            break;

        case Eeprom93CS66:
            if (ByteCount > 0x200)
                return ApiInvalidSize;
            break;

        default:
            if (ByteCount > PLX_EEPROM_SIZE)
            {
                DebugPrintf((
                    "WARNING - size (%Xh) too large for unknown EEPROM type\n",
                    ByteCount
                    ));
                ByteCount = PLX_EEPROM_SIZE;
            }
            break;
    }

    // Verify the user-space buffer is accessible
    if (bReadOperation)
    {
        bFlag =
            access_ok(
                VERIFY_WRITE,
                pBuffer,
                ByteCount
                );
    }
    else
    {
        bFlag =
            access_ok(
                VERIFY_READ,
                pBuffer,
                ByteCount
                );
    }

    if (bFlag == FALSE)
    {
        DebugPrintf((
            "ERROR - Unable to access user-space buffer (0x%08x)\n",
            (U32)pBuffer
            ));

        return ApiInvalidAddress;
    }

    // Read EEPROM values
    for (offset=0; offset < ByteCount; offset += sizeof(U32))
    {
        if (bReadOperation)
        {
            rc =
                PlxEepromReadByOffset(
                    pdx,
                    offset,
                    &EepromValue
                    );

            if (rc != ApiSuccess)
            {
                DebugPrintf(("ERROR - EEPROM read failed\n"));

                return rc;
            }

            /******************************************************
             * Endian conversion is needed since this function
             * returns data as it is ordered in the EEPROM, which
             * is by 16-bit offsets.  The EEPROM access by offset
             * function only supports access at 32-bit offsets.
             *****************************************************/
            EepromValue = ((EepromValue >> 16) | (EepromValue << 16));

            // Write value to user-mode buffer
            __put_user(
                EepromValue,
                (U32*)(((U32)pBuffer) + offset)
                );
        }
        else
        {
            // Get next 32-bit value to write
            __get_user(
                EepromValue,
                (U32*)(((U32)pBuffer) + offset)
                );

            /******************************************************
             * Endian conversion is needed since this function
             * expects data as it is ordered in the EEPROM, which
             * is by 16-bit offsets.  The EEPROM access by offset
             * function only supports access at 32-bit offsets.
             *****************************************************/
            EepromValue = ((EepromValue >> 16) | (EepromValue << 16));

            rc =
                PlxEepromWriteByOffset(
                    pdx,
                    offset,
                    EepromValue
                    );

            if (rc != ApiSuccess)
            {
                DebugPrintf(("ERROR - EEPROM write failed\n"));

                return rc;
            }
        }
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryAllocate
 *
 * Description:  Allocate physically contiguous page-locked memory
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryAllocate(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    BOOLEAN           bSmallerOk,
    VOID             *pOwner
    )
{
    U32               DecrementAmount;
    PLX_PHYSICAL_MEM *pBufferObj;


    // Initialize buffer information
    pPciMem->UserAddr     = (U32)NULL;
    pPciMem->PhysicalAddr = (U32)NULL;

    /*******************************************************
     * Verify size
     *
     * A size of 0 is valid because this function may
     * be called to allocate a common buffer of size 0;
     * therefore, the information is reset & return sucess.
     ******************************************************/
    if (pPciMem->Size == 0)
    {
        return ApiSuccess;
    }

    // Allocate memory for new list object
    pBufferObj =
        kmalloc(
            sizeof(PLX_PHYSICAL_MEM),
            GFP_KERNEL
            );

    if (pBufferObj == NULL)
    {
        DebugPrintf((
            "ERROR - Memory allocation for list object failed\n"
            ));

        return ApiInsufficientResources;
    }

    // Set buffer request size
    pBufferObj->PciMem.Size = pPciMem->Size;

    // Setup amount to reduce on failure
    DecrementAmount = (pPciMem->Size / 10);

    do
    {
        // Attempt to allocate the buffer
        pBufferObj->pKernelVa =
            Plx_pci_alloc_consistent_mem(
                pBufferObj
                );

        if (pBufferObj->pKernelVa == NULL)
        {
            // Reduce memory request size if requested
            if (bSmallerOk && (pBufferObj->PciMem.Size > PAGE_SIZE))
            {
                pBufferObj->PciMem.Size -= DecrementAmount;
            }
            else
            {
                // Release the list object
                kfree(
                    pBufferObj
                    );

                DebugPrintf((
                    "ERROR - Physical memory allocation failed\n"
                    ));

                pPciMem->Size = 0;

                return ApiInsufficientResources;
            }
        }
    }
    while (pBufferObj->pKernelVa == NULL);

    // Clear the buffer
    RtlZeroMemory(
        pBufferObj->pKernelVa,
        pBufferObj->PciMem.Size
        );

    DebugPrintf(("Allocated physical memory...\n"));

    DebugPrintf((
        "    Physical Addr: 0x%08x\n",
        pBufferObj->PciMem.PhysicalAddr
        ));

    DebugPrintf((
        "    Kernel VA    : 0x%08x\n",
        (U32)pBufferObj->pKernelVa
        ));

    DebugPrintf((
        "    Size         : %d Kb",
        (pBufferObj->PciMem.Size >> 10)
        ));

    if (pPciMem->Size != pBufferObj->PciMem.Size)
    {
        DebugPrintf_NoInfo((
            "  (req: %d Kb)\n",
            (pPciMem->Size >> 10)
            ));
    }
    else
    {
        DebugPrintf_NoInfo(("\n"));
    }

    // Record buffer owner
    pBufferObj->pOwner = pOwner;

    // Clear user address
    pBufferObj->PciMem.UserAddr = (U32)NULL;

    // Assign buffer to device if provided
    if (pdx != NULL)
    {
        // Return buffer information
        pPciMem->Size         = pBufferObj->PciMem.Size;
        pPciMem->PhysicalAddr = pBufferObj->PciMem.PhysicalAddr;

        // Add buffer object to list
        spin_lock(
            &(pdx->Lock_PhysicalMemList)
            );

        list_add_tail(
            &(pBufferObj->list),
            &(pdx->List_PhysicalMem)
            );

        spin_unlock(
            &(pdx->Lock_PhysicalMemList)
            );
    }
    else
    {
        // Store common buffer information
        pGbl_DriverObject->CommonBuffer = *pBufferObj;

        // Release the list object
        kfree(
            pBufferObj
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFree
 *
 * Description:  Free previously allocated physically contiguous page-locked memory
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryFree(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem
    )
{
    struct list_head *pList;
    PLX_PHYSICAL_MEM *pMemObject;


    spin_lock(
        &(pdx->Lock_PhysicalMemList)
        );

    pList = pdx->List_PhysicalMem.next;

    // Traverse list to find the desired list object
    while (pList != &(pdx->List_PhysicalMem))
    {
        // Get the object
        pMemObject =
            list_entry(
                pList,
                PLX_PHYSICAL_MEM,
                list
                );

        // Check if the physical addresses matches
        if (pMemObject->PciMem.PhysicalAddr == pPciMem->PhysicalAddr)
        {
            // Remove the object from the list
            list_del(
                pList
                );

            spin_unlock(
                &(pdx->Lock_PhysicalMemList)
                );

            // Release the buffer
            Plx_pci_free_consistent_mem(
                pMemObject
                );

            DebugPrintf((
                "Released physical memory at 0x%x (%d Kb)\n",
                pMemObject->PciMem.PhysicalAddr,
                (pMemObject->PciMem.Size >> 10)
                ));

            // Release the list object
            kfree(
                pMemObject
                );

            return ApiSuccess;
        }

        // Jump to next item in the list
        pList = pList->next;
    }

    spin_unlock(
        &(pdx->Lock_PhysicalMemList)
        );

    DebugPrintf((
        "ERROR - buffer object not found in list\n"
        ));

    return ApiInvalidData;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryMap
 *
 * Description:  Maps physical memory to User virtual address space
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryMap(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    BOOLEAN           bDeviceMem,
    VOID             *pOwner
    )
{
    int                rc;
    struct list_head  *pList;
    MAP_PARAMS_OBJECT *pMapObject;


    // Verify physical address
    if (pPciMem->PhysicalAddr == (U32)NULL)
    {
        DebugPrintf((
            "ERROR - Invalid physical address (0x%08x), cannot map to user space\n",
            pPciMem->PhysicalAddr
            ));

        return ApiInvalidAddress;
    }

    spin_lock(
        &(pdx->Lock_MapParamsList)
        );

    // Find the mapping parameters from the call to mmap
    pList = pdx->List_MapParams.next;

    // Find the object and remove it
    while (pList != &(pdx->List_MapParams))
    {
        // Get the object
        pMapObject =
            list_entry(
                pList,
                MAP_PARAMS_OBJECT,
                list
                );

        if ((pMapObject->pOwner       == pOwner) &&
            (pMapObject->vma.vm_start == pPciMem->UserAddr))
        {
            // Remove the object from the list
            list_del(
                pList
                );

            spin_unlock(
                &(pdx->Lock_MapParamsList)
                );

            DebugPrintf((
                "Found and removed map object (0x%08x) from list\n",
                (U32)pMapObject
                ));

            // Set flags for I/O resource mapping
            if (bDeviceMem)
                pMapObject->vma.vm_flags |= VM_IO;

            // Set the region as page-locked
            pMapObject->vma.vm_flags |= VM_RESERVED;

            /***********************************************************
             * Attempt to map the region
             *
             * Note:
             *
             * The remap_page_range() function prototype was modified
             * to add a parameter in kernel version 2.5.3.  The new
             * parameter is a pointer to the VMA structure.  The kernel
             * source in RedHat 9.0 (v2.4.20-8), however, also uses the
             * new parameter.  As a result, another #define is added if
             * RedHat 9.0 kernel source is used.
             **********************************************************/

#if ( (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,3)) || \
     ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20))) )
            rc =
                remap_page_range(
                    &pMapObject->vma,
                    pMapObject->vma.vm_start,
                    pPciMem->PhysicalAddr,
                    pMapObject->vma.vm_end - pMapObject->vma.vm_start,
                    pMapObject->vma.vm_page_prot
                    );
#else
            rc =
                remap_page_range(
                    pMapObject->vma.vm_start,
                    pPciMem->PhysicalAddr,
                    pMapObject->vma.vm_end - pMapObject->vma.vm_start,
                    pMapObject->vma.vm_page_prot
                    );
#endif

            if (rc != 0)
            {
                DebugPrintf((
                    "ERROR - Unable to map Physical address (0x%08x) ==> User space\n",
                    pPciMem->PhysicalAddr
                    ));

                rc = (int)ApiInsufficientResources;
            }
            else
            {
                DebugPrintf((
                    "Mapped Physical address (0x%08x) ==> User space (0x%08x)\n",
                    pPciMem->PhysicalAddr, (U32)pMapObject->vma.vm_start
                    ));

                rc = (int)ApiSuccess;
            }

            // Release the object
            kfree(
                pMapObject
                );

            return (RETURN_CODE)rc;
        }

        // Jump to next item in the list
        pList = pList->next;
    }

    spin_unlock(
        &(pdx->Lock_MapParamsList)
        );

    DebugPrintf((
        "ERROR - Map parameters object not found in list\n"
        ));

    return ApiInvalidData;
}




/******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryUnmap
 *
 * Description:  Unmap physical memory from User virtual address space
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPhysicalMemoryUnmap(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    VOID             *pOwner
    )
{
    // Handled by user-level API in Linux
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarMap
 *
 * Description:  Map a PCI BAR Space into User virtual space
 *
 ******************************************************************************/
RETURN_CODE
PlxPciBarMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    VOID             *pUserVa,
    VOID             *pOwner
    )
{
    // Handled elsewhere in Linux
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxPciBarUnmap
 *
 * Description:  Unmap a previously mapped PCI BAR Space from User virtual space
 *
 ******************************************************************************/
RETURN_CODE
PlxPciBarUnmap(
    DEVICE_EXTENSION *pdx,
    VOID             *UserVa,
    VOID             *pOwner
    )
{
    // Handled elsewhere in Linux
    return ApiUnsupportedFunction;
}
