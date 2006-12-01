#ifndef __API_TEST_H
#define __API_TEST_H

/******************************************************************************
 *
 * File Name:
 *
 *      ApiTest.h
 *
 * Description:
 *
 *      Header file for the API test
 *
 ******************************************************************************/


#include "PlxTypes.h"




/********************************************************
 *                  Definitions
 *******************************************************/
#define PLX_API_VERSION_STRING       "4.10.004"




/********************************************************
 *               Global Variables
 *******************************************************/
extern BOOLEAN mgEventCompleted;
extern U32     ChipTypeSelected;




/********************************************************
 *           Functions Prototypes
 *******************************************************/
void
PlxSdkErrorDisplay(
    RETURN_CODE code
    );

void
TestChipTypeGet(
    HANDLE hDevice
    );

void
TestDriverVersion(
    HANDLE hDevice
    );

void
TestPciBoardReset(
    HANDLE hDevice
    );

void
DisplayVirtualAddresses(
    HANDLE hDevice
    );

void
DisplayCommonBuffer(
    HANDLE hDevice
    );

void
TestPhysicalMemAllocate(
    HANDLE hDevice
    );

void
DisplayBarRanges(
    HANDLE hDevice
    );

void
DisplayRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestConfigRegAccess(
    DEVICE_LOCATION *pDev
    );

void
TestRegAccess(
    HANDLE hDevice
    );

void
TestSerialEeprom(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestDirectSlave(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestIoAccess(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestHotSwapRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestPowerManRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestVpdRegisters(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestPowerLevel(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable(
    HANDLE hDevice
    );

void
TestInterruptAttach(
    HANDLE hDevice
    );

void
TestRegisterDoorbell(
    HANDLE hDevice
    );

void
TestRegisterMailbox(
    HANDLE hDevice
    );

void
TestBlockDma(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestSglDma(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestDmaControlStatus(
    HANDLE hDevice
    );


#endif
