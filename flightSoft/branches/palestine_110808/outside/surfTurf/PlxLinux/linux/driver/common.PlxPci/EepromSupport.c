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
 *      EepromSupport.c
 *
 * Description:
 *
 *      This file contains EEPROM support functions.
 *
 * Revision History:
 *
 *      09-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "EepromSupport.h"
#include "PciSupport.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  Pci6000_EepromReadByOffset
 *
 * Description:  Read a value from the P-to-P EEPROM at a specified offset
 *
 ******************************************************************************/
void
Pci6000_EepromReadByOffset(
    PLX_DEVICE_NODE *pNode,
    U16              offset,
    U16             *pValue
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    switch (pNode->PlxChipType)
    {
        case 0x6152:
            Offset_EepromCtrl = 0xC8;
            break;

        case 0x6154:
        case 0x6150:
        case 0x6254:
        case 0x6520:
        case 0x6540:
            Offset_EepromCtrl = 0x54;
            break;

        case 0x6140:    // No EEPROM support
        default:
            DebugPrintf((
                "ERROR - Unsupported PLX chip type (%04X)\n",
                pNode->PlxChipType
                ));
            return;
    }

    // For Non-transparent mode chips, enable shadow registers
    if ((pNode->PlxChipType == 0x6254) ||
        (pNode->PlxChipType == 0x6540))
    {
        // Save the chip control register
        PLX_PCI_REG_READ(
            pNode,
            0xD8,
            &RegSave
            );

        // Enable shadow register access
        PLX_PCI_REG_WRITE(
            pNode,
            0xD8,
            RegSave | (1 << 6)
            );
    }

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM read command
    RegValue = ((U32)offset << 8) | 0x41;

    // Write EEPROM command
    PLX_PCI_REG_WRITE(
        pNode,
        Offset_EepromCtrl,
        RegValue
        );

    // Delay for a bit to let EEPROM read complete
    Plx_sleep(50);

    // Read data
    PLX_PCI_REG_READ(
        pNode,
        Offset_EepromCtrl,
        &RegValue
        );

    // Store data
    *pValue = (U16)(RegValue >> 16);

    if ((pNode->PlxChipType == 0x6254) ||
        (pNode->PlxChipType == 0x6540))
    {
        // Restore chip control register
        PLX_PCI_REG_WRITE(
            pNode,
            0xD8,
            RegSave | (1 << 6)
            );
    }
}




/******************************************************************************
 *
 * Function   :  Pci6000_EepromWriteByOffset
 *
 * Description:  Write a value to the EEPROM at a specified offset
 *
 ******************************************************************************/
void
Pci6000_EepromWriteByOffset(
    PLX_DEVICE_NODE *pNode,
    U16              offset,
    U16              value
    )
{
    U16 Offset_EepromCtrl;
    U32 RegSave;
    U32 RegValue;


    switch (pNode->PlxChipType)
    {
        case 0x6152:
            Offset_EepromCtrl = 0xC8;
            break;

        case 0x6154:
        case 0x6150:
        case 0x6254:
        case 0x6520:
        case 0x6540:
            Offset_EepromCtrl = 0x54;
            break;

        case 0x6140:    // No EEPROM support
        default:
            DebugPrintf((
                "ERROR - Unsupported PLX chip type (%04X)\n",
                pNode->PlxChipType
                ));
            return;
    }

    // For Non-transparent mode chips, enable shadow registers
    if ((pNode->PlxChipType == 0x6254) ||
        (pNode->PlxChipType == 0x6540))
    {
        // Save the chip control register
        PLX_PCI_REG_READ(
            pNode,
            0xD8,
            &RegSave
            );

        // Enable shadow register access
        PLX_PCI_REG_WRITE(
            pNode,
            0xD8,
            RegSave | (1 << 6)
            );
    }

    // Offset can only be 8-bits
    offset = offset & 0xFF;

    // Prepare EEPROM write command & data
    RegValue = (value << 16) | ((U32)offset << 8) | 0x43;

    // Write EEPROM command
    PLX_PCI_REG_WRITE(
        pNode,
        Offset_EepromCtrl,
        RegValue
        );

    // Delay for a bit to let EEPROM write complete
    Plx_sleep(50);

    if ((pNode->PlxChipType == 0x6254) ||
        (pNode->PlxChipType == 0x6540))
    {
        // Restore chip control register
        PLX_PCI_REG_WRITE(
            pNode,
            0xD8,
            RegSave | (1 << 6)
            );
    }
}
