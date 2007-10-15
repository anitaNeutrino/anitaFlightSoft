#ifndef __DRIVER_H
#define __DRIVER_H

/*******************************************************************************
 * Copyright (c) 2002 PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this software under specific terms and
 * conditions.  Use of any of the software or derviatives thereof in any
 * product without a PLX Technology chip is strictly prohibited.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL PLX'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO PLX FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      Driver.h
 *
 * Description:
 *
 *      The header file for the PLX Chip Windows Device driver.
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "DriverDefs.h"




/**********************************************
*               Functions
**********************************************/
int
init_module(
    void
    );

void
cleanup_module(
    void
    );

int
AddDevice(
    DRIVER_OBJECT  *pDriverObject,
    struct pci_dev *pPciDev
    );

int
RemoveDevice(
    DEVICE_OBJECT *fdo
    );

int
StartDevice(
    DEVICE_OBJECT *fdo
    );

VOID
StopDevice(
    DEVICE_OBJECT *fdo
    );

int
PciBusScan(
    DRIVER_OBJECT *pDriverObject
    );




#endif
