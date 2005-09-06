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
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "DriverDefs.h"




/**********************************************
 *               Functions
 *********************************************/
VOID
Plx_sleep(
    U32 delay
    );

RETURN_CODE
PlxDeviceFind(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U8               *pDeviceNumber
    );

U8
PlxPciBusScan(
    DRIVER_OBJECT *pDriverObject
    );

PLX_DEVICE_NODE *
GetDeviceNodeFromKey(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey
    );

BOOLEAN
PlxChipTypeSet(
    PLX_DEVICE_NODE *pNode
    );



#endif
