/*********************************************************************
 *
 * Module Name:
 *
 *     PciDevice.c
 *
 * Abstract:
 *
 *     PCI device functions
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v4.20
 *
 ********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "Monitor.h"
#include "PciDevice.h"
#include "PciRegs.h"
#include "PlxApi.h"
#include "ConsFunc.h"



/*************************************
 *            Globals
 ************************************/
DEVICE_NODE *pDeviceList;


struct _PCI_CLASS_CODES
{
    U8   BaseClass;
    U8   SubClass;
    U8   Interface;
    char StrClass[100];
} PciClassCodes[] =
    {
        {0x00, 0x00, 0x00, "Unknown"},
            {0x00, 0x01, 0xFF, "VGA-compatible device"},

        {0x01, 0xFF, 0xFF, "Mass storage controller"},
            {0x01, 0x00, 0x00, "SCSI bus controller"},
            {0x01, 0x01, 0xFF, "IDE controller"},
            {0x01, 0x02, 0x00, "Floppy disk controller"},
            {0x01, 0x03, 0x00, "IPI bus controller"},
            {0x01, 0x04, 0x00, "RAID controller"},
            {0x01, 0x05, 0x20, "ATA controller (single DMA)"},
            {0x01, 0x05, 0x30, "ATA controller (chained DMA)"},
            {0x01, 0x80, 0x00, "Other mass storage controller"},

        {0x02, 0xFF, 0xFF, "Network controller"},
            {0x02, 0x00, 0x00, "Ethernet controller"},
            {0x02, 0x01, 0x00, "Token ring controller"},
            {0x02, 0x02, 0x00, "FDDI controller"},
            {0x02, 0x03, 0x00, "ATM controller"},
            {0x02, 0x04, 0x00, "ISDN controller"},
            {0x02, 0x05, 0x00, "WorldFip controller"},
            {0x02, 0x06, 0xFF, "PCIMG 2.14 Multi-Computing"},
            {0x02, 0x80, 0x00, "Other network controller"},

        {0x03, 0xFF, 0xFF, "Display controller"},
            {0x03, 0x00, 0x00, "VGA-compatible display controller"},
            {0x03, 0x00, 0x01, "8514-compatible display controller"},
            {0x03, 0x01, 0x00, "XGA display controller"},
            {0x03, 0x02, 0x00, "3D display controller"},
            {0x03, 0x80, 0x00, "Other display controller"},

        {0x04, 0xFF, 0xFF, "Multimedia device"},
            {0x04, 0x00, 0x00, "Multimedia video device"},
            {0x04, 0x01, 0x00, "Multimedia audio device"},
            {0x04, 0x02, 0x00, "Computer telephony device"},
            {0x04, 0x80, 0x00, "Other multimedia device"},

        {0x05, 0xFF, 0xFF, "Memory controller"},
            {0x05, 0x00, 0x00, "RAM memory controller"},
            {0x05, 0x01, 0x00, "Flash memory controller"},
            {0x05, 0x80, 0x00, "Other memory controller"},

        {0x06, 0xFF, 0xFF, "Bridge device"},
            {0x06, 0x00, 0x00, "Host bridge device"},
            {0x06, 0x01, 0x00, "ISA bridge device"},
            {0x06, 0x02, 0x00, "EISA bridge device"},
            {0x06, 0x03, 0x00, "MCA bridge device"},
            {0x06, 0x04, 0x00, "PCI-to-PCI bridge device"},
            {0x06, 0x04, 0x01, "PCI-to-PCI bridge (subtractive decode)"},
            {0x06, 0x05, 0x00, "PCMCIA bridge device"},
            {0x06, 0x06, 0x00, "NuBus bridge device"},
            {0x06, 0x07, 0x00, "CardBus bridge device"},
            {0x06, 0x08, 0x00, "RACEway bridge (transparent mode)"},
            {0x06, 0x08, 0x01, "RACEway bridge (end-point mode)"},
            {0x06, 0x09, 0x40, "PCI-to-PCI bridge (primary bus to Host)"},
            {0x06, 0x09, 0x80, "PCI-to-PCI bridge (secondary bus to Host)"},
            {0x06, 0x0A, 0x00, "InfiniBand-to-PCI host bridge"},
            {0x06, 0x80, 0x00, "Other bridge device"},

        {0x07, 0xFF, 0xFF, "Simple communications controller"},
            {0x07, 0x00, 0x00, "Generic XT-compatible serial controller"},
            {0x07, 0x00, 0x01, "16450-compatible serial controller"},
            {0x07, 0x00, 0x02, "16550-compatible serial controller"},
            {0x07, 0x00, 0x03, "16650-compatible serial controller"},
            {0x07, 0x00, 0x04, "16750-compatible serial controller"},
            {0x07, 0x00, 0x05, "16850-compatible serial controller"},
            {0x07, 0x00, 0x06, "16950-compatible serial controller"},
            {0x07, 0x01, 0x00, "Parallel port"},
            {0x07, 0x01, 0x01, "Bi-directional parallel port"},
            {0x07, 0x01, 0x02, "ECP 1.x compliant parallel port"},
            {0x07, 0x01, 0x03, "IEEE 1284 controller"},
            {0x07, 0x01, 0xFE, "IEEE 1284 target device"},
            {0x07, 0x02, 0x00, "Multiport serial controller"},
            {0x07, 0x03, 0x00, "Generic modem"},
            {0x07, 0x03, 0x01, "Hayes-compatible modem (16450)"},
            {0x07, 0x03, 0x02, "Hayes-compatible modem (16550)"},
            {0x07, 0x03, 0x03, "Hayes-compatible modem (16650)"},
            {0x07, 0x03, 0x04, "Hayes-compatible modem (16750)"},
            {0x07, 0x04, 0x00, "GPIB (IEEE 488.1/2) controller"},
            {0x07, 0x05, 0x00, "Smart Card"},
            {0x07, 0x80, 0x00, "Other communications device"},

        {0x08, 0xFF, 0xFF, "Base system peripheral"},
            {0x08, 0x00, 0x00, "Generic 8259 PIC"},
            {0x08, 0x00, 0x01, "ISA PIC"},
            {0x08, 0x00, 0x02, "EISA PIC"},
            {0x08, 0x00, 0x10, "I/O APIC interrupt contoller"},
            {0x08, 0x00, 0x20, "I/O(x) APIC interrupt contoller"},
            {0x08, 0x01, 0x00, "Generic 8237 DMA controller"},
            {0x08, 0x01, 0x01, "ISA DMA controller"},
            {0x08, 0x01, 0x02, "EISA DMA controller"},
            {0x08, 0x02, 0x00, "Generic 8254 system timer"},
            {0x08, 0x02, 0x01, "ISA system timer"},
            {0x08, 0x02, 0x02, "EISA system timers (two timers)"},
            {0x08, 0x03, 0x00, "Generic RTC controller"},
            {0x08, 0x03, 0x01, "ISA RTC controller"},
            {0x08, 0x04, 0x00, "Generic PCI Hot-Plug controller"},
            {0x08, 0x80, 0x00, "Other system peripheral"},

        {0x09, 0xFF, 0xFF, "Input device"},
            {0x09, 0x00, 0x00, "Keyboard controller"},
            {0x09, 0x01, 0x00, "Digitizer (pen)"},
            {0x09, 0x02, 0x00, "Mouse controller"},
            {0x09, 0x03, 0x00, "Scanner controller"},
            {0x09, 0x04, 0x00, "Gameport controller (generic)"},
            {0x09, 0x04, 0x10, "Gameport controller"},
            {0x09, 0x80, 0x00, "Other input controller"},

        {0x0A, 0xFF, 0xFF, "Docking station"},
            {0x0A, 0x00, 0x00, "Generic docking station"},
            {0x0A, 0x80, 0x00, "Other type of docking station"},

        {0x0B, 0xFF, 0xFF, "Processor"},
            {0x0B, 0x00, 0x00, "386"},
            {0x0B, 0x01, 0x00, "486"},
            {0x0B, 0x02, 0x00, "Pentium"},
            {0x0B, 0x10, 0x00, "Alpha"},
            {0x0B, 0x20, 0x00, "PowerPC"},
            {0x0B, 0x30, 0x00, "MIPS"},
            {0x0B, 0x40, 0x00, "Co-processor"},

        {0x0C, 0xFF, 0xFF, "Serial bus controller"},
            {0x0C, 0x00, 0x00, "IEEE 1394 (FireWire) controller"},
            {0x0C, 0x00, 0x10, "IEEE 1394 (OpenHCI specification)"},
            {0x0C, 0x01, 0x00, "ACCESS bus controller"},
            {0x0C, 0x02, 0x00, "SSA controller"},
            {0x0C, 0x03, 0x00, "USB controller (Universal Host)"},
            {0x0C, 0x03, 0x10, "USB controller (Open Host)"},
            {0x0C, 0x03, 0x20, "USB2 controller (Enhanced controller)"},
            {0x0C, 0x03, 0x80, "USB controller (non-specific interface)"},
            {0x0C, 0x03, 0xFE, "USB device"},
            {0x0C, 0x04, 0x00, "Fibre Channel controller"},
            {0x0C, 0x05, 0x00, "SMBus(System Management Bus) controller"},
            {0x0C, 0x06, 0x00, "InfiniBand controller"},
            {0x0C, 0x07, 0x00, "IPMI SMIC interface controller"},
            {0x0C, 0x07, 0x01, "IPMI Keyboard controller style interface"},
            {0x0C, 0x07, 0x02, "IPMI Block transfer interface"},
            {0x0C, 0x08, 0x00, "SERCOS Inerfact Standard (IEC 61491)"},
            {0x0C, 0x09, 0x00, "CANbus controller"},

        {0x0D, 0xFF, 0xFF, "Wireless controller"},
            {0x0D, 0x00, 0x00, "iRDA-compatible wireless controller"},
            {0x0D, 0x01, 0x00, "Consumer IR wireless controller"},
            {0x0D, 0x10, 0x00, "RF wireless controller"},
            {0x0D, 0x11, 0x00, "Bluetooth wireless controller"},
            {0x0D, 0x12, 0x00, "Broadband wireless controller"},
            {0x0D, 0x80, 0x00, "Other type of wireless controller"},

        {0x0E, 0xFF, 0xFF, "Intelligent I/O controller"},
            {0x0E, 0x00, 0xFF, "I2O Specification 1.0 controller"},
            {0x0E, 0x00, 0x00, "I2O controller (Message FIFO @ 40h)"},

        {0x0F, 0xFF, 0xFF, "Satellite communication controller"},
            {0x0F, 0x01, 0x00, "TV communication controller"},
            {0x0F, 0x02, 0x00, "Audio communication controller"},
            {0x0F, 0x03, 0x00, "Voice communication controller"},
            {0x0F, 0x04, 0x00, "Data communication controller"},

        {0x10, 0xFF, 0xFF, "Encryption/Decryption controller"},
            {0x10, 0x00, 0x00, "Network & computing en/decryption"},
            {0x10, 0x10, 0x00, "Entertainment en/decryption"},
            {0x10, 0x80, 0x00, "Other en/decryption"},

        {0x11, 0xFF, 0xFF, "Data acquisition & signal processing cntrlr"},
            {0x11, 0x00, 0x00, "DPIO module"},
            {0x11, 0x01, 0x00, "Perfomance counters"},
            {0x11, 0x10, 0x00, "Communications synchronization test/measure"},
            {0x11, 0x20, 0x00, "Management card"},
            {0x11, 0x80, 0x00, "Other data acquisition/signal processing"},

        {0xFF, 0xFF, 0xFF, "Undefined"}
    };




