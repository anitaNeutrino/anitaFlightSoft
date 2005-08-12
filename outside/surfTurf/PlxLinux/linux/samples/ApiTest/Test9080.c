/******************************************************************************
 *
 * File Name:
 *
 *      Test9080.c
 *
 * Description:
 *
 *      Chip dependent API Tests
 *
 ******************************************************************************/


#include <memory.h>
#include "ConsFunc.h"
#include "PlxApi.h"
#include "PlxInit.h"
#include "Test9080.h"




/********************************************************
*
********************************************************/
void
TestPowerLevel9080(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    RETURN_CODE      rc;
    PLX_POWER_LEVEL  PowerLevel;


    PlxPrintf("  Setting Power Level to D0...... ");
    rc =
        PlxPowerLevelSet(
            hDevice,
            D0
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
        PlxPrintf("Ok\n");
    }


    PlxPrintf("  Setting Power Level to D2...... ");
    rc =
        PlxPowerLevelSet(
            hDevice,
            D2
            );

    if (rc == ApiInvalidPowerState)
        PlxPrintf("Ok (ApiInvalidPowerState returned)\n");
    else
    {
        PlxPrintf(
            "*ERROR* - ApiInvalidPowerState not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Getting Power Level............ ");
    PowerLevel =
        PlxPowerLevelGet(
            hDevice,
            &rc
            );

    if (rc == ApiSuccess)
    {
        if (PowerLevel == D0)
            PlxPrintf("Ok (Power Level = D0)\n");
        else
            PlxPrintf("*ERROR* - Power Level != D0\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - Unable to get Power Level, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestInterruptEnable9080(
    HANDLE hDevice
    )
{
    U32          RegValue;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    // Clear Interrupt fields
    memset(&PlxInterrupt, 0, sizeof(PLX_INTR));

    PlxPrintf("  Enabling DMA 0 Interrupt....... ");
    PlxInterrupt.PciDmaChannel0 = 1;
    rc =
        PlxIntrEnable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrEnable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");


    PlxPrintf("  Verifying DMA 0 Int enabled.... ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9080_INT_CTRL_STAT,
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

    if (RegValue & (1 << 18))
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - DMA 0 not enabled (bit 18)\n");

    // Clear interrupt field
    PlxInterrupt.PciDmaChannel0 = 0;


    PlxPrintf("  Enabling PCI Doorbell Int...... ");
    PlxInterrupt.PciDoorbell = 1;
    rc =
        PlxIntrEnable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrEnable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");

    PlxPrintf("  Verifying PCI Doorbell enabled. ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9080_INT_CTRL_STAT,
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

    if (RegValue & (1 << 9))
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - PCI Doorbell not enabled (bit 9)\n");

    // Clear interrupt field
    PlxInterrupt.PciDoorbell = 0;


    PlxPrintf("  Enabling PCI Interrupt......... ");
    PlxInterrupt.PciMainInt = 1;
    rc =
        PlxIntrEnable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrEnable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");

    // Clear interrupt field
    PlxInterrupt.PciMainInt = 0;


    PlxPrintf("  Clearing all Interrupts........ ");
    PlxInterrupt.PciDmaChannel0 = 1;
    PlxInterrupt.PciDoorbell    = 1;
    rc =
        PlxIntrDisable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrDisable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");

    PlxPrintf("  Verifying Ints are disabled.... ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9080_INT_CTRL_STAT,
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

    if (RegValue & ((1 << 18) | (1 << 9)))
    {
        PlxPrintf("*ERROR* - Ints are enabled, (val=0x%08x)\n", RegValue);
    }
    else
        PlxPrintf("Ok\n");
}
