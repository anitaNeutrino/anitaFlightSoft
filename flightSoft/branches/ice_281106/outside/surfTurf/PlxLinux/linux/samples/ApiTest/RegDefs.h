#ifndef REGDEFS_H
#define REGDEFS_H

/*********************************************************************
 *                                                                   *
 * Module Name:                                                      *
 *                                                                   *
 *     RegDefs.h                                                     *
 *                                                                   *
 * Abstract:                                                         *
 *                                                                   *
 *     Header for the Register definitions module                    *
 *                                                                   *
 ********************************************************************/


#include "PlxTypes.h"



/**********************************************
*               Definitions
**********************************************/
typedef struct _REGISTER_SET
{
    U16  Offset;
    U8   Description[80];
}REGISTER_SET;


extern REGISTER_SET Pci9050[];
extern REGISTER_SET Local9050[];
extern REGISTER_SET Pci9030[];
extern REGISTER_SET Local9030[];
extern REGISTER_SET Pci9080[];
extern REGISTER_SET Local9080[];
extern REGISTER_SET Pci9054[];
extern REGISTER_SET Local9054[];
extern REGISTER_SET Pci9656[];
extern REGISTER_SET Local9656[];



void
DisplayPciRegisterSet(
    DEVICE_LOCATION *pDev,
    REGISTER_SET    *RegSet
    );

void
DisplayLocalRegisterSet(
    HANDLE        DrvHandle,
    REGISTER_SET *RegSet
    );


#endif
