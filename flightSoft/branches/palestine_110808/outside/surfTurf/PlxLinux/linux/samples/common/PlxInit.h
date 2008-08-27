#ifndef PLXINIT_H
#define PLXINIT_H

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
 *      PlxInit.h
 *
 * Description:
 *
 *      Header file for the PlxInit.c module
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


#include "PlxTypes.h"




/******************************
*        Definitions
******************************/
typedef struct _API_ERRORS
{
    RETURN_CODE  code;
    char        *text;
} API_ERRORS;




/********************************************************************************
*        Functions
*********************************************************************************/
S8
SelectDevice(
    DEVICE_LOCATION *pDevice
    );

U32
DetermineChipType(
    DEVICE_LOCATION *pDevice
    );

char*
PlxSdkErrorText(
    RETURN_CODE code
    );




#ifdef __cplusplus
}
#endif

#endif
