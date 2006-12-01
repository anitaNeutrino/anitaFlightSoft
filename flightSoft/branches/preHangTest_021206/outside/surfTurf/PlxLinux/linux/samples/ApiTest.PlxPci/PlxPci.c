/******************************************************************************
 *
 * File Name:
 *
 *      PlxPci.c
 *
 * Description:
 *
 *
 *
 * Revision History:
 *
 *      07-11-03 : PCI SDK v4.20
 *
 ******************************************************************************/


#include "PciDrvApi.h"

#if defined(_WIN32)
    #include "ConsFunc.h"
    #include "PlxInit.h"
//    #include "..\\Common\\ConsFunc.h"
//    #include "..\\Common\\PlxInit.h"
#endif

#if defined(PLX_LINUX)
    #include "ConsFunc.h"
    #include "PlxInit.h"
#endif


// >>>>> Drv_ApiTest.h
#define PLX_API_VERSION_STRING       "4.20.001"


void
PlxSdkErrorDisplay(
    RETURN_CODE code
    );

void
TestEeprom(
    PLX_DEVICE_OBJECT *pDevice
    );
// Drv_ApiTest.h <<<<<



/**********************************************
*               Definitions
**********************************************/




/**********************************************
*               Functions
**********************************************/
void
TestChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice
    );




/********************************************************
*               Global Variables
********************************************************/
U32 ChipTypeSelected;
U8  ChipRevision;




/******************************************************************************
 *
 * Function   :  main
 *
 * Description:  The main entry point
 *
 *****************************************************************************/
int 
main(
    void
    )
{
    S8                DeviceSelected;
    RETURN_CODE       rc;
    PLX_DEVICE_KEY    DeviceKey;
    PLX_DEVICE_OBJECT Device;


    ConsoleInitialize();

    Cls();

    PlxPrintf("\n\n");
    PlxPrintf("\t\t   PLX PCI Service Sample Application\n");
    PlxPrintf("\t\t            July 11, 2003\n\n");


    /************************************
    *         Select Device
    ************************************/
    DeviceSelected =
        SelectDevice(
            &DeviceKey
            );

    if (DeviceSelected == -1)
    {
        ConsoleEnd();
        exit(0);
    }

    rc =
        PlxPci_DeviceOpen(
            &DeviceKey,
            &Device
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "\n"
            "    ERROR: Unable to find or select a PLX device\n"
            );

        PlxSdkErrorDisplay(rc);

        _Pause;

        ConsoleEnd();

        exit(-1);
    }

    Cls();

    // Display API Test version information
    PlxPrintf(
        "\n"
        "                       PLX API Test\n"
        "                        v%s\n"
        "    =================================================\n",
        PLX_API_VERSION_STRING
        );

    PlxPrintf(
        "\nSelected: %.4x %.4x [bus %02x  slot %02x  fn %02x]\n\n",
        DeviceKey.DeviceId, DeviceKey.VendorId,
        DeviceKey.bus, DeviceKey.slot, DeviceKey.function
        );

    // Get PLX Chip Type
    PlxPci_ChipTypeGet(
        &Device,
        &ChipTypeSelected,
        &ChipRevision
        );

    // Only PCI 6000 series supported now
    if ((ChipTypeSelected >> 12) != 6)
        ChipTypeSelected = 0;

    // Start Tests
    if (1)
    {
        /********************************************************
        *
        ********************************************************/
        TestChipTypeGet(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestEeprom(&Device);
        _PauseWithExit;
    }


    /************************************
    *        Close the Device
    ************************************/
    PlxPci_DeviceClose(
        &Device
        );

    PlxPrintf("\n\n");

    _Pause;

    ConsoleEnd();

    exit(0);
}




/******************************************************************************
 *
 * Function   :  TestChipTypeGet
 *
 * Description:  
 *
 *****************************************************************************/
