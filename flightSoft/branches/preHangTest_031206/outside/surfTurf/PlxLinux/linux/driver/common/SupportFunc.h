#ifndef __SUPPORT_FUNC_H
#define __SUPPORT_FUNC_H

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
 *      SupportFunc.h
 *
 * Description:
 *
 *      Header for additional support functions
 *
 * Revision History:
 *
 *      11-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "DriverDefs.h"




/**********************************************
 *               Functions
 *********************************************/
BOOLEAN
PlxSynchronizedRegisterModify(
    PLX_REG_DATA *pRegData
    );

VOID
PlxSearchDevices(
    DRIVER_OBJECT   *pdo,
    DEVICE_LOCATION *pDeviceCriteria,
    U32             *pSelection
    );

VOID
PlxInterruptWaitObjectRemove_ByTag(
    DEVICE_EXTENSION *pdx,
    U32               Tag
    );

VOID
PlxInterruptWaitObjectRemoveAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

int
PlxPciBarResourceMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    );

VOID
PlxPciBarResourcesUnmap(
    DEVICE_EXTENSION *pdx
    );

VOID
PlxPciPhysicalMemoryFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

VOID
PlxUserMappingRequestFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

VOID
PlxDmaChannelCleanup(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    );

VOID
PlxBlockDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    );

VOID
PlxSglDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    );

PVOID
PlxLockBufferAndBuildSgl(
    DEVICE_EXTENSION     *pdx,
    U8                    DmaIndex,
    DMA_TRANSFER_ELEMENT *pDma
    );

BOOLEAN
IsSupportedDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pDev
    );

void
Plx_dev_mem_to_user_8(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    );

void
Plx_dev_mem_to_user_16(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    );

void
Plx_dev_mem_to_user_32(
    void          *VaUser,
    void          *VaDev,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_8(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_16(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    );

void
Plx_user_to_dev_mem_32(
    void          *VaDev,
    void          *VaUser,
    unsigned long  count
    );

VOID
Plx_sleep(
    U32 delay
    );

int
Plx_stricmp(
    const char *string1,
    const char *string2
    );




#endif
