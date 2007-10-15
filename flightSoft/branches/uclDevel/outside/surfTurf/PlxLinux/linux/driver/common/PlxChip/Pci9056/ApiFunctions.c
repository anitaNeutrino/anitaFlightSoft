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
 *      10-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include <linux/slab.h>
#include "ApiFunctions.h"
#include "EepromSupport.h"
#include "PciSupport.h"
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
    U32          QueueCsr;
    U32          QueueCsr_Original;
    U32          RegisterValue;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;


    // Inbound Post Queue Interrupt Control/Status Register
    QueueCsr_Original =
        PLX_REG_READ(
            pdx,
            PCI9056_FIFO_CTRL_STAT
            );

    QueueCsr = QueueCsr_Original;


    if (pPlxIntr->OutboundPost)
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_OUTPOST_INT_MASK,
            0
            );
    }

    if (pPlxIntr->InboundPost)
        QueueCsr &= ~(1 << 4);

    if (pPlxIntr->OutboundOverflow)
    {
        RegData.BitsToSet |=  (1 << 1);
        QueueCsr          &= ~(1 << 6);
    }

    if (pPlxIntr->PciDmaChannel0)
    {
        RegData.BitsToSet |= (1 << 18);

        // Make sure DMA interrupt is routed to PCI
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA0_MODE
                );

        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA0_MODE,
            RegisterValue | (1 << 17)
            );
    }

    if (pPlxIntr->PciDmaChannel1)
    {
        RegData.BitsToSet |= (1 << 19);

        // Make sure DMA interrupt is routed to PCI
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA1_MODE
                );

        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA1_MODE,
            RegisterValue | (1 << 17)
            );
    }

    if (pPlxIntr->IopDmaChannel0)
    {
        RegData.BitsToSet |= (1 << 18);

        // Make sure DMA interrupt is routed to IOP
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA0_MODE
                );

        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA0_MODE,
            RegisterValue & ~(1 << 17)
            );
    }

    if (pPlxIntr->IopDmaChannel1)
    {
        RegData.BitsToSet |= (1 << 19);

        // Make sure DMA interrupt is routed to IOP
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA1_MODE
                );

        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA1_MODE,
            RegisterValue & ~(1 << 17)
            );
    }

    if (pPlxIntr->Mailbox0 || pPlxIntr->Mailbox1 ||
        pPlxIntr->Mailbox2 || pPlxIntr->Mailbox3)
    {
        RegData.BitsToSet |= (1 << 3);
    }

    if (pPlxIntr->IopDoorbell)
        RegData.BitsToSet |= (1 << 17);

    if (pPlxIntr->PciDoorbell)
        RegData.BitsToSet |= (1 << 9);

    if (pPlxIntr->IopToPciInt)
        RegData.BitsToSet |= (1 << 11);

    if (pPlxIntr->PciMainInt)
        RegData.BitsToSet |= (1 << 8);

    if (pPlxIntr->IopMainInt)
        RegData.BitsToSet |= (1 << 16);

    if (pPlxIntr->PciAbort)
        RegData.BitsToSet |= (1 << 10);

    if (pPlxIntr->AbortLSERR)
        RegData.BitsToSet |= (1 << 0);

    if (pPlxIntr->ParityLSERR)
        RegData.BitsToSet |= (1 << 1);

    if (pPlxIntr->RetryAbort)
        RegData.BitsToSet |= (1 << 12);

    if (pPlxIntr->PowerManagement)
        RegData.BitsToSet |= (1 << 4);

    if (pPlxIntr->LocalParityLSERR)
    {
        RegData.BitsToSet |= (1 << 0);
        RegData.BitsToSet |= (1 << 6);
    }

    // Write register values if they have changed

    if (RegData.BitsToSet != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    if (QueueCsr != QueueCsr_Original)
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_FIFO_CTRL_STAT,
            QueueCsr
            );
    }

    // Power Management Interrupt
    if (pPlxIntr->PciPME)
    {
        // Verify that the capability is enabled
        if (PlxIsNewCapabilityEnabled(
                pdx,
                CAPABILITY_POWER_MANAGEMENT
                ) == TRUE)
        {
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_PM_CSR,
                &RegisterValue
                );

            // Set PME Enable bit
            RegisterValue |= (1 << 8);

            PLX_PCI_REG_WRITE(
                pdx,
                PCI9056_PM_CSR,
                RegisterValue
                );
        }
    }

    // Hot Swap Interrupt
    if (pPlxIntr->Enum)
    {
        // Verify that the capability is enabled
        if (PlxIsNewCapabilityEnabled(
                pdx,
                CAPABILITY_HOT_SWAP
                ) == TRUE)
        {
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_HS_CAP_ID,
                &RegisterValue
                );

            // Turn off INS & EXT bits so we don't clear them
            RegisterValue &= ~((1 << 23) | (1 << 22));

            // Disable Enum Interrupt mask
            RegisterValue &= ~(1 << 17);

            PLX_PCI_REG_WRITE(
                pdx,
                PCI9056_HS_CAP_ID,
                RegisterValue
                );
        }
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
    U32          QueueCsr;
    U32          QueueCsr_Original;
    U32          RegisterValue;
    PLX_REG_DATA RegData;


    // Setup to synchronize access to Interrupt Control/Status Register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = 0;

    // Inbound Post Queue Interrupt Control/Status Register
    QueueCsr_Original =
        PLX_REG_READ(
            pdx,
            PCI9056_FIFO_CTRL_STAT
            );

    QueueCsr = QueueCsr_Original;


    if (pPlxIntr->OutboundPost)
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_OUTPOST_INT_MASK,
            (1 << 3)
            );
    }

    if (pPlxIntr->InboundPost)
        QueueCsr |= (1 << 4);

    if (pPlxIntr->OutboundOverflow)
        QueueCsr |= (1 << 6);

    if (pPlxIntr->PciDmaChannel0)
    {
        // Check if DMA interrupt is routed to PCI
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA0_MODE
                );

        if (RegisterValue & (1 << 17))
        {
            RegData.BitsToClear |= (1 << 18);

            // Make sure DMA interrupt is routed to IOP
            PLX_REG_WRITE(
                pdx,
                PCI9056_DMA0_MODE,
                RegisterValue & ~(1 << 17)
                );
        }
    }

    if (pPlxIntr->PciDmaChannel1)
    {
        // Check if DMA interrupt is routed to PCI
        RegisterValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA1_MODE
                );

        if (RegisterValue & (1 << 17))
        {
            RegData.BitsToClear |= (1 << 19);

            // Make sure DMA interrupt is routed to IOP
            PLX_REG_WRITE(
                pdx,
                PCI9056_DMA1_MODE,
                RegisterValue & ~(1 << 17)
                );
        }
    }

    if (pPlxIntr->IopDmaChannel0)
        RegData.BitsToClear |= (1 << 18);

    if (pPlxIntr->IopDmaChannel1)
        RegData.BitsToClear |= (1 << 19);

    if (pPlxIntr->Mailbox0 || pPlxIntr->Mailbox1 ||
        pPlxIntr->Mailbox2 || pPlxIntr->Mailbox3)
    {
        RegData.BitsToClear |= (1 << 3);
    }

    if (pPlxIntr->PciDoorbell)
        RegData.BitsToClear |= (1 << 9);

    if (pPlxIntr->IopDoorbell)
        RegData.BitsToClear |= (1 << 17);

    if (pPlxIntr->PciMainInt)
        RegData.BitsToClear |= (1 << 8);

    if (pPlxIntr->IopToPciInt)
        RegData.BitsToClear |= (1 << 11);

    if (pPlxIntr->IopMainInt)
        RegData.BitsToClear |= (1 << 16);

    if (pPlxIntr->PciAbort)
        RegData.BitsToClear |= (1 << 10);

    if (pPlxIntr->AbortLSERR)
        RegData.BitsToClear |= (1 << 0);

    if (pPlxIntr->ParityLSERR)
        RegData.BitsToClear |= (1 << 1);

    if (pPlxIntr->RetryAbort)
        RegData.BitsToClear |= (1 << 12);

    if (pPlxIntr->PowerManagement)
        RegData.BitsToClear |= (1 << 4);

    if (pPlxIntr->LocalParityLSERR)
        RegData.BitsToClear |= (1 << 6);


    // Write register values if they have changed

    if (RegData.BitsToClear != 0)
    {
        // Synchronize write of Interrupt Control/Status Register
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    if (QueueCsr != QueueCsr_Original)
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_FIFO_CTRL_STAT,
            QueueCsr
            );
    }

    // Power Management Interrupt
    if (pPlxIntr->PciPME)
    {
        // Verify that the capability is enabled
        if (PlxIsNewCapabilityEnabled(
                pdx,
                CAPABILITY_POWER_MANAGEMENT
                ) == TRUE)
        {
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_PM_CSR,
                &RegisterValue
                );

            // Disable PME interrupt
            RegisterValue &= ~(1 << 8);

            PLX_PCI_REG_WRITE(
                pdx,
                PCI9056_PM_CSR,
                RegisterValue
                );
        }
    }

    // Hot Swap Interrupt
    if (pPlxIntr->Enum)
    {
        // Verify that the capability is enabled
        if (PlxIsNewCapabilityEnabled(
                pdx,
                CAPABILITY_HOT_SWAP
                ) == TRUE)
        {
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_HS_CAP_ID,
                &RegisterValue
                );

            // Turn off INS & EXT bits so we don't clear them
            RegisterValue &= ~((1 << 23) | (1 << 22));

            // Enable Enum Interrupt mask
            RegisterValue |= (1 << 17);

            PLX_PCI_REG_WRITE(
                pdx,
                PCI9056_HS_CAP_ID,
                RegisterValue
                );
        }
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

    if (pdx->InterruptSource & INTR_TYPE_DOORBELL)
    {
        pPlxIntr->PciDoorbell = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_PCI_ABORT)
    {
        pPlxIntr->PciAbort = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_DMA_0)
    {
        pPlxIntr->PciDmaChannel0 = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_DMA_1)
    {
        pPlxIntr->PciDmaChannel1 = 1;
    }

    if (pdx->InterruptSource & INTR_TYPE_OUTBOUND_POST)
    {
        pPlxIntr->OutboundPost = 1;
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
    U32 RegisterValue;


    // Check if we support the power state
    if (PlxIsPowerLevelSupported(
            pdx,
            PowerLevel
            ) == FALSE)
    {
        return ApiInvalidPowerState;
    }

    // Get the power state
    PLX_PCI_REG_READ(
        pdx,
        PCI9056_PM_CSR,
        &RegisterValue
        );

    // Update the power state
    RegisterValue = (RegisterValue & ~0x3) | (PowerLevel - D0);

    // Set the new power state
    PLX_PCI_REG_WRITE(
        pdx,
        PCI9056_PM_CSR,
        RegisterValue
        );

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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_POWER_MANAGEMENT
            ) == FALSE)
    {
        *pPowerLevel = D0;
        return ApiInvalidPowerState;
    }

    // Get power state
    PLX_PCI_REG_READ(
        pdx,
        PCI9056_PM_CSR,
        &RegisterValue
        );

    *pPowerLevel = (PLX_POWER_LEVEL)((RegisterValue & 0x3) + D0);

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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_POWER_MANAGEMENT
            ) == FALSE)
    {
        *pValue = (U8)-1;

        return ApiPMNotSupported;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_PM_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)(RegisterValue & 0xFF);

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_POWER_MANAGEMENT
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiPMNotSupported;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_PM_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)((RegisterValue >> 8) & 0xFF);

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_HOT_SWAP
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiHSNotSupported;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_HS_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)(RegisterValue & 0xFF);

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_HOT_SWAP
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiHSNotSupported;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_HS_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)((RegisterValue >> 8) & 0xFF);

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_HOT_SWAP
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiHSNotSupported;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_HS_CAP_ID,
        &RegisterValue
        );

    // Clear bits other than status
    RegisterValue  = RegisterValue >> 16;
    RegisterValue &= (HS_LED_ON | HS_BOARD_REMOVED | HS_BOARD_INSERTED);

    *pValue = (U8)RegisterValue;

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_VPD
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiVpdNotEnabled;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_VPD_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)(RegisterValue & 0xFF);

    return ApiSuccess;
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_VPD
            ) == FALSE)
    {
        *pValue = (U8)-1;
        return ApiVpdNotEnabled;
    }

    PLX_PCI_REG_READ(
        pdx,
        PCI9056_VPD_CAP_ID,
        &RegisterValue
        );

    *pValue = (U8)((RegisterValue >> 8) & 0xFF);

    return ApiSuccess;
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
    S16 VpdRetries;
    S16 VpdPollCount;
    U32 RegisterValue;


    // Check for unaligned offset
    if (offset & 0x3)
    {
        *pValue = (U32)-1;
        return ApiInvalidOffset;
    }

    // EEDO Input must be disabled for VPD access
    RegisterValue =
        PLX_REG_READ(
            pdx,
            PCI9056_EEPROM_CTRL_STAT
            );

    PLX_REG_WRITE(
        pdx,
        PCI9056_EEPROM_CTRL_STAT,
        RegisterValue & ~(1 << 31)
        );

    // Prepare VPD command
    RegisterValue = ((U32)offset << 16) | 0x3;

    VpdRetries = VPD_COMMAND_MAX_RETRIES;
    do
    {
        /**************************************
        *  This loop will continue until the
        *  VPD reports success or until we reach
        *  the maximum number of retries
        **************************************/

        // Send VPD Command
        PLX_PCI_REG_WRITE(
            pdx,
            PCI9056_VPD_CAP_ID,
            RegisterValue
            );

        // Poll until VPD operation has completed
        VpdPollCount = VPD_STATUS_MAX_POLL;
        do
        {
            // Delay for a bit for VPD operation
            Plx_sleep(VPD_STATUS_POLL_DELAY);

            // Get VPD Status
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_VPD_CAP_ID,
                &RegisterValue
                );

            // Check for command completion
            if (RegisterValue & (1 << 31))
            {
                /*******************************************
                * The VPD successfully read the EEPROM. Get
                * the value & return a status of SUCCESS.
                *******************************************/

                // Get the VPD Data result
                PLX_PCI_REG_READ(
                    pdx,
                    PCI9056_VPD_DATA,
                    &RegisterValue
                    );

                *pValue = RegisterValue;

                return ApiSuccess;

            }
        }
        while (VpdPollCount--);
    }
    while (VpdRetries--);

    /******************************************
    * VPD access failed if we reach this 
    * point - return an ERROR status
    *******************************************/

    DebugPrintf(("ERROR - PlxPciVpdRead() failed, VPD timeout\n"));

    *pValue = (U32)-1;

    return ApiFailed;
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
    S16 VpdRetries;
    S16 VpdPollCount;
    U32 RegisterValue;


    // Check for unaligned offset
    if (offset & 0x3)
        return ApiInvalidOffset;

    // EEDO Input must be disabled for VPD access
    RegisterValue =
        PLX_REG_READ(
            pdx,
            PCI9056_EEPROM_CTRL_STAT
            );

    PLX_REG_WRITE(
        pdx,
        PCI9056_EEPROM_CTRL_STAT,
        RegisterValue & ~(1 << 31)
        );

    // Put write value into VPD Data register
    PLX_PCI_REG_WRITE(
        pdx,
        PCI9056_VPD_DATA,
        VpdData
        );

    // Prepare VPD command
    RegisterValue = (1 << 31) | ((U32)offset << 16) | 0x3;

    VpdRetries = VPD_COMMAND_MAX_RETRIES;
    do
    {
        /**************************************
        *  This loop will continue until the
        *  VPD reports success or until we reach
        *  the maximum number of retries
        **************************************/

        // Send VPD command
        PLX_PCI_REG_WRITE(
            pdx,
            PCI9056_VPD_CAP_ID,
            RegisterValue
            );

        // Poll until VPD operation has completed
        VpdPollCount = VPD_STATUS_MAX_POLL;
        do
        {
            // Delay for a bit for VPD operation
            Plx_sleep(VPD_STATUS_POLL_DELAY);

            // Get VPD Status
            PLX_PCI_REG_READ(
                pdx,
                PCI9056_VPD_CAP_ID,
                &RegisterValue
                );

            // Check for command completion
            if ((RegisterValue & (1 << 31)) == 0)
            {
                /*******************************************
                * The VPD successfully wrote to the EEPROM.
                *******************************************/
                return ApiSuccess;
            }
        }
        while (VpdPollCount--);
    }
    while (VpdRetries--);

    /******************************************
    * VPD access failed if we reach this 
    * point - return an ERROR status
    *******************************************/

    DebugPrintf(("ERROR - PlxPciVpdWrite() failed, VPD timeout\n"));

    return ApiFailed;
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
            PCI9056_EEPROM_CTRL_STAT
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
        Eeprom93CS56,
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
    U32 RegisterSave;


    // Verify the offset
    if ((offset & 0x3) || (offset > 0x200))
    {
        DebugPrintf(("ERROR - Invalid EEPROM offset\n"));
        return ApiInvalidOffset;
    }

    // Unprotect the EEPROM for write access
    RegisterSave =
        PLX_REG_READ(
            pdx,
            PCI9056_ENDIAN_DESC
            );

    PLX_REG_WRITE(
        pdx,
        PCI9056_ENDIAN_DESC,
        RegisterSave & ~(0xFF << 16)
        );

    // Write to EEPROM
    EepromWriteByOffset(
        pdx,
        Eeprom93CS56,
        offset,
        value
        );

    // Restore EEPROM Write-Protected Address Boundary
    PLX_REG_WRITE(
        pdx,
        PCI9056_ENDIAN_DESC,
        RegisterSave
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
    // Verify Mailbox ID
    if (MailboxId < MailBox0 || MailboxId > MailBox7)
    {
        *pValue = (U32)-1;
        return ApiInvalidRegister;
    }

    // Check if Mailbox 0 or 1
    if (MailboxId == MailBox0 || MailboxId == MailBox1)
    {
        *pValue =
            PLX_REG_READ(
                pdx,
                PCI9056_MAILBOX0 + (MailboxId * sizeof(U32))
                );
    }
    else
    {
        // Mailboxes 2 to 7 are not based from Malibox 0
        *pValue =
            PLX_REG_READ(
                pdx,
                PCI9056_MAILBOX2 + ((MailboxId-2) * sizeof(U32))
                );
    }

    return ApiSuccess;
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
    // Verify Mailbox ID
    if (MailboxId < MailBox0 || MailboxId > MailBox7)
    {
        return ApiInvalidRegister;
    }

    if (MailboxId == MailBox0 || MailboxId == MailBox1)
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_MAILBOX0 + (MailboxId * sizeof(U32)),
            value
            );
    }
    else
    {
        PLX_REG_WRITE(
            pdx,
            PCI9056_MAILBOX2 + ((MailboxId-2) * sizeof(U32)),
            value
            );
    }

    return ApiSuccess;
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
    *pValue = pdx->IntrDoorbellValue;

    return ApiSuccess;
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
    PLX_REG_WRITE(
        pdx,
        PCI9056_LOCAL_DOORBELL,
        value
        );

    return ApiSuccess;
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
    *pFrame =
        PLX_REG_READ(
            pdx,
            0x40
            );

    return ApiSuccess;
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
    PLX_REG_WRITE(
        pdx,
        0x40,
        Frame
        );

    return ApiSuccess;
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
    *pFrame =
        PLX_REG_READ(
            pdx,
            0x44
            );

    return ApiSuccess;
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
    PLX_REG_WRITE(
        pdx,
        0x44,
        Frame
        );

    return ApiSuccess;
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
    U8  i;
    U8  shift;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i     = 0;
            shift = 0;
            break;

        case PrimaryPciChannel1:
            i     = 1;
            shift = 8;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    // Verify that this channel has been opened
    if (pdx->DmaInfo[i].state == DmaStateClosed)
    {
        DebugPrintf(("ERROR - DMA Channel has not been opened\n"));
        return ApiDmaChannelUnavailable;
    }

    switch (command)
    {
        case DmaPause:
            // Pause the DMA Channel
            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT
                    );

            PLX_REG_WRITE(
                pdx,
                PCI9056_DMA_COMMAND_STAT,
                RegValue & ~((1 << 0) << shift)
                );

            // Check if the transfer has completed
            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT
                    );

            if (RegValue & ((1 << 4) << shift))
                return ApiDmaDone;
            break;

        case DmaResume:
            // Verify that the DMA Channel is paused
            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT
                    );

            if ((RegValue & (((1 << 4) | (1 << 0)) << shift)) == 0)
            {
                PLX_REG_WRITE(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT,
                    RegValue | ((1 << 0) << shift)
                    );
            }
            else
            {
                return ApiDmaNotPaused;
            }
            break;

        case DmaAbort:
            // Pause the DMA Channel
            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT
                    );

            PLX_REG_WRITE(
                pdx,
                PCI9056_DMA_COMMAND_STAT,
                RegValue & ~((1 << 0) << shift)
                );

            // Check if the transfer has completed
            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9056_DMA_COMMAND_STAT
                    );

            if (RegValue & ((1 << 4) << shift))
                return ApiDmaDone;

            // Abort the transfer (should cause an interrupt)
            PLX_REG_WRITE(
                pdx,
                PCI9056_DMA_COMMAND_STAT,
                RegValue | ((1 << 2) << shift)
                );
            break;

        default:
            return ApiDmaCommandInvalid;
    }

    return ApiSuccess;
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
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
        case PrimaryPciChannel1:
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    // Return the current DMA status

    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_COMMAND_STAT
            );

    if (channel == PrimaryPciChannel1)
        RegValue = RegValue >> 8;

    if ((RegValue & ((1 << 4) | (1 << 0))) == 0)
        return ApiDmaPaused;

    if (RegValue & (1 << 4))
        return ApiDmaDone;

    return ApiDmaInProgress;
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
    U8           i;
    U32          mode;
    U32          RegValue;
    U32          threshold;
    PLX_REG_DATA RegData;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i = 0;
            break;

        case PrimaryPciChannel1:
            i = 1;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify that we can open the channel
    if (pdx->DmaInfo[i].state != DmaStateClosed)
    {
        DebugPrintf(("ERROR - DMA channel already opened\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Open the channel
    pdx->DmaInfo[i].state = DmaStateBlock;

    // Record the Owner
    pdx->DmaInfo[i].pOwner = pOwner;

    // Mark DMA as free
    pdx->DmaInfo[i].bPending = FALSE;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    // Setup for synchronized access to Interrupt register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToClear = 0;

    spin_lock(
        &(pdx->Lock_HwAccess)
        );

    // Get DMA priority
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_LOCAL_DMA_ARBIT
            );

    // Clear priority
    RegValue &= ~((1 << 20) | (1 << 19));

    // Set the priority
    switch (pDesc->DmaChannelPriority)
    {
        case Channel0Highest:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue | (1 << 19)
                );
            break;

        case Channel1Highest:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue | (1 << 20)
                );
            break;

        case Rotational:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue
                );
            break;

        default:
            DebugPrintf((
                "WARNING - DmaChannelOpen() invalid priority state\n"
                ));
            break;
    }


    threshold =
        (pDesc->TholdForIopWrites <<  0) |
        (pDesc->TholdForIopReads  <<  4) |
        (pDesc->TholdForPciWrites <<  8) |
        (pDesc->TholdForPciReads  << 12);

    mode =
        (0 <<  9) |                  // No Chaining
        (1 << 10) |                  // Enable DMA Done interrupt
        (1 << 17) |                  // Route interrupts to PCI
        (pDesc->IopBusWidth              <<  0) |
        (pDesc->WaitStates               <<  2) |
        (pDesc->EnableReadyInput         <<  6) |
        (pDesc->EnableBTERMInput         <<  7) |
        (pDesc->EnableIopBurst           <<  8) |
        (pDesc->HoldIopAddrConst         << 11) |
        (pDesc->DemandMode               << 12) |
        (pDesc->EnableWriteInvalidMode   << 13) |
        (pDesc->EnableDmaEOTPin          << 14) |
        (pDesc->DmaStopTransferMode      << 15) |
        (pDesc->EnableDualAddressCycles  << 18) |
        (pDesc->ValidModeEnable          << 20);

    // Get DMA Threshold
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_THRESHOLD
            );

    if (channel == PrimaryPciChannel0)
    {
        // Setup threshold
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA_THRESHOLD,
            (RegValue & 0xffff0000) | threshold
            );

        // Write DMA mode
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA0_MODE,
            mode
            );

        // Enable DMA Channel interrupt
        RegData.BitsToSet = (1 << 18);
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }
    else
    {
        // Setup threshold
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA_THRESHOLD,
            (RegValue & 0x0000ffff) | (threshold << 16)
            );

        // Write DMA mode
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA1_MODE,
            mode
            );

        // Enable DMA Channel interrupt
        RegData.BitsToSet = (1 << 19);
        PlxSynchronizedRegisterModify(
            &RegData
            );
    }

    spin_unlock(
        &(pdx->Lock_HwAccess)
        );

    return ApiSuccess;
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
    U8  i;
    U8  shift;
    U16 OffsetDmaBase;
    U16 OffsetPciAddrHigh;
    U32 RegValue;


    // Setup transfer registers
    switch (channel)
    {
        case PrimaryPciChannel0:
            i                 = 0;
            shift             = 0;
            OffsetDmaBase     = PCI9056_DMA0_MODE;
            OffsetPciAddrHigh = PCI9056_DMA0_PCI_DAC;
            break;

        case PrimaryPciChannel1:
            i                 = 1;
            shift             = 8;
            OffsetDmaBase     = PCI9056_DMA1_MODE;
            OffsetPciAddrHigh = PCI9056_DMA1_PCI_DAC;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    // Verify that DMA is not in progress
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_COMMAND_STAT
            );

    if ((RegValue & ((1 << 4) << shift)) == 0)
    {
        DebugPrintf(("ERROR - DmaTransfer() Channel is active\n"));
        return ApiDmaInProgress;
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify DMA Channel was opened correctly
    if (pdx->DmaInfo[i].state != DmaStateBlock)
    {
        DebugPrintf(("ERROR - DMA Channel has not been opened for Block DMA\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Verify a DMA transfer is not pending
    if (pdx->DmaInfo[i].bPending == TRUE)
    {
        DebugPrintf((
            "ERROR - A DMA transfer is currently pending\n"
            ));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaInProgress;
    }

    // Mark DMA as pending
    pdx->DmaInfo[i].bPending = TRUE;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    spin_lock(
        &(pdx->Lock_HwAccess)
        );

    // Make sure DMA done interrupt is enabled
    RegValue =
        PLX_REG_READ(
            pdx,
            OffsetDmaBase
            );

    RegValue |= (1 << 10);

    PLX_REG_WRITE(
        pdx,
        OffsetDmaBase,
        RegValue
        );

    // Write PCI Address
    PLX_REG_WRITE(
        pdx,
        OffsetDmaBase + 0x4,
        pDmaData->u.PciAddrLow
        );

    // Write Local Address
    PLX_REG_WRITE(
        pdx,
        OffsetDmaBase + 0x8,
        pDmaData->LocalAddr
        );

    // Write Transfer Count
    PLX_REG_WRITE(
        pdx,
        OffsetDmaBase + 0xc,
        pDmaData->TransferCount
        );

    // Write Descriptor Pointer
    RegValue = (pDmaData->TerminalCountIntr << 2) |
               (pDmaData->LocalToPciDma     << 3);

    PLX_REG_WRITE(
        pdx,
        OffsetDmaBase + 0x10,
        RegValue
        );

    // Write the high PCI address
    PLX_REG_WRITE(
        pdx,
        OffsetPciAddrHigh,
        pDmaData->PciAddrHigh
        );

    // Enable DMA Channel
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_COMMAND_STAT
            );

    PLX_REG_WRITE(
        pdx,
        PCI9056_DMA_COMMAND_STAT,
        RegValue | ((1 << 0) << shift)
        );

    DebugPrintf(("Starting DMA transfer...\n"));

    // Start DMA
    PLX_REG_WRITE(
        pdx,
        PCI9056_DMA_COMMAND_STAT,
        RegValue | (((1 << 0) | (1 << 1)) << shift)
        );

    spin_unlock(
        &(pdx->Lock_HwAccess)
        );

    return ApiSuccess;
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
    U8  i;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i = 0;
            break;

        case PrimaryPciChannel1:
            i = 1;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    if (bCheckInProgress)
    {
        // Verify that DMA is not in progress
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA_COMMAND_STAT
                );

        if (channel == PrimaryPciChannel1)
            RegValue = RegValue >> 8;

        if ((RegValue & (1 << 4)) == 0)
        {
            if (RegValue & (1 << 0))
                return ApiDmaInProgress;
            else
                return ApiDmaPaused;
        }
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify DMA Channel was opened correctly
    if (pdx->DmaInfo[i].state != DmaStateBlock)
    {
        DebugPrintf(("ERROR - DMA Channel has not been opened for Block DMA\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Close the channel
    pdx->DmaInfo[i].state = DmaStateClosed;

    // Clear owner information
    pdx->DmaInfo[i].pOwner = NULL;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    return ApiSuccess;
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
    U8           i;
    U32          mode;
    U32          RegValue;
    U32          threshold;
    PLX_REG_DATA RegData;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i = 0;
            break;

        case PrimaryPciChannel1:
            i = 1;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify that we can open the channel
    if (pdx->DmaInfo[i].state != DmaStateClosed)
    {
        DebugPrintf(("ERROR - DMA channel already opened\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Open the channel
    pdx->DmaInfo[i].state = DmaStateSgl;

    // Record the Owner
    pdx->DmaInfo[i].pOwner = pOwner;

    // Mark DMA as free
    pdx->DmaInfo[i].bPending = FALSE;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    // Setup for synchronized access to Interrupt register
    RegData.pdx         = pdx;
    RegData.offset      = PCI9056_INT_CTRL_STAT;
    RegData.BitsToClear = 0;

    spin_lock(
        &(pdx->Lock_HwAccess)
        );

    // Get DMA priority
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_LOCAL_DMA_ARBIT
            );

    // Clear priority
    RegValue &= ~((1 << 20) | (1 << 19));

    // Set the priority
    switch (pDesc->DmaChannelPriority)
    {
        case Channel0Highest:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue | (1 << 19)
                );
            break;

        case Channel1Highest:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue | (1 << 20)
                );
            break;

        case Rotational:
            PLX_REG_WRITE(
                pdx,
                PCI9056_LOCAL_DMA_ARBIT,
                RegValue
                );
            break;

        default:
            DebugPrintf((
                "WARNING - DmaChannelOpen() invalid priority state\n"
                ));
            break;
    }


    threshold =
        (pDesc->TholdForIopWrites <<  0) |
        (pDesc->TholdForIopReads  <<  4) |
        (pDesc->TholdForPciWrites <<  8) |
        (pDesc->TholdForPciReads  << 12);

    mode =
        (1 <<  9) |                  // Enable Chaining
        (1 << 10) |                  // Enable DMA Done interrupt
        (1 << 17) |                  // Route interrupts to PCI
        (0 << 18) |                  // Disable Dual-Addressing
        (0 << 20) |                  // Disable Valid mode
        (pDesc->IopBusWidth              <<  0) |
        (pDesc->WaitStates               <<  2) |
        (pDesc->EnableReadyInput         <<  6) |
        (pDesc->EnableBTERMInput         <<  7) |
        (pDesc->EnableIopBurst           <<  8) |
        (pDesc->HoldIopAddrConst         << 11) |
        (pDesc->DemandMode               << 12) |
        (pDesc->EnableWriteInvalidMode   << 13) |
        (pDesc->EnableDmaEOTPin          << 14) |
        (pDesc->DmaStopTransferMode      << 15);

    // Keep track if local address should remain constant
    if (pDesc->HoldIopAddrConst)
        pdx->DmaInfo[i].bLocalAddrConstant = TRUE;

    // Get DMA Threshold
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_THRESHOLD
            );

    if (channel == PrimaryPciChannel0)
    {
        // Setup threshold
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA_THRESHOLD,
            (RegValue & 0xffff0000) | threshold
            );

        // Write DMA mode
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA0_MODE,
            mode
            );

        // Enable DMA Channel interrupt
        RegData.BitsToSet = (1 << 18);
        PlxSynchronizedRegisterModify(
            &RegData
            );

        // Clear Dual Address cycle register
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA0_PCI_DAC,
            0
            );
    }
    else
    {
        // Setup threshold
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA_THRESHOLD,
            (RegValue & 0x0000ffff) | (threshold << 16)
            );

        // Write DMA mode
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA1_MODE,
            mode
            );

        // Enable DMA Channel interrupt
        RegData.BitsToSet = (1 << 19);
        PlxSynchronizedRegisterModify(
            &RegData
            );

        // Clear Dual Address cycle register
        PLX_REG_WRITE(
            pdx,
            PCI9056_DMA1_PCI_DAC,
            0
            );
    }

    spin_unlock(
        &(pdx->Lock_HwAccess)
        );

    return ApiSuccess;
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
    U8    i;
    U8    shift;
    U16   OffsetDesc;
    U32   RegValue;
    PVOID SglPciAddress;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i          = 0;
            shift      = 0;
            OffsetDesc = PCI9056_DMA0_DESC_PTR;
            break;

        case PrimaryPciChannel1:
            i          = 1;
            shift      = 8;
            OffsetDesc = PCI9056_DMA1_DESC_PTR;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    // Verify that DMA is not in progress
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_COMMAND_STAT
            );

    if ((RegValue & ((1 << 4) << shift)) == 0)
    {
        DebugPrintf(("ERROR - DmaTransfer() Channel is active\n"));
        return ApiDmaInProgress;
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify DMA Channel was opened correctly
    if (pdx->DmaInfo[i].state != DmaStateSgl)
    {
        DebugPrintf(("ERROR - DMA channel has not been opened for SGL DMA\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Verify a DMA transfer is not pending
    if (pdx->DmaInfo[i].bPending)
    {
        DebugPrintf((
            "ERROR - A DMA transfer is currently in progress\n"
            ));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaInProgress;
    }

    // Set the DMA pending flag
    pdx->DmaInfo[i].bPending = TRUE;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    SglPciAddress =
        PlxLockBufferAndBuildSgl(
            pdx,
            i,
            pDmaData
            );

    if (SglPciAddress == NULL)
    {
        DebugPrintf(("ERROR - Unable to lock buffer and build SGL list\n"));
        pdx->DmaInfo[i].bPending = FALSE;
        return ApiDmaSglBuildFailed;
    }

    spin_lock(
        &(pdx->Lock_HwAccess)
        );

    // Write SGL physical address & set descriptors in PCI space
    PLX_REG_WRITE(
        pdx,
        OffsetDesc,
        (U32)SglPciAddress | (1 << 0)
        );

    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9056_DMA_COMMAND_STAT
            );

    // Enable DMA channel
    PLX_REG_WRITE(
        pdx,
        PCI9056_DMA_COMMAND_STAT,
        RegValue | ((1 << 0) << shift)
        );

    DebugPrintf(("Starting DMA transfer...\n"));

    // Start DMA
    PLX_REG_WRITE(
        pdx,
        PCI9056_DMA_COMMAND_STAT,
        RegValue | (((1 << 0) | (1 << 1)) << shift)
        );

    spin_unlock(
        &(pdx->Lock_HwAccess)
        );

    return ApiSuccess;
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
    U8  i;
    U32 RegValue;


    // Verify valid DMA channel
    switch (channel)
    {
        case PrimaryPciChannel0:
            i = 0;
            break;

        case PrimaryPciChannel1:
            i = 1;
            break;

        default:
            DebugPrintf(("ERROR - Invalid DMA channel\n"));
            return ApiDmaChannelInvalid;
    }

    if (bCheckInProgress)
    {
        // Verify that DMA is not in progress
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9056_DMA_COMMAND_STAT
                );

        if (channel == PrimaryPciChannel1)
            RegValue = RegValue >> 8;

        if ((RegValue & (1 << 4)) == 0)
        {
            if (RegValue & (1 << 0))
                return ApiDmaInProgress;
            else
                return ApiDmaPaused;
        }
    }

    spin_lock(
        &(pdx->Lock_DmaChannel)
        );

    // Verify DMA Channel was opened correctly
    if (pdx->DmaInfo[i].state != DmaStateSgl)
    {
        DebugPrintf(("ERROR - DMA channel has not been opened for SGL DMA\n"));

        spin_unlock(
            &(pdx->Lock_DmaChannel)
            );

        return ApiDmaChannelUnavailable;
    }

    // Close the channel
    pdx->DmaInfo[i].state = DmaStateClosed;

    // Clear owner information
    pdx->DmaInfo[i].pOwner = NULL;

    spin_unlock(
        &(pdx->Lock_DmaChannel)
        );

    return ApiSuccess;
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
    *pValue =
        PLX_REG_READ(
            pdx,
            PCI9056_ABORT_ADDRESS
            );

    return ApiSuccess;
}
