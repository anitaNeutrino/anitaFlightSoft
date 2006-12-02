/******************************************************************************
 *
 * File Name:
 *
 *      ApiTest.c
 *
 * Description:
 *
 *      Test API functionality
 *
 ******************************************************************************/


#include <memory.h>
#include <stdlib.h>
#include "ApiTest.h"
#include "ConsFunc.h"
#include "PciRegs.h"
#include "PlxApi.h"
#include "PlxInit.h"
#include "RegDefs.h"
#include "Test9050.h"
#include "Test9030.h"
#include "Test9080.h"
#include "Test9054.h"
#include "Test9656.h"




/********************************************************
*               Global Variables
********************************************************/
BOOLEAN mgEventCompleted;
U32     ChipTypeSelected;
U8      ChipRevision;




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
    U8               count;
    S8               DeviceSelected;
    HANDLE           PlxHandle;
    DEVICE_LOCATION  Device;


    ConsoleInitialize();

    Cls();

    /********************************************************
    *         Select Device
    ********************************************************/
    DeviceSelected =
        SelectDevice(
            &Device
            );

    if (DeviceSelected == -1)
    {
        ConsoleEnd();
        exit(0);
    }

    if (PlxPciDeviceOpen(
            &Device,
            &PlxHandle
            ) != ApiSuccess)
    {
        PlxPrintf(
            "\n"
            "             Error: ** No PLX Devices found **\n"
            "\n"
            "                  Press any key to exit..."
            );

        // Wait for keypress
        Plx_getch();

        PlxPrintf("\n");

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
        "\nSelected: %.4x %.4x [bus %.2x  slot %.2x]  (Handle=%04d)\n\n",
        Device.DeviceId, Device.VendorId,
        Device.BusNumber, Device.SlotNumber, (U32)PlxHandle
        );

    // Get PLX Chip Type
    PlxChipTypeGet(
        PlxHandle,
        &ChipTypeSelected,
        &ChipRevision
        );

    if (ChipTypeSelected == 0x0)
    {
        PlxPrintf(
            "\n   ERROR: Unable to determine PLX chip type\n"
            );

        PlxPciDeviceClose(
            PlxHandle
            );

        _Pause;

        ConsoleEnd();

        exit(-1);
    }


    // Start Tests
    if (1)
    {
        /********************************************************
        *
        ********************************************************/
        TestDriverVersion(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestChipTypeGet(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        DisplayVirtualAddresses(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        DisplayCommonBuffer(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPhysicalMemAllocate(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        DisplayBarRanges(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        DisplayRegisters(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestConfigRegAccess(&Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestRegAccess(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestRegisterDoorbell(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestRegisterMailbox(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestSerialEeprom(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestDirectSlave(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestIoAccess(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPowerManRegisters(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestHotSwapRegisters(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestVpdRegisters(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestInterruptEnable(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestInterruptAttach(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestBlockDma(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestSglDma(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestDmaControlStatus(PlxHandle);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPowerLevel(PlxHandle, &Device);
        _PauseWithExit;

        /********************************************************
        *
        ********************************************************/
        TestPciBoardReset(PlxHandle);
        _PauseWithExit;
    }

    PlxPrintf("\n");

    count = 5;
    while (count-- != 0)
    {
        PlxPrintf(
            "Close device in %x seconds...\r",
            count
            );

        Plx_Sleep_ms(1000);
    }

    PlxPrintf("Closing device...");

    // Close device
    PlxPciDeviceClose(
        PlxHandle
        );

    PlxPrintf("Ok           \n\n");

    ConsoleEnd();

    exit(0);
}




/********************************************************
*
********************************************************/
void
TestChipTypeGet(
    HANDLE hDevice
    )
{
    U8          Revision;
    U32         ChipType;
    RETURN_CODE rc;


    PlxPrintf("\nPlxChipTypeGet():\n");

    PlxPrintf("  Getting PLX Chip Type.......... ");
    rc =
        PlxChipTypeGet(
            hDevice,
            &ChipType,
            &Revision
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        PlxPrintf("Ok\n");

    PlxPrintf(
        "    Chip type:  %04x\n"
        "    Revision :    %02X\n",
        ChipType, Revision
        );
}




/********************************************************
*
********************************************************/
void
TestDriverVersion(
    HANDLE hDevice
    )
{
    U8          ApiVersionMajor;
    U8          ApiVersionMinor;
    U8          ApiVersionRevision;
    U8          DrvVersionMajor;
    U8          DrvVersionMinor;
    U8          DrvVersionRevision;
    RETURN_CODE rc;


    PlxPrintf("\nPlxXxxVersion():\n");

    PlxPrintf("  Getting API Version............ ");
    rc =
        PlxSdkVersion(
            &ApiVersionMajor,
            &ApiVersionMinor,
            &ApiVersionRevision
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf(
            "Ok  (Ver = %d.%d%d)\n",
            ApiVersionMajor, ApiVersionMinor, ApiVersionRevision
            );
    }


    PlxPrintf("  Getting Driver Version......... ");
    rc =
        PlxDriverVersion(
            hDevice,
            &DrvVersionMajor,
            &DrvVersionMinor,
            &DrvVersionRevision
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf(
            "Ok  (Ver = %d.%d%d)\n",
            DrvVersionMajor, DrvVersionMinor, DrvVersionRevision
            );
    }

    if ((ApiVersionMajor    != DrvVersionMajor) ||
        (ApiVersionMinor    != DrvVersionMinor) ||
        (ApiVersionRevision != DrvVersionRevision))
    {
        PlxPrintf("  WARNING: API and Driver versions should match\n");
    }
}




/********************************************************
*
********************************************************/
void
TestPciBoardReset(
    HANDLE hDevice
    )
{
    PlxPrintf("\nPlxPciBoardReset():\n");

    PlxPrintf("  Resetting PCI board............ ");

    PlxPciBoardReset(
        hDevice
        );

    PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
DisplayVirtualAddresses(
    HANDLE hDevice
    )
{
    U8          i;
    U32         Va[PCI_NUM_BARS];
    RETURN_CODE rc;


    PlxPrintf("\nPlxPciBarMap()...\n");

    for (i=0; i<PCI_NUM_BARS; i++)
    {
        if (i == 6)
            PlxPrintf("  Mapping Exp ROM Space.......... ");
        else
            PlxPrintf("  Mapping PCI BAR %d.............. ", i);

        rc =
            PlxPciBarMap(
                hDevice,
                i,
                &(Va[i])
                );

        PlxPrintf("Ok (rc = %s)\n", PlxSdkErrorText(rc));
    }

    PlxPrintf(
        "\n"
        "      BAR0:  0x%08x\n"
        "      BAR1:  0x%08x\n"
        "      BAR2:  0x%08x\n"
        "      BAR3:  0x%08x\n"
        "      BAR4:  0x%08x\n"
        "      BAR5:  0x%08x\n"
        "      EROM:  0x%08x\n",
        Va[0], Va[1], Va[2],
        Va[3], Va[4], Va[5], Va[6]
        );

    PlxPrintf("\n");

    for (i=0; i<PCI_NUM_BARS; i++)
    {
        if (i == 6)
            PlxPrintf("  Unmapping Exp ROM Space........ ");
        else
            PlxPrintf("  Unmapping PCI BAR %d............ ", i);

        rc =
            PlxPciBarUnmap(
                hDevice,
                &(Va[i])
                );

        PlxPrintf("Ok (rc = %s)\n", PlxSdkErrorText(rc));
    }
}




/********************************************************
*
********************************************************/
void
DisplayCommonBuffer(
    HANDLE hDevice
    )
{
    PCI_MEMORY  CommonBuffer;
    RETURN_CODE rc;


    PlxPrintf("\nPlxPciCommonBufferXxx():\n");


    PlxPrintf("  Get Common buffer properties....... ");
    rc =
        PlxPciCommonBufferProperties(
            hDevice,
            &CommonBuffer
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - rc=%s\n", PlxSdkErrorText(rc));
    }
    else
    {
        PlxPrintf("Ok\n");
    }


    PlxPrintf("  Map Common buffer to user space.... ");
    rc =
        PlxPciCommonBufferMap(
            hDevice,
            &(CommonBuffer.UserAddr)
            );

    if (rc != ApiSuccess)
    {
        // Handle case where buffer not allocated
        if (CommonBuffer.Size == 0)
        {
            PlxPrintf("Ok (rc=%s)\n", PlxSdkErrorText(rc));
        }
        else
        {
            PlxPrintf("*ERROR* - rc=%s\n", PlxSdkErrorText(rc));
        }
    }
    else
    {
        PlxPrintf("Ok\n");
    }

    PlxPrintf(
        "      Physical Addr: 0x%08x\n"
        "      Virtual Addr : 0x%08x\n"
        "      Size         : %d Kb\n",
        CommonBuffer.PhysicalAddr,
        CommonBuffer.UserAddr,
        (CommonBuffer.Size >> 10)
        );

    PlxPrintf("\n");
    PlxPrintf("  Unmap Common buffer................ ");
    rc =
        PlxPciCommonBufferUnmap(
            hDevice,
            &(CommonBuffer.UserAddr)
            );

    if (rc != ApiSuccess)
    {
        // Handle case where buffer not allocated
        if (CommonBuffer.Size == 0)
        {
            PlxPrintf("Ok (rc=%s)\n", PlxSdkErrorText(rc));
        }
        else
        {
            PlxPrintf("*ERROR* - rc=%s\n", PlxSdkErrorText(rc));
        }
    }
    else
    {
        PlxPrintf("Ok\n");
    }
}




/********************************************************
*
********************************************************/
void
TestPhysicalMemAllocate(
    HANDLE hDevice
    )
{
    U32         RequestSize_1;
    U32         RequestSize_2;
    PCI_MEMORY  CommonBuffer_1;
    PCI_MEMORY  CommonBuffer_2;
    RETURN_CODE rc;


    PlxPrintf("\nPlxPciPhysicalMemoryXxx():\n");

    RequestSize_1 = 0x500000;
    RequestSize_2 = 0x1000000;


    PlxPrintf("  Allocate buffer 1.... ");
    CommonBuffer_1.Size = RequestSize_1;
    rc =
        PlxPciPhysicalMemoryAllocate(
            hDevice,
            &CommonBuffer_1,
            TRUE             // Smaller buffer ok
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Unable to allocate DMA buffer\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");

    PlxPrintf("  Map buffer 1......... ");
    rc =
        PlxPciPhysicalMemoryMap(
            hDevice,
            &CommonBuffer_1
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - (rc=%s)\n", PlxSdkErrorText(rc));
    }
    else
    {
        PlxPrintf("Ok\n");
    }

    PlxPrintf(
        "      Physical Addr: 0x%08x\n"
        "      Virtual Addr : 0x%08x\n"
        "      Size         : %d Kb",
        CommonBuffer_1.PhysicalAddr,
        CommonBuffer_1.UserAddr,
        (CommonBuffer_1.Size >> 10)
        );

    if (RequestSize_1 != CommonBuffer_1.Size)
    {
        PlxPrintf(
            " (req=%d Kb)\n\n",
            (RequestSize_1 >> 10)
            );
    }
    else
    {
        PlxPrintf("\n\n");
    }


    PlxPrintf("  Allocate buffer 2.... ");
    CommonBuffer_2.Size = RequestSize_2;
    rc =
        PlxPciPhysicalMemoryAllocate(
            hDevice,
            &CommonBuffer_2,
            FALSE           // Smaller buffer not ok
            );

    if (rc == ApiSuccess ||
        rc == ApiInsufficientResources)
    {
        if (rc == ApiInsufficientResources)
            PlxPrintf("Ok (rc=ApiInsufficientResources)\n");
        else
            PlxPrintf("Ok\n");


        PlxPrintf("  Map buffer 2......... ");
        rc =
            PlxPciPhysicalMemoryMap(
                hDevice,
                &CommonBuffer_2
                );

        PlxPrintf("Ok (rc=%s)\n", PlxSdkErrorText(rc));


        PlxPrintf(
            "      Physical Addr: 0x%08x\n"
            "      Virtual Addr : 0x%08x\n"
            "      Size         : %d Kb",
            CommonBuffer_2.PhysicalAddr,
            CommonBuffer_2.UserAddr,
            (CommonBuffer_2.Size >> 10)
            );

        if (RequestSize_2 != CommonBuffer_2.Size)
        {
            PlxPrintf(
                " (req=%d Kb)\n\n",
                (RequestSize_2 >> 10)
                );
        }
        else
        {
            PlxPrintf("\n\n");
        }
    }
    else
    {
        PlxPrintf("*ERROR* - Unable to allocate DMA buffer\n");
        PlxSdkErrorDisplay(rc);
    }

    PlxPrintf("\n");
    PlxPrintf("  Unmap buffer 1....... ");
    rc =
        PlxPciPhysicalMemoryUnmap(
            hDevice,
            &CommonBuffer_1
            );

    PlxPrintf("Ok (rc=%s)\n", PlxSdkErrorText(rc));

    PlxPrintf("  Free buffer 1........ ");
    rc =
        PlxPciPhysicalMemoryFree(
            hDevice,
            &CommonBuffer_1
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Unable to free DMA buffer\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf("Ok\n");
    }

    PlxPrintf("  Unmap buffer 2....... ");
    rc =
        PlxPciPhysicalMemoryUnmap(
            hDevice,
            &CommonBuffer_2
            );

    PlxPrintf("Ok (rc=%s)\n", PlxSdkErrorText(rc));

    PlxPrintf("  Free buffer 2........ ");
    rc =
        PlxPciPhysicalMemoryFree(
            hDevice,
            &CommonBuffer_2
            );

    if (rc != ApiSuccess)
    {
        if (CommonBuffer_2.PhysicalAddr == (U32)NULL)
        {
            PlxPrintf(
                "Ok (rc=%s)\n",
                PlxSdkErrorText(rc)
                );
        }
        else
        {
            PlxPrintf("*ERROR* - Unable to free DMA buffer\n");
            PlxSdkErrorDisplay(rc);
        }
    }
    else
    {
        PlxPrintf("Ok\n");
    }
}




/********************************************************
*
********************************************************/
void
DisplayRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U16           RegSetSize;
    REGISTER_SET *RegPci;
    REGISTER_SET *RegLocal;


    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9050:
            RegPci     = Pci9050;
            RegLocal   = Local9050;
            RegSetSize = 0x54;
            break;

        case 0x9030:
            RegPci     = Pci9030;
            RegLocal   = Local9030;
            RegSetSize = 0x58;
            break;

        case 0x9080:
            RegPci     = Pci9080;
            RegLocal   = Local9080;
            RegSetSize = 0xec;
            break;

        case 0x9054:
            RegPci     = Pci9054;
            RegLocal   = Local9054;
            RegSetSize = 0xec;
            break;

        case 0x9656:
        case 0x9056:
            RegPci     = Pci9656;
            RegLocal   = Local9656;
            RegSetSize = 0xf4;
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    /********************************************************
    *
    ********************************************************/
    PlxPrintf("\nPCI Registers:\n");
    DisplayPciRegisterSet(
        pDev,
        RegPci
        );
    _Pause;

    /********************************************************
    *
    ********************************************************/
    PlxPrintf("\nLocal Registers:\n");
    DisplayLocalRegisterSet(
        hDevice,
        RegLocal
        );
    _Pause;
}




/********************************************************
*
********************************************************/
void
DisplayBarRanges(
    HANDLE hDevice
    )
{
    U8          BarRegNum;
    U32         BarRegValue;
    RETURN_CODE rc;


    PlxPrintf("\nBAR Ranges:\n");

    for (BarRegNum = 0; BarRegNum < 6; BarRegNum++)
    {
        PlxPrintf("  BAR %d: ", BarRegNum);

        rc =
            PlxPciBarRangeGet(
                hDevice,
                BarRegNum,
                &BarRegValue
                );

        if (rc != ApiSuccess)
        {
            PlxPrintf("*ERROR* - ...BarRangeGet() failed\n");
            PlxSdkErrorDisplay(rc);
        }
        else
        {
            PlxPrintf("0x%08x  ", BarRegValue);
            if (BarRegValue >= (1 << 10))
                PlxPrintf("(%5d Kb)\n", (BarRegValue >> 10));
            else
                PlxPrintf("(%5d Bytes)\n", BarRegValue);
        }
    }
}




/********************************************************
*
********************************************************/
void
TestConfigRegAccess(
    DEVICE_LOCATION *pDev
    )
{
    U16          RegOffset;
    U32          RegValue_1;
    U32          RegValue_2;
    U32          RegValue_3;
    RETURN_CODE  rc;


    PlxPrintf("\nPlxPciConfigRegisterXxx():\n");

    // Use Cache Line Size PCI register
    RegOffset = 0xc;

    PlxPrintf("  Reading PCI Register (%02Xh)..... ", RegOffset);
    RegValue_1 =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - ConfigRead() failed, code = %d\n",
            rc
            );
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);


    // Prepare write value
    RegValue_2 = (RegValue_1 & ~0xFF) | 0xA5;

    PlxPrintf("  Writing PCI Register........... ");
    rc =
        PlxPciConfigRegisterWrite(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &RegValue_2
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_2);


    PlxPrintf("  Verifying write operation...... ");
    RegValue_3 =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigRead() failed, code = %d\n", rc);
        return;
    }

    if (RegValue_3 == RegValue_2)
        PlxPrintf("Ok (0x%08x)\n", RegValue_3);
    else
        PlxPrintf("*ERROR*  -  wrote: %08x   read: %08x\n",
               RegValue_2, RegValue_3);


    PlxPrintf("  Restoring PCI Register......... ");
    rc =
        PlxPciConfigRegisterWrite(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &RegValue_1
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigWrite failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);
}




/********************************************************
*
********************************************************/
void
TestRegAccess(
    HANDLE hDevice
    )
{
    U16         RegOffset;
    U32         RegValue_1;
    U32         RegValue_2;
    U32         RegValue_3;
    RETURN_CODE rc;


    PlxPrintf("\nPlxRegisterXxx():\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9050:
            RegOffset  = PCI9050_REMAP_SPACE0;
            RegValue_2 = 0xFFFFFFFF & ~(0xF << 28) & ~(1 << 1);   // Write all 1's to register, except reserved bits
            break;

        case 0x9030:
            RegOffset  = PCI9030_REMAP_SPACE0;
            RegValue_2 = 0xFFFFFFFF & ~(0xF << 28) & ~(1 << 1);   // Write all 1's to register, except reserved bits
            break;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            RegOffset  = PCI9054_SPACE0_REMAP;
            RegValue_2 = 0xFFFFFFFF & ~(1 << 1);                // Write all 1's to register, except reserved bits
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    PlxPrintf("  Reading Register (%02xh)......... ", RegOffset);
    RegValue_1 =
        PlxRegisterRead(
            hDevice,
            RegOffset,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxRegisterRead() failed, code = %d\n", rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);


    PlxPrintf("  Writing Register............... ");
    rc =
        PlxRegisterWrite(
            hDevice,
            RegOffset,
            RegValue_2
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxRegisterWrite failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_2);


    PlxPrintf("  Verifying write operation...... ");
    RegValue_3 =
        PlxRegisterRead(
            hDevice,
            RegOffset,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxRegisterRead() failed, code = %d\n",
            rc
            );
        return;
    }

    if (RegValue_3 == RegValue_2)
        PlxPrintf("Ok (0x%08x)\n", RegValue_3);
    else
        PlxPrintf("*ERROR*  -  wrote: %08x   read: %08x\n",
               RegValue_2, RegValue_3);

    PlxPrintf("  Restoring Register............. ");
    rc =
        PlxRegisterWrite(
            hDevice,
            RegOffset,
            RegValue_1
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxRegisterWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);
}




/********************************************************
*
********************************************************/
void
TestIoAccess(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U16         RegOffset;
    U32         RegBar1;
    U32         RegValue_1;
    U32         RegValue_2;
    U32         RegValue_3;
    RETURN_CODE rc;


    PlxPrintf("\nPlxIoPortXxx():\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9050:
            RegOffset  = PCI9050_REMAP_SPACE0;
            RegValue_2 = 0xFFFFFFFF & ~(0xF << 28) & ~(1 << 1);   // Write all 1's to register, except reserved bits
            break;

        case 0x9030:
            RegOffset  = PCI9030_REMAP_SPACE0;
            RegValue_2 = 0xFFFFFFFF & ~(0xF << 28) & ~(1 << 1);   // Write all 1's to register, except reserved bits
            break;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            RegOffset  = PCI9054_SPACE0_REMAP;
            RegValue_2 = 0xFFFFFFFF & ~(1 << 1);                // Write all 1's to register, except reserved bits
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    PlxPrintf("  Reading PCI BAR 1.............. ");
    RegBar1 =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            CFG_BAR1,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigRead() failed, code = %d\n", rc);
        return;
    }

    if ((RegBar1 & (1 << 0)) == 0)
    {
        PlxPrintf("*ERROR* - BAR1 (0x%08x) invalid I/O space\n", RegBar1);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegBar1);

    // Clear I/O enable bit
    RegBar1 &= ~(1 << 0);


    // Read local register
    PlxPrintf("  Reading Register (%02xh)......... ", RegOffset);
    rc =
        PlxIoPortRead(
            hDevice,
            RegBar1 + RegOffset,
            BitSize32,
            &RegValue_1
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxIoPortRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);


    // Write to local register
    PlxPrintf("  Writing Register............... ");
    rc =
        PlxIoPortWrite(
            hDevice,
            RegBar1 + RegOffset,
            BitSize32,
            &RegValue_2
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxIoPortWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_2);


    // Verify Write operation
    PlxPrintf("  Verifying write operation...... ");
    rc =
        PlxIoPortRead(
            hDevice,
            RegBar1 + RegOffset,
            BitSize32,
            &RegValue_3
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxIoPortRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegValue_3 == RegValue_2)
    {
        PlxPrintf(
            "Ok (0x%08x)\n",
            RegValue_3
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR*  -  wrote: %08x   read: %08x\n",
            RegValue_2, RegValue_3
            );
    }


    // Restore local register
    PlxPrintf("  Restoring Register............. ");
    rc =
        PlxIoPortWrite(
            hDevice,
            RegBar1 + RegOffset,
            BitSize32,
            &RegValue_1
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxIoPortWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue_1);
}




/********************************************************
*
********************************************************/
void
TestSerialEeprom(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U32         ReadSave;
    U32         ReadValue;
    U32         WriteValue;
    U32         CompareValue;
    BOOLEAN     EepromPresent;
    EEPROM_TYPE EepType;
    RETURN_CODE rc;


    PlxPrintf("\nPlxSerialEepromXxx():\n");

    PlxPrintf("  Checking if EEPROM present..... ");
    EepromPresent =
        PlxSerialEepromPresent(
            hDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - API call failed, code = %d\n",
            rc
            );
        return;
    }

    if (EepromPresent)
        PlxPrintf("Ok (Valid EEPROM present)\n");
    else
        PlxPrintf("Ok (EEPROM not present or is Blank)\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9050:
        case 0x9080:
            EepType = Eeprom93CS46;
            CompareValue = (pDev->VendorId << 16) | pDev->DeviceId;
            break;

        case 0x9030:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            EepType = Eeprom93CS56;
            CompareValue = (pDev->VendorId << 16) | pDev->DeviceId;
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }


    // Read from EEPROM
    PlxPrintf("  Read EEPROM offset 0........... ");

    rc =
        PlxSerialEepromRead(
            hDevice,
            EepType,
            &ReadSave,
            sizeof(U32)
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (EepromPresent == FALSE)
        CompareValue = (U32)-1;

    if (ReadSave == CompareValue)
    {
        PlxPrintf("Ok  - (value = 0x%08x)\n", ReadSave);
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Val (0x%08x) != ID (0x%08x)\n",
            ReadSave, CompareValue
            );
    }


    // Write to EEPROM
    PlxPrintf("  Write EEPROM offset 0.......... ");

    WriteValue = 0x123406A5;
    rc =
        PlxSerialEepromWrite(
            hDevice,
            EepType,
            &WriteValue,
            sizeof(U32)
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf(
            "Ok  - (value = 0x%08x)\n",
            WriteValue
            );
    }


    // Verify Write
    PlxPrintf("  Verify write operation......... ");

    rc =
        PlxSerialEepromRead(
            hDevice,
            EepType,
            &ReadValue,
            sizeof(U32)
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
                "Ok  - (value = 0x%08x)\n",
                ReadValue
                );
        }
        else
        {
            PlxPrintf(
                "*ERROR* - Rd (0x%08x) != Wr (0x%08x)\n",
                ReadValue, WriteValue
                );
        }
    }


    // Restore Original Value
    PlxPrintf("  Restore EEPROM offset 0........ ");

    rc =
        PlxSerialEepromWrite(
            hDevice,
            EepType,
            &ReadSave,
            sizeof(U32)
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        PlxPrintf("Ok\n");


    // Read from EEPROM by offset
    PlxPrintf("  Read EEPROM by offset.......... ");

    rc =
        PlxSerialEepromReadByOffset(
            hDevice,
            0,
            &ReadSave
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    PlxPrintf(
        "Ok  - (value = 0x%08x)\n",
        ReadSave
        );


    // Write to EEPROM by offset
    PlxPrintf("  Write EEPROM by offset......... ");

    WriteValue = 0x123406A5;
    rc =
        PlxSerialEepromWriteByOffset(
            hDevice,
            0,
            WriteValue
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API call failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf(
            "Ok  - (value = 0x%08x)\n",
            WriteValue
            );
    }


    // Verify Write
    PlxPrintf("  Verify write by offset......... ");

    rc =
        PlxSerialEepromReadByOffset(
            hDevice,
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
                "Ok  - (value = 0x%08x)\n",
                ReadValue
                );
        }
        else
        {
            PlxPrintf(
                "*ERROR* - Rd (0x%08x) != Wr (0x%08x)\n",
                ReadValue, WriteValue
                );
        }
    }


    // Restore Original Value
    PlxPrintf("  Restore EEPROM by offset....... ");

    rc =
        PlxSerialEepromWriteByOffset(
            hDevice,
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




/********************************************************
*
********************************************************/
void
TestDirectSlave(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U16         i;
    U32         DevVenId;
    U32         Offset;
    U32         LocAddress;
    U32         ReadBuffer[0x4];
    U32         WriteBuffer[0x4];
    IOP_SPACE   Space;
    RETURN_CODE rc;


    PlxPrintf("\nPlxBusIopXxx():\n");

    DevVenId = (((U32)pDev->DeviceId) << 16) | pDev->VendorId;

    // Setup parameters based on device
    switch (DevVenId)
    {
        case 0x905010b5:        // 9050 RDK
        case 0x520110b5:        // 9052 RDK    
            Offset     = 0x00000110;
            LocAddress = 0x01000000;
            Space      = IopSpace2;
            break;

        case 0x300110b5:        // 9030 RDK-LITE
        case 0x30c110b5:        // cPCI 9030 RDK-LITE
            Offset     = 0x00000110;
            LocAddress = 0x00000000;
            Space      = IopSpace0;
            break;

        case 0x040110b5:        // 9080 RDK-401b
        case 0x186010b5:        // 9054 RDK-860
        case 0xc86010b5:        // cPCI 9054 RDK-860
        case 0x96c210b5:        // cPCI 9656 RDK-860
        case 0x56c210b5:        // cPCI 9056 RDK-860
            Offset     = 0x00000110;
            LocAddress = 0x00100000;
            Space      = IopSpace0;
            break;

        case 0x086010b5:        // 9080 RDK-860
            Offset     = 0x00000110;
            LocAddress = 0x10100000;
            Space      = IopSpace0;
            break;

        case 0x540610b5:        // 9054 RDK-LITE
            Offset     = 0x00000110;
            LocAddress = 0x20000000;
            Space      = IopSpace0;
            break;

        case 0x960110b5:        // 9656 RDK-LITE
        case 0x560110b5:        // 9056 RDK-LITE
            Offset     = 0x00000110;
            LocAddress = 0x00000000;
            Space      = IopSpace0;
            break;

        default:
            PlxPrintf(
                "  ERROR - Test not configured for device (%04X_%04X)\n",
                pDev->DeviceId, pDev->VendorId
                );
            return;
    }


    // First test without remapping
    PlxPrintf(
        "  Without Remapping: Space %d, 32-bit, offset = 0x%08x\n",
        Space, Offset
        );

    PlxPrintf("    Preparing buffers............ ");
    for (i=0; i < (sizeof(WriteBuffer) >> 2); i++)
        WriteBuffer[i] = i;

    memset(
        ReadBuffer,
        0,
        sizeof(ReadBuffer)
        );

    PlxPrintf("Ok\n");

    PlxPrintf("    Writing Data to Device....... ");
    rc =
        PlxBusIopWrite(
            hDevice,
            Space,
            Offset,
            FALSE,
            WriteBuffer,
            sizeof(WriteBuffer),
            BitSize32
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxBusIopWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Reading Data from Device..... ");
    rc =
        PlxBusIopRead(
            hDevice,
            Space,
            Offset,
            FALSE,
            ReadBuffer,
            sizeof(ReadBuffer),
            BitSize32
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxBusIopRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Verifying data............... ");
    if (memcmp(
            WriteBuffer,
            ReadBuffer,
            sizeof(WriteBuffer)
            ) != 0)
    {
        PlxPrintf("*ERROR* - Buffers do not match\n");
        return;
    }
    PlxPrintf("Ok\n");


    // Now test an absolute address with remapping
    PlxPrintf(
        "\n  With Remapping: Space %d, 8-bit, address = 0x%08x\n",
        Space, LocAddress
        );

    PlxPrintf("    Preparing buffers............ ");
    memset(
        ReadBuffer,
        0,
        sizeof(ReadBuffer)
        );
    PlxPrintf("Ok\n");

    PlxPrintf("    Writing Data to IOP.......... ");
    rc =
        PlxBusIopWrite(
            hDevice,
            Space,
            LocAddress,
            TRUE,
            WriteBuffer,
            sizeof(WriteBuffer),
            BitSize8
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxBusIopWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Reading Data from IOP........ ");
    rc =
        PlxBusIopRead(
            hDevice,
            Space,
            LocAddress,
            TRUE,
            ReadBuffer,
            sizeof(ReadBuffer),
            BitSize8
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxBusIopRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Verifying data............... ");
    if (memcmp(
            WriteBuffer,
            ReadBuffer,
            sizeof(WriteBuffer)
            ) != 0)
    {
        PlxPrintf("*ERROR*  -  Buffers do not match\n");
        return;
    }
    PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
TestHotSwapRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U8          RegField;
    U16         RegOffset;
    U32         RegValue;
    RETURN_CODE rc;


    PlxPrintf("\nPlxHotSwapXxx():\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9030:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            RegOffset = PCI9054_HS_CAP_ID;
            break;

        case 0x9050:
        case 0x9080:
            PlxPrintf("  Calling PlxHotSwapIdRead()..... ");
            rc = 0;
            PlxHotSwapIdRead(
                hDevice,
                &rc
                );
            if (rc == ApiHSNotSupported)
            {
                PlxPrintf("Ok (ApiHSNotSupported returned)\n");
            }
            else
            {
                PlxPrintf(
                    "*ERROR* - ApiHSNotSupported not ret, rc=%d\n",
                    rc
                    );
            }

            PlxPrintf("  Calling PlxHotSwapNcpRead().... ");
            rc = 0;
            PlxHotSwapNcpRead(
                hDevice,
                &rc
                );
            if (rc == ApiHSNotSupported)
            {
                PlxPrintf("Ok (ApiHSNotSupported returned)\n");
            }
            else
            {
                PlxPrintf(
                    "*ERROR* - ApiHSNotSupported not ret, rc=%d\n",
                    rc
                    );
            }

            PlxPrintf("  Calling PlxHotSwapStatus()..... ");
            rc = 0;
            PlxHotSwapStatus(
                hDevice,
                &rc
                );
            if (rc == ApiHSNotSupported)
            {
                PlxPrintf("Ok (ApiHSNotSupported returned)\n");
            }
            else
            {
                PlxPrintf(
                    "*ERROR* - ApiHSNotSupported not ret, rc=%d\n",
                    rc
                    );
            }
            return;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    PlxPrintf("  Reading Hot Swap Register...... ");
    RegValue =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigRegisterRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok (0x%08x)\n", RegValue);

    // Test ID
    PlxPrintf("  Reading Hot Swap ID............ ");
    RegField =
        PlxHotSwapIdRead(
            hDevice,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxHotSwapIdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == (RegValue & 0xFF))
    {
        PlxPrintf(
            "Ok  - ID (%02Xh) matches reg (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - ID (%02Xh) doesn't matches reg field (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }

    // Test NCP
    PlxPrintf("  Reading Hot Swap NCP........... ");
    RegField =
        PlxHotSwapNcpRead(
            hDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxHotSwapNcpRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == ((RegValue >> 8) & 0xFF))
    {
        PlxPrintf(
            "Ok  - NCP (%02Xh) matches reg (%02Xh)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - NCP (%02Xh) doesn't matches reg field (%02Xh)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }


    // Test status
    PlxPrintf("  Reading Hot Swap Status........ ");
    RegField =
        PlxHotSwapStatus(
            hDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxHotSwapStatus() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    // Clear bits not checked in HS Status
    RegValue &= 0xFFC8FFFF;

    if (RegField == ((RegValue >> 16) & 0xFF))
    {
        PlxPrintf(
            "Ok  - Status (%02Xh) matches reg (%02Xh)\n",
            RegField, ((RegValue >> 16) & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Status (%02Xh) != reg field (%02Xh)\n",
            RegField, ((RegValue >> 16) & 0xFF)
            );
    }
}




/********************************************************
*
********************************************************/
void
TestPowerManRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U8          RegField;
    U16         RegOffset;
    U32         RegValue;
    RETURN_CODE rc;


    PlxPrintf("\nPlxPmXxx():\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9030:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            RegOffset = PCI9054_PM_CAP_ID;
            break;

        case 0x9050:
        case 0x9080:
            PlxPrintf("  Calling PlxPmIdRead().......... ");
            rc = 0;
            PlxPmIdRead(
                hDevice,
                &rc
                );
            if (rc == ApiPMNotSupported)
            {
                PlxPrintf(
                    "Ok (ApiPMNotSupported returned)\n"
                    );
            }
            else
            {
                PlxPrintf(
                    "*ERROR* - ApiPMNotSupported not ret, rc=%d\n",
                    rc
                    );
            }

            PlxPrintf("  Calling PlxPmNcpRead()......... ");
            rc = 0;
            PlxPmNcpRead(
                hDevice,
                &rc
                );
            if (rc == ApiPMNotSupported)
                PlxPrintf("Ok (ApiPMNotSupported returned)\n");
            else
                PlxPrintf("*ERROR* - ApiPMNotSupported not ret, rc=%d\n", rc);
            return;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    PlxPrintf("  Reading Power Management Reg... ");
    RegValue =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegOffset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigRegisterRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok  - (value = 0x%08x)\n", RegValue);


    // Test ID
    PlxPrintf("  Reading Power Management ID.... ");
    RegField =
        PlxPmIdRead(
            hDevice,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxPmIdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == (RegValue & 0xFF))
    {
        PlxPrintf(
            "Ok  - ID (%02Xh) matches reg (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - ID (%02Xh) doesn't matches reg field (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }

    // Test NCP
    PlxPrintf("  Reading Power Management NCP... ");
    RegField =
        PlxPmNcpRead(
            hDevice,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxPmNcpRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == ((RegValue >> 8) & 0xFF))
    {
        PlxPrintf(
            "Ok  - NCP (%02Xh) matches reg (%02Xh)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - NCP (%02Xh) doesn't matches reg field (%02Xh)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }
}




/********************************************************
*
********************************************************/
void
TestVpdRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U8          RegField;
    U8          Shift_WriteProtect;
    U16         RegVpdOffset;
    U16         RegWriteProtectOffset;
    U32         RegValue;
    U32         RegSave;
    U32         RegWriteProtect;
    U32         RegValueToCompare;
    U32         TempValue;
    RETURN_CODE rc;


    PlxPrintf("\nPlxVpdXxx():\n");

    // Setup parameters for test
    switch (ChipTypeSelected)
    {
        case 0x9030:
            RegVpdOffset          = PCI9030_VPD_CAP_ID;
            Shift_WriteProtect    = 16;
            RegWriteProtectOffset = PCI9030_INT_CTRL_STAT;
            RegValueToCompare     = (pDev->DeviceId << 16 | pDev->VendorId);
            break;

        case 0x9054:
        case 0x9056:
        case 0x9656:
            RegVpdOffset          = PCI9054_VPD_CAP_ID;
            Shift_WriteProtect    = 16;
            RegWriteProtectOffset = PCI9054_ENDIAN_DESC;
            RegValueToCompare     = (pDev->DeviceId << 16 | pDev->VendorId);
            break;

        case 0x9050:
        case 0x9080:
            PlxPrintf("  Calling PlxVpdIdRead()......... ");
            rc = 0;
            PlxVpdIdRead(
                hDevice,
                &rc
                );
            if (rc == ApiVPDNotSupported)
                PlxPrintf("Ok (ApiVPDNotSupported returned)\n");
            else
                PlxPrintf("*ERROR* - ApiVPDNotSupported not ret, rc=%d\n", rc);

            PlxPrintf("  Calling PlxVpdNcpRead()........ ");
            rc = 0;
            PlxVpdNcpRead(
                hDevice,
                &rc
                );
            if (rc == ApiVPDNotSupported)
            {
                PlxPrintf("Ok (ApiVPDNotSupported returned)\n");
            }
            else
            {
                PlxPrintf(
                    "*ERROR* - ApiVPDNotSupported not ret, rc=%d\n",
                    rc
                    );
            }

            PlxPrintf("  Calling PlxVpdRead()........... ");
            rc = 0;
            PlxVpdRead(
                hDevice,
                0x0,
                &rc
                );

            if (rc == ApiVPDNotSupported)
                PlxPrintf("Ok (ApiVPDNotSupported returned)\n");
            else
                PlxPrintf("*ERROR* - ApiVPDNotSupported not ret, rc=%d\n", rc);

            PlxPrintf("  Calling PlxVpdWrite().......... ");
            rc =
                PlxVpdWrite(
                    hDevice,
                    0x0,
                    0x0
                    );
            if (rc == ApiVPDNotSupported)
            {
                PlxPrintf("Ok (ApiVPDNotSupported returned)\n");
            }
            else
            {
                PlxPrintf("*ERROR* - ApiVPDNotSupported not ret, rc=%d\n", rc);
            }
            return;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }

    PlxPrintf("  Reading VPD Register........... ");
    RegValue =
        PlxPciConfigRegisterRead(
            pDev->BusNumber,
            pDev->SlotNumber,
            RegVpdOffset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - ConfigRegisterRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok  - (value = 0x%08x)\n", RegValue);


    // Test ID
    PlxPrintf("  Reading VPD ID................. ");
    RegField =
        PlxVpdIdRead(
            hDevice,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdIdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == (RegValue & 0xFF))
    {
        PlxPrintf(
            "Ok  - ID (%02Xh) matches reg (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - ID (%02Xh) doesn't matches reg field (%02Xh)\n",
            RegField, (RegValue & 0xFF)
            );
    }

    // Test NCP
    PlxPrintf("  Reading VPD NCP................ ");
    RegField =
        PlxVpdNcpRead(
            hDevice,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdNcpRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegField == ((RegValue >> 8) & 0xFF))
    {
        PlxPrintf(
            "Ok  - NCP (0x%02X) == reg (0x%02X)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - NCP (0x%02X) != reg field (0x%02X)\n",
            RegField, ((RegValue >> 8) & 0xFF)
            );
    }

    // Test VPD read from EEPROM
    PlxPrintf("  Reading EEPROM 0 using VPD..... ");
    RegValue =
        PlxVpdRead(
            hDevice,
            0x0,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (RegValue == RegValueToCompare)
    {
        PlxPrintf(
            "Ok  - VPD (0x%08x) == ID (0x%08x)\n",
            RegValue, RegValueToCompare
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - VPD (0x%08x) != ID (0x%08x)\n",
            RegValue, RegValueToCompare
            );
    }


    // Get the write protect boundary offset
    PlxPrintf("  Get Write-Protect boundary..... ");
    RegWriteProtect =
        PlxRegisterRead(
            hDevice,
            RegWriteProtectOffset,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxRegisterRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    RegWriteProtect = ((RegWriteProtect >> Shift_WriteProtect) & 0xFF) * 4;
    PlxPrintf(
        "Ok  - (boundary = 0x%02X)\n",
        RegWriteProtect
        );


    // Test write to EEPROM using VPD
    PlxPrintf(
        "  Reading EEPROM 0x%03x using VPD. ",
        RegWriteProtect
        );

    RegSave =
        PlxVpdRead(
            hDevice,
            (U16)RegWriteProtect,
            &rc
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf(
        "Ok  - (value = 0x%08x)\n",
        RegSave
        );

    PlxPrintf(
        "  Writing EEPROM 0x%03x using VPD. ",
        RegWriteProtect
        );
    TempValue = 0x01234567;

    rc =
        PlxVpdWrite(
            hDevice,
            (U16)RegWriteProtect,
            TempValue
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf(
        "Ok  - (value = 0x%08x)\n",
        TempValue
        );

    PlxPrintf("  Verifying value was written.... ");
    RegValue =
        PlxVpdRead(
            hDevice,
            (U16)RegWriteProtect,
            &rc
            );
    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdRead() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    if (TempValue == RegValue)
    {
        PlxPrintf(
            "Ok  - Value written correctly\n"
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Value not written, read 0x%08x\n",
            RegValue
            );
    }


    // Clear EEPROM value
    PlxPrintf("  Restore EEPROM value........... ");

    rc =
        PlxVpdWrite(
            hDevice,
            (U16)RegWriteProtect,
            RegSave
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - PlxVpdWrite() failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }
    PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
TestPowerLevel(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    PlxPrintf("\nPlxPowerLevelXxx():\n");

    switch (ChipTypeSelected)
    {
        case 0x9050:
            TestPowerLevel9050(
                hDevice,
                pDev
                );
            break;

        case 0x9030:
            TestPowerLevel9030(
                hDevice,
                pDev
                );
            break;

        case 0x9080:
            TestPowerLevel9080(
                hDevice,
                pDev
                );
            break;

        case 0x9054:
            TestPowerLevel9054(
                hDevice,
                pDev
                );
            break;

        case 0x9056:
        case 0x9656:
            TestPowerLevel9656(
                hDevice,
                pDev
                );
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }
}




/********************************************************
*
********************************************************/
void
TestInterruptEnable(
    HANDLE hDevice
    )
{
    PlxPrintf("\nPlxIntrXxxx():\n");

    switch (ChipTypeSelected)
    {
        case 0x9050:
            TestInterruptEnable9050(
                hDevice
                );
            break;

        case 0x9030:
            TestInterruptEnable9030(
                hDevice
                );
            break;

        case 0x9080:
            TestInterruptEnable9080(
                hDevice
                );
            break;

        case 0x9054:
            TestInterruptEnable9054(
                hDevice
                );
            break;

        case 0x9056:
        case 0x9656:
            TestInterruptEnable9656(
                hDevice
                );
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }
}




/********************************************************
*
********************************************************/
void
TestInterruptAttach(
    HANDLE hDevice
    )
{
    PlxPrintf("\nPlxIntrAttach():\n");

    if (ChipTypeSelected == 0x9050)
    {
        TestInterruptAttach9050(
            hDevice
            );

        return;
    }

    if (ChipTypeSelected == 0x9030)
    {
        TestInterruptAttach9030(
            hDevice
            );

        return;
    }

    PlxPrintf("  ** IntrAttach() is tested in DMA transfer for this chip **\n");
}




/********************************************************
*
********************************************************/
void
TestRegisterDoorbell(
    HANDLE hDevice
    )
{
    U8           BitPosInt;
    U16          OffsetInt;
    U16          OffsetDoorbell;
    U32          RegSave;
    U32          RegValue;
    RETURN_CODE  rc;


    PlxPrintf("\nPlxRegisterDoorbellXxx():\n");
    
    switch (ChipTypeSelected)
    {
        case 0x9050:
            TestRegisterDoorbell9050(
                hDevice
                );
            return;

        case 0x9030:
            TestRegisterDoorbell9030(
                hDevice
                );
            return;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            BitPosInt      = 16;
            OffsetInt      = 0x68;
            OffsetDoorbell = 0x60;
            break;

        default:
            PlxPrintf("*** Unsupported chip - bypassing test ***\n");
            return;
    }


    PlxPrintf("  Disable Local Interrupts....... ");
    RegSave =
        PlxRegisterRead(
            hDevice,
            OffsetInt,
            &rc
            );

    PlxRegisterWrite(
        hDevice,
        OffsetInt,
        RegSave & ~(1 << BitPosInt)
        );
    PlxPrintf("Ok\n");


    PlxPrintf("  Writing to Doorbell Register... ");
    rc =
        PlxRegisterDoorbellSet(
            hDevice,
            0x12345678
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok (val = 0x12345678)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to write doorbell, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Verifying Doorbell Write....... ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            OffsetDoorbell,
            &rc
            );

    if (RegValue == 0x12345678)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Doorbell (0x%x) != 0x12345678\n",
            RegValue
            );
    }


    PlxPrintf("  Restore Local Interrupts....... ");
    PlxRegisterWrite(
        hDevice,
        OffsetInt,
        RegSave
        );
    PlxPrintf("Ok\n");


    PlxPrintf("  Verifying Doorbell is cleared.. ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            OffsetDoorbell,
            &rc
            );

    if (RegValue == 0)
    {
        PlxPrintf(
            "Ok - Doorbell cleared\n"
            );
    }
    else
    {
        PlxPrintf(
            "Ok (Doorbell not cleared, no local CPU)\n"
            );
    }


    PlxPrintf("  Get Last Doorbell Int Value.... ");
    RegValue =
        PlxRegisterDoorbellRead(
            hDevice,
            &rc
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf(
            "Ok (val = 0x%08x)\n",
            RegValue
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to read doorbell, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestRegisterMailbox(
    HANDLE hDevice
    )
{
    U32         RegSave;
    U32         RegValue;
    RETURN_CODE rc;


    PlxPrintf("\nPlxRegisterMailboxXxx():\n");
    
    if (ChipTypeSelected == 0x9050)
    {
        TestRegisterMailbox9050(
            hDevice
            );
        return;
    }

    if (ChipTypeSelected == 0x9030)
    {
        TestRegisterMailbox9030(
            hDevice
            );
        return;
    }


    PlxPrintf("  Reading Mailbox Register 3..... ");
    RegSave =
        PlxRegisterMailboxRead(
            hDevice,
            MailBox3,
            &rc
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf(
            "Ok (val = 0x%08x)\n",
            RegSave
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to read Mailbox, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Writing Mailbox Register 3..... ");
    rc =
        PlxRegisterMailboxWrite(
            hDevice,
            MailBox3,
            0x12345678
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok (val = 0x12345678)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to write Mailbox, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Verifying Mailbox value........ ");
    RegValue =
        PlxRegisterMailboxRead(
            hDevice,
            MailBox3,
            &rc
            );

    if (rc == ApiSuccess)
    {
        if (RegValue == 0x12345678)
        {
            PlxPrintf("Ok (Reg matches value)\n");
        }
        else
        {
            PlxPrintf(
                "*ERROR* - Reg (0x%08x) != 0x12345678\n",
                RegValue
                );
        }
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to read Mailbox, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Restoring Mailbox Register 3... ");
    rc =
        PlxRegisterMailboxWrite(
            hDevice,
            MailBox3,
            RegSave
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf(
            "Ok (val = 0x%08x)\n",
            RegSave
            );
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to write Mailbox, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestBlockDma(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U32                   i;
    U32                   VaRead;
    U32                   VaWrite;
    U32                   DevVenId;
    U32                   Stat_Errors;
    U32                   LocalAddress;
    U32                   TransferSize;
    HANDLE                hInterruptEvent;
    PLX_INTR              PlxInterrupt;
    PCI_MEMORY            PciBuffer;
    RETURN_CODE           rc;
    DMA_CHANNEL_DESC      DmaDesc;
    DMA_TRANSFER_ELEMENT  DmaData;


    PlxPrintf("\nPlxBlockDmaXxx():\n");

    if (ChipTypeSelected == 0x9050)
    {
        TestBlockDma9050(
            hDevice
            );
        return;
    }

    if (ChipTypeSelected == 0x9030)
    {
        TestBlockDma9030(
            hDevice
            );
        return;
    }

    DevVenId = (((U32)pDev->DeviceId) << 16) | pDev->VendorId;

    // Set DMA parameters based on device
    switch (DevVenId)
    {
        case 0x040110b5:        // 9080 RDK-401b
        case 0x186010b5:        // 9054 RDK-860
        case 0xc86010b5:        // cPCI 9054 RDK-860
        case 0x96c210b5:        // cPCI 9656 RDK-860
        case 0x56c210b5:        // cPCI 9056 RDK-860
            LocalAddress = 0x00100000;
            TransferSize = 0x00200000;
            break;

        case 0x086010b5:        // 9080 RDK-860
            LocalAddress = 0x10100000;
            TransferSize = 0x00200000;
            break;

        case 0x540610b5:        // 9054 RDK-LITE
            LocalAddress = 0x20000000;
            TransferSize = 0x00020000;  // 128k
            break;

        case 0x960110b5:        // 9656 RDK-LITE
        case 0x560110b5:        // 9056 RDK-LITE
            LocalAddress = 0x00000000;
            TransferSize = 0x00020000;  // 128k
            break;

        default:
            PlxPrintf(
                "  ERROR - Test not configured for device (%04X_%04X)\n",
                pDev->DeviceId, pDev->VendorId
                );
            return;
    }

    // Allocate a physical buffer for DMA
    PlxPrintf("  Allocate a physical buffer..... ");

    PciBuffer.Size = TransferSize * 2;

    rc =
        PlxPciPhysicalMemoryAllocate(
            hDevice,
            &PciBuffer,
            TRUE            // Smaller buffer ok
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf(
            "Ok (PhysAddr=0x%08X  Size=%d Kb)\n",
            PciBuffer.PhysicalAddr, (PciBuffer.Size >> 10)
            );
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
        return;
    }

    // Map buffer into user space
    PlxPrintf("  Map Buffer into User Space..... ");
    rc =
        PlxPciPhysicalMemoryMap(
            hDevice,
            &PciBuffer
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf(
            "Ok (VA=0x%08x)\n",
            PciBuffer.UserAddr
            );
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);

        // Release the buffer
        PlxPciPhysicalMemoryFree(
            hDevice,
            &PciBuffer
            );
        
        return;
    }

    // Calculate the maximum transfer size
    TransferSize =
        min(
            TransferSize,
            PciBuffer.Size / 2
            );

    PlxPrintf("  Preparing buffer data.......... ");
    for (i=0; i < TransferSize; i+=sizeof(U32))
        *(U32*)(PciBuffer.UserAddr + i) = i;
    PlxPrintf("Ok\n");


    // Initialize the DMA channel
    PlxPrintf("  Opening Block DMA Channel 0.... ");
    DmaDesc.EnableReadyInput         = 1;
    DmaDesc.EnableBTERMInput         = 0;
    DmaDesc.EnableIopBurst           = 0;
    DmaDesc.EnableWriteInvalidMode   = 0;
    DmaDesc.EnableDmaEOTPin          = 0;
    DmaDesc.DmaStopTransferMode      = AssertBLAST;
    DmaDesc.HoldIopAddrConst         = 0;
    DmaDesc.HoldIopSourceAddrConst   = 0;
    DmaDesc.HoldIopDestAddrConst     = 0;
    DmaDesc.DemandMode               = 0;
    DmaDesc.EnableTransferCountClear = 0;
    DmaDesc.WaitStates               = 0;
    DmaDesc.IopBusWidth              = 3;
    DmaDesc.EOTEndLink               = 0;
    DmaDesc.ValidStopControl         = 0;
    DmaDesc.ValidModeEnable          = 0;
    DmaDesc.EnableDualAddressCycles  = 0;
    DmaDesc.Reserved1                = 0;
    DmaDesc.TholdForIopWrites        = 0;
    DmaDesc.TholdForIopReads         = 0;
    DmaDesc.TholdForPciWrites        = 0;
    DmaDesc.TholdForPciReads         = 0;
    DmaDesc.EnableFlybyMode          = 0;
    DmaDesc.FlybyDirection           = 0;
    DmaDesc.EnableDoneInt            = 1;
    DmaDesc.Reserved2                = 0;
    DmaDesc.DmaChannelPriority       = Channel0Highest;

    rc =
        PlxDmaBlockChannelOpen(
            hDevice,
            PrimaryPciChannel0,
            &DmaDesc
            );

    if (rc == ApiSuccess)
        PlxPrintf("Ok\n");
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Transfer data to local
    PlxPrintf("  Transfer DMA Block to Local.... ");
    DmaData.u.PciAddrLow      = PciBuffer.PhysicalAddr;
    DmaData.PciAddrHigh       = 0x0;
    DmaData.LocalAddr         = LocalAddress;
    DmaData.TransferCount     = TransferSize;
    DmaData.LocalToPciDma     = 0;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaBlockTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaData,
            FALSE        // Wait for completion
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Setup interrupt wait for second DMA
    PlxPrintf("  Attach event to DMA Interrupt.. ");

    // Clear interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxInterrupt.PciDmaChannel0 = 1;
    rc =
        PlxIntrAttach(
            hDevice,
            PlxInterrupt,
            &hInterruptEvent
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        PlxPrintf(
            "Ok (hEvent=0x%08x)\n",
            (U32)hInterruptEvent
            );
    }


    // Transfer the Data back to PCI
    PlxPrintf("  Transfer data back to PCI...... ");
    DmaData.u.PciAddrLow      = PciBuffer.PhysicalAddr + TransferSize;
    DmaData.PciAddrHigh       = 0x0;
    DmaData.LocalAddr         = LocalAddress;
    DmaData.TransferCount     = TransferSize;
    DmaData.LocalToPciDma     = 1;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaBlockTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaData,
            TRUE         // Don't wait for completion
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");

        PlxPrintf("  Wait for interrupt event....... ");

        rc =
            PlxIntrWait(
                hDevice,
                hInterruptEvent,
                10 * 1000
                );

        switch (rc)
        {
            case ApiSuccess:
                PlxPrintf("Ok (DMA 0 Int received)\n");

                memset(
                    &PlxInterrupt,
                    0,
                    sizeof(PLX_INTR)
                    );

                PlxPrintf("  Verifying DMA Int Status....... ");
                rc =
                    PlxIntrStatusGet(
                        hDevice,
                        &PlxInterrupt
                        );

                if (rc != ApiSuccess)
                    PlxPrintf("*ERROR* - PlxIntrStatusGet() failed, code=%d\n", rc);
                else
                {
                    if (PlxInterrupt.PciDmaChannel0 == 1)
                        PlxPrintf("Ok (DMA 0 Int reported)\n");
                    else
                        PlxPrintf("*ERROR* - DMA 0 Int not reported\n");
                }
                break;

            case ApiWaitTimeout:
                PlxPrintf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case ApiWaitCanceled:
                PlxPrintf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                PlxPrintf("*ERROR* - API failed\n");
                PlxSdkErrorDisplay(rc);
                break;
        }
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Close DMA Channel
    PlxPrintf("  Closing Block DMA Channel 0.... ");
    rc =
        PlxDmaBlockChannelClose(
            hDevice,
            PrimaryPciChannel0
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Compare the data
    PlxPrintf("  Compare data buffers........... ");

    VaWrite     = PciBuffer.UserAddr;
    VaRead      = PciBuffer.UserAddr + TransferSize;
    Stat_Errors = 0;

    for (i=0; i<TransferSize; i += sizeof(U32))
    {
        if (*(U32*)VaWrite != *(U32*)VaRead)
        {
            if (Stat_Errors == 0)
            {
                PlxPrintf("\n");
            }

            Stat_Errors++;

            PlxPrintf(
                "     DCmp: Error - Offset: %08x     Wrote: %08x    Read: %08x\n",
                i, *(U32*)VaWrite, *(U32*)VaRead
                );

            if (Stat_Errors == 10)
            {
                PlxPrintf(
                    "     DCmp: HALTED -- too many errors (%d found)\n",
                    Stat_Errors
                    );
                break;
            }
        }

        VaWrite += sizeof(U32);
        VaRead  += sizeof(U32);
    }

    if (Stat_Errors == 0)
    {
        PlxPrintf("Ok\n");
    }
    else if (Stat_Errors < 10)
    {
        PlxPrintf(
            "     DCmp: %d errors found\n",
            Stat_Errors
            );
    }

    // Unmap buffer from user space
    PlxPrintf("  Unmap Buffer from User Space... ");
    rc =
        PlxPciPhysicalMemoryUnmap(
            hDevice,
            &PciBuffer
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Release the buffer
    PlxPrintf("  Release physical buffer........ ");

    rc =
        PlxPciPhysicalMemoryFree(
            hDevice,
            &PciBuffer
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
}




/********************************************************
*
********************************************************/
void
TestSglDma(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U8                   *pDmaBuffer;
    U32                   i;
    U32                   VaRead;
    U32                   VaWrite;
    U32                   DevVenId;
    U32                   Stat_Errors;
    U32                   LocalAddress;
    U32                   TransferSize;
    HANDLE                hInterruptEvent;
    PLX_INTR              PlxInterrupt;
    RETURN_CODE           rc;
    DMA_CHANNEL_DESC      DmaDesc;
    DMA_TRANSFER_ELEMENT  DmaData;


    PlxPrintf("\nPlxSglDmaXxx():\n");

    if (ChipTypeSelected == 0x9050)
    {
        TestSglDma9050(
            hDevice
            );
        return;
    }

    if (ChipTypeSelected == 0x9030)
    {
        TestSglDma9030(
            hDevice
            );
        return;
    }

    DevVenId = (((U32)pDev->DeviceId) << 16) | pDev->VendorId;

    // Set DMA parameters based on device
    switch (DevVenId)
    {
        case 0x040110b5:        // 9080 RDK-401b
        case 0x186010b5:        // 9054 RDK-860
        case 0xc86010b5:        // cPCI 9054 RDK-860
        case 0x96c210b5:        // cPCI 9656 RDK-860
        case 0x56c210b5:        // cPCI 9056 RDK-860
            LocalAddress = 0x00100000;
            TransferSize = 0x00100000;
            break;

        case 0x086010b5:        // 9080 RDK-860
            LocalAddress = 0x10100000;
            TransferSize = 0x00100000;
            break;

        case 0x540610b5:        // 9054 RDK-LITE
            LocalAddress = 0x20000000;
            TransferSize = 0x00020000;  // 128k
            break;

        case 0x960110b5:        // 9656 RDK-LITE
        case 0x560110b5:        // 9056 RDK-LITE
            LocalAddress = 0x00000000;
            TransferSize = 0x00020000;  // 128k
            break;

        default:
            PlxPrintf(
                "  ERROR - Test not configured for device (%04X_%04X)\n",
                pDev->DeviceId, pDev->VendorId
                );
            return;
    }


    PlxPrintf("  Allocate mem for buffer........ ");
    pDmaBuffer =
        (U8*)malloc(
                 TransferSize * 2
                 );

    if (pDmaBuffer == NULL)
    {
        PlxPrintf("ERROR - allocation failed\n");
        return;
    }
    PlxPrintf(
        "Ok (Va=0x%08x size=%d Kb)\n",
        pDmaBuffer, (TransferSize * 2) >> 10
        );


    PlxPrintf("  Preparing buffer data.......... ");
    for (i=0; i < TransferSize; i+=sizeof(U32))
        *(U32*)(pDmaBuffer + i) = i;
    PlxPrintf("Ok\n");


    PlxPrintf("  Opening SGL DMA Channel 0...... ");
    DmaDesc.EnableReadyInput         = 1;
    DmaDesc.EnableBTERMInput         = 0;
    DmaDesc.EnableIopBurst           = 0;
    DmaDesc.EnableWriteInvalidMode   = 0;
    DmaDesc.EnableDmaEOTPin          = 0;
    DmaDesc.DmaStopTransferMode      = AssertBLAST;
    DmaDesc.HoldIopAddrConst         = 0;
    DmaDesc.HoldIopSourceAddrConst   = 0;
    DmaDesc.HoldIopDestAddrConst     = 0;
    DmaDesc.DemandMode               = 0;
    DmaDesc.EnableTransferCountClear = 0;
    DmaDesc.WaitStates               = 0;
    DmaDesc.IopBusWidth              = 3;
    DmaDesc.EOTEndLink               = 0;
    DmaDesc.ValidStopControl         = 0;
    DmaDesc.ValidModeEnable          = 0;
    DmaDesc.EnableDualAddressCycles  = 0;
    DmaDesc.Reserved1                = 0;
    DmaDesc.TholdForIopWrites        = 0;
    DmaDesc.TholdForIopReads         = 0;
    DmaDesc.TholdForPciWrites        = 0;
    DmaDesc.TholdForPciReads         = 0;
    DmaDesc.EnableFlybyMode          = 0;
    DmaDesc.FlybyDirection           = 0;
    DmaDesc.EnableDoneInt            = 0;
    DmaDesc.Reserved2                = 0;
    DmaDesc.DmaChannelPriority       = Channel0Highest;

    rc =
        PlxDmaSglChannelOpen(
            hDevice,
            PrimaryPciChannel0,
            &DmaDesc
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }


    // Transfer data to local
    PlxPrintf("  Transfer buffer to local....... ");
    DmaData.u.UserVa          = (U32)pDmaBuffer;
    DmaData.LocalAddr         = LocalAddress;
    DmaData.TransferCount     = TransferSize;
    DmaData.LocalToPciDma     = 0;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaSglTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaData,
            FALSE         // Wait for completion
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    // Setup interrupt wait for second DMA
    PlxPrintf("  Attach event to DMA Interrupt.. ");

    // Clear interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxInterrupt.PciDmaChannel0 = 1;
    rc =
        PlxIntrAttach(
            hDevice,
            PlxInterrupt,
            &hInterruptEvent
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }
    else
    {
        PlxPrintf(
            "Ok (hEvent=0x%08x)\n",
            hInterruptEvent
            );
    }


    // Transfer the Data back to PCI
    PlxPrintf("  Transfer data back to PCI...... ");
    DmaData.u.UserVa          = (U32)pDmaBuffer + TransferSize;
    DmaData.LocalAddr         = LocalAddress;
    DmaData.TransferCount     = TransferSize;
    DmaData.LocalToPciDma     = 1;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaSglTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaData,
            TRUE         // Don't wait for completion
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");

        PlxPrintf("  Wait for interrupt event....... ");

        rc =
            PlxIntrWait(
                hDevice,
                hInterruptEvent,
                10 * 1000
                );

        switch (rc)
        {
            case ApiSuccess:
                PlxPrintf("Ok (DMA 0 Int received)\n");

                memset(
                    &PlxInterrupt,
                    0,
                    sizeof(PLX_INTR)
                    );

                PlxPrintf("  Verifying DMA Int Status....... ");
                rc =
                    PlxIntrStatusGet(
                        hDevice,
                        &PlxInterrupt
                        );

                if (rc != ApiSuccess)
                    PlxPrintf("*ERROR* - PlxIntrStatusGet() failed, code=%d\n", rc);
                else
                {
                    if (PlxInterrupt.PciDmaChannel0 == 1)
                        PlxPrintf("Ok (DMA 0 Int reported)\n");
                    else
                        PlxPrintf("*ERROR* - DMA 0 Int not reported\n");
                }
                break;

            case ApiWaitTimeout:
                PlxPrintf("*ERROR* - Timeout waiting for Int Event\n");
                break;

            case ApiWaitCanceled:
                PlxPrintf("*ERROR* - Interrupt event cancelled\n");
                break;

            default:
                PlxPrintf("*ERROR* - API failed\n");
                PlxSdkErrorDisplay(rc);
                break;
        }
    }
    else
    {
        PlxPrintf(
            "*ERROR* - rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }


    // Close DMA Channel
    PlxPrintf("  Closing SGL DMA Channel 0...... ");
    rc =
        PlxDmaSglChannelClose(
            hDevice,
            PrimaryPciChannel0
            );

    if (rc == ApiSuccess)
    {
        PlxPrintf("Ok\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }


    // Compare the data
    PlxPrintf("  Compare data buffers........... ");

    VaWrite     = (U32)pDmaBuffer;
    VaRead      = (U32)pDmaBuffer + TransferSize;
    Stat_Errors = 0;

    for (i=0; i<TransferSize; i += sizeof(U32))
    {
        if (*(U32*)VaWrite != *(U32*)VaRead)
        {
            if (Stat_Errors == 0)
            {
                PlxPrintf("\n");
            }

            Stat_Errors++;

            PlxPrintf(
                "     DCmp: Error - Offset: %08x     Wrote: %08x    Read: %08x\n",
                i, *(U32*)VaWrite, *(U32*)VaRead
                );

            if (Stat_Errors == 10)
            {
                PlxPrintf(
                    "     DCmp: HALTED -- too many errors (%d found)\n",
                    Stat_Errors
                    );
                break;
            }
        }

        VaWrite += sizeof(U32);
        VaRead  += sizeof(U32);
    }

    if (Stat_Errors == 0)
    {
        PlxPrintf("Ok\n");
    }
    else if (Stat_Errors < 10)
    {
        PlxPrintf(
            "     DCmp: %d errors found\n",
            Stat_Errors
            );
    }


    // Release the buffer
    PlxPrintf("  Release DMA buffer............. ");
    free(
        pDmaBuffer
        );
    PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
TestDmaControlStatus(
    HANDLE hDevice
    )
{
    PlxPrintf("\nPlxDmaCntrl/Status():\n");

    if (ChipTypeSelected == 0x9050)
    {
        TestDmaControlStatus9050(
            hDevice
            );

        return;
    }

    if (ChipTypeSelected == 0x9030)
    {
        TestDmaControlStatus9030(
            hDevice
            );

        return;
    }

    PlxPrintf("  ** DMA Control & Status are not tested for this chip **\n");
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
