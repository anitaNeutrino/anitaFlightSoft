#ifndef __EEPROM_SUPPORT_H
#define __EEPROM_SUPPORT_H

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
 *      EepromSupport.h
 *
 * Description:
 *
 *      The include file for EEPROM support functions.
 *
 * Revision History:
 *
 *      05-01-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "DriverDefs.h"



#ifdef __cplusplus
extern "C" {
#endif




/**********************************************
*               Definitions
**********************************************/
#define EE46_CMD_LEN    9       /* Bits in instructions */
#define EE56_CMD_LEN    11      /* Bits in instructions */
#define EE66_CMD_LEN    11      /* Bits in instructions */
#define EE_READ         0x0180  /* 01 1000 0000 read instruction */
#define EE_WRITE        0x0140  /* 01 0100 0000 write instruction */
#define EE_WREN         0x0130  /* 01 0011 0000 write enable instruction */
#define EE_WRALL        0x0110  /* 01 0001 0000 write all registers */
#define EE_PRREAD       0x0180  /* 01 1000 0000 read address stored in Protect Register */
#define EE_PRWRITE      0x0140  /* 01 0100 0000 write the address into PR */
#define EE_WDS          0x0100  /* 01 0000 0000 write disable instruction */
#define EE_PREN         0x0130  /* 01 0011 0000 protect enable instruction */
#define EE_PRCLEAR      0x01FF  /* 01 1111 1111 clear protect register instr */
#define EE_PRDS         0x0100  /* 01 0000 0000 ONE TIME ONLY, permenant */




/**********************************************
*               Functions
**********************************************/
void
EepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    U16               offset,
    U32              *pValue
    );

void
EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    U16               offset,
    U32               value
    );

void
EepromSendCommand(
    DEVICE_EXTENSION *pdx,
    U32               EepromCommand,
    U8                DataLengthInBits
    );

void
EepromClock(
    DEVICE_EXTENSION *pdx
    );



#ifdef __cplusplus
}
#endif

#endif
