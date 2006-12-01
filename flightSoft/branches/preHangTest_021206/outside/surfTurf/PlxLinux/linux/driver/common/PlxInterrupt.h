#ifndef __PLX_INTERRUPT_H
#define __PLX_INTERRUPT_H

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
 *      PlxInterrupt.h
 *
 * Description:
 *
 *      Driver Interrupt functions
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include <linux/sched.h>




/**********************************************
*               Definitions
**********************************************/
#define INTR_TYPE_NONE                  0              // Interrupt identifiers
#define INTR_TYPE_LOCAL_1               (1 << 0)
#define INTR_TYPE_LOCAL_2               (1 << 1)
#define INTR_TYPE_PCI_ABORT             (1 << 2)
#define INTR_TYPE_DOORBELL              (1 << 3)
#define INTR_TYPE_OUTBOUND_POST         (1 << 4)
#define INTR_TYPE_DMA_0                 (1 << 5)
#define INTR_TYPE_DMA_1                 (1 << 6)
#define INTR_TYPE_DMA_2                 (1 << 7)
#define INTR_TYPE_DMA_3                 (1 << 8)
#define INTR_TYPE_SOFTWARE              (1 << 9)




/**********************************************
*               Functions
**********************************************/
void
OnInterrupt(
    int             irq,
    void           *dev_id,
    struct pt_regs *regs
    );

void
DpcForIsr(
    void *pArg1
    );




#endif
