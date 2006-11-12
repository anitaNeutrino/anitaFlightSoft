#ifndef __TEST_9050_H
#define __TEST_9050_H

/******************************************************************************
 *
 * File Name:
 *
 *      Test9050.h
 *
 * Description:
 *
 *      Header for chip dependent API tests
 *
 ******************************************************************************/


#include "Reg9050.h"
#include "PlxTypes.h"




/********************************************************
*           Functions Prototypes
********************************************************/
void
TestPowerLevel9050(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable9050(
    HANDLE hDevice
    );

void
TestInterruptAttach9050(
    HANDLE hDevice
    );

#if defined(_WIN32)
DWORD WINAPI
InterruptAttachThread9050_1(
    LPVOID pParam
    );

DWORD WINAPI
InterruptAttachThread9050_2(
    LPVOID pParam
    );
#endif

void
TestRegisterDoorbell9050(
    HANDLE hDevice
    );

void
TestRegisterMailbox9050(
    HANDLE hDevice
    );

void
TestBlockDma9050(
    HANDLE hDevice
    );

void
TestSglDma9050(
    HANDLE hDevice
    );

void
TestDmaControlStatus9050(
    HANDLE hDevice
    );


#endif
