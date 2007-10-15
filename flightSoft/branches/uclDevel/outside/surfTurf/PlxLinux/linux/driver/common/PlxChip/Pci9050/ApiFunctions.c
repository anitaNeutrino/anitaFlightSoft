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
 *      ApiFunctions.c
 *
 * Description:
 *
 *      This file contains the implementation of the PCI API functions
 *      specific to a PLX Chip.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include <linux/slab.h>
#include "ApiFunctions.h"
#include "EepromSupport.h"
#include "PlxInterrupt.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  PlxRegisterRead
 *
 * Description:  Reads a PLX chip local register
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterRead(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    )
{
    // Make sure register offset is valid
    if ((offset & 0x3) || (offset >= MAX_PLX_REG_OFFSET))
    {
        DebugPrintf(("ERROR - invalid register offset (0x%x)\n", offset));

        *pValue = (U32)-1;

        return ApiInvalidRegister;
    }

    *pValue =
        PLX_REG_READ(
            pdx,
            offset
            );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterWrite
 *
 * Description:  Writes a value to a PLX local register
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterWrite(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    )
{
    // Make sure register offset is valid
    if ((offset & 0x3) || (offset >= MAX_PLX_REG_OFFSET))
    {
        DebugPrintf(("ERROR - invalid register offset (0x%x)\n", offset));
        return ApiInvalidRegister;
    }

    PLX_REG_WRITE(
        pdx,
        offset,
        value
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciIntrEnable
 *
 * Description:  Enables specific interupts of the PLX Chip
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIntrEnable(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    )
{
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9050_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMainInt)
        RegData.BitsToSet |= (1 << 6);

    if (pPlxIntr->IopToPciInt)
        RegData.BitsToSet |= (1 << 0);

    if (pPlxIntr->IopToPciInt_2)
        RegData.BitsToSet |= (1 << 3);

    // Write register values if they have changed

    if (RegData.BitsToSet != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciIntrDisable
 *
 * Description:  Disables specific interrupts of the PLX Chip
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIntrDisable(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    )
{
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9050_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    if (pPlxIntr->PciMainInt)
        RegData.BitsToClear |= (1 << 6);

    if (pPlxIntr->IopToPciInt)
        RegData.BitsToClear |= (1 << 0);

    if (pPlxIntr->IopToPciInt_2)
        RegData.BitsToClear |= (1 << 3);

    // Write register values if they have changed

    if (RegData.BitsToClear != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciIntrStatusGet
 *
 * Description:  Returns the last interrupts to occur on this device
 *
 ******************************************************************************/
RETURN_CODE
PlxPciIntrStatusGet(
    DEVICE_EXTENSION *pdx,
    PLX_INTR         *pPlxIntr
    )
{
    RtlZeroMemory(
        pPlxIntr,
        sizeof(PLX_INTR)
        );

    if (pdx->InterruptSource & INTR_TYPE_LOCAL_1)
    {
        pPlxIntr->IopToPciInt = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_LOCAL_2)
    {
        pPlxIntr->IopToPciInt_2 = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_SOFTWARE)
    {
        pPlxIntr->SwInterrupt = 1;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciPowerLevelSet
 *
 * Description:  Set the Power level
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPowerLevelSet(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL   PowerLevel
    )
{
    if (PowerLevel != D0)
        return ApiInvalidPowerState;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciPowerLevelGet
 *
 * Description:  Get the Power Level
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPowerLevelGet(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL  *pPowerLevel
    )
{
    *pPowerLevel = D0;

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxPciPmIdRead
 *
 * Description:  Read the Power Management ID value
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPmIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiPMNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciPmNcpRead
 *
 * Description:  Read the Power Management Next Capabilities Pointer
 *
 ******************************************************************************/
RETURN_CODE
PlxPciPmNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiPMNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciHotSwapIdRead
 *
 * Description:  Read the HotSwap ID value
 *
 ******************************************************************************/
RETURN_CODE
PlxPciHotSwapIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiHSNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciHotSwapNcpRead
 *
 * Description:  Read the HotSwap Next Capabilities Pointer
 *
 ******************************************************************************/
RETURN_CODE
PlxPciHotSwapNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiHSNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciHotSwapStatus
 *
 * Description:  Read the HotSwap status register
 *
 ******************************************************************************/
RETURN_CODE
PlxPciHotSwapStatus(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiHSNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciVpdIdRead
 *
 * Description:  Read the Vital Product Data ID value
 *
 ******************************************************************************/
RETURN_CODE
PlxPciVpdIdRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiVPDNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciVpdNcpRead
 *
 * Description:  Read the Vital Product Data Next Capabilities Pointer
 *
 ******************************************************************************/
RETURN_CODE
PlxPciVpdNcpRead(
    DEVICE_EXTENSION *pdx,
    U8               *pValue
    )
{
    *pValue = (U8)-1;

    return ApiVPDNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciVpdRead
 *
 * Description:  Read the Vital Product Data
 *
 ******************************************************************************/
RETURN_CODE
PlxPciVpdRead(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    )
{
    *pValue = (U32)-1;

    return ApiVPDNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxPciVpdWrite
 *
 * Description:  Write to the Vital Product Data
 *
 ******************************************************************************/
RETURN_CODE
PlxPciVpdWrite(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               VpdData
    )
{
    return ApiVPDNotSupported;
}




/******************************************************************************
 *
 * Function   :  PlxEepromPresent
 *
 * Description:  Determine if a programmed EEPROM is present on the device
 *
 ******************************************************************************/
RETURN_CODE
PlxEepromPresent(
    DEVICE_EXTENSION *pdx,
    BOOLEAN          *pFlag
    )
{
    U32 RegisterValue;


    // Get EEPROM status register
    RegisterValue =
        PLX_REG_READ(
            pdx,
            PCI9050_EEPROM_CTRL
            );

    if (RegisterValue & (1 << 28))
    {
        *pFlag = TRUE;
    }
    else
    {
        *pFlag = FALSE;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxEepromReadByOffset
 *
 * Description:  Read a value from the EEPROM at a specified offset
 *
 ******************************************************************************/
RETURN_CODE
PlxEepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32              *pValue
    )
{
    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return ApiInvalidOffset;
    }

    // Read EEPROM
    EepromReadByOffset(
        pdx,
        Eeprom93CS46,
        offset,
        pValue
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset
 *
 * Description:  Write a 32-bit value to the EEPROM at a specified offset
 *
 ******************************************************************************/
RETURN_CODE
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    U16               offset,
    U32               value
    )
{
    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return ApiInvalidOffset;
    }

    // 9050 does not protect the EEPROM from write access, do nothing

    // Write to EEPROM
    EepromWriteByOffset(
        pdx,
        Eeprom93CS46,
        offset,
        value
        );

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterMailboxRead
 *
 * Description:  Reads a valid Mailbox register from the PLX device
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterMailboxRead(
    DEVICE_EXTENSION *pdx,
    MAILBOX_ID        MailboxId,
    U32              *pValue
    )
{
    *pValue = (U32)-1;

    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterMailboxWrite
 *
 * Description:  Write to one of the PLX device's Mailbox registers
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterMailboxWrite(
    DEVICE_EXTENSION *pdx,
    MAILBOX_ID        MailboxId,
    U32               value
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterDoorbellRead
 *
 * Description:  Returns the last doorbell interrupt value
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterDoorbellRead(
    DEVICE_EXTENSION *pdx,
    U32              *pValue
    )
{
    *pValue = (U32)-1;

    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxRegisterDoorbellWrite
 *
 * Description:  Sets the local Doorbell Register
 *
 ******************************************************************************/
RETURN_CODE
PlxRegisterDoorbellWrite(
    DEVICE_EXTENSION *pdx,
    U32               value
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxMuInboundPortRead
 *
 * Description:  Read the Inbound messaging port of a PLX device
 *
 ******************************************************************************/
RETURN_CODE
PlxMuInboundPortRead(
    DEVICE_EXTENSION *pdx,
    U32              *pFrame
    )
{
    *pFrame = (U32)-1;

    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxMuInboundPortWrite
 *
 * Description:  Write a posted message frame to the inbound port
 *
 ******************************************************************************/
RETURN_CODE
PlxMuInboundPortWrite(
    DEVICE_EXTENSION *pdx,
    U32               Frame
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxMuOutboundPortRead
 *
 * Description:  Reads the posted message frame from the outbound port
 *
 ******************************************************************************/
RETURN_CODE
PlxMuOutboundPortRead(
    DEVICE_EXTENSION *pdx,
    U32              *pFrame
    )
{
    *pFrame = (U32)-1;

    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxMuOutboundPortWrite
 *
 * Description:  Writes to the outbound port with a free message frame
 *
 ******************************************************************************/
RETURN_CODE
PlxMuOutboundPortWrite(
    DEVICE_EXTENSION *pdx,
    U32               Frame
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaControl
 *
 * Description:  Control the DMA engine
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaControl(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_COMMAND       command
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaStatus
 *
 * Description:  Get status of a DMA channel
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaStatus(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockChannelOpen
 *
 * Description:  Requests usage of a device's DMA channel
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaBlockChannelOpen(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc,
    VOID             *pOwner
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockTransfer
 *
 * Description:  Performs DMA block transfer
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaBlockTransfer(
    DEVICE_EXTENSION     *pdx,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *pDmaData,
    VOID                 *pIrp
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaBlockChannelClose
 *
 * Description:  Close a previously opened channel
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaBlockChannelClose(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    BOOLEAN           bCheckInProgress
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglChannelOpen
 *
 * Description:  Open a DMA channel for SGL mode
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaSglChannelOpen(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    DMA_CHANNEL_DESC *pDesc,
    VOID             *pOwner
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglTransfer
 *
 * Description:  Performs a DMA SGL transfer
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaSglTransfer(
    DEVICE_EXTENSION     *pdx,
    DMA_CHANNEL           channel,
    DMA_TRANSFER_ELEMENT *pDmaData,
    VOID                 *pIrp
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxDmaSglChannelClose
 *
 * Description:  Close a previously opened channel
 *
 ******************************************************************************/
RETURN_CODE
PlxDmaSglChannelClose(
    DEVICE_EXTENSION *pdx,
    DMA_CHANNEL       channel,
    BOOLEAN           bCheckInProgress
    )
{
    return ApiUnsupportedFunction;
}




/******************************************************************************
 *
 * Function   :  PlxAbortAddrRead
 *
 * Description:  Read the PCI Abort Address Register
 *
 ******************************************************************************/
RETURN_CODE
PlxAbortAddrRead(
    DEVICE_EXTENSION *pdx,
    U32              *pValue
    )
{
    *pValue = (U32)-1;

    return ApiUnsupportedFunction;
}
