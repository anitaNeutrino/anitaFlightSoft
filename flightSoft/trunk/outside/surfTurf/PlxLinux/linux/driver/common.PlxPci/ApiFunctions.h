#ifndef __API_FUNCTIONS_H
#define __API_FUNCTIONS_H

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
 *      ApiFunctions.h
 *
 * Description:
 *
 *      The header file for the Driver Service module.
 *
 * Revision History:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "DriverDefs.h"
#include "PlxTypes.h"




/**********************************************
*               Functions
**********************************************/
RETURN_CODE
PlxChipTypeGet(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U32              *pChipType,
    U8               *pRevision
    );

VOID
PlxPciBoardReset(
    DEVICE_EXTENSION *pdx
    );

RETURN_CODE
PlxEepromPresent(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    BOOLEAN          *pFlag
    );

RETURN_CODE
PlxEepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16               offset,
    U16              *pValue
    );

RETURN_CODE
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16               offset,
    U16               value
    );



#endif