/*********************************************************************
 *
 * Function   :  DeviceListCreate
 *
 * Description:  
 *
 ********************************************************************/
void
DeviceListCreate(
    void
    )
{
    U8                 bus;
    U8                 slot;
    U8                 function;
    U32                RegValue;
    HANDLE             hDevice;
    BOOLEAN            bPciService;
    BOOLEAN            bMultiFunction;
    RETURN_CODE        rc;
    DEVICE_NODE       *pNode;
    PLX_DEVICE_OBJECT  DeviceObj;


    PlxPrintf(
        "Searching for installed PLX devices..."
        );

    // Clear device structure
    DeviceObj.Key.bus         = PCI_FIELD_IGNORE;
    DeviceObj.Key.slot        = PCI_FIELD_IGNORE;
    DeviceObj.Key.function    = PCI_FIELD_IGNORE;
    DeviceObj.Key.DeviceId    = PCI_FIELD_IGNORE;
    DeviceObj.Key.VendorId    = PCI_FIELD_IGNORE;
    DeviceObj.Key.SubDeviceId = PCI_FIELD_IGNORE;
    DeviceObj.Key.SubVendorId = PCI_FIELD_IGNORE;
    DeviceObj.Key.Revision    = PCI_FIELD_IGNORE;

    // Check if PLX PCI Service driver is loaded
    if (PlxPci_DeviceFind(
            &DeviceObj.Key,
            0
            ) == ApiSuccess)
    {
        bPciService = TRUE;
    }
    else
    {
        bPciService = FALSE;
    }


    // Find all devices
    for (bus=0; bus < 15; bus++)
    {
        for (slot=0; slot < 32; slot++)
        {
            function       = 0;
            bMultiFunction = FALSE;

            do
            {
                if (bPciService)
                {
                    RegValue =
                        PlxPci_PciRegisterRead(
                            bus,
                            slot,
                            function,
                            0x0,        // PCI Device/Vendor ID offset
                            &rc
                            );
                }
                else
                {
                    RegValue =
                        PlxPciRegisterRead_Unsupported(
                            bus,
                            PCI_DEVFN(slot, function),
                            0x0,        // PCI Device/Vendor ID offset
                            &rc
                            );
                }

                if ((rc == ApiSuccess) && (RegValue != (U32)-1))
                {
                    // Add device to list
                    pNode =
                        DeviceNodeAdd(
                            bus,
                            slot,
                            function
                            );

                    // Set device properties
                    if (pNode != NULL)
                    {
                        // Attempt to open device with standard driver
                        rc =
                            PlxPciDeviceOpen(
                                &(pNode->Location),
                                &hDevice
                                );

                        if (rc == ApiSuccess)
                        {
                            // Note the access method
                            pNode->PciMethod = PCI_ACCESS_DRIVER;

                            // Get device type & version
                            PlxChipTypeGet(
                                hDevice,
                                &(pNode->PlxChipType),
                                &(pNode->PlxRevision)
                                );

                            // Release the device
                            PlxPciDeviceClose(
                                hDevice
                                );

                            // Copy device information to new node
                            pNode->Key.DeviceId = pNode->Location.DeviceId;
                            pNode->Key.VendorId = pNode->Location.VendorId;

                            // Store PCI header type
                            RegValue =
                                PlxPciConfigRegisterRead(
                                    pNode->Key.bus,
                                    pNode->Key.slot,
                                    0x0C,       // Header Type/Cacheline PCI register offset
                                    NULL
                                    );

                            pNode->PciHeaderType = (U8)(RegValue >> 16);

                            // Store PCI Class Information
                            pNode->PciClass =
                                PlxPciConfigRegisterRead(
                                    pNode->Key.bus,
                                    pNode->Key.slot,
                                    0x08,       // Class code / revision register
                                    NULL
                                    );
                        }
                        else if (bPciService == FALSE)
                        {
                            // Default to standard driver access method
                            pNode->PciMethod = PCI_ACCESS_DRIVER;

                            // Set device information
                            pNode->PlxChipType = 0;
                            pNode->PlxRevision = 0;

                            pNode->Location.DeviceId = (U16)(RegValue >> 16);
                            pNode->Location.VendorId = (U16)RegValue;

                            pNode->Key.DeviceId = pNode->Location.DeviceId;
                            pNode->Key.VendorId = pNode->Location.VendorId;

                            // Store PCI header type
                            RegValue =
                                PlxPciRegisterRead_Unsupported(
                                    bus,
                                    PCI_DEVFN(slot, function),
                                    0x0C,       // Header Type/Cacheline PCI register offset
                                    NULL
                                    );

                            pNode->PciHeaderType = (U8)(RegValue >> 16);

                            // Store PCI Class Information
                            pNode->PciClass =
                                PlxPciRegisterRead_Unsupported(
                                    bus,
                                    PCI_DEVFN(slot, function),
                                    0x08,       // Class code / revision register
                                    NULL
                                    );
                        }
                        else
                        {
                            //
                            // Use PCI Service Driver
                            //

                            // Note the access method
                            pNode->PciMethod = PCI_ACCESS_SERVICE;

                            // Select device to get properties
                            rc =
                                PlxPci_DeviceOpen(
                                    &pNode->Key,
                                    &DeviceObj
                                    );

                            if (rc != ApiSuccess)
                                break;

                            // Copy device information to new node
                            pNode->Key = DeviceObj.Key;

                            pNode->Location.DeviceId = pNode->Key.DeviceId;
                            pNode->Location.VendorId = pNode->Key.VendorId;

                            // Get device type & version
                            PlxPci_ChipTypeGet(
                                &DeviceObj,
                                &(pNode->PlxChipType),
                                &(pNode->PlxRevision)
                                );

                            // Release the device
                            PlxPci_DeviceClose(
                                &DeviceObj
                                );

                            // Store PCI header type
                            RegValue =
                                PlxPci_PciRegisterRead(
                                    pNode->Key.bus,
                                    pNode->Key.slot,
                                    pNode->Key.function,
                                    0x0C,       // Header Type/Cacheline PCI register offset
                                    NULL
                                    );

                            pNode->PciHeaderType = (U8)(RegValue >> 16);

                            // Store PCI Class Information
                            pNode->PciClass =
                                PlxPci_PciRegisterRead(
                                    pNode->Key.bus,
                                    pNode->Key.slot,
                                    pNode->Key.function,
                                    0x08,       // Class code/Revision PCI register offset
                                    NULL
                                    );
                        }

                        // Check for multi-function device
                        if (pNode->PciHeaderType & (1 << 7))
                        {
                            bMultiFunction = TRUE;
                        }
                    }
                }

                // Increment function
                function++;
            }
            while (bMultiFunction && (function <= 7));
        }
    }

    PlxPrintf(
        "\r                                       "
        );
}




