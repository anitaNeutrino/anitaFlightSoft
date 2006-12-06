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
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "DriverDefs.h"
#include "PlxTypes.h"




/**********************************************
*               Functions
**********************************************/
RETURN_CODE
PlxRegisterRead(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    );

RETURN_CODE
PlxRegisterWrite(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    );

RETURN_CODE
PlxPciIntrEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    );

RETURN_CODE
PlxPciIntrDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    );

RETURN_CODE
PlxPciIntrStatusGet(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    );

RETURN_CODE
PlxPciPowerLevelSet(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL   PowerLevel
    );

RETURN_CODE
PlxPciPowerLevelGet(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL  *pPowerLevel
    );

RETURN_CODE
PlxPciPmIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciPmNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciHotSwapIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciHotSwapNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciHotSwapStatus(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciVpdIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciVpdNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    );

RETURN_CODE
PlxPciVpdRead(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    );

RETURN_CODE
PlxPciVpdWrite(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               VpdData
    );

RETURN_CODE
PlxEepromPresent(
    DEVICE_EXTENSION *pdx,
    BOOLEAN          *pFlag
    );

RETURN_CODE
PlxEepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    );

RETURN_CODE
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    );

RETURN_CODE
PlxRegisterMailboxRead(
    DEVICE_EXTENSION *pdx,
    MAILBOX_ID        MailboxId,
    U32              *pValue
    );

RETURN_CODE
PlxRegisterMailboxWrite(
    DEVICE_EXTENSION *pdx,
    MAILBOX_ID        MailboxId,
    U32               value
    );

RETURN_CODE
PlxRegisterDoorbellRead(
    DEVICE_EXTENSION *pdx,
    U32              *pValue
    );

RETURN_CODE
PlxRegisterDoorbellWrite(
    DEVICE_EXTENSION *pdx,
    U32               value
    );

RETURN_CODE
PlxMuInboundPortRead(
    DEVICE_EXTENSION *pdx,
    U32              *pFrame
    );

RETURN_CODE
PlxMuInboundPortWrite(
    DEVICE_EXTENSION *pdx,
    U32               Frame
    );

RETURN_CODE
PlxMuOutboundPortRead(
    DEVICE_EXTENSION *pdx,
    U32              *pFrame
    );

RETURN_CODE
PlxMuOutboundPortWrite(
    DEVICE_EXTENSION *pdx,
    U32               Frame
    );

RETURN_CODE
PlxDmaControl(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_COMMAND       command
    );

RETURN_CODE
PlxDmaStatus(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel
    );

RETURN_CODE
PlxDmaBlockChannelOpen(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc,
    VOID             *pOwner
    );

RETURN_CODE
PlxDmaBlockTransfer(
    DEVICE_EXTENSION     *pdx,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *dmaData,
    VOID                 *pIrp
    );

RETURN_CODE
PlxDmaBlockChannelClose(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    BOOLEAN           bCheckInProgress
    );

RETURN_CODE
PlxDmaSglChannelOpen(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc,
    VOID             *pOwner
    );

RETURN_CODE
PlxDmaSglTransfer(
    DEVICE_EXTENSION     *pdx,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *dmaData,
    VOID                 *pIrp
    );

RETURN_CODE
PlxDmaSglChannelClose(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    BOOLEAN           bCheckInProgress
    );

RETURN_CODE
PlxAbortAddrRead(
    DEVICE_EXTENSION *pdx,
    U32              *pValue
    );




#endif
