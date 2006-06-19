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
 *      05-01-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "EepromSupport.h"
#include "SupportFunc.h"




/**********************************************
*               Definitions
**********************************************/
// EEPROM Control register offset
#if !defined(REG_EEPROM_CTRL)
    #error ERROR: 'REG_EEPROM_CTRL' not defined
#endif




/******************************************************************************
 *
 * Function   :  EepromReadByOffset
 *
 * Description:  Read a value from the EEPROM at a specified offset
 *
 ******************************************************************************/
void
EepromReadByOffset(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    U16               offset,
    U32              *pValue
    )
{
    S8   BitPos;
    S8   CommandShift;
    S8   CommandLength;
    U16  count;
    U32  RegisterValue;


    switch (EepromType)
    {
        case Eeprom93CS46:
            CommandShift  = 0;
            CommandLength = EE46_CMD_LEN;
            break;

        case Eeprom93CS56:
            CommandShift  = 2;
            CommandLength = EE56_CMD_LEN;
            break;

        case Eeprom93CS66:
            CommandShift  = 2;
            CommandLength = EE66_CMD_LEN;
            break;

        default:
            *pValue = (U32)-1;
            return;
    }

    // Send EEPROM read command and offset to EEPROM
    EepromSendCommand(
        pdx,
        (EE_READ << CommandShift) | (offset / 2),
        CommandLength
        );

    /*****************************************************
     * Note: The EEPROM write ouput bit (26) is set here
     *       because it is required before EEPROM read
     *       operations on the 9054.  It does not affect
     *       behavior of non-9054 chips.
     *
     *       The EEDO Input enable bit (31) is required for
     *       some chips.  Since it is a reserved bit in older
     *       chips, there is no harm in setting it for all.
     ****************************************************/

    // Set EEPROM write output bit
    RegisterValue =
        PLX_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Set EEDO Input enable for some PLX chips
    RegisterValue |= (1 << 31);

    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue | (1 << 26)
        );

    // Get 32-bit value from EEPROM - one bit at a time
    for (BitPos = 0; BitPos < 32; BitPos++)
    {
        // Trigger the EEPROM clock
        EepromClock(
            pdx
            );

        /*****************************************************
         * Note: After the EEPROM clock, a delay is sometimes
         *       needed to let the data bit propagate from the
         *       EEPROM to the PLX chip.  If a sleep mechanism
         *       is used, the result is an extremely slow EEPROM
         *       access since the delay resolution is large and
         *       is required for every data bit read.
         *
         *       Rather than using the standard sleep mechanism,
         *       the code, instead, reads the PLX register
         *       multiple times.  This is harmless and provides
         *       enough delay for the EEPROM data to propagate.
         ****************************************************/

        for (count=0; count < 20; count++)
        {
            // Get the result bit
            RegisterValue =
                PLX_REG_READ(
                    pdx,
                    REG_EEPROM_CTRL
                    );
        }

        // Get bit value and shift into result
        if (RegisterValue & (1 << 27))
        {
            *pValue = (*pValue << 1) | 1;
        }
        else
        {
            *pValue = (*pValue << 1);
        }
    }

    // Clear EEDO Input enable for some PLX chips
    RegisterValue &= ~(1 << 31);

    // Clear Chip Select and all other EEPROM bits
    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue & ~(0xF << 24)
        );
}




/******************************************************************************
 *
 * Function   :  EepromWriteByOffset
 *
 * Description:  Write a value to the EEPROM at a specified offset
 *
 ******************************************************************************/