/*********************************************************************
 *
 * Function   :  DeviceListDisplay
 *
 * Description:  
 *
 ********************************************************************/
void
DeviceListDisplay(
    void
    )
{
    int          index;
    DEVICE_NODE *pNode;


    if (pDeviceList == NULL)
    {
        PlxPrintf("Error:  ** No PLX Devices found **\n");
        return;
    }

    PlxPrintf(
        "\n"
	    "    dev bus slot fn  Dev  Ven  Chip Rv  Device Type\n"
	    "   --------------------------------------------------------\n"
        );

    // Traverse the list and diplay items
    index = 0;
    pNode = pDeviceList;

    while (pNode != NULL)
    {
        if (pNode->bSelected)
        {
            PlxPrintf(" ==>");
        }
        else
        {
            PlxPrintf("    ");
        }

        // Display device description
        PlxPrintf(
            "%2X   %02X  %02X  %02X  %04X %04X ",
            index,
            pNode->Key.bus,
            pNode->Key.slot,
            pNode->Key.function,
            pNode->Key.DeviceId,
            pNode->Key.VendorId
            );

        if (pNode->PlxChipType != 0)
        {
            // Check for 9052 chip, which is 9050 rev 2
            if ((pNode->PlxChipType == 0x9050) &&
                (pNode->PlxRevision == 2))
            {
                PlxPrintf("9052 01");
            }
            else
            {
                PlxPrintf(
                    "%04X ",
                    pNode->PlxChipType
                    );

                if (pNode->PlxRevision == 0)
                {
                    PlxPrintf(
                        "??"
                        );
                }
                else
                {
                    PlxPrintf(
                        "%02X",
                        pNode->PlxRevision
                        );
                }
            }
        }
        else
        {
            PlxPrintf(
                " --  --"
                );
        }

        PlxPrintf(
	        "  %s\n",
            Device_GetClassString(pNode->PciClass)
            );

        // Jump to next node
        index++;
        pNode = pNode->pNext;
    }
}




