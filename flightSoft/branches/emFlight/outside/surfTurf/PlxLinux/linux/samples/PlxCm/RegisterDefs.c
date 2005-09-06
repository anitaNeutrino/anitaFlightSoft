/******************************************************************************
 *
 * File Name:
 *
 *      RegisterDefs.c
 *
 * Description:
 *
 *      Register defintions
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include "RegisterDefs.h"




/*************************************************
 *              Default PCI Registers
 ************************************************/
REGISTER_SET PciDefault[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST | Header Type | Latency Timer | Cache Line Size"},
    {0x010, "PCI Base Address 0"},
    {0x014, "PCI Base Address 1"},
    {0x018, "PCI Base Address 2"},
    {0x01c, "PCI Base Address 3"},
    {0x020, "PCI Base Address 4"},
    {0x024, "PCI Base Address 5"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x030, "PCI Base Address-to-Local Expansion ROM"},
    {0x034, "Extended Capability Pointer"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x000, ""}
};




/*************************************************
 *            9050 and 9052 Registers
 ************************************************/
REGISTER_SET Lcr9050[] =
{
    {0x000, "Space 0 Range"},
    {0x004, "Space 1 Range"},
    {0x008, "Space 2 Range"},
    {0x00c, "Space 3 Range"},
    {0x010, "Expansion ROM Range"},
    {0x014, "Space 0 Local Base Address (Remap)"},
    {0x018, "Space 1 Local Base Address (Remap)"},
    {0x01c, "Space 2 Local Base Address (Remap)"},
    {0x020, "Space 3 Local Base Address (Remap)"},
    {0x024, "Expansion ROM Local Base Address (Remap)"},
    {0x028, "Space 0 Bus Region Descriptor"},
    {0x02c, "Space 1 Bus Region Descriptor"},
    {0x030, "Space 2 Bus Region Descriptor"},
    {0x034, "Space 3 Bus Region Descriptor"},
    {0x038, "Expansion ROM Bus Region Descriptor"},
    {0x03c, "Chip Select 0 Base Address"},
    {0x040, "Chip Select 1 Base Address"},
    {0x044, "Chip Select 2 Base Address"},
    {0x048, "Chip Select 3 Base Address"},
    {0x04c, "Interrupt Control/Status"},
    {0x050, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x000, ""}
};



REGISTER_SET Eep9050[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Class Code | Revision ID"},
    {0x008, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x00c, "Interrupt Pin | Interrupt Line"},
    {0x010, "Space 0 Range"},
    {0x014, "Space 1 Range"},
    {0x018, "Space 2 Range"},
    {0x01c, "Space 3 Range"},
    {0x020, "Expansion ROM Range"},
    {0x024, "Space 0 Local Base Address (Remap)"},
    {0x028, "Space 1 Local Base Address (Remap)"},
    {0x02c, "Space 2 Local Base Address (Remap)"},
    {0x030, "Space 3 Local Base Address (Remap)"},
    {0x034, "Expansion ROM Local Base Address (Remap)"},
    {0x038, "Space 0 Bus Region Descriptor"},
    {0x03c, "Space 1 Bus Region Descriptor"},
    {0x040, "Space 2 Bus Region Descriptor"},
    {0x044, "Space 3 Bus Region Descriptor"},
    {0x048, "Expansion ROM Bus Region Descriptor"},
    {0x04c, "Chip Select 0 Base Address"},
    {0x050, "Chip Select 1 Base Address"},
    {0x054, "Chip Select 2 Base Address"},
    {0x058, "Chip Select 3 Base Address"},
    {0x05c, "Interrupt Control/Status"},
    {0x060, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x000, ""}
};




/*************************************************
 *                  9030 Registers
 ************************************************/
REGISTER_SET Lcr9030[] =
{
    {0x000, "Space 0 Range"},
    {0x004, "Space 1 Range"},
    {0x008, "Space 2 Range"},
    {0x00c, "Space 3 Range"},
    {0x010, "Expansion ROM Range"},
    {0x014, "Space 0 Local Base Address (Remap)"},
    {0x018, "Space 1 Local Base Address (Remap)"},
    {0x01c, "Space 2 Local Base Address (Remap)"},
    {0x020, "Space 3 Local Base Address (Remap)"},
    {0x024, "Expansion ROM Local Base Address (Remap)"},
    {0x028, "Space 0 Bus Region Descriptor"},
    {0x02c, "Space 1 Bus Region Descriptor"},
    {0x030, "Space 2 Bus Region Descriptor"},
    {0x034, "Space 3 Bus Region Descriptor"},
    {0x038, "Expansion ROM Bus Region Descriptor"},
    {0x03c, "Chip Select 0 Base Address"},
    {0x040, "Chip Select 1 Base Address"},
    {0x044, "Chip Select 2 Base Address"},
    {0x048, "Chip Select 3 Base Address"},
    {0x04c, "EEPROM Write Protect | Interrupt Control/Status"},
    {0x050, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x054, "General Purpose I/O Control"},
    {0x000, ""}
};



