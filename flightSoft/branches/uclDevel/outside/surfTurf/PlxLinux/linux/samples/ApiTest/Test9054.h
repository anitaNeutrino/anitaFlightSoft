#ifndef __TEST_9054_H
#define __TEST_9054_H

/******************************************************************************
 *
 * File Name:
 *
 *      Test9054.h
 *
 * Description:
 *
 *      Header for chip dependent API tests
 *
 ******************************************************************************/


#include "Reg9054.h"
#include "PlxTypes.h"




/********************************************************
*           Functions Prototypes
********************************************************/
void
TestPowerLevel9054(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable9054(
    HANDLE hDevice
    );



#endif
