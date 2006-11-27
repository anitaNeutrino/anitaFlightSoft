#ifndef _REGISTER_DEFS_H
#define _REGISTER_DEFS_H

/******************************************************************************
 *
 * File Name:
 *
 *      RegisterDefs.h
 *
 * Description:
 *
 *      Definitions for the register sets
 *
 * Revision History:
 *
 *      04-30-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "PlxTypes.h"




/*************************************
 *          Definitions
 ************************************/
#define REGS_LCR            1
#define REGS_RTR            2
#define REGS_DMA            3
#define REGS_MQR            4
#define REGS_MCR            5


typedef struct _REGISTER_SET
{
    U16  Offset;
    U8   Description[80];
} REGISTER_SET;




/*************************************
 *            Globals
 ************************************/
extern REGISTER_SET PciDefault[];

extern REGISTER_SET Lcr9050[];
extern REGISTER_SET Eep9050[];

extern REGISTER_SET Lcr9030[];
extern REGISTER_SET Eep9030[];

extern REGISTER_SET Lcr9080[];
extern REGISTER_SET Rtr9080[];
extern REGISTER_SET Dma9080[];
extern REGISTER_SET Mqr9080[];
extern REGISTER_SET Eep9080[];

extern REGISTER_SET Pci9054[];
extern REGISTER_SET Lcr9054[];
extern REGISTER_SET Dma9054[];
extern REGISTER_SET Eep9054[];

extern REGISTER_SET Lcr9656[];
extern REGISTER_SET Eep9656[];




#endif
