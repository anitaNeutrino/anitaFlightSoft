#ifndef __PLX_IOCTL_H
#define __PLX_IOCTL_H

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
 *      PlxIoctl.h
 *
 * Description:
 *
 *      This file contains the common I/O Control messages shared between
 *      the driver and the PCI API.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#if defined(_WIN32) && !defined(PLX_DRIVER)
    #include <winioctl.h>
#elif defined(PLX_LINUX)
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

#include "PlxTypes.h"


#ifdef __cplusplus
extern "C" {
#endif




// Device Management functions parameters
typedef struct _MANAGEMENTDATA
{
    U16 offset;
    U32 value;
    union
    {
        BOOLEAN         bFlag;
        DEVICE_LOCATION Device;
        PLX_DEVICE_KEY  Key;
    } u;
} MANAGEMENTDATA;


//  Bus Memory and IOP transactions parameters
typedef struct _BUSIOPDATA
{
    IOP_SPACE    IopSpace;
    U32          Address;
    U32          Buffer;
    U32          TransferSize;
    BOOLEAN      bRemap;
    ACCESS_TYPE  AccessType;
} BUSIOPDATA;


// DMA transaction parameters
typedef struct _DMADATA
{
    DMA_CHANNEL channel;
    union
    {
        DMA_CHANNEL_DESC     desc;
        DMA_TRANSFER_ELEMENT TxParams;
        DMA_COMMAND          command;
        U32                  TxCount;
    } u;
} DMADATA;


// This structure will hold all other datatypes
typedef struct _MISCDATA
{
    U32 data[2];
    union
    {
        PLX_INTR    IntrInfo;
        EEPROM_TYPE EepromType;
        PCI_MEMORY  PciMemory;
    } u;
} MISCDATA;


//  Used to pass all arguments down to the driver
typedef struct _IOCTLDATA
{
    RETURN_CODE ReturnCode;
    union
    {
        MANAGEMENTDATA MgmtData;
        BUSIOPDATA     BusIopData;
        DMADATA        DmaData;
        MISCDATA       MiscData;
    } u;
} IOCTLDATA;


#if defined(_WIN32)
    /**********************************************************
     * Note: Codes 0-2047 (0-7FFh) are reserved by Microsoft
     *       Coded 2048-4095 (800h-FFFh) are reserved for OEMs
     *********************************************************/
    #define PLX_IOCTL_CODE_BASE      0x800

    #define IOCTL_MSG( code )        CTL_CODE(                \
                                         FILE_DEVICE_UNKNOWN, \
                                         code,                \
                                         METHOD_BUFFERED,     \
                                         FILE_ANY_ACCESS      \
                                         )
#elif defined(PLX_LINUX) || defined(LINUX_DRIVER)

    #define PLX_IOCTL_CODE_BASE      0x0
    #define PLX_MAGIC                'P'

    #define IOCTL_MSG( code )        _IOWR(         \
                                         PLX_MAGIC, \
                                         code,      \
                                         IOCTLDATA  \
                                         )
#endif


typedef enum _DRIVER_MSGS
{
    MSG_DEVICE_INIT       = PLX_IOCTL_CODE_BASE,
    MSG_DRIVER_VERSION,
    MSG_CHIP_TYPE_GET,
    MSG_PCI_BOARD_RESET,
    MSG_PCI_DEVICE_FIND,
    MSG_PCI_BAR_GET,
    MSG_PCI_BAR_RANGE_GET,
    MSG_PCI_BAR_MAP,
    MSG_PCI_BAR_UNMAP,
    MSG_PHYSICAL_MEM_ALLOCATE,
    MSG_PHYSICAL_MEM_FREE,
    MSG_PHYSICAL_MEM_MAP,
    MSG_PHYSICAL_MEM_UNMAP,
    MSG_COMMON_BUFFER_PROPERTIES,
    MSG_PCI_REGISTER_READ,
    MSG_PCI_REGISTER_WRITE,
    MSG_PCI_REG_READ_UNSUPPORTED,
    MSG_PCI_REG_WRITE_UNSUPPORTED,
    MSG_REGISTER_READ,
    MSG_REGISTER_WRITE,
    MSG_INTR_ENABLE,
    MSG_INTR_DISABLE,
    MSG_INTR_ATTACH,
    MSG_INTR_WAIT,
    MSG_INTR_DETACH,
    MSG_INTR_STATUS_GET,
    MSG_BUS_IOP_READ,
    MSG_BUS_IOP_WRITE,
    MSG_IO_PORT_READ,
    MSG_IO_PORT_WRITE,
    MSG_POWER_LEVEL_SET,
    MSG_POWER_LEVEL_GET,
    MSG_PM_ID_READ,
    MSG_PM_NCP_READ,
    MSG_HS_ID_READ,
    MSG_HS_NCP_READ,
    MSG_HS_STATUS,
    MSG_VPD_ID_READ,
    MSG_VPD_NCP_READ,
    MSG_VPD_READ,
    MSG_VPD_WRITE,
    MSG_EEPROM_PRESENT,
    MSG_EEPROM_READ,
    MSG_EEPROM_READ_BY_OFFSET,
    MSG_EEPROM_WRITE,
    MSG_EEPROM_WRITE_BY_OFFSET,
    MSG_MAILBOX_READ,
    MSG_MAILBOX_WRITE,
    MSG_DOORBELL_READ,
    MSG_DOORBELL_WRITE,
    MSG_MU_INBOUND_PORT_READ,
    MSG_MU_INBOUND_PORT_WRITE,
    MSG_MU_OUTBOUND_PORT_READ,
    MSG_MU_OUTBOUND_PORT_WRITE,
    MSG_MU_OUTINDEX_READ,             // No longer supported
    MSG_MU_OUTINDEX_WRITE,            // No longer supported
    MSG_DMA_CONTROL,
    MSG_DMA_STATUS,
    MSG_DMA_BLOCK_CHANNEL_OPEN,
    MSG_DMA_BLOCK_TRANSFER,
    MSG_DMA_BLOCK_CHANNEL_CLOSE,
    MSG_DMA_SGL_OPEN,
    MSG_DMA_SGL_TRANSFER,
    MSG_DMA_SGL_CLOSE,
    MSG_ABORTADDR_READ
} DRIVER_MSGS;




