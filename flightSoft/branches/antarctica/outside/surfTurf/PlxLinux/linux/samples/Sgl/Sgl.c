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
 *      Sgl.c
 *
 * Description:
 *
 *      The "SGL" sample application, which demonstrates how to
 *      transfer a user-mode buffer using the PLX API.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
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




/**********************************************
*               Functions
**********************************************/
void 
TestSglDma(
    HANDLE hDevice
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
    HANDLE          hDevice;
    RETURN_CODE     rc;
    DEVICE_LOCATION Device;


    ConsoleInitialize();

    Cls();

    PlxPrintf("\n\n");
    PlxPrintf("\t\t     PLX SGL DMA Sample Application\n");
    PlxPrintf("\t\t              June 2002\n");


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
    TestSglDma(
        hDevice
        );




    /************************************
    *        Close the Device
    ************************************/
    PlxPciDeviceClose(
        hDevice
        );

    _Pause;

    PlxPrintf("\n\n");

    ConsoleEnd();

    exit(0);
}




/********************************************************
*
********************************************************/
void
TestSglDma(
    HANDLE hDevice
    )
{
    U8                    DmaBuffer[0x20];
    U8                    Revision;
    U32                   ChipType;
    U32                   LocalAddress;
    HANDLE                hInterruptEvent;
    PLX_INTR              PlxInterrupt;
    RETURN_CODE           rc;
    DMA_CHANNEL_DESC      DmaDesc;
    DMA_TRANSFER_ELEMENT  DmaData;


    PlxChipTypeGet(
        hDevice,
        &ChipType,
        &Revision
        );

    if (ChipType == 0x9050 ||
        ChipType == 0x9030)
    {
        PlxPrintf("ERROR: DMA not supported by the selected device\n");
        return;
    }


    PlxPrintf(
        "Description:\n"
        "     This sample will demonstrate the SGL DMA API function.\n"
        "     It will transfer data from the local-side to a user-mode\n"
        "     buffer and wait for the DMA interrupt.\n"
        );

    PlxPrintf(
        "\n"
        " WARNING: There is no safeguard mechanism to protect against invalid\n"
        "          local bus addresses.  Please be careful when selecting local\n"
        "          addresses to transfer data to/from.  System crashes will result\n"
        "          if an invalid address is accessed.\n"
        "\n\n"
        );


    PlxPrintf("Please enter a valid local address --> ");
    Plx_scanf("%x", &LocalAddress);
    PlxPrintf("\n");

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
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - Unable to open DMA, rc=%s\n",  PlxSdkErrorText(rc));


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
            "*ERROR* - PlxIntrAttach() failed, rc=%s\n",
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

    
    PlxPrintf("  Transferring DMA SGL........... ");
    DmaData.u.UserVa          = (U32)DmaBuffer;     // Virtual address of buffer
    DmaData.PciAddrHigh       = 0;                  // Not used, but clear in case
    DmaData.LocalAddr         = LocalAddress;       // Local bus address
    DmaData.TransferCount     = sizeof(DmaBuffer);
    DmaData.LocalToPciDma     = 1;
    DmaData.TerminalCountIntr = 0;

    rc =
        PlxDmaSglTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaData,
            TRUE                 // Don't wait for completion
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
                PlxPrintf("*ERROR* - API failed (rc=%s)\n", PlxSdkErrorText(rc));
                break;
        }
    }
    else
    {
        PlxPrintf(
            "*ERROR* - SGL DMA tranfer failed, rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }


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
            "*ERROR* - Unable to close DMA channel, rc=%s\n",
            PlxSdkErrorText(rc)
            );
    }
}
