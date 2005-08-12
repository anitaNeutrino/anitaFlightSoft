#ifndef __TEST_9656_H
#define __TEST_9656_H

/******************************************************************************
 *
 * File Name:
 *
 *      Test9656.h
 *
 * Description:
 *
 *      Header for chip dependent API tests
 *
 ******************************************************************************/


#include "Reg9656.h"
#include "PlxTypes.h"




/********************************************************
*           Functions Prototypes
********************************************************/
void
TestPowerLevel9656(
    HANDLE           hDevice,
    DEVICE_LOCATION *pDev
    );

void
TestInterruptEnable9656(
    HANDLE hDevice
    );



#endif