#define PLX_IOCTL_DEVICE_INIT                   IOCTL_MSG( MSG_DEVICE_INIT )
#define PLX_IOCTL_PCI_DEVICE_FIND               IOCTL_MSG( MSG_PCI_DEVICE_FIND )

#define PLX_IOCTL_DRIVER_VERSION                IOCTL_MSG( MSG_DRIVER_VERSION )
#define PLX_IOCTL_CHIP_TYPE_GET                 IOCTL_MSG( MSG_CHIP_TYPE_GET )
#define PLX_IOCTL_PCI_BOARD_RESET               IOCTL_MSG( MSG_PCI_BOARD_RESET )

#define PLX_IOCTL_PCI_BAR_GET                   IOCTL_MSG( MSG_PCI_BAR_GET )
#define PLX_IOCTL_PCI_BAR_RANGE_GET             IOCTL_MSG( MSG_PCI_BAR_RANGE_GET )
#define PLX_IOCTL_PCI_BAR_MAP                   IOCTL_MSG( MSG_PCI_BAR_MAP )
#define PLX_IOCTL_PCI_BAR_UNMAP                 IOCTL_MSG( MSG_PCI_BAR_UNMAP )

#define PLX_IOCTL_PHYSICAL_MEM_ALLOCATE         IOCTL_MSG( MSG_PHYSICAL_MEM_ALLOCATE )
#define PLX_IOCTL_PHYSICAL_MEM_FREE             IOCTL_MSG( MSG_PHYSICAL_MEM_FREE )
#define PLX_IOCTL_PHYSICAL_MEM_MAP              IOCTL_MSG( MSG_PHYSICAL_MEM_MAP )
#define PLX_IOCTL_PHYSICAL_MEM_UNMAP            IOCTL_MSG( MSG_PHYSICAL_MEM_UNMAP )
#define PLX_IOCTL_COMMON_BUFFER_PROPERTIES      IOCTL_MSG( MSG_COMMON_BUFFER_PROPERTIES )

#define PLX_IOCTL_PCI_REGISTER_READ             IOCTL_MSG( MSG_PCI_REGISTER_READ )
#define PLX_IOCTL_PCI_REGISTER_WRITE            IOCTL_MSG( MSG_PCI_REGISTER_WRITE )
#define PLX_IOCTL_PCI_REG_READ_UNSUPPORTED      IOCTL_MSG( MSG_PCI_REG_READ_UNSUPPORTED )
#define PLX_IOCTL_PCI_REG_WRITE_UNSUPPORTED     IOCTL_MSG( MSG_PCI_REG_WRITE_UNSUPPORTED )

#define PLX_IOCTL_REGISTER_READ                 IOCTL_MSG( MSG_REGISTER_READ )
#define PLX_IOCTL_REGISTER_WRITE                IOCTL_MSG( MSG_REGISTER_WRITE )

#define PLX_IOCTL_MAILBOX_READ                  IOCTL_MSG( MSG_MAILBOX_READ )
#define PLX_IOCTL_MAILBOX_WRITE                 IOCTL_MSG( MSG_MAILBOX_WRITE )
#define PLX_IOCTL_DOORBELL_READ                 IOCTL_MSG( MSG_DOORBELL_READ )
#define PLX_IOCTL_DOORBELL_WRITE                IOCTL_MSG( MSG_DOORBELL_WRITE )

