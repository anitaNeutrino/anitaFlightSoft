/*******************************************************************************
 * Copyright (c) 2002 PLX Technology, Inc.
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
 *      04-31-02 : PCI SDK v3.50
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
    U32               RegPciInt;
    U32               InterruptSource;
    DEVICE_EXTENSION *pdx;


    // Get the device extension
    pdx = (DEVICE_EXTENSION *)dev_id;

    // Read interrupt status register
    RegPciInt =
        PLX_REG_READ(
            pdx,
            PCI9050_INT_CTRL_STAT
            );

    // Check for master PCI interrupt enable
    if ((RegPciInt & (1 << 6)) == 0)
        return;

    // Verify that an interrupt is truly active

    // Clear the interrupt type flag
    InterruptSource = INTR_TYPE_NONE;

    // Check if Local Interrupt 1 is active and not masked
    if ((RegPciInt & (1 << 2)) && (RegPciInt & (1 << 0)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_1;
    }

    // Check if Local Interrupt 2 is active and not masked
    if ((RegPciInt & (1 << 5)) && (RegPciInt & (1 << 3)))
    {
        InterruptSource |= INTR_TYPE_LOCAL_2;
    }

    // Software Interrupt
    if (RegPciInt & (1 << 7))
    {
        InterruptSource |= INTR_TYPE_SOFTWARE;
    }

    // Return if no interrupts are active
    if (InterruptSource == INTR_TYPE_NONE)
        return;

    // At this point, the device interrupt is verified

    // Mask the PCI Interrupt
    PLX_REG_WRITE(
        pdx,
        PCI9050_INT_CTRL_STAT,
        RegPciInt & ~(1 << 6)
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
 * Note       :  The 9052 supports Edge-triggerable interrupts as well as level
 *               triggered interrupts.  The 9050 only supports level triggered
 *               interrupts.  The interrupt masking code below handles both cases.
 *               If the chip is a 9050, the same code is used but should work
 *               ok since edge triggerable interrupts will always be disabled.
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

        // Check if this is an edge-triggered interrupt
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9050_INT_CTRL_STAT
                );

        if ((RegValue & (1 << 1)) && (RegValue & (1 << 8)))
        {
            // Clear edge-triggered interrupt
            PLX_REG_WRITE(
                pdx,
                PCI9050_INT_CTRL_STAT,
                RegValue | (1 << 10)
                );
        }
        else
        {
            // Mask Local Interrupt 1
            PLX_REG_WRITE(
                pdx,
                PCI9050_INT_CTRL_STAT,
                RegValue & ~(1 << 0)
                );
        }

        spin_unlock(
            &(pdx->Lock_Isr)
            );
    }

    // Local Interrupt 2
    if (IntSource & INTR_TYPE_LOCAL_2)
    {
        // Synchronize access to Interrupt Control/Status Register
        spin_lock(
            &(pdx->Lock_Isr)
            );

        // Check if this is an edge-triggered interrupt
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9050_INT_CTRL_STAT
                );

        if ((RegValue & (1 << 4)) && (RegValue & (1 << 9)))
        {
            // Clear edge-triggered interrupt
            PLX_REG_WRITE(
                pdx,
                PCI9050_INT_CTRL_STAT,
                RegValue | (1 << 11)
                );
        }
        else
        {
            // Mask Local Interrupt 2
            PLX_REG_WRITE(
                pdx,
                PCI9050_INT_CTRL_STAT,
                RegValue & ~(1 << 3)
                );
        }

        spin_unlock(
            &(pdx->Lock_Isr)
            );
    }

    // Software Interrupt
    if (IntSource & INTR_TYPE_SOFTWARE)
    {
        // Synchronize access to Interrupt Control/Status Register
        spin_lock(
            &(pdx->Lock_Isr)
            );

        // Clear the software interrupt
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9050_INT_CTRL_STAT
                );

        PLX_REG_WRITE(
            pdx,
            PCI9050_INT_CTRL_STAT,
            RegValue & ~(1 << 7)
            );

        spin_unlock(
            &(pdx->Lock_Isr)
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
