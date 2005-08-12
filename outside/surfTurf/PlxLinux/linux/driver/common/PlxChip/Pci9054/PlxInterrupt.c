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
 *      PlxInterrupt.c
 *
 * Description:
 *
 *      This file handles interrupts for the PLX device
 *
 * Notes:
 *
 *      This driver handles interrupt sharing with other PCI device drivers.
 *      The PLX chip is checked to see if it is the true source of an interrupt.
 *      If an interrupt is truly active, the interrupt source is cleared and
 *      any waiting user-mode applications are notified.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include <asm/system.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/interrupt.h>  // Note: interrupt.h must be last to avoid compiler errors

#include "DriverDefs.h"
#include "PlxInterrupt.h"
#include "PciSupport.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  OnInterrupt
 *
 * Description:  The Interrupt Service Routine for the PLX device
 *
 ******************************************************************************/
void
OnInterrupt(
    int             irq,
    void           *dev_id,
    struct pt_regs *regs
    )
{
    U32               RegValue;
    U32               RegPciInt;
    U32               InterruptSource;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Read interrupt status register
    RegPciInt =
        PLX_REG_READ(
            pdx,
            PCI9054_INT_CTRL_STAT
            );

    /****************************************************
     * If the chip is in a low power state, then local
     * register reads are disabled and will always return
     * 0xFFFFFFFF.  If the PLX chip's interrupt is shared
     * with another PCI device, the PXL ISR may continue
     * to be called.  This case is handled to avoid
     * erroneous reporting of an active interrupt.
     ***************************************************/
    if (RegPciInt == 0xFFFFFFFF)
        return;

    // Check for master PCI interrupt enable
    if ((RegPciInt & (1 << 8)) == 0)
        return;

    // Verify that an interrupt is truly active

    // Clear the interrupt type flag
    InterruptSource = INTR_TYPE_NONE;

    // Check if PCI Doorbell Interrupt is active and not masked
    if ((RegPciInt & (1 << 13)) && (RegPciInt & (1 << 9)))
    {
        InterruptSource |= INTR_TYPE_DOORBELL;
    }

    // Check if PCI Abort Interrupt is active and not masked
    if ((RegPciInt & (1 << 14)) && (RegPciInt & (1 << 10)))
    {
        InterruptSource |= INTR_TYPE_PCI_ABORT;
    }

    // Check if Local->PCI Interrupt is active and not masked
    if ((RegPciInt & (1 << 15)) && (RegPciInt & (1 << 11)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_1;
    }

    // Check if DMA Channel 0 Interrupt is active and not masked
    if ((RegPciInt & (1 << 21)) && (RegPciInt & (1 << 18)))
    {
        // Verify the DMA interrupt is routed to PCI
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA0_MODE
                );

        if (RegValue & (1 << 17))
        {
            InterruptSource |= INTR_TYPE_DMA_0;
        }
    }

    // Check if DMA Channel 1 Interrupt is active and not masked
    if ((RegPciInt & (1 << 22)) && (RegPciInt & (1 << 19)))
    {
        // Verify the DMA interrupt is routed to PCI
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA1_MODE
                );

        if (RegValue & (1 << 17))
        {
            InterruptSource |= INTR_TYPE_DMA_1;
        }
    }

    // Check if MU Outbound Post interrupt is active
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9054_OUTPOST_INT_STAT
            );

    if (RegValue & (1 << 3))
    {
        // Check if MU Outbound Post interrupt is not masked
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_OUTPOST_INT_MASK
                );

        if ((RegValue & (1 << 3)) == 0)
        {
            InterruptSource |= INTR_TYPE_OUTBOUND_POST;
        }
    }

    // Return if no interrupts are active
    if (InterruptSource == INTR_TYPE_NONE)
        return;

    // At this point, the device interrupt is verified

    // Mask the PCI Interrupt
    PLX_REG_WRITE(
        pdx,
        PCI9054_INT_CTRL_STAT,
        RegPciInt & ~(1 << 8)
        );

    //
    // Schedule deferred procedure (DPC) to complete interrupt processing
    //

    // Reset task structure
    Plx_tq_struct_reset_list(
        pdx->Task_DpcForIsr
        );

    pdx->Task_DpcForIsr.sync = 0;

    // Provide interrupt source to DPC
    pdx->InterruptSource = InterruptSource;

    // Add task to system immediate queue
    queue_task(
        &(pdx->Task_DpcForIsr),
        &tq_immediate
        );

    // Mark queue for Bottom-half processing
    mark_bh(
        IMMEDIATE_BH
        );
}




