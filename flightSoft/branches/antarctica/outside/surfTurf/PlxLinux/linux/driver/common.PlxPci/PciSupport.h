#ifndef __PCI_SUPPORT_H
#define __PCI_SUPPORT_H

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
 *      PciSupport.h
 *
 * Description:
 *
 *      The header file for PCI support functions.
 *
 * Revision History:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/pci.h>
#include "DriverDefs.h"
#include "PciRegs.h"




/**********************************************
 *               Definitions
 *********************************************/
#if !defined(PCI_ANY_ID)
    #define PCI_ANY_ID                      (~0)
#endif

#define PLX_PCI_REG_READ(pNode, offset, pValue)  \
    PlxPciRegisterRead(        \
        (pNode)->Key.bus,      \
        (pNode)->Key.slot,     \
        (pNode)->Key.function, \
        (offset),              \
        (pValue)               \
        )

#define PLX_PCI_REG_WRITE(pNode, offset, value) \
    PlxPciRegisterWrite(       \
        (pNode)->Key.bus,      \
        (pNode)->Key.slot,     \
        (pNode)->Key.function, \
        (offset),              \
        (value)                \
        )

// Kernel v2.4 already provides the following functions
#if defined(LINUX_24)
    #define Plx_pci_find_device              pci_find_device
    #define Plx_pci_resource_len(pdx, bar)   pci_resource_len((pdx)->pPciDevice, (bar))
#endif




/**********************************************
 *               Functions
 *********************************************/
RETURN_CODE
PlxPciRegisterRead(
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    );

RETURN_CODE
PlxPciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    );

// Kernel v2.2 specific functions
#if defined(LINUX_22)

struct pci_dev*
Plx_pci_find_device(
    unsigned int          vendor,
    unsigned int          device,
    const struct pci_dev *from
    );

#endif  // LINUX_22




#endif