void
EepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    EEPROM_TYPE       EepromType,
    U16               offset,
    U32               value
    )
{
    S8   i;
    S8   BitPos;
    S8   CommandShift;
    S8   CommandLength;
    U16  EepromValue;
    S32  Timeout;
    U32  RegisterValue;


    switch (EepromType)
    {
        case Eeprom93CS46:
            CommandShift  = 0;
            CommandLength = EE46_CMD_LEN;
            break;

        case Eeprom93CS56:
            CommandShift  = 2;
            CommandLength = EE56_CMD_LEN;
            break;

        case Eeprom93CS66:
            CommandShift  = 2;
            CommandLength = EE66_CMD_LEN;
            break;

        default:
            return;
    }

    // Write EEPROM 16-bits at a time
    for (i=0; i<2; i++)
    {
        // Set 16-bit value to write
        if (i == 0)
        {
            EepromValue = (U16)(value >> 16);
        }
        else
        {
            EepromValue = (U16)value;

            // Update offset
            offset = offset + sizeof(U16);
        }

        // Send Write_Enable command to EEPROM
        EepromSendCommand(
            pdx,
            (EE_WREN << CommandShift),
            CommandLength
            );

        // Send EEPROM Write command and offset to EEPROM
        EepromSendCommand(
            pdx,
            (EE_WRITE << CommandShift) | (offset / 2),
            CommandLength
            );

        RegisterValue =
            PLX_REG_READ(
                pdx,
                REG_EEPROM_CTRL
                );

        // Clear all EEPROM bits
        RegisterValue &= ~(0xF << 24);

        // Make sure EEDO Input is disabled for some PLX chips
        RegisterValue &= ~(1 << 31);

        // Enable EEPROM Chip Select
        RegisterValue |= (1 << 25);

        // Write 16-bit value to EEPROM - one bit at a time
        for (BitPos = 15; BitPos >= 0; BitPos--)
        {
            // Get bit value and shift into result
            if (EepromValue & (1 << BitPos))
            {
                PLX_REG_WRITE(
                    pdx,
                    REG_EEPROM_CTRL,
                    RegisterValue | (1 << 26)
                    );
            }
            else
            {
                PLX_REG_WRITE(
                    pdx,
                    REG_EEPROM_CTRL,
                    RegisterValue
                    );
            }

            // Trigger the EEPROM clock
            EepromClock(
                pdx
                );
        }

        // Deselect Chip
        PLX_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegisterValue & ~(1 << 25)
            );

        // Re-select Chip
        PLX_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegisterValue | (1 << 25)
            );

        /*****************************************************
         * Note: After the clocking in the last data bit, a
         *       delay is needed to let the EEPROM internally
         *       complete the write operation.  If a sleep
         *       mechanism is used, the result is an extremely
         *       slow EEPROM access since the delay resolution
         *       is too large.
         *
         *       Rather than using the standard sleep mechanism,
         *       the code, instead, reads the PLX register
         *       multiple times.  This is harmless and provides
         *       enough delay for the EEPROM write to complete.
         ****************************************************/

        // A small delay is needed to let EEPROM complete
        Timeout = 0;
        do
        {
            RegisterValue =
                PLX_REG_READ(
                    pdx,
                    REG_EEPROM_CTRL
                    );

            Timeout++;
        }
        while (((RegisterValue & (1 << 27)) == 0) && (Timeout < 20000));

        // Send Write_Disable command to EEPROM
        EepromSendCommand(
            pdx,
            EE_WDS << CommandShift,
            CommandLength
            );

        // Clear Chip Select and all other EEPROM bits
        PLX_REG_WRITE(
            pdx,
            REG_EEPROM_CTRL,
            RegisterValue & ~(0xF << 24)
            );
    }
}




/******************************************************************************
 *
 * Function   :  EepromSendCommand
 *
 * Description:  Sends a Command to the EEPROM
 *
 ******************************************************************************/
void
EepromSendCommand(
    DEVICE_EXTENSION *pdx,
    U32               EepromCommand,
    U8                DataLengthInBits
    )
{
    S8  BitPos;
    U32 RegisterValue;


    RegisterValue =
        PLX_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Clear all EEPROM bits
    RegisterValue &= ~(0xF << 24);

    // Toggle EEPROM's Chip select to get it out of Shift Register Mode
    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue
        );

    // Enable EEPROM Chip Select
    RegisterValue |= (1 << 25);

    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue
        );

    // Send EEPROM command - one bit at a time
    for (BitPos = (S8)(DataLengthInBits-1); BitPos >= 0; BitPos--)
    {
        // Check if current bit is 0 or 1
        if (EepromCommand & (1 << BitPos))
        {
            PLX_REG_WRITE(
                pdx,
                REG_EEPROM_CTRL,
                RegisterValue | (1 << 26)
                );
        }
        else
        {
            PLX_REG_WRITE(
                pdx,
                REG_EEPROM_CTRL,
                RegisterValue
                );
        }

        EepromClock(
            pdx
            );
    }
}




/******************************************************************************
 *
 * Function   :  EepromClock
 *
 * Description:  Sends the clocking sequence to the EEPROM
 *
 ******************************************************************************/
void
EepromClock(
    DEVICE_EXTENSION *pdx
    )
{
    S8  i;
    U32 RegisterValue;


    RegisterValue =
        PLX_REG_READ(
            pdx,
            REG_EEPROM_CTRL
            );

    // Set EEPROM clock High
    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue | (1 << 24)
        );

    // Need a small delay, perform dummy register reads
    for (i=0; i<20; i++)
    {
        RegisterValue =
            PLX_REG_READ(
                pdx,
                REG_EEPROM_CTRL
                );
    }

    // Set EEPROM clock Low
    PLX_REG_WRITE(
        pdx,
        REG_EEPROM_CTRL,
        RegisterValue & ~(1 << 24)
        );
}
