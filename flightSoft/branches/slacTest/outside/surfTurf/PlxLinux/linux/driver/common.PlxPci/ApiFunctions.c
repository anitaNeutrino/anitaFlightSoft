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
 *      11-01-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "ApiFunctions.h"
#include "EepromSupport.h"
#include "SupportFunc.h"




/******************************************************************************
 *
 * Function   :  PlxChipTypeGet
 *
 * Description:  Returns PLX chip type and revision
 *
 ******************************************************************************/
RETURN_CODE
PlxChipTypeGet(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U32              *pChipType,
    U8               *pRevision
    )
{
    PLX_DEVICE_NODE *pNode;


    pNode =
        GetDeviceNodeFromKey(
            pdx,
            pKey
            );

    if (pNode == NULL)
    {
        DebugPrintf(("ERROR - Invalid Device node\n"));

        *pChipType = 0x0;
        *pRevision = 0x0;

        return ApiInvalidDeviceInfo;
    }
    else
    {
        *pChipType = pNode->PlxChipType;
        *pRevision = pNode->Revision;
    }

    return ApiSuccess;
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
    PLX_DEVICE_KEY   *pKey,
    BOOLEAN          *pFlag
    )
{
    U16          Value_Test;
    U16          Value_Original;
    RETURN_CODE  rc;


    /**************************************************************
     * Since the PCI 6000 series do not report whether an
     * EEPROM is present or not, the driver checks for one
     * as follows:
     *
     *  - Read an EEPROM location outside PLX chip reserved area
     *  - Write the inverse of the value to the location
     *  - Read back and compare
     *  - If value was modified, an EEPROM is present
     *  - Restore original value
     *************************************************************/

    // Get original value from unused location
    rc =
        PlxEepromReadByOffset(
            pdx,
            pKey,
            EEPROM_TEST_OFFSET,
            &Value_Original
            );

    if (rc != ApiSuccess)
    {
        return rc;
    }

    // Write inverse of original value
    rc =
        PlxEepromWriteByOffset(
            pdx,
            pKey,
            EEPROM_TEST_OFFSET,
            (U16)~Value_Original
            );

    if (rc != ApiSuccess)
    {
        return rc;
    }

    // Read updated value
    rc =
        PlxEepromReadByOffset(
            pdx,
            pKey,
            EEPROM_TEST_OFFSET,
            &Value_Test
            );

    if (rc != ApiSuccess)
    {
        return rc;
    }

    // Check if value was written properly
    if (Value_Test == (U16)~Value_Original)
    {
        *pFlag = TRUE;
    }
    else
    {
        *pFlag = FALSE;
    }

    // Restore original value
    PlxEepromWriteByOffset(
        pdx,
        pKey,
        EEPROM_TEST_OFFSET,
        Value_Original
        );

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
    PLX_DEVICE_KEY   *pKey,
    U16               offset,
    U16              *pValue
    )
{
    PLX_DEVICE_NODE *pNode;


    pNode =
        GetDeviceNodeFromKey(
            pdx,
            pKey
            );

    if (pNode == NULL)
    {
        DebugPrintf(("ERROR - Invalid Device node\n"));
        *pValue = -1;
        return ApiInvalidDeviceInfo;
    }

    // Make sure offset is aligned on 16-bit boundary
    if (offset & (1 << 0))
    {
        return ApiInvalidOffset;
    }

    switch (pNode->PlxChipType)
    {
        case 0x6152:
        case 0x6154:
        case 0x6150:
        case 0x6254:
        case 0x6520:
        case 0x6540:
            Pci6000_EepromReadByOffset(
                pNode,
                offset,
                pValue
                );
            break;

        case 0x6140:  // 6140 does not support an EEPROM
        case 0x9050:
        case 0x9080:
        case 0x9030:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        default:
	        return ApiUnsupportedFunction;
    }

    return ApiSuccess;
}




/******************************************************************************
 *
 * Function   :  PlxEepromWriteByOffset
 *
 * Description:  Write a value to the EEPROM at a specified offset
 *
 ******************************************************************************/
RETURN_CODE
PlxEepromWriteByOffset(
    DEVICE_EXTENSION *pdx,
    PLX_DEVICE_KEY   *pKey,
    U16               offset,
    U16               value
    )
{
    PLX_DEVICE_NODE *pNode;


    pNode =
        GetDeviceNodeFromKey(
            pdx,
            pKey
            );

    if (pNode == NULL)
    {
        DebugPrintf(("ERROR - Invalid Device node\n"));
        return ApiInvalidDeviceInfo;
    }

    switch (pNode->PlxChipType)
    {
        case 0x6152:
        case 0x6154:
        case 0x6150:
        case 0x6254:
        case 0x6520:
        case 0x6540:
            Pci6000_EepromWriteByOffset(
                pNode,
                offset,
                value
                );
            break;

        case 0x6140:  // 6140 does not support an EEPROM
        case 0x9050:
        case 0x9080:
        case 0x9030:
        case 0x9054:
        case 0x9056:
        case 0x9656:
        default:
	        return ApiUnsupportedFunction;
    }

    return ApiSuccess;
}