/*********************************************************************
 *
 * Function   :  DeviceListFree
 *
 * Description:  
 *
 ********************************************************************/
void
DeviceListFree(
    void
    )
{
    DEVICE_NODE *pNode;


    // Traverse list and free all nodes
    while (pDeviceList != NULL)
    {
        pNode = pDeviceList;

        pDeviceList = pNode->pNext;

        // Release node
        free(
            pNode
            );
    }
}




/*****************************************************************************************
 *
 * Function   :  DeviceNodeAdd
 *
 * Description:  Adds a device node to the PCI device list
 *
 *****************************************************************************************/
DEVICE_NODE *
DeviceNodeAdd(
    U8 bus,
    U8 slot,
    U8 function
    )
{
    BOOLEAN      bAddDevice;
    DEVICE_NODE *pNode;
    DEVICE_NODE *pNodeCurr;
    DEVICE_NODE *pNodePrev;


    // Check if node already exists
    if (DeviceNodeExist(
            bus,
            slot,
            function
            ))
    {
        return NULL;
    }

    // Allocate a new node for the device list
    pNode = (DEVICE_NODE*)malloc(sizeof(DEVICE_NODE));

    if (pNode == NULL)
        return NULL;

    // Set device key information
    pNode->Key.bus         = bus;
    pNode->Key.slot        = slot;
    pNode->Key.function    = function;
    pNode->Key.VendorId    = PCI_FIELD_IGNORE;
    pNode->Key.DeviceId    = PCI_FIELD_IGNORE;
    pNode->Key.SubVendorId = PCI_FIELD_IGNORE;
    pNode->Key.SubDeviceId = PCI_FIELD_IGNORE;
    pNode->Key.Revision    = PCI_FIELD_IGNORE;

    pNode->Location.BusNumber       = bus;
    pNode->Location.SlotNumber      = slot;
    pNode->Location.DeviceId        = (U16)-1;
    pNode->Location.VendorId        = (U16)-1;
    pNode->Location.SerialNumber[0] = '\0';

    // Mark the device as not selected
    pNode->bSelected = FALSE;

    /*******************************************
     *
     *      Add node to sorted device list
     *
     ******************************************/

    pNodePrev = NULL;
    pNodeCurr = pDeviceList;

    // Traverse list & add device in proper order
    while (1)
    {
        bAddDevice = FALSE;

        // Check for end-of-list
        if (pNodeCurr == NULL)
        {
            bAddDevice = TRUE;
        }
        else
        {
            // Compare bus numbers
            if (bus < pNodeCurr->Key.bus)
            {
                bAddDevice = TRUE;
            }
            else if (bus == pNodeCurr->Key.bus)
            {
                // Compare slot number
                if (slot < pNodeCurr->Key.slot)
                {
                    bAddDevice = TRUE;
                }
                else if (slot == pNodeCurr->Key.slot)
                {
                    // Compare function number
                    if (function < pNodeCurr->Key.function)
                    {
                        bAddDevice = TRUE;
                    }
                }
            }
        }

        if (bAddDevice)
        {
            if (pNodePrev == NULL)
            {
                pDeviceList = pNode;
            }
            else
            {
                pNodePrev->pNext = pNode;
            }

            pNode->pNext = pNodeCurr;

            return pNode;
        }

        // Store previous node
        pNodePrev = pNodeCurr;

        // Jump to next node
        pNodeCurr = pNodeCurr->pNext;
    }
}




