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
 *      DSlave.c
 *
 * Description:
 *
 *      The "Direct Slave" sample application, which demonstrates how to perform
 *      a transfer using the PLX API.
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
    PlxPrintf("\t\t   PLX Direct Slave Sample Application\n");
    PlxPrintf("\t\t            March 31, 2002\n\n");


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
    U8           Revision;
    U32          i;
    U32          ChipType;
    U32          LocalAddress;
    U32         *pBufferDest;
    U32         *pBufferSrc;
    IOP_SPACE    IopSpace;
    RETURN_CODE  rc;


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

    PlxChipTypeGet(
        hDevice,
        &ChipType,
        &Revision
        );

    // Setup parameters for test
    switch (ChipType)
    {
        case 0x9050:
            IopSpace = IopSpace2;
            break;

        case 0x9030:
        case 0x9080:
        case 0x9054:
        case 0x9056:
        case 0x9656:
            IopSpace = IopSpace0;
            break;

        default:
            PlxPrintf("\nERROR:  Undefined PLX Chip type\n");
            return;
    }

    // First test without remapping
    PlxPrintf(
        "  Without Remapping: Space %d, 32-bit, offset = 0\n",
        IopSpace
        );

    PlxPrintf("    Preparing buffers............ ");
    pBufferDest = (U32*)malloc(SIZE_BUFFER);
    if (pBufferDest == NULL)
    {
        PlxPrintf("*ERROR* - Destination buffer allocation failed\n");
        return;
    }

    pBufferSrc  = (U32*)malloc(SIZE_BUFFER);
    if (pBufferSrc == NULL)
    {
        PlxPrintf("*ERROR* - Source buffer allocation failed\n");
        return;
    }
    PlxPrintf("Ok\n");

    PlxPrintf("    Preparing buffer data........ ");
    for (i=0; i < (SIZE_BUFFER >> 2); i++)
        pBufferSrc[i] = i;

    // Clear destination buffer
    memset(
        pBufferDest,
        0,
        SIZE_BUFFER
        );
    PlxPrintf("Ok\n");


    PlxPrintf("    Writing Data to Local Bus.... ");
    rc =
        PlxBusIopWrite(
            hDevice,
            IopSpace,
            0x0,
            FALSE,             // No Re-map
            pBufferSrc,
            SIZE_BUFFER,
            BitSize32
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Write failed, code = %s\n", PlxSdkErrorText(rc));
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Reading Data from Local Bus.. ");
    rc =
        PlxBusIopRead(
            hDevice,
            IopSpace,
            0x0,
            FALSE,               // No Re-map
            pBufferDest,
            SIZE_BUFFER,
            BitSize32
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Read failed, code = %s\n", PlxSdkErrorText(rc));
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Verifying data............... ");
    if (memcmp(
            pBufferSrc,
            pBufferDest,
            SIZE_BUFFER
            ) != 0)
    {
        PlxPrintf("*ERROR* - Buffers do not match\n");
        return;
    }
    PlxPrintf("Ok\n");



    // Now test an absolute address with remapping
    PlxPrintf(
        "\n  With Remapping: Space %d, 8-bit, address = 0x%08x\n",
        IopSpace, LocalAddress
        );


    PlxPrintf("    Preparing buffers............ ");
    memset(
        pBufferDest,
        0,
        SIZE_BUFFER
        );
    PlxPrintf("Ok\n");


    PlxPrintf("    Writing Data to Local Bus.... ");
    rc =
        PlxBusIopWrite(
            hDevice,
            IopSpace,
            LocalAddress,
            TRUE,              // Re-map local window
            pBufferSrc,
            SIZE_BUFFER,
            BitSize8
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Write failed, code = %s\n", PlxSdkErrorText(rc));
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Reading Data from Local Bus.. ");
    rc =
        PlxBusIopRead(
            hDevice,
            IopSpace,
            LocalAddress,
            TRUE,              // Re-map local window
            pBufferDest,
            SIZE_BUFFER,
            BitSize8
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf("*ERROR* - Read failed, code = %s\n", PlxSdkErrorText(rc));
        return;
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
        return;
    }
    PlxPrintf("Ok\n");


    PlxPrintf("    Freeing buffers.............. ");
    if (pBufferDest != NULL)
        free(pBufferDest);

    if (pBufferSrc != NULL)
        free(pBufferSrc);
    PlxPrintf("Ok\n");
}
