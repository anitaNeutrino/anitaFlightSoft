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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
    RegData.BitsToSet   = (1 << 6);
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
    RegData.offset      = PCI9030_INT_CTRL_STAT;
    RegData.BitsToSet   = 0;
    RegData.BitsToClear = (1 << 6);

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
    U8  EepromPresent;
    U32 RegValue;
    U32 RegInterrupt;
    U32 RegHotSwap;
    U32 RegPowerMgmnt;
    U32 RegIntCtrlStatus;


    // Added to avoid compiler warnings
    RegIntCtrlStatus = 0;

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

    // Determine if an EEPROM is present
    RegValue =
        PLX_REG_READ(
            pdx,
            PCI9030_EEPROM_CTRL
            );

    // Make sure S/W Reset & EEPROM reload bits are clear
    RegValue &= ~((1 << 30) | (1 << 29));

    // Remember if EEPROM is present
    EepromPresent = (U8)((RegValue >> 28) & (1 << 0));

    // Save some registers if EEPROM present
    if (EepromPresent)
    {
        RegIntCtrlStatus =
            PLX_REG_READ(
                pdx,
                PCI9030_INT_CTRL_STAT
                );

        PLX_PCI_REG_READ(
            pdx,
            CFG_INT_LINE,
            &RegInterrupt
            );

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_HS_CAP_ID,
            &RegHotSwap
            );

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_PM_CSR,
            &RegPowerMgmnt
            );
    }

    // Issue Software Reset to hold PLX chip in reset
    PLX_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue | (1 << 30)
        );

    // Delay for a bit
    Plx_sleep(100);

    // Bring chip out of reset
    PLX_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue
        );

    // Issue EEPROM reload in case now programmed
    PLX_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue | (1 << 29)
        );

    // Delay for a bit
    Plx_sleep(10);

    // Clear EEPROM reload
    PLX_REG_WRITE(
        pdx,
        PCI9030_EEPROM_CTRL,
        RegValue
        );

    // If EEPROM was present, restore registers
    if (EepromPresent)
    {
        // Mask interrupt clear bits
        RegIntCtrlStatus &= ~((1 << 11) | (1 << 10));

        PLX_REG_WRITE(
            pdx,
            PCI9030_INT_CTRL_STAT,
            RegIntCtrlStatus
            );

        PLX_PCI_REG_WRITE(
            pdx,
            CFG_INT_LINE,
            RegInterrupt
            );

        // Mask out HS bits that can be cleared
        RegHotSwap &= ~((1 << 23) | (1 << 22) | (1 << 17));

        PLX_PCI_REG_WRITE(
            pdx,
            PCI9030_HS_CAP_ID,
            RegHotSwap
            );

        // Mask out PM bits that can be cleared
        RegPowerMgmnt &= ~(1 << 15);

        PLX_PCI_REG_READ(
            pdx,
            PCI9030_PM_CSR,
            &RegPowerMgmnt
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

    if (pPlxIntr->IopToPciInt_2)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_LOCAL_2;

    if (pPlxIntr->SwInterrupt)
        pWaitObject->NotifyOnInterrupt |= INTR_TYPE_SOFTWARE;
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
    switch (IopSpace)
    {
        case IopSpace0:
            *pBarIndex        = 2;
            *pOffset_RegRemap = PCI9030_REMAP_SPACE0;
            break;

        case IopSpace1:
            *pBarIndex        = 3;
            *pOffset_RegRemap = PCI9030_REMAP_SPACE1;
            break;

        case IopSpace2:
            *pBarIndex        = 4;
            *pOffset_RegRemap = PCI9030_REMAP_SPACE2;
            break;

        case IopSpace3:
            *pBarIndex        = 5;
            *pOffset_RegRemap = PCI9030_REMAP_SPACE3;
            break;

        case ExpansionRom:
            *pBarIndex        = 6;
            *pOffset_RegRemap = PCI9030_REMAP_EXP_ROM;
            break;

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
    // Nothing to do for this chip
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
    U32 RegisterValue;


    // Check if New capabilities are enabled
    if (PlxIsNewCapabilityEnabled(
            pdx,
            CAPABILITY_POWER_MANAGEMENT
            ) == FALSE)
    {
        return FALSE;
    }

    // Verify that the Power State is supported
    switch (PowerLevel)
    {
        case D0:
        case D3Hot:
            break;

        case D1:
        case D2:
            // Get the Power Control/Status
            PLX_PCI_REG_READ(
                pdx,
                PCI9030_PM_CAP_ID,
                &RegisterValue
                );

            if ((PowerLevel == D1) && !(RegisterValue & (1 << 25)))
                return FALSE;

            if ((PowerLevel == D2) && !(RegisterValue & (1 << 26)))
                return FALSE;
            break;

        case D0Uninitialized:    // D0Uninitialized cannot be set by software
        case D3Cold:             // D3Cold cannot be set by software
        default:
            return FALSE;
    }

    return TRUE;
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
    U32 RegisterValue;
    U32 RegisterPmCapability;
    U32 RegisterHsCapability;
    U32 RegisterVpdCapability;


    // Check if New capabilities are enabled
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_COMMAND,
        &RegisterValue
        );

    if ((RegisterValue & (1 << 20)) == 0)
        return FALSE;

    // Get capabilities pointer
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_CAP_PTR,
        &RegisterValue
        );

    // Mask non-relevant bits
    RegisterValue = RegisterValue & 0xFF;

    if (RegisterValue == 0)
        return FALSE;

    // Get PM capabilities pointer
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_PM_CAP_ID,
        &RegisterPmCapability
        );

    // Mask non-relevant bits
    RegisterPmCapability = (RegisterPmCapability >> 8) & 0xFF;

    // Get HS capabilities pointer
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_HS_CAP_ID,
        &RegisterHsCapability
        );

    // Mask non-relevant bits
    RegisterHsCapability = (RegisterHsCapability >> 8) & 0xFF;

    // Get VPD capabilities pointer
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_VPD_CAP_ID,
        &RegisterVpdCapability
        );

    // Mask non-relevant bits
    RegisterVpdCapability = (RegisterVpdCapability >> 8) & 0xFF;

    if (CapabilityToVerify & CAPABILITY_POWER_MANAGEMENT)
    {
        if ( (RegisterValue         != PCI9030_PM_CAP_ID) &&
             (RegisterPmCapability  != PCI9030_PM_CAP_ID) &&
             (RegisterHsCapability  != PCI9030_PM_CAP_ID) &&
             (RegisterVpdCapability != PCI9030_PM_CAP_ID) )
        {
            return FALSE;
        }
    }

    if (CapabilityToVerify & CAPABILITY_HOT_SWAP)
    {
        if ( (RegisterValue         != PCI9030_HS_CAP_ID) &&
             (RegisterPmCapability  != PCI9030_HS_CAP_ID) &&
             (RegisterHsCapability  != PCI9030_HS_CAP_ID) &&
             (RegisterVpdCapability != PCI9030_HS_CAP_ID) )
        {
            return FALSE;
        }
    }

    if (CapabilityToVerify & CAPABILITY_VPD)
    {
        if ( (RegisterValue         != PCI9030_VPD_CAP_ID) &&
             (RegisterPmCapability  != PCI9030_VPD_CAP_ID) &&
             (RegisterHsCapability  != PCI9030_VPD_CAP_ID) &&
             (RegisterVpdCapability != PCI9030_VPD_CAP_ID) )
        {
            return FALSE;
        }
    }

    return TRUE;
}




