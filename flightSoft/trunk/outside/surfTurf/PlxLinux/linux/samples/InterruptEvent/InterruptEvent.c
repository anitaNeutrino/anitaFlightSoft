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
 *      InterruptEvent.c
 *
 * Description:
 *
 *      The "Interrupt Event" sample application, which demonstrates how to
 *      register and wait for specific interrupt events using the PLX API.
 *
 * Revision History:
 *
 *      05-01-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "PlxApi.h"

#if defined(_WIN32)
    #include "..\\Common\\ConsFunc.h"
    #include "..\\Common\\PlxInit.h"
#endif

#if defined(PLX_LINUX)
    #include "ConsFunc.h"
    #include "PlxInit.h"
#endif

#include "Reg9030.h"    // Used for triggering local interrupt on these chips
#include "Reg9050.h"




/**********************************************
*               Globals
**********************************************/
U32 ChipType;




/**********************************************
*               Functions
**********************************************/
void
TestAttachDma(
    HANDLE hDevice
    );

void
TestAttachLocalInt(
    HANDLE hDevice
    );

void
PlxSdkErrorDisplay(
    RETURN_CODE code
    );




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
    S8              DeviceSelected;
    U8              Revision;
    HANDLE          hDevice;
    RETURN_CODE     rc;
    DEVICE_LOCATION Device;


    ConsoleInitialize();

    Cls();

    PlxPrintf("\n\n");
    PlxPrintf("\t\t   PLX Interrupt Attach Sample Application\n");
    PlxPrintf("\t\t                   April 2003\n");


    /************************************
    *         Select Device
    ************************************/
    DeviceSelected =
        SelectDevice(
            &Device
            );

    if (DeviceSelected == -1)
    {
        ConsoleEnd();
        exit(0);
    }

    rc =
        PlxPciDeviceOpen(
            &Device,
            &hDevice
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("\n   ERROR: Unable to find or select a PLX device\n");

        _Pause;

        ConsoleEnd();

        exit(-1);
    }

    PlxPrintf(
        "\nSelected: %.4x %.4x [bus %.2x  slot %.2x]\n\n",
        Device.DeviceId, Device.VendorId,
        Device.BusNumber, Device.SlotNumber
        );



    /************************************
    *        Perform the Test
    ************************************/
    PlxChipTypeGet(
        hDevice,
        &ChipType,
        &Revision
        );

    switch (ChipType)
    {
        case 0x9030:
        case 0x9050:
            TestAttachLocalInt(
                hDevice
                );
            break;

        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            TestAttachDma(
                hDevice
                );
            break;

        default:
            PlxPrintf("\nERROR:  Undefined PLX Chip type\n");
            break;
    }




    /************************************
    *        Close the Device
    ************************************/
    PlxPciDeviceClose(
        hDevice
        );

    PlxPrintf("\n");

    _Pause;

    PlxPrintf("\n\n");

    ConsoleEnd();

    exit(0);
}




