#ifndef __PCI_SUPPORT_H
#define __PCI_SUPPORT_H

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
 *      PciSupport.h
 *
 * Description:
 *
 *      The header file for PCI access functions.
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
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

// Macros for PCI register access
#define PLX_PCI_REG_READ(pdx, offset, pBuf)  \
    pci_read_config_dword(                   \
        (pdx)->pPciDevice,                   \
        (offset),                            \
        (pBuf)                               \
        )

#define PLX_PCI_REG_WRITE(pdx, offset, value) \
    pci_write_config_dword(                   \
        (pdx)->pPciDevice,                    \
        (offset),                             \
        (value)                               \
        )

// Unsupported calls not needed for Linux - bus scan is allowed
#define PlxPciRegisterRead_Unsupported       PlxPciRegisterRead
#define PlxPciRegisterWrite_Unsupported      PlxPciRegisterWrite

// Kernel v2.4 already provides the following functions
#if defined(LINUX_24)
    #define Plx_pci_find_device              pci_find_device
    #define Plx_pci_resource_len(pdx, bar)   pci_resource_len((pdx)->pPciDevice, (bar))
#endif




/**********************************************
 *               Functions
 *********************************************/
U32
PlxPciRegisterRead(
    DEVICE_EXTENSION *pdx,
    U8                bus,
    U8                slot,
    U16               offset,
    RETURN_CODE      *pReturnCode
    );

RETURN_CODE
PlxPciRegisterWrite(
    DEVICE_EXTENSION *pdx,
    U8                bus,
    U8                slot,
    U16               offset,
    U32               value
    );

void*
Plx_pci_alloc_consistent_mem(
    PLX_PHYSICAL_MEM *pMemObject
    );

void
Plx_pci_free_consistent_mem(
    PLX_PHYSICAL_MEM *pMemObject
    );

// Kernel v2.2 specific functions
#if defined(LINUX_22)

struct pci_dev*
Plx_pci_find_device(
    unsigned int          vendor,
    unsigned int          device,
    const struct pci_dev *from
    );

U32
Plx_pci_resource_len(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    );

#endif  // LINUX_22




#endif