/*****************************************************************************************
 *
 * Function   :  DeviceNodeExist
 *
 * Description:  Returns TRUE is a device already exists in the PCI device list
 *
 *****************************************************************************************/
BOOL
DeviceNodeExist(
    U8 bus,
    U8 slot,
    U8 function
    )
{
    DEVICE_NODE *pNode;


    pNode = pDeviceList;

    // Traverse list and free all nodes
    while (pNode != NULL)
    {
        // Check for a match by key
        if ((bus      == pNode->Key.bus)  &&
            (slot     == pNode->Key.slot) &&
            (function == pNode->Key.function))
        {
            return TRUE;
        }

        // Go to next device
        pNode = pNode->pNext;
    }

    return FALSE;
}




/******************************************************************************
 *
 * Function   :  GetDevice
 *
 * Description:
 *
 ******************************************************************************/
DEVICE_NODE*
GetDevice(
    U8      index,
    BOOLEAN bPlxOnly
    )
{
    U8           count;
    DEVICE_NODE *pNode;


    // Traverse the list to return desired node
    count = 0;
    pNode = pDeviceList;

    while (pNode != NULL)
    {
        if (bPlxOnly)
        {
            // Increment count only if a PLX device
            if (pNode->PlxChipType != 0)
            {
                count++;
            }
        }
        else
        {
            count++;
        }

        // Check if desired device
        if (index == (count - 1))
        {
            return pNode;
        }

        // Jump to next node
        pNode = pNode->pNext;
    }

    // If no PLX devices, return first device
    if (bPlxOnly)
        return pDeviceList;

    return NULL;
}




