#ifndef __PLXCHIP_H
#define __PLXCHIP_H

/*******************************************************************************
 * Copyright (c) 2003 PLX Technology, Inc.
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
 *      PlxChip.h
 *
 * Description:
 *
 *      This file contains definitions specific to a PLX Chip.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "Reg9050.h"




/**********************************************
*               Definitions
**********************************************/
#define PLX_CHIP_TYPE                       0x9050
#define PLX_DRIVER_NAME                     "Pci9050"
#define PLX_DRIVER_NAME_UNICODE             L"Pci9050"
#define PLX_DEFAULT_ID                      L"905010b5"
#define PLX_DEFAULT_DEV_VEN_ID              0x905010b5

#define REG_EEPROM_CTRL                     PCI9050_EEPROM_CTRL      // EEPROM Control register offset
#define PLX_EEPROM_SIZE                     PCI9050_EEPROM_SIZE      // EEPROM size (bytes) used by PLX Chip
#define MAX_PLX_REG_OFFSET                  PCI9050_MAX_REG_OFFSET   // Max PCI register offset
#define MIN_SIZE_MAPPING_BAR_0              0x100                    // Min Size of BAR0 required mapping
#define DEFAULT_SIZE_COMMON_BUFFER          0x00000000               // Default size of Common Buffer




#endif