/********************************************************
*
********************************************************/
void
TestAttachDma(
    HANDLE hDevice
    )
{
    U32                   DmaIndex;
    U32                   LocalAddress;
    HANDLE                hInterruptEvent;
    PLX_INTR              PlxInterrupt;
    PCI_MEMORY            PciBuffer;
    RETURN_CODE           rc;
    DMA_CHANNEL           DmaChannel;
    DMA_CHANNEL_DESC      DmaDesc;
    DMA_TRANSFER_ELEMENT  DmaData;


    PlxPrintf(
        "Description:\n"
        "     This sample will test the Interrupt Attach API by\n"
        "     initiating a Block DMA transfer and waiting for the\n"
        "     DMA interrupt.\n"
        );

    PlxPrintf(
        "\n"
        " WARNING: There is no safeguard mechanism to protect against invalid\n"
        "          local bus addresses.  Please be careful when selecting local\n"
        "          addresses to transfer data to/from.  System crashes will result\n"
        "          if an invalid address is accessed.\n"
        "\n\n"
        );

    PlxPrintf("Please enter a valid local address ----> ");
    Plx_scanf("%x", &LocalAddress);

    PlxPrintf("Please select a DMA channel (0 or 1) --> ");
    Plx_scanf("%x", &DmaIndex);
    PlxPrintf("\n");

    if (DmaIndex == 0)
        DmaChannel = PrimaryPciChannel0;
    else if (DmaIndex == 1)
        DmaChannel = PrimaryPciChannel1;
    else
    {
        PlxPrintf("ERROR: Unsupported DMA channel, test aborted\n");
        return;
    }

    // Get DMA buffer parameters
    PlxPciCommonBufferProperties(
        hDevice,
        &PciBuffer
        );


    // Initialize the DMA channel
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
    DmaDesc.IopBusWidth              = 2;   // 32-bit
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

    PlxPrintf("  Opening Block DMA Channel %i.... ", DmaIndex);
    rc =
        PlxDmaBlockChannelOpen(
            hDevice,
            DmaChannel,
            &DmaDesc
            );

    if (rc == ApiSuccess)
        PlxPrintf("Ok\n");
    else
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }


    PlxPrintf("  Register for notification...... ");

    // Clear interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    // Setup to wait for either DMA channel
    PlxInterrupt.PciDmaChannel0 = 1;
    PlxInterrupt.PciDmaChannel1 = 1;

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


    // Transfer the Data
    PlxPrintf("  Transferring DMA Block......... ");
    DmaData.u.PciAddrLow      = PciBuffer.PhysicalAddr;
    DmaData.PciAddrHigh       = 0x0;
    DmaData.LocalAddr         = LocalAddress;
    DmaData.TransferCount     = 0x10;
    DmaData.LocalToPciDma     = 1;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaBlockTransfer(
            hDevice,
            DmaChannel,
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
                PlxPrintf("Ok (DMA Int received)\n");
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
    PlxPrintf("  Closing Block DMA Channel...... ");
    rc =
        PlxDmaBlockChannelClose(
            hDevice,
            DmaChannel
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
TestAttachLocalInt(
    HANDLE hDevice
    )
{
    U16          OffsetRegInt;
    U32          RegisterValue;
    HANDLE       hInterruptEvent;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    PlxPrintf(
        "Description:\n"
        "     This sample will test the Interrupt Attach API by manually triggering\n"
        "     the Software interrupt of the PLX chip.  This is a feature of the PLX\n"
        "     chip which forces the device to generate a generic PCI interrupt.\n"
        "\n\n"
        );

    _Pause;

    switch (ChipType)
    {
        case 0x9030:
            OffsetRegInt = PCI9030_INT_CTRL_STAT;
            break;

        case 0x9050:
            OffsetRegInt = PCI9050_INT_CTRL_STAT;
            break;

        default:
            PlxPrintf("\nERROR:  Undefined PLX Chip type\n");
            return;
    }

    // Clear Interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxPrintf("  Enable the PCI Interrupt........... ");
    PlxInterrupt.PciMainInt    = 1;
    rc =
        PlxIntrEnable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
        PlxPrintf("Ok\n");


    PlxPrintf("  Register for Int. notification..... ");

    // Clear interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    // Setup to wait for the Software interrupt
    PlxInterrupt.SwInterrupt = 1;
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


    // Manually trigger interrupt
    PlxPrintf("  Trigger & wait for Interrupt....... ");

    // Delay a bit before triggering interrupt
    Plx_Sleep_ms(500);

    // Get Interrupt Control/Status register
    RegisterValue =
        PlxRegisterRead(
            hDevice,
            OffsetRegInt,
            &rc
            );

    // Manually trigger Software interrupt
    PlxRegisterWrite(
        hDevice,
        OffsetRegInt,
        RegisterValue | (1 << 7)
        );

    // Wait for interrupt event
    rc =
        PlxIntrWait(
            hDevice,
            hInterruptEvent,
            10 * 1000
            );

    switch (rc)
    {
        case ApiSuccess:
            PlxPrintf("Ok (Software Int received)\n");
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

    // Clear interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    // Verify last interrupt received
    PlxPrintf("  Last interrupt reported............ ");
    rc =
        PlxIntrStatusGet(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - API failed\n");
        PlxSdkErrorDisplay(rc);
    }
    else
    {
        if (PlxInterrupt.SwInterrupt == 1)
        {
            PlxPrintf("Ok (Software Interrupt)\n");
        }
        else
        {
            PlxPrintf("*ERROR* - No interrupt reported\n");
        }
    }
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