/******************************************************************************
 *
 * Function   :  Device_GetClassString
 *
 * Description:
 *
 ******************************************************************************/
char*
Device_GetClassString(
    U32 PciClass
    )
{
    U16 Index_BestFit;
    U16 Index_Current;
    U16 BestScore;
    U16 CurrentScore;


    Index_BestFit = 0;
    Index_Current = 0;

    BestScore = 0;

    do
    {
        // Reset the score
        CurrentScore = 0;

        // Check for match on base class
        if (((U8)(PciClass >> 24)) == 
            PciClassCodes[Index_Current].BaseClass)
        {
            CurrentScore = MATCH_BASE_EXACT;
        }
        else
        {
            if (PciClassCodes[Index_Current].BaseClass == 0xFF)
                CurrentScore = MATCH_BASE_GENERIC;
        }

        // Check for match on sub class
        if (CurrentScore & (MATCH_BASE))
        {
            if (((U8)(PciClass >> 16)) == 
                PciClassCodes[Index_Current].SubClass)
            {
                CurrentScore += MATCH_SUBCLASS_EXACT;
            }
            else
            {
                if (PciClassCodes[Index_Current].SubClass == 0xFF)
                    CurrentScore += MATCH_SUBCLASS_GENERIC;
            }
        }

        // Check for match on interface
        if (CurrentScore & (MATCH_SUBCLASS))
        {
            if (((U8)(PciClass >> 8)) == 
                PciClassCodes[Index_Current].Interface)
            {
                CurrentScore += MATCH_INTERFACE_EXACT;
            }
            else
            {
                if (PciClassCodes[Index_Current].Interface == 0xFF)
                    CurrentScore += MATCH_INTERFACE_GENERIC;
            }
        }

        // Check if we have a better match
        if (CurrentScore > BestScore)
        {
            BestScore     = CurrentScore;
            Index_BestFit = Index_Current;
        }

        // Increment to next class
        Index_Current++;
    }
    while (PciClassCodes[Index_Current].BaseClass != 0xFF);

    return PciClassCodes[Index_BestFit].StrClass;
}