/******************************************************************************
 *
 * Function   :  PlxHiddenRegisterRead
 *
 * Description:  Read a hidden register
 *
 ******************************************************************************/
U32
PlxHiddenRegisterRead(
    DEVICE_EXTENSION *pdx,
    U16               offset
    )
{
    S8  i;
    U32 RegisterPm;
    U32 RegisterSave;
    U32 RegisterDataScale;
    U32 RegisterDataSelect;


    if ((offset != PCI9030_PM_DATA_SELECT) &&
        (offset != PCI9030_PM_DATA_SCALE))
    {
        return (U32)-1;
    }

    // Save the PM register
    PLX_PCI_REG_READ(
        pdx,
        PCI9030_PM_CSR,
        &RegisterSave
        );

    // Turn off PME status so we don't clear it
    RegisterSave &= ~(1 << 15);

    RegisterDataScale  = 0;
    RegisterDataSelect = 0;

    i = 0;
    while (i != -1)
    {
        // Clear Data_Select
        RegisterPm = RegisterSave & ~(0x7 << 9);

        // Setup Data_Select
        RegisterPm = RegisterPm | (i << 9);

        // Write to the PM Control Register
        PLX_PCI_REG_WRITE(
            pdx,
            PCI9030_PM_CSR,
            RegisterPm
            );

        // Get the result
        PLX_PCI_REG_READ(
            pdx,
            PCI9030_PM_CSR,
            &RegisterPm
            );

        // Build the hidden registers & increment to next selection
        switch (i)
        {
            case 0:
                i = 3;
                RegisterDataSelect |= (RegisterPm >> 24) << 0;
                RegisterDataScale  |= ((RegisterPm >> 13) & 0x3) << 0;
                break;

            case 3:
                i = 4;
                RegisterDataSelect |= (RegisterPm >> 24) << 8;
                RegisterDataScale  |= ((RegisterPm >> 13) & 0x3) << 2;
                break;

            case 4:
                i = 7;
                RegisterDataSelect |= (RegisterPm >> 24) << 16;
                RegisterDataScale  |= ((RegisterPm >> 13) & 0x3) << 4;
                break;

            case 7:
                i = -1;
                RegisterDataSelect |= (RegisterPm >> 24) << 24;
                RegisterDataScale  |= ((RegisterPm >> 13) & 0x3) << 6;
                break;
        }
    }

    // Restore the PM register
    PLX_PCI_REG_WRITE(
        pdx,
        PCI9030_PM_CSR,
        RegisterSave
        );

    if (offset == PCI9030_PM_DATA_SELECT)
    {
        return RegisterDataSelect;
    }

    if (offset == PCI9030_PM_DATA_SCALE)
    {
        return RegisterDataScale;
    }

    return (U32)-1;
}