REGISTER_SET Eep9030[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "PCI Command | Status"},
    {0x008, "Class Code | Revision ID"},
    {0x00c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x010, "New Capability Pointer"},
    {0x014, "Interrupt Pin | Interrupt Line"},
    {0x018, "Power Management Capabilities | Next Capability Ptr"},
    {0x01c, "Power Mngmnt Control/Status"},
    {0x020, "Hot Swap Control | Next Capability Ptr"},
    {0x024, "VPD Control | Next Capability Pointer"},
    {0x028, "Space 0 Range"},
    {0x02c, "Space 1 Range"},
    {0x030, "Space 2 Range"},
    {0x034, "Space 3 Range"},
    {0x038, "Expansion ROM Range"},
    {0x03c, "Space 0 Local Base Address (Remap)"},
    {0x040, "Space 1 Local Base Address (Remap)"},
    {0x044, "Space 2 Local Base Address (Remap)"},
    {0x048, "Space 3 Local Base Address (Remap)"},
    {0x04c, "Expansion ROM Local Base Address (Remap)"},
    {0x050, "Space 0 Bus Region Descriptor"},
    {0x054, "Space 1 Bus Region Descriptor"},
    {0x058, "Space 2 Bus Region Descriptor"},
    {0x05c, "Space 3 Bus Region Descriptor"},
    {0x060, "Expansion ROM Bus Region Descriptor"},
    {0x064, "Chip Select 0 Base Address"},
    {0x068, "Chip Select 1 Base Address"},
    {0x06c, "Chip Select 2 Base Address"},
    {0x070, "Chip Select 3 Base Address"},
    {0x074, "Interrupt Control/Status"},
    {0x078, "EEPROM Ctrl | PCI Target Response | User I/O | Init Ctrl"},
    {0x07c, "General Purpose I/O Control"},
    {0x080, "Power Managment Select"},
    {0x084, "Power Managment Scale"},
    {0x000, ""}
};




/*************************************************
 *                 9080 Registers
 ************************************************/
REGISTER_SET Lcr9080[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Big/Little Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x000, ""}
};



REGISTER_SET Rtr9080[] =
{
    {0x078, "Mailbox 0"},
    {0x07c, "Mailbox 1"},
    {0x048, "Mailbox 2"},
    {0x04c, "Mailbox 3"},
    {0x050, "Mailbox 4"},
    {0x054, "Mailbox 5"},
    {0x058, "Mailbox 6"},
    {0x05c, "Mailbox 7"},
    {0x060, "PCI-to-Local Doorbell"},
    {0x064, "Local-to-PCI Doorbell"},
    {0x068, "Interrupt Control/Status"},
    {0x06c, "EEPROM Control, PCI Cmd Codes, User I/O Ctrl, Init Ctrl"},
    {0x070, "Hardcoded Device/Vendor ID"},
    {0x074, "Hardcoded Revision ID"},
    {0x000, ""}
};



REGISTER_SET Dma9080[] =
{
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
    {0x0a8, "DMA Ch 1 Cmd/Stat|DMA Ch 0 Cmd/Stat"},
    {0x0ac, "Mode/DMA Arbitration"},
    {0x0b0, "DMA Channels Threshold"},
    {0x000, ""}
};



REGISTER_SET Mqr9080[] =
{
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



REGISTER_SET Eep9080[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID|Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "PCI Base Address-to-Local Expansion ROM"},
    {0x000, ""}
};




/*************************************************
 *                 9054 Registers
 ************************************************/