/******************************************************************************
 *
 * Function   :  PlxCm_DeviceOpen
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_DeviceOpen(
    DEVICE_NODE *pNode
    )
{
    if (pNode->PciMethod == PCI_ACCESS_DRIVER)
    {
        return PlxPciDeviceOpen(
                   &pNode->Location,
                   &Gbl_hDevice
                   );
    }
    else
    {
        return PlxPci_DeviceOpen(
                   &pNode->Key,
                   &Gbl_DeviceObj
                   );
    }
}




/******************************************************************************
 *
 * Function   :  PlxCm_DeviceClose
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_DeviceClose(
    DEVICE_NODE *pNode
    )
{
    RETURN_CODE rc;


    if (pNode->PciMethod == PCI_ACCESS_DRIVER)
    {
        rc =
            PlxPciDeviceClose(
                &Gbl_hDevice
                );

        Gbl_hDevice = INVALID_HANDLE_VALUE;
    }
    else
    {
        rc =
            PlxPci_DeviceClose(
                &Gbl_DeviceObj
                );
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxCm_PciRegisterRead
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_PciRegisterRead(
    U8   bus,
    U8   slot,
    U8   function,
    U16  offset,
    U32 *pValue
    )
{
    RETURN_CODE rc;


    if (pDeviceSelected->PciMethod == PCI_ACCESS_DRIVER)
    {
        *pValue =
            PlxPciRegisterRead_Unsupported(
                bus,
                PCI_DEVFN(slot, function),
                offset,
                &rc
                );
    }
    else
    {
        *pValue =
            PlxPci_PciRegisterRead(
                bus,
                slot,
                function,
                offset,
                &rc
                );
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxCm_PciRegisterWrite
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_PciRegisterWrite(
    U8  bus,
    U8  slot,
    U8  function,
    U16 offset,
    U32 value
    )
{
    RETURN_CODE rc;


    if (pDeviceSelected->PciMethod == PCI_ACCESS_DRIVER)
    {
        rc =
            PlxPciRegisterWrite_Unsupported(
                bus,
                (U8)((function << 5) | slot),
                offset,
                value
                );
    }
    else
    {
        rc =
            PlxPci_PciRegisterWrite(
                bus,
                slot,
                function,
                offset,
                value
                );
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxCm_EepromPresent
 *
 * Description:
 *
 ******************************************************************************/
