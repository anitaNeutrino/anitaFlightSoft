/******************************************************************************
 *
 * File Name:
 *
 *      Test9030.c
 *
 * Description:
 *
 *      Chip dependent API Tests
 *
 ******************************************************************************/


#include <memory.h>
#include "ApiTest.h"
#include "ConsFunc.h"
#include "PlxApi.h"
#include "Test9030.h"




/********************************************************
* Globals
********************************************************/
#if defined(_WIN32)
    static BOOLEAN mgEventCompleted_1;
    static BOOLEAN mgEventCompleted_2;
#endif




/********************************************************
*
********************************************************/
void
TestPowerLevel9030(
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
                PCI9030_PM_CSR,
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
                    PlxPrintf("Ok (ApiInvalidPowerState returned)\n");
                }
                break;

            case D0:
                PlxPrintf("  Setting to D0.................. ");
                if (rc != ApiSuccess)
                {
                    PlxPrintf("*ERROR* - Unable to set to D0, code=%d\n", rc);
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

            case D1:
                PlxPrintf("  Setting to D1.................. ");
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

            case D2:
                PlxPrintf("  Setting to D2.................. ");
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

            case D3Hot:
                PlxPrintf("  Setting to D3Hot............... ");
                if (rc != ApiSuccess)
                {
                    PlxPrintf(
                        "*ERROR* - Unable to set to D3Hot, rc=%d\n",
                        rc
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
                    PlxPrintf("Ok (ApiInvalidPowerState returned)\n");
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
TestInterruptEnable9030(
    HANDLE hDevice
    )
{
    U32          RegValue;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    // Check polarity setting
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
            &rc
            );

    if ((RegValue & (1 << 2)) || (RegValue & (1 << 5)))
    {
        // Reverse Interrupt 1 Polarity
	    if (RegValue & (1 << 2))
		{
			RegValue = RegValue ^ (1 << 1);
			PlxRegisterWrite(
				hDevice,
				PCI9030_INT_CTRL_STAT,
				RegValue
				);
		}

        // Reverse Interrupt 2 Polarity
        if (RegValue & (1 << 5))
        {
			RegValue = RegValue ^ (1 << 4);
			PlxRegisterWrite(
                hDevice,
                PCI9030_INT_CTRL_STAT,
                RegValue
                );
        }
    }

    // Clear Interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxPrintf("  Enabling Local Interrupt 1..... ");
    PlxInterrupt.IopToPciInt = 1;
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


    PlxPrintf("  Verifying Local Int 1 enabled.. ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
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

    if (RegValue & (1 << 0))
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - Int 1 not enabled (bit 0)\n");

    // Clear interrupt field
    PlxInterrupt.IopToPciInt = 0;


    PlxPrintf("  Enabling Local Interrupt 2..... ");
    PlxInterrupt.IopToPciInt_2 = 1;
    rc =
        PlxIntrEnable(
            hDevice,
            &PlxInterrupt
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - IntrEnable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");

    PlxPrintf("  Verifying Local Int 2 enabled.. ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
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

    if (RegValue & (1 << 3))
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - Interrupt 2 not enabled (bit 3)\n");

    // Clear interrupt field
    PlxInterrupt.IopToPciInt_2 = 0;


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
            "*ERROR* - IntrEnable() failed, code = %d\n",
            rc
            );
    }
    else
        PlxPrintf("Ok\n");

    PlxPrintf("  Verifying PCI Int is enabled... ");
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
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

    if (RegValue & (1 << 6))
        PlxPrintf("Ok\n");
    else
        PlxPrintf("*ERROR* - PCI Interrupt not enabled (bit 6)\n");

    // Clear interrupt field
    PlxInterrupt.PciMainInt = 0;


    PlxPrintf("  Clearing all Interrupts........ ");
    PlxInterrupt.IopToPciInt   = 1;
    PlxInterrupt.IopToPciInt_2 = 1;
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
            PCI9030_INT_CTRL_STAT,
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

    if (RegValue & ((1 << 3) | (1 << 0)))
    {
        PlxPrintf(
            "*ERROR* - Ints are enabled, (val=0x%08x)\n",
            RegValue
            );
    }
    else
        PlxPrintf("Ok\n");
}




/********************************************************
*
********************************************************/
void
TestInterruptAttach9030(
    HANDLE hDevice
    )
{
#if defined(_WIN32)
    U32          RegValue;
    DWORD        pThreadId;
    HANDLE       ThreadHandle_1;
    HANDLE       ThreadHandle_2;
    BOOLEAN      InterruptReported;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    // Clear Interrupt fields
    memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    // Disable PCI interrupt
    PlxInterrupt.PciMainInt = 1;
    PlxIntrDisable(
        hDevice,
        &PlxInterrupt
        );

    // Get Interrupt Control/Status register
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
            &rc
			);

    // Verify polarity of Int 1
    if (RegValue & (1 << 2))
    {
        // Int active, reverse polarity so not triggering interrupt
        RegValue ^= (1 << 1);
    }

    // Verify polarity of Int 2
    if (RegValue & (1 << 5))
    {
        // Int active, reverse polarity so not triggering interrupt
        RegValue ^= (1 << 4);
    }

    // Update register
    PlxRegisterWrite(
        hDevice,
        PCI9030_INT_CTRL_STAT,
        RegValue
        );

    PlxPrintf("  Enable Local 1,2 & PCI Ints.... ");
    PlxInterrupt.PciMainInt    = 1;
    PlxInterrupt.IopToPciInt   = 1;
    PlxInterrupt.IopToPciInt_2 = 1;
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


    PlxPrintf("  Spawning Attach thread 1....... ");

    mgEventCompleted = FALSE;
    ThreadHandle_1 =
        CreateThread(
            NULL,                          // Security
            0,                             // Same stack size
            InterruptAttachThread9030_1,   // Thread start routine
            (LPVOID)hDevice,               // Handle parameter
            0,                             // Creation flags
            &pThreadId                     // Thread ID
            );

    if (ThreadHandle_1 == NULL)
    {
        PlxPrintf("*ERROR* - CreateThread() failed\n");
        return;
    }

    // Delay to let thread 1 start
    Sleep(500);

    PlxPrintf("  Spawning Attach thread 2....... ");

    mgEventCompleted_2 = FALSE;
    ThreadHandle_2 =
        CreateThread(
            NULL,                          // Security
            0,                             // Same stack size
            InterruptAttachThread9030_2,   // Thread start routine
            (LPVOID)hDevice,               // Handle parameter
            0,                             // Creation flags
            &pThreadId                     // Thread ID
            );

    if (ThreadHandle_2 == NULL)
    {
        PlxPrintf("*ERROR* - CreateThread() failed\n");
        return;
    }

    // Delay to let thread 2 start
    Sleep(500);

    PlxPrintf("  Waiting for Interrupt Events... ");


    // Delay a bit before triggering interrupt
    Sleep(500);

    // Get Interrupt Control/Status register
    RegValue =
        PlxRegisterRead(
            hDevice,
            PCI9030_INT_CTRL_STAT,
            &rc
			);

    // Write to polarity to trigger interrupt 1
    PlxRegisterWrite(
        hDevice,
        PCI9030_INT_CTRL_STAT,
        RegValue ^ (1 << 1)
        );

    // Delay for a bit
    Sleep(300);

    // Write to polarity to trigger interrupt 2
    PlxRegisterWrite(
        hDevice,
        PCI9030_INT_CTRL_STAT,
        RegValue ^ (1 << 4)
        );


    while ((mgEventCompleted_1 == FALSE) ||
           (mgEventCompleted_2 == FALSE))
    {
        Sleep(1);
    }

    PlxPrintf("\n");


    // Restore Polarity bits
    PlxRegisterWrite(
        hDevice,
        PCI9030_INT_CTRL_STAT,
        RegValue
        );


    // Clear interrupt fields
	memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

	// Verify we received a Local 1 Interrupt
	PlxPrintf("  Last interrupt reported........ ");
	rc =
        PlxIntrStatusGet(
        hDevice,
        &PlxInterrupt
        );

    if (rc != ApiSuccess)
    {
	    PlxPrintf(
            "*ERROR* - PlxIntrStatusGet() failed, code = %d\n",
            rc
            );
    }
    else
    {
        InterruptReported = FALSE;

		if (PlxInterrupt.IopToPciInt == 1)
        {
			PlxPrintf("Local Interrupt 1");
            InterruptReported = TRUE;
        }

        if (PlxInterrupt.IopToPciInt_2 == 1)
        {
			PlxPrintf("Local Interrupt 2");
            InterruptReported = TRUE;
        }

        if (InterruptReported == FALSE)
            PlxPrintf("*ERROR* - No interrupt reported");

        PlxPrintf("\n");
	}
#endif
}




/********************************************************
*
********************************************************/
void
TestRegisterDoorbell9030(
    HANDLE hDevice
    )
{
    RETURN_CODE rc;


    PlxPrintf("  Reading Doorbell Register...... ");
    PlxRegisterDoorbellRead(
        hDevice,
        &rc
        );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf("*ERROR* - UnsupportedFunction not ret, rc=%d\n", rc);
    }


    PlxPrintf("  Writing Doorbell Register...... ");
    rc =
        PlxRegisterDoorbellSet(
            hDevice,
            0x0
            );
    if (rc == ApiUnsupportedFunction)
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestRegisterMailbox9030(
    HANDLE hDevice
    )
{
    RETURN_CODE  rc;


    PlxPrintf("  Reading Mailbox Register....... ");
    PlxRegisterMailboxRead(
        hDevice,
        MailBox3,
        &rc
        );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Writing Mailbox Register....... ");
    rc =
        PlxRegisterMailboxWrite(
            hDevice,
            MailBox3,
            0x0
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestBlockDma9030(
    HANDLE hDevice
    )
{
    RETURN_CODE          rc;
    DMA_CHANNEL_DESC     DmaDesc;
    DMA_TRANSFER_ELEMENT DmaElement;


    PlxPrintf("  Opening Block DMA Channel 0.... ");
    rc =
        PlxDmaBlockChannelOpen(
            hDevice,
            PrimaryPciChannel0,
            &DmaDesc
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Transferring DMA Block......... ");
    rc =
        PlxDmaBlockTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaElement,
            TRUE
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Closing Block DMA Channel 0.... ");
    rc =
        PlxDmaBlockChannelClose(
            hDevice,
            PrimaryPciChannel0
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestSglDma9030(
    HANDLE hDevice
    )
{
    RETURN_CODE          rc;
    DMA_CHANNEL_DESC     DmaDesc;
    DMA_TRANSFER_ELEMENT DmaElement;


    PlxPrintf("  Opening SGL DMA Channel 0...... ");
    rc =
        PlxDmaSglChannelOpen(
            hDevice,
            PrimaryPciChannel0,
            &DmaDesc
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Transferring DMA SGL........... ");
    rc =
        PlxDmaSglTransfer(
            hDevice,
            PrimaryPciChannel0,
            &DmaElement,
            TRUE
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Closing SGL DMA Channel 0...... ");
    rc =
        PlxDmaSglChannelClose(
            hDevice,
            PrimaryPciChannel0
            );

    if (rc == ApiUnsupportedFunction)
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }
}




/********************************************************
*
********************************************************/
void
TestDmaControlStatus9030(
    HANDLE hDevice
    )
{
    RETURN_CODE rc;


    PlxPrintf("  Issuing DmaStart Command....... ");
    rc =
        PlxDmaControl(
            hDevice,
            PrimaryPciChannel0,
            DmaStart
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }


    PlxPrintf("  Getting DMA Status............. ");
    rc =
        PlxDmaStatus(
            hDevice,
            PrimaryPciChannel0
            );

    if (rc == ApiUnsupportedFunction)
    {
        PlxPrintf("Ok (ApiUnsupportedFunction returned)\n");
    }
    else
    {
        PlxPrintf(
            "*ERROR* - UnsupportedFunction not ret, rc=%d\n",
            rc
            );
    }
}




#if defined(_WIN32)
/********************************************************
*
********************************************************/
DWORD WINAPI
InterruptAttachThread9030_1(
    LPVOID pParam
    )
{
    DWORD        EventStatus;
    HANDLE       hDevice;
    HANDLE       hInterruptEvent;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    PlxPrintf("Ok (Thread 1 started)\n");

    hDevice = (HANDLE)pParam;

    // Clear interrupt fields
	memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxPrintf("  Attach Event to Interrupt 1.... ");
    PlxInterrupt.PciMainInt  = 0;
    PlxInterrupt.IopToPciInt = 1;
    rc =
        PlxIntrAttach(
            hDevice,
            PlxInterrupt,
            &hInterruptEvent
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrAttach() failed, rc=%d\n",
            rc
            );
        mgEventCompleted = TRUE;
        return 0;
    }
    PlxPrintf(
        "Ok (hEvent=0x%08x)\n",
        hInterruptEvent
        );


    // Wait for interrupt event
    EventStatus =
        WaitForSingleObject(
            hInterruptEvent,
            10 * 1000
            );

    switch (EventStatus)
    {
        case WAIT_OBJECT_0:
            PlxPrintf("*Local Int 1 Rcvd*  ");
            break;

        case WAIT_TIMEOUT:
            PlxPrintf("*Local Int 1 Timeout* ");
            break;

        case WAIT_FAILED:
            PlxPrintf("*ERROR* - Failed while waiting for interrupt\n");
            break;

        default:
            PlxPrintf("*ERROR* - Unknown, EventStatus = 0x%08x\n", EventStatus);
            break;
    }

    mgEventCompleted_1 = TRUE;

    return 0;
}




/********************************************************
*
********************************************************/
DWORD WINAPI
InterruptAttachThread9030_2(
    LPVOID pParam
    )
{
    DWORD        EventStatus;
    HANDLE       hDevice;
    HANDLE       hInterruptEvent;
    PLX_INTR     PlxInterrupt;
    RETURN_CODE  rc;


    PlxPrintf("Ok (Thread 2 started)\n");

    hDevice = (HANDLE)pParam;

    // Clear interrupt fields
	memset(
        &PlxInterrupt,
        0,
        sizeof(PLX_INTR)
        );

    PlxPrintf("  Attach Event to Interrupt 2.... ");
    PlxInterrupt.PciMainInt    = 0;
    PlxInterrupt.IopToPciInt_2 = 1;
    rc =
        PlxIntrAttach(
            hDevice,
            PlxInterrupt,
            &hInterruptEvent
            );

    if (rc != ApiSuccess)
    {
        PlxPrintf(
            "*ERROR* - PlxIntrAttach() failed, rc=%d\n",
            rc
            );
        mgEventCompleted_2 = TRUE;
        return 0;
    }
    PlxPrintf(
        "Ok (hEvent=0x%08x)\n",
        hInterruptEvent
        );


    // Wait for interrupt event
    EventStatus =
        WaitForSingleObject(
            hInterruptEvent,
            10 * 1000
            );

    switch (EventStatus)
    {
        case WAIT_OBJECT_0:
            PlxPrintf("*Local Int 2 Rcvd*  ");
            break;

        case WAIT_TIMEOUT:
            PlxPrintf("*Local Int 2 Timeout* ");
            break;

        case WAIT_FAILED:
            PlxPrintf("*ERROR* - Failed while waiting for interrupt\n");
            break;

        default:
            PlxPrintf("*ERROR* - Unknown, EventStatus = 0x%08x\n", EventStatus);
            break;
    }

    mgEventCompleted_2 = TRUE;

    return 0;
}
#endif
