#ifndef __TEST_9030_H
#define __TEST_9030_H

/******************************************************************************
 *
 * File Name:
 *
 *      Test9030.h
 *
 * Description:
 *
 *      Header for chip dependent API tests
 *
 ******************************************************************************/


#include "Reg9030.h"
#include "PlxTypes.h"




/********************************************************
*           Functions Prototypes
********************************************************/
void
TestPowerLevel9030(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable9030(
    HANDLE hDevice
    );

void
TestInterruptAttach9030(
    HANDLE hDevice
    );

#if defined(_WIN32)
DWORD WINAPI
InterruptAttachThread9030_1(
    LPVOID pParam
    );

DWORD WINAPI
InterruptAttachThread9030_2(
    LPVOID pParam
    );
#endif

void
TestRegisterDoorbell9030(
    HANDLE hDevice
    );

void
TestRegisterMailbox9030(
    HANDLE hDevice
    );

void
TestBlockDma9030(
    HANDLE hDevice
    );

void
TestSglDma9030(
    HANDLE hDevice
    );

void
TestDmaControlStatus9030(
    HANDLE hDevice
    );


#endif
