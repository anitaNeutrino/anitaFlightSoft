/******************************************************************************
 *
 * File Name:
 *
 *      Test9054.c
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
#include "Test9054.h"




/********************************************************
*
********************************************************/
void
TestPowerLevel9054(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    )
{
    U32              RegValue;
    BOOLEAN          TestFinished;
    RETURN_CODE      rc;
    RETURN_CODE      rcReg;
    PLX_POWER_LEVEL  PowerLevel;
    PLX_POWER_LEVEL  PowerLevelStatus;


    TestFinished = FALSE;
    PowerLevel   = D0Uninitialized;
    do
    {
        rc =
            PlxPowerLevelSet(
                hDevice,
                PowerLevel
                );

        RegValue =
            PlxPciConfigRegisterRead(
                pDev->BusNumber,
                pDev->SlotNumber,
                PCI9054_PM_CSR,
                &rcReg
                );

        switch (PowerLevel)
        {
            case D0Uninitialized:
                PlxPrintf("  Setting to D0Uninitialized..... ");
                if (rc != ApiInvalidPowerState)
                {
                    PlxPrintf(
                        "*ERROR* - InvalidPowerState not ret, rc=%d\n",
                        rc
                        );
                }
                else
                {
                    PlxPrintf(
                        "Ok (ApiInvalidPowerState returned)\n"
                        );
                }
                break;

            case D0:
                PlxPrintf("  Setting to D0.................. ");
                if (rc != ApiSuccess)
                {
                    PlxPrintf(
                        "*ERROR* - rc = %s\n",
                        PlxSdkErrorText(rc)
                        );
                }
                else
                {
                    PlxPrintf("Ok\n");
                    PlxPrintf("  Verifying D0 state............. ");
                    PowerLevelStatus =
                        PlxPowerLevelGet(
                            hDevice,
                            &rc
                            );

                    if (rc == ApiSuccess)
                    {
                        if (PowerLevelStatus != D0)
                        {
                            PlxPrintf(
                                "*ERROR* - D0 not returned, state=%d\n",
                                PowerLevelStatus
                                );
                        }
                        else
                        {
                            if ((RegValue & (0x3)) != 0)
                            {
                                PlxPrintf(
                                    "*ERROR* - Reg (0x%08X) doesn't reflect D0\n",
                                    RegValue
                                    );
                            }
                            else
                            {
                                PlxPrintf("Ok\n");
                            }
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - Unable to get Power Level, code=%d\n",
                            rc
                            );
                    }
                }
                break;

            case D1:
                PlxPrintf("  Setting to D1.................. ");
                if (rc != ApiSuccess)
                {
                    if (rc == ApiInvalidPowerState)
                    {
                        if ((RegValue & (1 << 25)) == 0)
                        {
                            PlxPrintf(
                                "Ok (Chip reports D1 not supported)\n"
                                );
                        }
                        else
                        {
                            PlxPrintf(
                                "*ERROR* - rc = ApiInvalidPowerState\n"
                                );
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - rc = %s\n",
                            PlxSdkErrorText(rc)
                            );
                    }
                }
                else
                {
                    PlxPrintf("Ok\n");
                    PlxPrintf("  Verifying D1 state............. ");
                    PowerLevelStatus =
                        PlxPowerLevelGet(
                            hDevice,
                            &rc
                            );

                    if (rc == ApiSuccess)
                    {
                        if (PowerLevelStatus != D1)
                        {
                            PlxPrintf(
                                "*ERROR* - D1 not returned, state=%d\n",
                                PowerLevelStatus
                                );
                        }
                        else
                        {
                            if ((RegValue & (0x3)) != 1)
                            {
                                PlxPrintf(
                                    "*ERROR* - Reg (0x%08X) doesn't reflect D1\n",
                                    RegValue
                                    );
                            }
                            else
                            {
                                PlxPrintf("Ok\n");
                            }
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - Unable to get Power Level, code=%d\n",
                            rc
                            );
                    }
                }
                break;

            case D2:
                PlxPrintf("  Setting to D2.................. ");
                if (rc != ApiSuccess)
                {
                    if (rc == ApiInvalidPowerState)
                    {
                        if ((RegValue & (1 << 26)) == 0)
                        {
                            PlxPrintf(
                                "Ok (Chip reports D2 not supported)\n"
                                );
                        }
                        else
                        {
                            PlxPrintf(
                                "*ERROR* - rc = ApiInvalidPowerState\n"
                                );
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - rc = %s\n",
                            PlxSdkErrorText(rc)
                            );
                    }
                }
                else
                {
                    PlxPrintf("Ok\n");
                    PlxPrintf("  Verifying D2 state............. ");
                    PowerLevelStatus =
                        PlxPowerLevelGet(
                            hDevice,
                            &rc
                            );

                    if (rc == ApiSuccess)
                    {
                        if (PowerLevelStatus != D2)
                        {
                            PlxPrintf(
                                "*ERROR* - D2 not returned, state=%d\n",
                                PowerLevelStatus
                                );
                        }
                        else
                        {
                            if ((RegValue & (0x3)) != 2)
                            {
                                PlxPrintf(
                                    "*ERROR* - Reg (0x%08X) doesn't reflect D2\n",
                                    RegValue
                                    );
                            }
                            else
                                PlxPrintf("Ok\n");
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - Unable to get Power Level, code=%d\n",
                            rc
                            );
                    }
                }
                break;

            case D3Hot:
                PlxPrintf("  Setting to D3Hot............... ");
                if (rc != ApiSuccess)
                {
                    PlxPrintf(
                        "*ERROR* - rc = %s\n",
                        PlxSdkErrorText(rc)
                        );
                }
                else
                {
                    PlxPrintf("Ok\n");
                    PlxPrintf("  Verifying D3Hot state.......... ");
                    PowerLevelStatus =
                        PlxPowerLevelGet(
                            hDevice,
                            &rc
                            );

                    if (rc == ApiSuccess)
                    {
                        if (PowerLevelStatus != D3Hot)
                        {
                            PlxPrintf(
                                "*ERROR* - D3Hot not returned, state=%d\n",
                                PowerLevelStatus
                                );
                        }
                        else
                        {
                            if ((RegValue & (0x3)) != 3)
                            {
                                PlxPrintf(
                                    "*ERROR* - Reg (0x%08X) doesn't reflect D3\n",
                                    RegValue
                                    );
                            }
                            else
                                PlxPrintf("Ok\n");
                        }
                    }
                    else
                    {
                        PlxPrintf(
                            "*ERROR* - Unable to get Power Level, rc=%d\n",
                            rc
                            );
                    }
                }
                break;

            case D3Cold:
                PlxPrintf("  Setting to D3Cold.............. ");
                if (rc != ApiInvalidPowerState)
                {
                    PlxPrintf(
                        "*ERROR* - InvalidPowerState not ret, rc=%d\n",
                        rc
                        );
                }
                else
                {
                    PlxPrintf(
                        "Ok (ApiInvalidPowerState returned)\n"
                        );
                }

                TestFinished = TRUE;
                break;
        }

        // Increment to next Power State
        PowerLevel++;
    }
    while (TestFinished == FALSE);

    PlxPrintf("  Restoring to full power (D0)... ");
    PlxPowerLevelSet(
        hDevice,
        D0
        );
    PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
TestInterruptEnable9054(
    HANDLE hDevice
    )
{
    U32          RegValue;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    // Clear Interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

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
            PCI9054_INT_CTRL_STAT,
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
            PCI9054_INT_CTRL_STAT,
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
            PCI9054_INT_CTRL_STAT,
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
        PlxPrintf(
            "*ERROR* - Ints are enabled, (val=0x%08x)\n",
            RegValue
            );
    }
    else
        PlxPrintf("Ok\n");
}