/******************************************************************************
 *
 * Function   :  DpcForIsr
 *
 * Description:  This routine will be triggered by the ISR to service an interrupt.
 *
 ******************************************************************************/
void
DpcForIsr(
    void *pArg1
    )
{
    U32               RegValue;
    U32               IntSource;
    struct list_head *pList;
    struct list_head *pCurrent;
    INTR_WAIT_OBJECT *pElement;
    DEVICE_EXTENSION *pdx;


    // Get device extension
    pdx = (DEVICE_EXTENSION *)pArg1;

    // Copy interrupt source
    IntSource = pdx->InterruptSource;

    // Local Interrupt 1
    if (IntSource & INTR_TYPE_LOCAL_1)
    {
        // Synchronize access to Interrupt Control/Status Register
        spin_lock(
            &(pdx->Lock_Isr)
            );

        // Mask local interrupt 1 since true source is unknown
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_INT_CTRL_STAT
                );

        RegValue &= ~(1 << 11);

        // Mask Local Interrupt 1
        PLX_REG_WRITE(
            pdx,
            PCI9054_INT_CTRL_STAT,
            RegValue
            );

        spin_unlock(
            &(pdx->Lock_Isr)
            );
    }

    // Doorbell Interrupt
    if (IntSource & INTR_TYPE_DOORBELL)
    {
        // Get Doorbell register
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_PCI_DOORBELL
                );

        // Clear Doorbell interrupt
        PLX_REG_WRITE(
            pdx,
            PCI9054_PCI_DOORBELL,
            RegValue
            );

        // Save this value in case it is requested later
        pdx->IntrDoorbellValue = RegValue;
    }

    // PCI Abort interrupt
    if (IntSource & INTR_TYPE_PCI_ABORT)
    {
        // Get the PCI Command register
        PLX_PCI_REG_READ(
            pdx,
            CFG_COMMAND,
            &RegValue
            );

        // Write to back to clear PCI Abort
        PLX_PCI_REG_WRITE(
            pdx,
            CFG_COMMAND,
            RegValue
            );
    }

    // DMA Channel 0 interrupt
    if (IntSource & INTR_TYPE_DMA_0)
    {
        // Get DMA Control/Status
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA_COMMAND_STAT
                );

        // Clear DMA interrupt
        PLX_REG_WRITE(
            pdx,
            PCI9054_DMA_COMMAND_STAT,
            RegValue | (1 << 3)
            );

        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA0_MODE
                );

        // Check if SGL is enabled & cleanup
        if (RegValue & (1 << 9))
        {
            PlxSglDmaTransferComplete(
                pdx,
                0
                );
        }
        else
        {
            PlxBlockDmaTransferComplete(
                pdx,
                0
                );
        }
    }

    // DMA Channel 1 interrupt
    if (IntSource & INTR_TYPE_DMA_1)
    {
        // Get DMA Control/Status
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA_COMMAND_STAT
                );

        // Clear DMA interrupt
        PLX_REG_WRITE(
            pdx,
            PCI9054_DMA_COMMAND_STAT,
            RegValue | (1 << 11)
            );

        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9054_DMA1_MODE
                );

        // Check if SGL is enabled & cleanup
        if (RegValue & (1 << 9))
        {
            PlxSglDmaTransferComplete(
                pdx,
                1
                );
        }
        else
        {
            PlxBlockDmaTransferComplete(
                pdx,
                1
                );
        }
    }

    // Outbound post FIFO interrupt
    if (IntSource & INTR_TYPE_OUTBOUND_POST)
    {
        // Mask Outbound Post interrupt
        PLX_REG_WRITE(
            pdx,
            PCI9054_OUTPOST_INT_MASK,
            (1 << 3)
            );
    }


    /***********************************************
     * Wake-up any processes waiting on interrupts
     **********************************************/

    spin_lock(
        &(pdx->Lock_InterruptWaitList)
        );

    // Get Interrupt wait list
    pList = pdx->List_InterruptWait.next;

    // Traverse wait objects and wake-up processes
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

        // Check if the interrupt events match events registered for
        if (pElement->NotifyOnInterrupt & (U32)IntSource)
        {
            DebugPrintf((
                "DPC waking up Interrupt wait object (0x%08x)\n",
                (U32)pElement
                ));

            // Set state to non-pending
            pElement->bPending = FALSE;

            // Wake-up any process waiting on object
            wake_up_interruptible(
                &(pElement->WaitQueue)
                );
        }
    }

    spin_unlock(
        &(pdx->Lock_InterruptWaitList)
        );

    // Re-enable the PCI interrupt
    PlxChipPciInterruptEnable(
        pdx
        );
}