#define PLX_IOCTL_INTR_ENABLE                   IOCTL_MSG( MSG_INTR_ENABLE )
#define PLX_IOCTL_INTR_DISABLE                  IOCTL_MSG( MSG_INTR_DISABLE )
#define PLX_IOCTL_INTR_ATTACH                   IOCTL_MSG( MSG_INTR_ATTACH )
#define PLX_IOCTL_INTR_WAIT                     IOCTL_MSG( MSG_INTR_WAIT )
#define PLX_IOCTL_INTR_DETACH                   IOCTL_MSG( MSG_INTR_DETACH )
#define PLX_IOCTL_INTR_STATUS_GET               IOCTL_MSG( MSG_INTR_STATUS_GET )

#define PLX_IOCTL_BUS_IOP_READ                  IOCTL_MSG( MSG_BUS_IOP_READ )
#define PLX_IOCTL_BUS_IOP_WRITE                 IOCTL_MSG( MSG_BUS_IOP_WRITE )
#define PLX_IOCTL_IO_PORT_READ                  IOCTL_MSG( MSG_IO_PORT_READ )
#define PLX_IOCTL_IO_PORT_WRITE                 IOCTL_MSG( MSG_IO_PORT_WRITE )

#define PLX_IOCTL_POWER_LEVEL_SET               IOCTL_MSG( MSG_POWER_LEVEL_SET )
#define PLX_IOCTL_POWER_LEVEL_GET               IOCTL_MSG( MSG_POWER_LEVEL_GET )
#define PLX_IOCTL_PM_ID_READ                    IOCTL_MSG( MSG_PM_ID_READ )
#define PLX_IOCTL_PM_NCP_READ                   IOCTL_MSG( MSG_PM_NCP_READ )

#define PLX_IOCTL_HS_ID_READ                    IOCTL_MSG( MSG_HS_ID_READ )
#define PLX_IOCTL_HS_NCP_READ                   IOCTL_MSG( MSG_HS_NCP_READ )
#define PLX_IOCTL_HS_STATUS                     IOCTL_MSG( MSG_HS_STATUS )

#define PLX_IOCTL_VPD_ID_READ                   IOCTL_MSG( MSG_VPD_ID_READ )
#define PLX_IOCTL_VPD_NCP_READ                  IOCTL_MSG( MSG_VPD_NCP_READ )
#define PLX_IOCTL_VPD_READ                      IOCTL_MSG( MSG_VPD_READ )
#define PLX_IOCTL_VPD_WRITE                     IOCTL_MSG( MSG_VPD_WRITE )

#define PLX_IOCTL_EEPROM_PRESENT                IOCTL_MSG( MSG_EEPROM_PRESENT )
#define PLX_IOCTL_EEPROM_READ                   IOCTL_MSG( MSG_EEPROM_READ )
#define PLX_IOCTL_EEPROM_READ_BY_OFFSET         IOCTL_MSG( MSG_EEPROM_READ_BY_OFFSET )
#define PLX_IOCTL_EEPROM_WRITE                  IOCTL_MSG( MSG_EEPROM_WRITE )
#define PLX_IOCTL_EEPROM_WRITE_BY_OFFSET        IOCTL_MSG( MSG_EEPROM_WRITE_BY_OFFSET )

#define PLX_IOCTL_MU_INBOUND_PORT_READ          IOCTL_MSG( MSG_MU_INBOUND_PORT_READ )
#define PLX_IOCTL_MU_INBOUND_PORT_WRITE         IOCTL_MSG( MSG_MU_INBOUND_PORT_WRITE )
#define PLX_IOCTL_MU_OUTBOUND_PORT_READ         IOCTL_MSG( MSG_MU_OUTBOUND_PORT_READ )
#define PLX_IOCTL_MU_OUTBOUND_PORT_WRITE        IOCTL_MSG( MSG_MU_OUTBOUND_PORT_WRITE )

#define PLX_IOCTL_DMA_CONTROL                   IOCTL_MSG( MSG_DMA_CONTROL )
#define PLX_IOCTL_DMA_STATUS                    IOCTL_MSG( MSG_DMA_STATUS )

#define PLX_IOCTL_DMA_BLOCK_CHANNEL_OPEN        IOCTL_MSG( MSG_DMA_BLOCK_CHANNEL_OPEN )
#define PLX_IOCTL_DMA_BLOCK_TRANSFER            IOCTL_MSG( MSG_DMA_BLOCK_TRANSFER )
#define PLX_IOCTL_DMA_BLOCK_CHANNEL_CLOSE       IOCTL_MSG( MSG_DMA_BLOCK_CHANNEL_CLOSE )

#define PLX_IOCTL_DMA_SGL_OPEN                  IOCTL_MSG( MSG_DMA_SGL_OPEN )
#define PLX_IOCTL_DMA_SGL_TRANSFER              IOCTL_MSG( MSG_DMA_SGL_TRANSFER )
#define PLX_IOCTL_DMA_SGL_CLOSE                 IOCTL_MSG( MSG_DMA_SGL_CLOSE )

#define PLX_IOCTL_ABORTADDR_READ                IOCTL_MSG( MSG_ABORTADDR_READ )




#ifdef __cplusplus
}
#endif

#endif
