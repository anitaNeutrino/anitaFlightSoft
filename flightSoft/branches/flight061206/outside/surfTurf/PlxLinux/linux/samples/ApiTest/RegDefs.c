/*********************************************************************
 *
 * Module Name:
 *
 *     RegDefs.c
 *
 * Abstract:
 *
 *     Register access functions
 *
 ********************************************************************/


#include "ConsFunc.h"
#include "PlxApi.h"
#include "RegDefs.h"



/*************************************************
*                 9050 Registers
*************************************************/
REGISTER_SET Pci9050[] = {
    {0x000, "Device ID|Vendor ID"},
    {0x004, "Status|Command"},
    {0x008, "Base Class|Sub Class|Interface|Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 for Memory Mapped Runtime Regs"},
    {0x014, "PCI Base Address 1 for I/O mapped runtime registers"},
    {0x018, "PCI Base Address 2 for Local Address Space 0"},
    {0x01c, "PCI Base Address 3 for Local Address Space 1"},
    {0x020, "PCI Base Address 4 for Local Address Space 2"},
    {0x024, "PCI Base Address 5 for Local Address Space 3"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID|Subsystem Vendor ID"},
    {0x030, "PCI Base Address to Local Expansion ROM"},
    {0x034, "Reserved"},
    {0x038, "Reserved"},
    {0x03c, "Max Lat|Min Grant|Interrupt Pin|Interrupt Line"},
    {0x000, ""}
    };

REGISTER_SET Local9050[] = {
    {0x000, "Range for PCI to Local Address Space 0"},
    {0x004, "Range for PCI to Local Address Space 1"},
    {0x008, "Range for PCI to Local Address Space 2"},
    {0x00c, "Range for PCI to Local Address Space 3"},
    {0x010, "Range for PCI to Local Expansion ROM"},
    {0x014, "Local Base Address (Remap) for PCI to Local Space 0"},
    {0x018, "Local Base Address (Remap) for PCI to Local Space 1"},
    {0x01c, "Local Base Address (Remap) for PCI to Local Space 2"},
    {0x020, "Local Base Address (Remap) for PCI to Local Space 3"},
    {0x024, "Local Base Address (Remap) for PCI to Expansion ROM"},
    {0x028, "Local Bus Region Descriptor for PCI to Local Space 0"},
    {0x02c, "Local Bus Region Descriptor for PCI to Local Space 1"},
    {0x030, "Local Bus Region Descriptor for PCI to Local Space 2"},
    {0x034, "Local Bus Region Descriptor for PCI to Local Space 3"},
    {0x038, "Local Bus Region Descriptor for PCI to Expansion ROM"},
    {0x03c, "Local Chip Select 0 Base Address"},
    {0x040, "Local Chip Select 1 Base Address"},
    {0x044, "Local Chip Select 2 Base Address"},
    {0x048, "Local Chip Select 3 Base Address"},
    {0x04c, "Reserved|Interrupt Control/Status"},
    {0x050, "User I/O, PCI Target, EEPROM Ctrl, Initialization Ctrl"},
    {0x000, ""}
    };


/*************************************************
*                 9030 Registers
*************************************************/
REGISTER_SET Pci9030[] = {
    {0x000, "Device ID|Vendor ID"},
    {0x004, "Status|Command"},
    {0x008, "Base Class|Sub Class|Interface|Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 for Memory Mapped Runtime Regs"},
    {0x014, "PCI Base Address 1 for I/O mapped runtime registers"},
    {0x018, "PCI Base Address 2 for Local Address Space 0"},
    {0x01c, "PCI Base Address 3 for Local Address Space 1"},
    {0x020, "PCI Base Address 4 for Local Address Space 2"},
    {0x024, "PCI Base Address 5 for Local Address Space 3"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID|Subsystem Vendor ID"},
    {0x030, "PCI Base Address to Local Expansion ROM"},
    {0x034, "Reserved|Next Cap Pointer"},
    {0x038, "Reserved"},
    {0x03c, "Max Lat|Min Grant|Interrupt Pin|Interrupt Line"},
    {0x040, "Power Capability|Next Item Pointer|Capability ID"},
    {0x044, "Power Mgmt Data|Bridge Ext|Power Mgmt Control/Status"},
    {0x048, "Reserved|Hot Swap Cntrl/Stat|Next Cap Ptr|Hot Swap ID"},
    {0x04c, "VPD Flag|VPD Address|Next Cap Pointer|VPD ID"},
    {0x050, "VPD Data"},
    {0x000, ""}
    };

REGISTER_SET Local9030[] = {
    {0x000, "Range for PCI to Local Address Space 0"},
    {0x004, "Range for PCI to Local Address Space 1"},
    {0x008, "Range for PCI to Local Address Space 2"},
    {0x00c, "Range for PCI to Local Address Space 3"},
    {0x010, "Range for PCI to Local Expansion ROM"},
    {0x014, "Local Base Address (Remap) for PCI to Local Space 0"},
    {0x018, "Local Base Address (Remap) for PCI to Local Space 1"},
    {0x01c, "Local Base Address (Remap) for PCI to Local Space 2"},
    {0x020, "Local Base Address (Remap) for PCI to Local Space 3"},
    {0x024, "Local Base Address (Remap) for PCI to Expansion ROM"},
    {0x028, "Local Bus Region Descriptor for PCI to Local Space 0"},
    {0x02c, "Local Bus Region Descriptor for PCI to Local Space 1"},
    {0x030, "Local Bus Region Descriptor for PCI to Local Space 2"},
    {0x034, "Local Bus Region Descriptor for PCI to Local Space 3"},
    {0x038, "Local Bus Region Descriptor for PCI to Expansion ROM"},
    {0x03c, "Local Chip Select 0 Base Address"},
    {0x040, "Local Chip Select 1 Base Address"},
    {0x044, "Local Chip Select 2 Base Address"},
    {0x048, "Local Chip Select 3 Base Address"},
    {0x04c, "Reserved|EEPROM Write Prot|Interrupt Control/Status"},
    {0x050, "EEPROM Control, PCI Target Response, Initialization Control"},
    {0x054, "Reserved|General Purpose I/O Control"},
    {0x070, "Hidden Reg 1: Power Mngmt Data Select/Consumed/Disipated"},
    {0x074, "Hidden Reg 2: Power Mngmt Data Scale/Consumed/Disipated"},
    {0x000, ""}
    };



/*************************************************
*                 9080 Registers
*************************************************/
REGISTER_SET Pci9080[] = {
    {0x000, "Device ID|Vendor ID"},
    {0x004, "Status|Command"},
    {0x008, "Base Class|Sub Class|Interface|Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 for Memory Mapped Runtime Regs"},
    {0x014, "PCI Base Address 1 for I/O Mapped Runtime Regs"},
    {0x018, "PCI Base Address 2 for Local Address Space 0"},
    {0x01c, "PCI Base Address 3 for Local Address Space 1"},
    {0x020, "PCI Base Address 4 for Local Address Space 2"},
    {0x024, "PCI Base Address 5 for Local Address Space 3"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem ID|Subsystem Vendor ID"},
    {0x030, "PCI Base Address to Local Expansion ROM"},
    {0x034, "Reserved"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat|Min_Grant|Interrupt Pin|Interrupt Line"},
    {0x000, ""}
    };

REGISTER_SET Local9080[] = {
    {0x000, "Range for PCI to Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI to Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Big/Little Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI to Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI->Local Access"},
    {0x01c, "Range for Direct Master to PCI"},
    {0x020, "Local Base Address for Direct Master to PCI Memory"},
    {0x024, "Local Base Address for Direct Master to PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master -> PCI"},
    {0x02c, "PCI Configuration Address Reg for Direct Master -> PCI/CFG"},
    {0x0f0, "Range for PCI to Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI to Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI->Local Access"},
    {0x040, "Mailbox Register 0"},
    {0x044, "Mailbox Register 1"},
    {0x048, "Mailbox Register 2"},
    {0x04c, "Mailbox Register 3"},
    {0x050, "Mailbox Register 4"},
    {0x054, "Mailbox Register 5"},
    {0x058, "Mailbox Register 6"},
    {0x05c, "Mailbox Register 7"},
    {0x060, "PCI-to-Local Doorbell Register"},
    {0x064, "Local-to-PCI Doorbell Register"},
    {0x068, "Interrupt Control/Status"},
    {0x06c, "EEPROM Control, PCI Cmd Codes, User I/O Ctrl, Init Ctrl"},
    {0x070, "Device ID|Vendor ID"},
    {0x074, "Unused|Revision ID"},
    {0x080, "DMA Ch 0 Mode"},
    {0x084, "DMA Ch 0 PCI Address"},
    {0x088, "DMA Ch 0 Local Address"},
    {0x08c, "DMA Ch 0 Transfer Byte Count"},
    {0x090, "DMA Ch 0 Descriptor Pointer"},
    {0x094, "DMA Ch 1 Mode"},
    {0x098, "DMA Ch 1 PCI Address"},
    {0x09c, "DMA Ch 1 Local Address"},
    {0x0a0, "DMA Ch 1 Transfer Byte Count"},
    {0x0a4, "DMA Ch 1 Descriptor Pointer"},
    {0x0a8, "Reserved|DMA Ch 1 Cmd/Stat|DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode/DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0x030, "Outbound Post Queue Interrupt Status"},
    {0x034, "Outbound Post Queue Interrupt Mask"},
    {0x0c0, "Messaging Unit Configuration"},
    {0x0c4, "Queue Base Address"},
    {0x0c8, "Inbound Free Head Pointer"},
    {0x0cc, "Inbound Free Tail Pointer"},
    {0x0d0, "Inbound Post Head Pointer"},
    {0x0d4, "Inbound Post Tail Pointer"},
    {0x0d8, "Outbound Free Head Pointer"},
    {0x0dc, "Outbound Free Tail Pointer"},
    {0x0e0, "Outbound Post Head Pointer"},
    {0x0e4, "Outbound Post Tail Pointer"},
    {0x0e8, "Queue Status/Control"},
    {0x000, ""}
    };



/*************************************************
*                 9054 Registers
*************************************************/
REGISTER_SET Pci9054[] = {
    {0x000, "Device ID|Vendor ID"},
    {0x004, "Status|Command"},
    {0x008, "Base Class|Sub Class|Interface|Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 for Memory Mapped Runtime Regs"},
    {0x014, "PCI Base Address 1 for I/O mapped runtime registers"},
    {0x018, "PCI Base Address 2 for Local Address Space 0"},
    {0x01c, "PCI Base Address 3 for Local Address Space 1"},
    {0x020, "PCI Base Address 4 for Local Address Space 2"},
    {0x024, "PCI Base Address 5 for Local Address Space 3"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem ID|Subsystem Vendor ID"},
    {0x030, "PCI Base Address to Local Expansion ROM"},
    {0x034, "Reserved"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat|Min_Grant|Interrupt Pin|Interrupt Line"},
    {0x040, "Power Capability|Next Item Pointer|Capability ID"},
    {0x044, "Power Mgmt Data|Bridge Ext|Power Mgmt Control/Status"},
    {0x048, "Reserved|Hot Swap Cntrl/Stat|Next Cap Ptr|Hot Swap ID"},
    {0x04c, "VPD Flag|VPD Address|Next Cap Pointer|VPD ID"},
    {0x050, "VPD Data"},
    {0x000, ""}
    };

REGISTER_SET Local9054[] = {
    {0x000, "Range for PCI to Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI to Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Reserved|EEPROM Write Prot|Misc Ctrl|Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI to Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI->Local Access"},
    {0x01c, "Range for Direct Master to PCI"},
    {0x020, "Local Base Address for Direct Master to PCI Memory"},
    {0x024, "Local Base Address for Direct Master to PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master to PCI"},
    {0x02c, "PCI Configuration Address Reg for Direct Master to PCI/CFG"},
    {0x0f0, "Range for PCI to Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI to Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI->Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master -> PCI"},
    {0x040, "Mailbox Register 0"},
    {0x044, "Mailbox Register 1"},
    {0x048, "Mailbox Register 2"},
    {0x04c, "Mailbox Register 3"},
    {0x050, "Mailbox Register 4"},
    {0x054, "Mailbox Register 5"},
    {0x058, "Mailbox Register 6"},
    {0x05c, "Mailbox Register 7"},
    {0x060, "PCI-to-Local Doorbell Register"},
    {0x064, "Local-to-PCI Doorbell Register"},
    {0x068, "Interrupt Control/Status"},
    {0x06c, "EEPROM Control, PCI Cmd Codes, User I/O Ctrl, Init Ctrl"},
    {0x070, "Device ID|Vendor ID"},
    {0x074, "Unused|Revision ID"},
    {0x080, "DMA Ch 0 Mode"},
    {0x084, "DMA Ch 0 PCI Address"},
    {0x088, "DMA Ch 0 Local Address"},
    {0x08c, "DMA Ch 0 Transfer Byte Count"},
    {0x090, "DMA Ch 0 Descriptor Pointer"},
    {0x094, "DMA Ch 1 Mode"},
    {0x098, "DMA Ch 1 PCI Address"},
    {0x09c, "DMA Ch 1 Local Address"},
    {0x0a0, "DMA Ch 1 Transfer Byte Count"},
    {0x0a4, "DMA Ch 1 Descriptor Pointer"},
    {0x0a8, "Reserved|DMA Ch 1 Cmd/Stat|DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode/DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0x0b4, "DMA Ch 0 PCI Dual Address (Upper 32 bits)"},
    {0x0b8, "DMA Ch 1 PCI Dual Address (Upper 32 bits)"},
    {0x030, "Outbound Post Queue Interrupt Status"},
    {0x034, "Outbound Post Queue Interrupt Mask"},
    {0x0c0, "Messaging Unit Configuration"},
    {0x0c4, "Queue Base Address"},
    {0x0c8, "Inbound Free Head Pointer"},
    {0x0cc, "Inbound Free Tail Pointer"},
    {0x0d0, "Inbound Post Head Pointer"},
    {0x0d4, "Inbound Post Tail Pointer"},
    {0x0d8, "Outbound Free Head Pointer"},
    {0x0dc, "Outbound Free Tail Pointer"},
    {0x0e0, "Outbound Post Head Pointer"},
    {0x0e4, "Outbound Post Tail Pointer"},
    {0x0e8, "Queue Status/Control"},
    {0x000, ""}
    };


/*************************************************
*                 9656 Registers
*************************************************/
REGISTER_SET Pci9656[] = {
    {0x000, "Device ID|Vendor ID"},
    {0x004, "Status|Command"},
    {0x008, "Base Class|Sub Class|Interface|Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 for Memory Mapped Runtime Regs"},
    {0x014, "PCI Base Address 1 for I/O mapped runtime registers"},
    {0x018, "PCI Base Address 2 for Local Address Space 0"},
    {0x01c, "PCI Base Address 3 for Local Address Space 1"},
    {0x020, "PCI Base Address 4 for Local Address Space 2"},
    {0x024, "PCI Base Address 5 for Local Address Space 3"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem ID|Subsystem Vendor ID"},
    {0x030, "PCI Base Address to Local Expansion ROM"},
    {0x034, "Reserved"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat|Min_Grant|Interrupt Pin|Interrupt Line"},
    {0x040, "Power Capability|Next Item Pointer|Capability ID"},
    {0x044, "Power Mgmt Data|Bridge Ext|Power Mgmt Control/Status"},
    {0x048, "Reserved|Hot Swap Cntrl/Stat|Next Cap Ptr|Hot Swap ID"},
    {0x04c, "VPD Flag|VPD Address|Next Cap Pointer|VPD ID"},
    {0x050, "VPD Data"},
    {0x000, ""}
    };

REGISTER_SET Local9656[] = {
    {0x000, "Range for PCI to Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI to Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Reserved|EEPROM Write Prot|Misc Ctrl|Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI to Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI->Local Access"},
    {0x01c, "Range for Direct Master to PCI"},
    {0x020, "Local Base Address for Direct Master to PCI Memory"},
    {0x024, "Local Base Address for Direct Master to PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master to PCI"},
    {0x02c, "PCI Configuration Address Reg for Direct Master to PCI/CFG"},
    {0x0f0, "Range for PCI to Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI to Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI->Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master -> PCI"},
    {0x040, "Mailbox Register 0"},
    {0x044, "Mailbox Register 1"},
    {0x048, "Mailbox Register 2"},
    {0x04c, "Mailbox Register 3"},
    {0x050, "Mailbox Register 4"},
    {0x054, "Mailbox Register 5"},
    {0x058, "Mailbox Register 6"},
    {0x05c, "Mailbox Register 7"},
    {0x060, "PCI-to-Local Doorbell Register"},
    {0x064, "Local-to-PCI Doorbell Register"},
    {0x068, "Interrupt Control/Status"},
    {0x06c, "EEPROM Control, PCI Cmd Codes, User I/O Ctrl, Init Ctrl"},
    {0x070, "Device ID|Vendor ID"},
    {0x074, "Unused|Revision ID"},
    {0x080, "DMA Ch 0 Mode"},
    {0x084, "DMA Ch 0 PCI Address"},
    {0x088, "DMA Ch 0 Local Address"},
    {0x08c, "DMA Ch 0 Transfer Byte Count"},
    {0x090, "DMA Ch 0 Descriptor Pointer"},
    {0x094, "DMA Ch 1 Mode"},
    {0x098, "DMA Ch 1 PCI Address"},
    {0x09c, "DMA Ch 1 Local Address"},
    {0x0a0, "DMA Ch 1 Transfer Byte Count"},
    {0x0a4, "DMA Ch 1 Descriptor Pointer"},
    {0x0a8, "Reserved|DMA Ch 1 Cmd/Stat|DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode/DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0x0b4, "DMA Ch 0 PCI Dual Address (Upper 32 bits)"},
    {0x0b8, "DMA Ch 1 PCI Dual Address (Upper 32 bits)"},
    {0x030, "Outbound Post Queue Interrupt Status"},
    {0x034, "Outbound Post Queue Interrupt Mask"},
    {0x0c0, "Messaging Unit Configuration"},
    {0x0c4, "Queue Base Address"},
    {0x0c8, "Inbound Free Head Pointer"},
    {0x0cc, "Inbound Free Tail Pointer"},
    {0x0d0, "Inbound Post Head Pointer"},
    {0x0d4, "Inbound Post Tail Pointer"},
    {0x0d8, "Outbound Free Head Pointer"},
    {0x0dc, "Outbound Free Tail Pointer"},
    {0x0e0, "Outbound Post Head Pointer"},
    {0x0e4, "Outbound Post Tail Pointer"},
    {0x0e8, "Queue Status/Control"},
    {0x100, "PCI Arbiter Control"},
    {0x104, "PCI Abort Address"},
    {0x000, ""}
    };




/********************************************************
*
********************************************************/
void
DisplayPciRegisterSet(
    DEVICE_LOCATION *pDev,
    REGISTER_SET    *RegSet
    )
{
    U8           i;
    U32          RegValue;
    RETURN_CODE  rc;


    i = 0;
    while (RegSet[i].Description[0] != '\0')
    {
        RegValue =
            PlxPciConfigRegisterRead(
                pDev->BusNumber,
                pDev->SlotNumber,
                RegSet[i].Offset,
                &rc
                );

        PlxPrintf(
            "  0x%03x: %08x  %s\n", 
            RegSet[i].Offset,
            RegValue,
            RegSet[i].Description
            );

        if (i != 0 && !(i & 0xF))
        {
            _Pause;
        }

        i++;
    }
}




/********************************************************
*
********************************************************/
void
DisplayLocalRegisterSet(
    HANDLE        hDevice,
    REGISTER_SET *RegSet
    )
{
    U8           i;
    U32          RegValue;
    RETURN_CODE  rc;


    i = 0;
    while (RegSet[i].Description[0] != '\0')
    {
        RegValue =
            PlxRegisterRead(
                hDevice,
                RegSet[i].Offset,
                &rc
                );

        PlxPrintf(
            "  0x%03x: %08x  %s\n", 
            RegSet[i].Offset,
            RegValue,
            RegSet[i].Description
            );

        if (i != 0 && !(i & 0xF))
        {
            _Pause;
        }

        i++;
    }
}
