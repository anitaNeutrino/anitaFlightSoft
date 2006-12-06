#ifndef __TEST_9080_H
#define __TEST_9080_H

/******************************************************************************
 *
 * File Name:
 *
 *      Test9080.h
 *
 * Description:
 *
 *      Header for chip dependent API tests
 *
 ******************************************************************************/


#include "Reg9080.h"
#include "PlxTypes.h"




/********************************************************
*           Functions Prototypes
********************************************************/
void
TestPowerLevel9080(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable9080(
    HANDLE hDevice
    );



#endif
