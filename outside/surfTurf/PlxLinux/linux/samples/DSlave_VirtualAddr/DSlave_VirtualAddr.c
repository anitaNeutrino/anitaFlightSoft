/*******************************************************************************
 * Copyright (c) 2002 PLX Technology, Inc.
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
 *      DSlave_VirtualAddr.c
 *
 * Description:
 *
 *      The "Direct Slave" sample application, which demonstrates how to perform
 *      a transfer using the virtual address obtained with the PLX API.
 *
 * Revision History:
 *
 *      05-31-02 : PCI SDK v3.50
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
*               Definitions
**********************************************/
#define SIZE_BUFFER         0x100           // Number of bytes to transfer




/**********************************************
*               Functions
**********************************************/
void
TestDirectSlave(
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
    PlxPrintf("\t\t PLX Direct Slave Virtual Address Sample App\n");
    PlxPrintf("\t\t               June 4, 2002\n\n");


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
    TestDirectSlave(
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




/******************************************************************************
 *
 * Function   :  TestDirectSlave
 *
 * Description:  Performs a direct slave transfer using the PlxBusIopXxx API
 *
 *****************************************************************************/
void
TestDirectSlave(
    HANDLE hDevice
    )
{
    U8   index;
    U8   Revision;
    U8  *pBufferSrc;
    U8  *pBufferDest;
    U32  i;
    U32  ChipType;
    U32  VaPciBar;
    U32  Offset;


    PlxPrintf(
        "\n"
        " WARNING: This sample does not adjust remap registers.  It will\n"
        "          access the local bus based on the current remap setting.\n"
        "          System crashes will result if an invalid address is\n"
        "          accessed.  This sample is for reference purposes only.\n"
        );

    PlxPrintf("\n\n");
    PlxPrintf("Please enter a valid offset --> ");
    Plx_scanf("%x", &Offset);

    // Test an absolute address with remapping
    PlxPrintf(
        "\n  Without Remapping: offset = 0x%08x (%d bytes)\n",
        Offset, SIZE_BUFFER
        );

    PlxChipTypeGet(
        hDevice,
        &ChipType,
        &Revision
        );

    // Setup parameters for test
    switch (ChipType)
    {
        case 0x9050:
            index = 3;
            break;

        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            index = 2;
            break;

        default:
            PlxPrintf("\nERROR:  Undefined PLX Chip type\n");
            return;
    }

    PlxPrintf("    Get Virtual Addr of space.... ");
    PlxPciBarMap(
        hDevice,
        index,
        &VaPciBar
        );

    // Verify virtual address
    if (VaPciBar == (U32)-1 || VaPciBar == (U32)NULL)
    {
        PlxPrintf("*ERROR* - Invalid VA (%08Xh)\n", VaPciBar);
        return;
    }
    PlxPrintf("Ok (%08Xh)\n", VaPciBar);


    PlxPrintf("    Allocate buffers............. ");
    pBufferDest = (U8*)malloc(SIZE_BUFFER);
    if (pBufferDest == NULL)
    {
        PlxPrintf("*ERROR* - Destination buffer allocation failed\n");
        return;
    }

    pBufferSrc = (U8*)malloc(SIZE_BUFFER);
    if (pBufferSrc == NULL)
    {
        PlxPrintf("*ERROR* - Source buffer allocation failed\n");
        return;
    }
    PlxPrintf("Ok\n");

    PlxPrintf("    Preparing buffer data........ ");
    for (i=0; i < SIZE_BUFFER; i += sizeof(U32))
        *(U32*)(pBufferSrc + i) = i;
    PlxPrintf("Ok\n");


    PlxPrintf("    Write 8-bit data to local.... ");
    memcpy(
        (void *)(VaPciBar + Offset),
        pBufferSrc,
        SIZE_BUFFER
        );
    PlxPrintf("Ok\n");


    PlxPrintf("    Read 8-bit data from local... ");
    memcpy(
        pBufferDest,
        (void *)(VaPciBar + Offset),
        SIZE_BUFFER
        );
    PlxPrintf("Ok\n");


    PlxPrintf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        PlxPrintf("*ERROR*  -  Buffers do not match\n");
    }
    else
    {
        PlxPrintf("Ok\n");
    }


    PlxPrintf("    Write 32-bit data to local... ");
    for (i=0; i<SIZE_BUFFER; i += sizeof(U32))
    {
        *(U32*)(VaPciBar + Offset + i) = *(U32*)(pBufferSrc + i);
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Read 32-bit data from local.. ");
    for (i=0; i<SIZE_BUFFER; i += sizeof(U32))
    {
        *(U32*)(pBufferDest + i) = *(U32*)(VaPciBar + Offset + i);
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        PlxPrintf("*ERROR*  -  Buffers do not match\n");
    }
    else
    {
        PlxPrintf("Ok\n");
    }


    PlxPrintf("    Freeing buffers.............. ");
    if (pBufferDest != NULL)
        free(pBufferDest);

    if (pBufferSrc != NULL)
        free(pBufferSrc);
    PlxPrintf("Ok\n");


    PlxPrintf("    Unmap PCI Space.............. ");
    PlxPciBarUnmap(
        hDevice,
        &VaPciBar
        );
    PlxPrintf("Ok\n");
}
