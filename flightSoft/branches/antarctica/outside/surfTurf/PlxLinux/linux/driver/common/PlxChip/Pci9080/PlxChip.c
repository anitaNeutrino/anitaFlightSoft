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
 *      PlxChip.c
 *
 * Description:
 *
 *      This file contains functions specific to a PLX Chip.
 *
 * Revision History:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "DriverDefs.h"
#include "PciSupport.h"
#include "PlxInterrupt.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  PlxChipPciInterruptEnable
 *
 * Description:  Enables the PLX Chip PCI Interrupt
 *
 *****************************************************************************/
BOOLEAN
PlxChipPciInterruptEnable(
    DEVICE_EXTENSION *pdx
    )
{
    PLX_REG_DATA RegData;


    // Setup for synchronized register access
    RegData.pdx         = pdx;
    RegData.offset      = PCI9080_INT_CTRL_STAT;
    RegData.BitsToSet   = (1 << 8);
    RegData.BitsToClear = 0;

    PlxSynchronizedRegisterModify(
        &RegData
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxChipPciInterruptDisable
 *
 * Description:  Disables the PLX Chip PCI Interrupt
 *
 *****************************************************************************/
BOOLEAN
PlxChipPciInterruptDisable(
    DEVICE_EXTENSION *pdx
    )
{
    PLX_REG_DATA RegData;


    // Setup for synchronized register access
    RegData.pdx         = pdx;
    RegData.offset      = PCI9080_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = (1 << 8);

    PlxSynchronizedRegisterModify(
        &RegData
        );

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxPciBoardReset
 *
 * Description:  Resets a device using software reset feature of PLX chip
 *
 ******************************************************************************/
VOID
PlxPciBoardReset(
    DEVICE_EXTENSION *pdx
    )
{
    U8  MU_Enabled;
    U8  EepromPresent;
    U32 RegValue;
    U32 RegInterrupt;
    U32 RegMailbox0;
    U32 RegMailbox1;


    // Added to avoid compiler warnings
    RegMailbox0 = 0;
    RegMailbox1 = 0;

    // Clear any PCI errors
    PLX_PCI_REG_READ(
        pdx,
        CFG_COMMAND,
        &RegValue
        );

    if (RegValue & (0xf8 << 24))
    {
        // Write value back to clear aborts
        PLX_PCI_REG_WRITE(
            pdx,
            CFG_COMMAND,
            RegValue
            );
    }

    // Save state of I2O Decode Enable
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9080_FIFO_CTRL_STAT
            );

    MU_Enabled = (U8)(RegValue & (1 << 0));

    // Determine if an EEPROM is present
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9080_EEPROM_CTRL_STAT
            );

    // Make sure S/W Reset & EEPROM reload bits are clear
    RegValue &= ~((1 << 30) | (1 << 29));

    // Remember if EEPROM is present
    EepromPresent = (U8)((RegValue >> 28) & (1 << 0));

    // Save some registers if EEPROM present
    if (RegValue & (1 << 28))
    {
        RegMailbox0 =
            PLX_REG_READ(
                pdx,
                PCI9080_MAILBOX0
                );

        RegMailbox1 =
            PLX_REG_READ(
                pdx,
                PCI9080_MAILBOX1
                );

        PLX_PCI_REG_READ(
            pdx,
            CFG_INT_LINE,
            &RegInterrupt
            );
    }

    // Issue Software Reset to hold PLX chip in reset
    PLX_REG_WRITE(
        pdx,
        PCI9080_EEPROM_CTRL_STAT,
        RegValue | (1 << 30)
        );

    // Delay for a bit
    Plx_sleep(100);

    // Bring chip out of reset
    PLX_REG_WRITE(
        pdx,
        PCI9080_EEPROM_CTRL_STAT,
        RegValue
        );

    // Issue EEPROM reload in case now programmed
    PLX_REG_WRITE(
        pdx,
        PCI9080_EEPROM_CTRL_STAT,
        RegValue | (1 << 29)
        );

    // Delay for a bit
    Plx_sleep(10);

    // Clear EEPROM reload
    PLX_REG_WRITE(
        pdx,
        PCI9080_EEPROM_CTRL_STAT,
        RegValue
        );

    // Restore I2O Decode Enable state
    if (MU_Enabled)
    {
        // Save state of I2O Decode Enable
        RegValue =
            PLX_REG_READ(
                pdx,
                PCI9080_FIFO_CTRL_STAT
                );

        PLX_REG_WRITE(
            pdx,
            PCI9080_FIFO_CTRL_STAT,
            RegValue | (1 << 0)
            );
    }

    // If EEPROM was present, restore registers
    if (EepromPresent)
    {
        // Restore saved registers
        PLX_REG_WRITE(
            pdx,
            PCI9080_MAILBOX0,
            RegMailbox0
            );

        PLX_REG_WRITE(
            pdx,
            PCI9080_MAILBOX1,
            RegMailbox1
            );

        PLX_PCI_REG_WRITE(
            pdx,
            CFG_INT_LINE,
            RegInterrupt
            );
    }
}




/******************************************************************************
 *
 * Function   :  PlxChipSetInterruptNotifyFlags
 *
 * Description:  Sets the interrupt notification flags of a wait object
 *
 ******************************************************************************/
VOID
PlxChipSetInterruptNotifyFlags(
    PLX_INTR         *pPlxIntr,
    INTR_WAIT_OBJECT *pWaitObject
    )
{
    // Clear notify events
    pWaitObject->NotifyOnInterrupt = INTR_TYPE_NONE;

    if (pPlxIntr->IopToPciInt)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_LOCAL_1;

    if (pPlxIntr->PciAbort)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_PCI_ABORT;

    if (pPlxIntr->PciDoorbell)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_DOORBELL;

    if (pPlxIntr->OutboundPost)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_OUTBOUND_POST;

    if (pPlxIntr->PciDmaChannel0)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_DMA_0;

    if (pPlxIntr->PciDmaChannel1)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_DMA_1;
}




/******************************************************************************
 *
 * Function   :  PlxChipGetSpace
 *
 * Description:  Returns the BAR index and remap register offset for a space
 *
 ******************************************************************************/
VOID
PlxChipGetSpace(
    DEVICE_EXTENSION *pdx,
    IOP_SPACE         IopSpace,
    U8               *pBarIndex,
    U16              *pOffset_RegRemap
    )
{
    U32 RegValue;


    switch (IopSpace)
    {
        case IopSpace0:
            *pBarIndex        = 2;
            *pOffset_RegRemap = PCI9080_SPACE0_REMAP;
            break;

        case IopSpace1:
            *pOffset_RegRemap = PCI9080_SPACE1_REMAP;

            /*********************************************************************
             *  Note: Space 1 is a special case.  If the I2O Decode Enable bit
             *        is set, BAR3 is moved to BAR0.
             ********************************************************************/

            RegValue =
                PLX_REG_READ(
                    pdx,
                    PCI9080_FIFO_CTRL_STAT
                    );

            if (RegValue & (1 << 0))
            {
                // I2O Decode is enbled, use BAR0 for Space 1
                *pBarIndex = 0;
            }
            else
            {
                *pBarIndex = 3;
            }
            break;

        case ExpansionRom:
            *pBarIndex        = 6;
            *pOffset_RegRemap = PCI9080_EXP_ROM_REMAP;
            break;

        case IopSpace2:
        case IopSpace3:
        default:
            *pBarIndex = (U8)-1;
            DebugPrintf(("ERROR - Invalid Space\n"));
            break;
    }
}




/******************************************************************************
 *
 * Function   :  PlxChipRestoreAfterDma
 *
 * Description:  Restore chip to correct state after DMA completion
 *
 ******************************************************************************/
VOID
PlxChipRestoreAfterDma(
    DEVICE_EXTENSION *pdx,
    U8                DmaIndex
    )
{
    // Nothing to do for this chip
}




/******************************************************************************
 *
 * Function   :  PlxChipPostCommonBufferProperties
 *
 * Description:  Post the common buffer properties to the device
 *
 ******************************************************************************/
VOID
PlxChipPostCommonBufferProperties(
    DEVICE_EXTENSION *pdx,
    U32               PhysicalAddress,
    U32               Size
    )
{
    // Write the Physical Address
    PLX_REG_WRITE(
        pdx,
        PCI9080_MAILBOX3,
        PhysicalAddress
        );

    // Write the Size
    PLX_REG_WRITE(
        pdx,
        PCI9080_MAILBOX4,
        Size
        );
}




/******************************************************************************
 *
 * Function   :  PlxIsPowerLevelSupported
 *
 * Description:  Verify that the power level is supported
 *
 ******************************************************************************/
BOOLEAN
PlxIsPowerLevelSupported(
    DEVICE_EXTENSION *pdx,
    PLX_POWER_LEVEL   PowerLevel
    )
{
    if ((PowerLevel == D0) || (PowerLevel == D3Hot))
        return TRUE;

    return FALSE;
}




/******************************************************************************
 *
 * Function   :  PlxIsNewCapabilityEnabled
 *
 * Description:  Verifies access to the new Capabilities registers
 *
 ******************************************************************************/
BOOLEAN
PlxIsNewCapabilityEnabled(
    DEVICE_EXTENSION *pdx,
    U8                CapabilityToVerify
    )
{
    // No New Capabilities in this chip
    return FALSE;
}