BOOLEAN
PlxCm_EepromPresent(
    RETURN_CODE *pReturnCode
    )
{
    if (pDeviceSelected->PciMethod == PCI_ACCESS_DRIVER)
    {
        return PlxSerialEepromPresent(
                   Gbl_hDevice,
                   pReturnCode
                   );
    }
    else
    {
        return PlxPci_EepromPresent(
                   &Gbl_DeviceObj,
                   pReturnCode
                   );
    }
}




/******************************************************************************
 *
 * Function   :  PlxCm_EepromReadByOffset
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_EepromReadByOffset(
    U16  offset,
    U32 *pValue
    )
{
    RETURN_CODE rc;


    if (pDeviceSelected->PciMethod == PCI_ACCESS_DRIVER)
    {
        rc =
            PlxSerialEepromReadByOffset(
                Gbl_hDevice,
                offset,
                pValue
                );
    }
    else
    {
        rc =
            PlxPci_EepromReadByOffset(
                &Gbl_DeviceObj,
                offset,
                (U16*)pValue
                );
    }

    return rc;
}




/******************************************************************************
 *
 * Function   :  PlxCm_EepromWriteByOffset
 *
 * Description:
 *
 ******************************************************************************/
RETURN_CODE
PlxCm_EepromWriteByOffset(
    U16 offset,
    U32 value
    )
{
    RETURN_CODE rc;


    if (pDeviceSelected->PciMethod == PCI_ACCESS_DRIVER)
    {
        rc =
            PlxSerialEepromWriteByOffset(
                Gbl_hDevice,
                offset,
                value
                );
    }
    else
    {
        rc =
            PlxPci_EepromWriteByOffset(
                &Gbl_DeviceObj,
                offset,
                (U16)value
                );
    }

    return rc;
}