REGISTER_SET Pci9054[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Status | Command"},
    {0x008, "Base Class | Sub Class | Interface | Revision ID"},
    {0x00c, "BIST|Header Type|Latency Timer|Cache Line Size"},
    {0x010, "PCI Base Address 0 (Memory-mapped PLX registers)"},
    {0x014, "PCI Base Address 1 (I/O-mapped PLX registers)"},
    {0x018, "PCI Base Address 2 (Local Space 0)"},
    {0x01c, "PCI Base Address 3 (Local Space 1)"},
    {0x020, "PCI Base Address 4 (Reserved)"},
    {0x024, "PCI Base Address 5 (Reserved)"},
    {0x028, "Cardbus CIS Pointer"},
    {0x02c, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x030, "PCI Base Address-to-Local Expansion ROM"},
    {0x034, "Next Capability Pointer"},
    {0x038, "Reserved"},
    {0x03c, "Max_Lat | Min_Grant| Interrupt Pin | Interrupt Line"},
    {0x040, "Power Capability | Next Item Pointer | Capability ID"},
    {0x044, "Power Mgmt Data | Bridge Ext | Power Mgmt Control/Status"},
    {0x048, "Hot Swap Cntrl/Stat|Next Cap Ptr | Hot Swap ID"},
    {0x04c, "VPD Flag | VPD Address | Next Cap Pointer | VPD ID"},
    {0x050, "VPD Data"},
    {0x000, ""}
};



REGISTER_SET Lcr9054[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Reserved|EEPROM Write Prot|Misc Ctrl|Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address Reg for Direct Master-to-PCI CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master-to-PCI"},
    {0x000, ""}
};



REGISTER_SET Dma9054[] =
{
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
    {0x000, ""}
};



REGISTER_SET Eep9054[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "VPD Boundary | Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "Hot Swap Cntrl/Stat|Next Cap Ptr|Hot Swap ID"},
    {0x000, ""}
};




/*************************************************
 *            9656 and 9056 Registers
 ************************************************/

/******************************************
 * The following register sets are
 * identical to other PLX chips:
 *
 * - PCI registers           (9054)
 * - Run-time registers      (9080)
 * - DMA registers           (9054)
 * - Message Queue registers (9080)
 *****************************************/

REGISTER_SET Lcr9656[] =
{
    {0x000, "Range for PCI-to-Local Address Space 0"},
    {0x004, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x008, "Mode/DMA Arbitration"},
    {0x00c, "Reserved|EEPROM Write Prot|Misc Ctrl|Endian Descriptor"},
    {0x010, "Range for PCI-to-Local Expansion ROM"},
    {0x014, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x018, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x01c, "Range for Direct Master-to-PCI"},
    {0x020, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x024, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x028, "PCI Base Address for Direct Master-to-PCI"},
    {0x02c, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x0f0, "Range for PCI-to-Local Address Space 1"},
    {0x0f4, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x0f8, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x0fc, "PCI Base Dual Address (Remap) for Direct Master-to-PCI"},
    {0x100, "PCI Arbiter Control"},
    {0x104, "PCI Abort Address"},
    {0x000, ""}
};



REGISTER_SET Eep9656[] =
{
    {0x000, "Device ID | Vendor ID"},
    {0x004, "Base Class | Sub Class | Interface | Revision ID"},
    {0x008, "Max_Lat | Min_Grant | Interrupt Pin | Interrupt Line"},
    {0x00c, "Mailbox 0"},
    {0x010, "Mailbox 1"},
    {0x014, "Range for PCI-to-Local Address Space 0"},
    {0x018, "Local Base Address (Remap) for PCI-to-Local Space 0"},
    {0x01c, "Local Mode/DMA Arbitration"},
    {0x020, "VPD Boundary | Big/Little Endian Descriptor"},
    {0x024, "Range for PCI-to-Local Expansion ROM"},
    {0x028, "Local Base Address for PCI-to-Local Expansion ROM and BREQ"},
    {0x02c, "Local Bus Region Descriptor (Space 0) for PCI-to-Local Access"},
    {0x030, "Range for Direct Master-to-PCI"},
    {0x034, "Local Base Address for Direct Master-to-PCI Memory"},
    {0x038, "Local Base Address for Direct Master-to-PCI IO/CFG"},
    {0x03c, "PCI Base Address for Direct Master-to-PCI"},
    {0x040, "PCI Configuration Address for Direct Master-to-PCI IO/CFG"},
    {0x044, "Subsystem Device ID | Subsystem Vendor ID"},
    {0x048, "Range for PCI-to-Local Address Space 1"},
    {0x04c, "Local Base Address (Remap) for PCI-to-Local Space 1"},
    {0x050, "Local Bus Region Descriptor (Space 1) for PCI-to-Local Access"},
    {0x054, "Hot Swap Cntrl/Stat | Next Capability Ptr | Hot Swap ID"},
    {0x058, "PCI Arbiter Control"},
    {0x05c, "Power Management Capabilities | Next Capability Ptr"},
    {0x060, "Power Mngmnt Control/Status"},
    {0x000, ""}
};
