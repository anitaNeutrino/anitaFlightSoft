#ifndef __COMMON_API_H
#define __COMMON_API_H

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
 *      CommonApi.h
 *
 * Description:
 *
 *      Header for common API functions
 *
 * Revision History:
 *
 *      10-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "DriverDefs.h"




/**********************************************
*               Functions
**********************************************/
VOID
PlxChipTypeGet(
    DEVICE_EXTENSION *pdx,
    U32              *pChipType,
    U8               *pRevision
    );

RETURN_CODE
PlxPciIntrAttach(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr,
    U32              *pTag,
    VOID             *pOwner
    );

RETURN_CODE
PlxPciIntrWait(
    DEVICE_EXTENSION *pdx,
    U32               Tag,
    U32               Timeout_ms
    );

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
    );

RETURN_CODE
PlxPciIoPortTransfer(
    U32          IoPort,
    ACCESS_TYPE  AccessType,
    VOID        *pValue,
    BOOLEAN      bReadOperation
    );

RETURN_CODE
PlxEepromAccess(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    VOID             *pBuffer,
    U32               ByteCount,
    BOOLEAN           bReadOperation
    );

RETURN_CODE
PlxPciPhysicalMemoryAllocate(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    BOOLEAN           bSmallerOk,
    VOID             *pOwner
    );

RETURN_CODE
PlxPciPhysicalMemoryFree(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem
    );

RETURN_CODE
PlxPciPhysicalMemoryMap(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    BOOLEAN           bDeviceMem,
    VOID             *pOwner
    );

RETURN_CODE
PlxPciPhysicalMemoryUnmap(
    DEVICE_EXTENSION *pdx,
    PCI_MEMORY       *pPciMem,
    VOID             *pOwner
    );

RETURN_CODE
PlxPciBarMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex,
    VOID             *pUserVa,
    VOID             *pOwner
    );

RETURN_CODE
PlxPciBarUnmap(
    DEVICE_EXTENSION *pdx,
    VOID             *UserVa,
    VOID             *pOwner
    );




#endif