void
TestChipTypeGet(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U8          Revision;
    U32         ChipType;
    RETURN_CODE rc;


    PlxPrintf("\nPlxPci_ChipTypeGet():\n");

    PlxPrintf("  Getting PLX Chip Type.......... ");
    rc =
        PlxPci_ChipTypeGet(
            pDevice,
            &ChipType,
            &Revision
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf("Ok\n");
    }


    PlxPrintf(
        "    Chip type:  %04x",
        ChipType
        );

    if (ChipType == 0)
    {
        PlxPrintf(" (Non-PLX chip)");
    }
    PlxPrintf("\n");

    PlxPrintf(
        "    Revision :    %02X\n",
        Revision
        );
}




/********************************************************
*
********************************************************/
void
TestEeprom(
    PLX_DEVICE_OBJECT *pDevice
    )
{
    U16         ReadSave;
    U16         ReadValue;
    U16         WriteValue;
    BOOLEAN     bEepromPresent;
    RETURN_CODE rc;


    PlxPrintf("\nPlxPci_EepromXxx():\n");

    PlxPrintf("  Checking if EEPROM present..... ");
    bEepromPresent =
        PlxPci_EepromPresent(
            pDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) && (ChipTypeSelected == 0))
        {
            PlxPrintf("Ok (Non-PLX rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            PlxPrintf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
            return;
        }
    }
    else
    {
        if (bEepromPresent)
            PlxPrintf("Ok (Valid EEPROM present)\n");
        else
            PlxPrintf("Ok (EEPROM not present or is Blank)\n");
    }


    // Read from EEPROM by offset
    PlxPrintf("  Read EEPROM by offset.......... ");

    rc =
        PlxPci_EepromReadByOffset(
            pDevice,
            0,
            &ReadSave
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) && (ChipTypeSelected == 0))
        {
            PlxPrintf("Ok (Non-PLX rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            PlxPrintf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
            return;
        }
    }
    else
    {
        PlxPrintf(
            "Ok  - (value = 0x%04x)\n",
            ReadSave
            );
    }


    // Write to EEPROM by offset
    PlxPrintf("  Write EEPROM by offset......... ");

    WriteValue = 0x06A5;
    rc =
        PlxPci_EepromWriteByOffset(
            pDevice,
            0,
            WriteValue
            );

    if (rc != ApiSuccess)
    {
        if ((rc == ApiUnsupportedFunction) && (ChipTypeSelected == 0))
        {
            PlxPrintf("Ok (Non-PLX rc=ApiUnsupportedFunction)\n");
        }
        else
        {
            PlxPrintf("*ERROR* - API call failed\n");
            PlxSdkErrorDisplay(rc);
            return;
        }
    }
    else
    {
        PlxPrintf(
            "Ok  - (value = 0x%04x)\n",
            WriteValue
            );
    }


    // No need to continue for non-PLX devices
    if (ChipTypeSelected == 0)
    {
        return;
    }


    // Verify Write
    PlxPrintf("  Verify write by offset......... ");

    rc =
        PlxPci_EepromReadByOffset(
            pDevice,
            0,
            &ReadValue
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        if (ReadValue == WriteValue)
        {
            PlxPrintf(
                "Ok  - (value = 0x%04x)\n",
                ReadValue
                );
        }
        else
        {
            PlxPrintf(
                "*ERROR* - Rd (0x%04x) != Wr (0x%04x)\n",
                ReadValue, WriteValue
                );
        }
    }


    // Restore Original Value
    PlxPrintf("  Restore EEPROM by offset....... ");

    rc =
        PlxPci_EepromWriteByOffset(
            pDevice,
            0,
            ReadSave
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        PlxPrintf("Ok\n");
}




/*********************************************************************
 *
 * Function   :  PlxSdkErrorDisplay
 *
 * Description:  Displays an error code and halts if requested
 *
 ********************************************************************/
void
PlxSdkErrorDisplay(
    RETURN_CODE code
    )
{
    PlxPrintf(
        "\tAPI Error: %s (%03Xh)\n",
        PlxSdkErrorText(code),
        code
        );
}
