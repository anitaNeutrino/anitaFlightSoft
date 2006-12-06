/*********************************************************************
 *
 * File Name:
 *
 *      MonitorCommands.c
 *
 * Description:
 *
 *      Monitor command functions
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ********************************************************************/


#include <ctype.h>
#include <string.h>
#include "ConsFunc.h"
#include "CommandLine.h"
#include "Monitor.h"
#include "MonitorCommands.h"
#include "PciRegs.h"
#include "PlxApi.h"




/**********************************************************
 *
 * Function   :  Command_Version
 *
 * Description:  Display monitor version information
 *
 *********************************************************/
void
Command_Version(
    void
    )
{
    PlxPrintf(
        "PLX Console Monitor, v%d.%d%d\n\n",
        MONITOR_VERSION_MAJOR,
        MONITOR_VERSION_MINOR,
        MONITOR_VERSION_REVISION
        );
}




/**********************************************************
 *
 * Function   :  Command_Help
 *
 * Description:  Implements the monitor help command
 *
 *********************************************************/
void
Command_Help(
    void
    )
{
    PlxPrintf("\n");
    PlxPrintf("       -----------  General Commands  -----------\n");
    PlxPrintf("  cls     Clear terminal display\n");
    PlxPrintf("  reset   Reset the selected device\n");
    PlxPrintf("  ver     Display monitor version information\n");
    PlxPrintf("  help    Display help screen\n");

    PlxPrintf("\n");
    PlxPrintf("       -----------  Register Access  ------------\n");
    PlxPrintf("  pcr     Display PCI Configuration Registers\n");
    PlxPrintf("  lcr     Display Local Configuration Registers\n");
    PlxPrintf("  rtr     Display Run-time Registers\n");
    PlxPrintf("  dma     Display DMA Registers\n");
    PlxPrintf("  mqr     Display Message Queue Registers\n");
    PlxPrintf("  eep     Display EEPROM values\n");

    PlxPrintf("\n");
    PlxPrintf("       ------------  Memory Access  ------------\n");
    PlxPrintf("  db      Read memory  8-bits at a time\n");
    PlxPrintf("  dw      Read memory 16-bits at a time\n");
    PlxPrintf("  dl      Read memory 32-bits at a time\n");
    PlxPrintf("  dq      Read memory 64-bits at a time (if supported)\n");
    PlxPrintf("  eb      Write  8-bit data to memory\n");
    PlxPrintf("  ew      Write 16-bit data to memory\n");
    PlxPrintf("  el      Write 32-bit data to memory\n");
    PlxPrintf("  eq      Write 64-bit data to memory (if supported)\n");

    _Pause;

    PlxPrintf("\n");
    PlxPrintf("       -----------  I/O Port Access  ------------\n");
    PlxPrintf("  ib      Read I/O port  8-bits at a time\n");
    PlxPrintf("  iw      Read I/O port 16-bits at a time\n");
    PlxPrintf("  il      Read I/O port 32-bits at a time\n");
    PlxPrintf("  ob      Write  8-bit data to I/O port\n");
    PlxPrintf("  ow      Write 16-bit data to I/O port\n");
    PlxPrintf("  ol      Write 32-bit data to I/O port\n");

    PlxPrintf("\n");
    PlxPrintf("       --------------  Utilities  ---------------\n");
    PlxPrintf("  mf      Interactive memory fill\n");
    PlxPrintf("  ms      Interactive memory search\n");
    PlxPrintf("  mt8     Test memory in block mode (8-bit)\n");
    PlxPrintf("  mt16    Test memory in block mode (16-bit)\n");
    PlxPrintf("  mt32    Test memory in block mode (32-bit)\n");

    PlxPrintf("\n");
    PlxPrintf("       ------------  Miscellaneous  -------------\n");
    PlxPrintf("  scan    Scan all PCI buses\n");
    PlxPrintf("  vars    Display PCI BAR virtual addresses\n");
    PlxPrintf("  buff    Display Common buffer properties\n");

    PlxPrintf("\n");
}




/**********************************************************
 *
 * Function   :  Command_Dev
 *
 * Description:  Selects a new device
 *
 *********************************************************/
void
Command_Dev(
    char *Args
    )
{
    U8           DevNum;
    char         Device[10];
    DEVICE_NODE *pDeviceNew;


    GetNextArg(
        &Args,
        Device
        );

    if (!strlen(Device))
    {
        DeviceListDisplay();
        PlxPrintf("\n");
        return;
    }

    // Convert device number text to number
    DevNum =
        (U8)htol(
                Device
                );

    // Select new device
    pDeviceNew =
        GetDevice(
            DevNum,
            FALSE
            );

    if (pDeviceNew == NULL)
    {
        PlxPrintf("Error: Invalid device selection\n\n");
        return;
    }

    // Close existing device
    if (Gbl_hDevice != INVALID_HANDLE_VALUE)
    {
        // Unmap PCI BAR spaces
        PciSpacesUnmap();

        // Unmap common buffer
        PlxPciCommonBufferUnmap(
            Gbl_hDevice,
            &Gbl_PciBufferInfo.UserAddr
            );

        // Close current device
        PlxCm_DeviceClose(
            pDeviceSelected
            );

        Gbl_hDevice = INVALID_HANDLE_VALUE;
    }

    // De-select current device
    pDeviceSelected->bSelected = FALSE;

    // Select the new device
    pDeviceSelected = pDeviceNew;

    // Open the device if PLX device
    if (pDeviceSelected->PlxChipType != 0)
    {
        PlxPciDeviceOpen(
            &(pDeviceSelected->Location),
            &Gbl_hDevice
            );

        // Get virtual addresses for valid PCI spaces
        PciSpacesMap();

        // Get Common buffer properties
        GetCommonBufferProperties();
    }

    // Mark new device as selected
    pDeviceSelected->bSelected = TRUE;

    PlxPrintf(
        "Selected: %.4x %.4x [",
        pDeviceSelected->Key.DeviceId,
        pDeviceSelected->Key.VendorId
        );

    if (pDeviceSelected->PlxChipType != 0)
    {
        if ((pDeviceSelected->PlxChipType == 0x9050) &&
            (pDeviceSelected->PlxRevision == 0x2))
        {
            PlxPrintf("9052 - ");
        }
        else
        {
            PlxPrintf(
                "%04lX - ",
                pDeviceSelected->PlxChipType
                );
        }
    }

    PlxPrintf(
        "bus %.2x  slot %.2x  fn %.2x]\n\n",
        pDeviceSelected->Key.bus,
        pDeviceSelected->Key.slot,
        pDeviceSelected->Key.function
        );
}




/**********************************************************
 *
 * Function   :  Command_Variables
 *
 * Description:
 *
 *********************************************************/
void
Command_Variables(
    void
    )
{
    U8      i;
    BOOLEAN bVarExists;


    // Only display properties for PLX devices
    if (pDeviceSelected->PlxChipType == 0)
    {
        PlxPrintf(
            "Variables:  N/A with non-PLX devices\n\n"
            );

        return;
    }

    bVarExists = FALSE;

    for (i=0; i<6; i++)
    {
        if (pDeviceSelected->Va_PciBar[i] != 0)
        {
            if (!bVarExists)
            {
                // Display variables
                PlxPrintf(
                    "              Virtual\n"
                    "  Variable    Address    Description\n"
                    " ===============================================\n"
                    );

                bVarExists = TRUE;
            }

            PlxPrintf(
                "    V%d       %08lX    BAR %d Virtual Address",
                i,
                pDeviceSelected->Va_PciBar[i],
                i
                );

            if (i == 0)
            {
                PlxPrintf(
                    " (PLX registers)"
                    );
            }

            PlxPrintf(
                "\n"
                );
        }
    }

    if (!bVarExists)
    {
        PlxPrintf("Variables:  -- No mapped PCI spaces exist --\n");
    }
    else
    {
        PlxPrintf(
            "\n"
            "NOTE:\n"
            "   The use of variables in the command-line is not yet\n"
            "   supported.  The BARs are mapped, therefore, the region\n"
            "   is still available by typing the virtual address directly.\n"
            "   For example, if V2=82010000, use \"dl 82010000\".\n"
            );
    }
    PlxPrintf(
        "\n"
        );
}




/**********************************************************
 *
 * Function   :  Command_DisplayCommonBufferProperties
 *
 * Description:
 *
 *********************************************************/
void
Command_DisplayCommonBufferProperties(
    void
    )
{
    // Only display properties for valid buffer
    if ((Gbl_PciBufferInfo.Size == 0) ||
        (pDeviceSelected->PlxChipType == 0))
    {
        // Check if device is supported
        if (pDeviceSelected->PlxChipType == 0)
        {
            PlxPrintf(
                "Host common buffer:  N/A with non-PLX devices\n\n"
                );
        }
        else
        {
            PlxPrintf(
                "Host common buffer:  N/A with %04lX device\n\n",
                pDeviceSelected->PlxChipType
                );
        }

        return;
    }

    // Display properties
    PlxPrintf(
        "Host common buffer (%04lX device):\n"
        "  PCI address : %08lXh\n"
        "  Virtual addr: %08lXh\n"
        "  Size        : %08lXh (%ld Kb)\n\n",
        pDeviceSelected->PlxChipType,
        Gbl_PciBufferInfo.PhysicalAddr,
        Gbl_PciBufferInfo.UserAddr,
        Gbl_PciBufferInfo.Size,
        (Gbl_PciBufferInfo.Size >> 10)
        );
}




/**********************************************************
 *
 * Function   :  Command_Scan
 *
 * Description:
 *
 *********************************************************/
void
Command_Scan(
    void
    )
{
    U8          bus;
    U8          slot;
    U8          function;
    U8          DevicesFound;
    U32         RegValue;
    BOOLEAN     bMultiFunction;
    RETURN_CODE rc;


    DevicesFound = 0;

    PlxPrintf(
        "\n"
        " Bus Slot Fn  Dev  Ven   Device Type\n"
        "===============================================\n"
        );

    for (bus=0; bus < 32; bus++)
    {
        // Check for user abort
        if (Plx_kbhit())
        {
            break;
        }

        for (slot=0; slot < 32; slot++)
        {
            // Check for user abort
            if (Plx_kbhit())
            {
                break;
            }

            function       = 0;
            bMultiFunction = FALSE;

            do
            {
                PlxPrintf(
                    "  %02x  %02x  %02x",
                    bus,
                    slot,
                    function
                    );

                rc =
                    PlxCm_PciRegisterRead(
                        bus,
                        slot,
                        function,
                        CFG_VENDOR_ID,
                        &RegValue
                        );

                if ((rc == ApiSuccess) && (RegValue != (U32)-1))
                {
                    PlxPrintf(
                        "  %04x %04x",
                        (U16)(RegValue >> 16),
                        (U16)RegValue
                        );

                    /* Get device class */
                    PlxCm_PciRegisterRead(
                        bus,
                        slot,
                        function,
                        CFG_REV_ID,
                        &RegValue
                        );

                    PlxPrintf(
                        "  %s\n",
                        Device_GetClassString(RegValue)
                        );

                    /* Get Header type */
                    PlxCm_PciRegisterRead(
                        bus,
                        slot,
                        function,
                        CFG_CACHE_SIZE,
                        &RegValue
                        );

                    // Check for multi-function device
                    if ((U8)(RegValue >> 16) & (1 << 7))
                    {
                        bMultiFunction = TRUE;
                    }

                    DevicesFound++;
                }
                else
                {
                    PlxPrintf("\r");
                }

                // Increment function
                function++;
            }
            while (bMultiFunction && (function <= 7));
        }
    }

    PlxPrintf(
        "               \n"
        "PCI Bus Scan: "
        );

    if (Plx_kbhit())
    {
        // Clear the character
        Plx_getch();
        PlxPrintf("-Aborted-\n\n");
    }
    else
    {
        PlxPrintf(
            "%d devices found\n\n",
            DevicesFound
            );
    }
}




/**********************************************************
 *
 * Function   :  Command_Reset
 *
 * Description:  Resets a PLX device
 *
 *********************************************************/
void
Command_Reset(
    void
    )
{
    // Check if device is supported
    if (pDeviceSelected->PlxChipType == 0)
    {
        PlxPrintf("Error: This command only supports PLX devices\n\n");
        return;
    }

    PlxPrintf("Resetting device....");

    PlxPciBoardReset(
        Gbl_hDevice
        );

    PlxPrintf("Ok\n\n");
}




/**********************************************************
 *
 * Function   :  Command_PciRegs
 *
 * Description:  Implements the PCI register command
 *
 *********************************************************/
void
Command_PciRegs(
    char *Args
    )
{
    U8            i;
    U8            NumArgs;
    U8            LineCount;
    U16           Offset;
    U32           Value;
    char          buffer[255];
    REGISTER_SET *RegSet;


    // Select correct data set
    switch (pDeviceSelected->PlxChipType)
    {
        case 0x9050:
        case 0x9080:
            RegSet = PciDefault;
            break;

        case 0x9030:
        case 0x9056:
        case 0x9656:
        case 0x9054:
            RegSet = Pci9054;
            break;

        default:
            RegSet = PciDefault;
            break;
    }

    NumArgs = 0;
    Offset  = 0;
    Value   = 0;

    do
    {
        GetNextArg(
            &Args,
            buffer
            );

        i = (U8)strlen(buffer);

        if (i != 0)
        {
            NumArgs++;
            if (NumArgs == 1)
                Offset = (U16)htol(buffer);
            else
                Value = (U32)htol(buffer);
        }
    }
    while (i != 0);

    i         = 0;
    LineCount = 0;

    while ((RegSet[i].Offset != 0) || (RegSet[i].Description[0] != '\0'))
    {
        if (NumArgs == 0)
        {
            LineCount++;
            if (LineCount == 23)
            {
                PlxPrintf(" -- Press any key to continue ---");
                Plx_getch();

                PlxPrintf("\r                                 \r");
                LineCount = 0;
            }

            PlxCm_PciRegisterRead(
                pDeviceSelected->Key.bus,
                pDeviceSelected->Key.slot,
                pDeviceSelected->Key.function,
                RegSet[i].Offset,
                &Value
                );

            PlxPrintf(
                " %03x: %08lx  %s\n",
                RegSet[i].Offset,
                Value,
                RegSet[i].Description
                );
        }
        else
        {
            if (RegSet[i].Offset == Offset)
            {
                if (NumArgs == 1)
                {
                    PlxCm_PciRegisterRead(
                        pDeviceSelected->Key.bus,
                        pDeviceSelected->Key.slot,
                        pDeviceSelected->Key.function,
                        Offset,
                        &Value
                        );

                    PlxPrintf(
                        " %03x: %08lx  %s\n",
                        Offset,
                        Value,
                        RegSet[i].Description
                        );

                    return;
                }
                else
                {
                    PlxCm_PciRegisterWrite(
                        pDeviceSelected->Key.bus,
                        pDeviceSelected->Key.slot,
                        pDeviceSelected->Key.function,
                        Offset,
                        Value
                        );

                    return;
                }
            }
        }                

        i++;
    }

    if (NumArgs == 1)
    {
        PlxCm_PciRegisterRead(
            pDeviceSelected->Key.bus,
            pDeviceSelected->Key.slot,
            pDeviceSelected->Key.function,
            Offset,
            &Value
            );

        PlxPrintf(
            " %03x: %08lx  (unknown register)\n",
            Offset,
            Value
            );
    }
    else if (NumArgs > 1)
    {
        PlxCm_PciRegisterWrite(
            pDeviceSelected->Key.bus,
            pDeviceSelected->Key.slot,
            pDeviceSelected->Key.function,
            Offset,
            Value
            );
    }
}




/**********************************************************
 *
 * Function   :  Command_Regs
 *
 * Description:  Implements the register commands
 *
 *********************************************************/
void
Command_Regs(
    U8    Regs,
    char *Args
    )
{
    U8            i;
    U8            NumArgs;
    U8            LineCount;
    U16           Offset;
    U32           Value;
    char          buffer[255];
    RETURN_CODE   rc;
    REGISTER_SET *RegSet;


    // Check if device is supported
    if (pDeviceSelected->PlxChipType == 0)
    {
        PlxPrintf("Error: This command only supports PLX devices\n\n");
        return;
    }

    RegSet = NULL;

    // Select desired register set
    switch (pDeviceSelected->PlxChipType)
    {
        case 0x9050:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9050;
                    break;
            }
            break;

        case 0x9030:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9030;
                    break;
            }
            break;

        case 0x9080:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9080;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9080;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        case 0x9056:
        case 0x9656:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9656;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9054;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        case 0x9054:
            switch (Regs)
            {
                case REGS_LCR:
                    RegSet = Lcr9054;
                    break;

                case REGS_RTR:
                    RegSet = Rtr9080;
                    break;

                case REGS_DMA:
                    RegSet = Dma9054;
                    break;

                case REGS_MQR:
                    RegSet = Mqr9080;
                    break;
            }
            break;

        default:
            PlxPrintf(
                "** Local registers not available on %lX devices **\n\n",
                pDeviceSelected->PlxChipType
                );
            return;
    }

    if (RegSet == NULL)
    {
        PlxPrintf(
            "Error:  Invalid register set for %lX\n\n",
            pDeviceSelected->PlxChipType
            );
        return;
    }

    NumArgs = 0;
    Offset  = 0;
    Value   = 0;

    do
    {
        GetNextArg(
            &Args,
            buffer
            );

        i = (U8)strlen(buffer);

        if (i != 0)
        {
            NumArgs++;
            if (NumArgs == 1)
                Offset = (U16)htol(buffer);
            else
                Value = (U32)htol(buffer);
        }
    }
    while (i != 0);

    i         = 0;
    LineCount = 0;

    while ((RegSet[i].Offset != 0) || (RegSet[i].Description[0] != '\0'))
    {
        if (NumArgs == 0)
        {
            LineCount++;
            if (LineCount == 24)
            {
                PlxPrintf(" -- Press any key to continue ---");
                Plx_getch();

                PlxPrintf("\r                                 \r");
                LineCount = 0;
            }

            Value =
                PlxRegisterRead(
                    Gbl_hDevice,
                    RegSet[i].Offset,
                    &rc
                    );

            PlxPrintf(
                " %03x: %08lx  %s\n",
                RegSet[i].Offset,
                Value,
                RegSet[i].Description
                );
        }
        else
        {
            if (RegSet[i].Offset == Offset)
            {
                if (NumArgs == 1)
                {
                    Value =
                        PlxRegisterRead(
                            Gbl_hDevice,
                            Offset,
                            &rc
                            );

                    PlxPrintf(
                        " %03x: %08lx  %s\n",
                        Offset,
                        Value,
                        RegSet[i].Description
                        );

                    return;
                }
                else
                {
                    PlxRegisterWrite(
                        Gbl_hDevice,
                        Offset,
                        Value
                        );

                    return;
                }
            }
        }                

        i++;
    }

    if (NumArgs == 1)
    {
        Value =
            PlxRegisterRead(
                Gbl_hDevice,
                Offset,
                &rc
                );

        PlxPrintf(
            " %03x: %08lx  (unknown register)\n",
            Offset,
            Value
            );
    }
    else if (NumArgs > 1)
    {
        PlxRegisterWrite(
            Gbl_hDevice,
            Offset,
            Value
            );
    }
}




/**********************************************************
 *
 * Function   :  Command_Eep
 *
 * Description:  Implements the EEPROM commands
 *
 *********************************************************/
void
Command_Eep(
    char *Args
    )
{
    U8            i;
    U8            NumArgs;
    U8            LineCount;
    U16           Offset;
    U32           Value;
    char          buffer[255];
    RETURN_CODE   rc;
    REGISTER_SET *RegSet;


    // EEPROM access requires standard driver for PLX 9000 series
    if ((pDeviceSelected->PlxChipType > 0x9000) &&
        (pDeviceSelected->PciMethod != PCI_ACCESS_DRIVER))
    {
        PlxPrintf(
            "Error: PLX %04X driver must be loaded for EEPROM access\n\n",
            pDeviceSelected->PlxChipType
            );
        return;
    }

    // Select correct data set
    switch (pDeviceSelected->PlxChipType)
    {
        case 0x9050:
            RegSet = Eep9050;
            break;

        case 0x9030:
            RegSet = Eep9030;
            break;

        case 0x9080:
            RegSet = Eep9080;
            break;

        case 0x9054:
            RegSet = Eep9054;
            break;

        case 0x9056:
        case 0x9656:
            RegSet = Eep9656;
            break;

        case 0:
            PlxPrintf("Error: This command only supports PLX devices\n\n");
            return;

        default:
            PlxPrintf(
                "** %lX EEPROM access not implemented\n\n",
                pDeviceSelected->PlxChipType
                );
            return;
    }

    // Check if an EEPROM is present
    if (PlxCm_EepromPresent(
            &rc
            ) == FALSE)
    {
        PlxPrintf(" -- The PLX chip reports no EEPROM present --\n");
        PlxPrintf("\n");
        PlxPrintf("Do you want to proceed? [y/n]?");

        // Wait for input
        i = (U8)Plx_getch();

        PlxPrintf("%c\n\n", i);

        if (i != 'y' && i != 'Y')
        {
            return;
        }
    }

    NumArgs = 0;
    Offset  = 0;
    Value   = 0;

    do
    {
        GetNextArg(
            &Args,
            buffer
            );

        i = (U8)strlen(buffer);

        if (i != 0)
        {
            NumArgs++;
            if (NumArgs == 1)
                Offset = (U16)htol(buffer);
            else
                Value = (U32)htol(buffer);
        }
    }
    while (i != 0);

    i         = 0;
    LineCount = 0;

    while ((RegSet[i].Offset != 0) || (RegSet[i].Description[0] != '\0'))
    {
        if (NumArgs == 0)
        {
            LineCount++;
            if (LineCount == 24)
            {
                PlxPrintf(" -- Press any key to continue ---");
                Plx_getch();
                PlxPrintf("\r                                 \r");
                LineCount = 0;
            }

            PlxCm_EepromReadByOffset(
                RegSet[i].Offset,
                &Value
                );

            PlxPrintf(
                " %03x: %08lx  %s\n",
                RegSet[i].Offset,
                Value,
                RegSet[i].Description
                );
        }
        else
        {
            if (RegSet[i].Offset == Offset)
            {
                if (NumArgs == 1)
                {
                    PlxCm_EepromReadByOffset(
                        Offset,
                        &Value
                        );

                    PlxPrintf(
                        " %03x: %08lx  %s\n",
                        Offset,
                        Value,
                        RegSet[i].Description
                        );

                    return;
                }
                else
                {
                    PlxCm_EepromWriteByOffset(
                        Offset,
                        Value
                        );

                    return;
                }
            }
        }                

        i++;
    }

    if (NumArgs == 1)
    {
        PlxCm_EepromReadByOffset(
            Offset,
            &Value
            );

        PlxPrintf(
            " %03x: %08lx  (unknown EEPROM location)\n",
            Offset,
            Value
            );
    }
    else if (NumArgs > 1)
    {
        PlxCm_EepromWriteByOffset(
            Offset,
            Value
            );
    }
}




/**********************************************************
 *
 * Function   :  Command_MemRead
 *
 * Description:
 *
 *********************************************************/
void
Command_MemRead(
    char *Args,
    char  size
    )
{
    int         i;
    int         x;
    int         CharsToPrint;
    int         SpacesToInsert;
    char        buffer[20];
    char       *CurrArg;
    U8          Repeat;
    U32         EndAddr;
    static U32  CurrAddr = 0;
    BOOLEAN     Done;
    BOOLEAN     LineComplete;
    ULONGLONG   CurrValue;


    do
    {
        i       = 0;
        Repeat  = 0;
        CurrArg = Args;
        EndAddr = CurrAddr + 0x80;

        do
        {
            GetNextArg(
                &CurrArg,
                buffer
                );

            if (!strlen(buffer))
                break;

            if ((buffer[0] == 'r') || (buffer[0] == 'R'))
            {
                Repeat = 1;
            }
            else
            {
                i++;
                if (i == 1)
                {
                    CurrAddr = (U32)htol(buffer);
                    EndAddr  = CurrAddr + 0x80;
                }
                else
                {
                    EndAddr = CurrAddr + (U32)htol(buffer);
                }
            }
        }
        while (CurrArg != NULL);

        i              = 0;
        Done           = FALSE;
        LineComplete   = FALSE;
        CharsToPrint   = 0;
        SpacesToInsert = 0;

        while (!Done)
        {
            if (Plx_kbhit())
            {
                // Clear the character
                Plx_getch();

                Repeat = 0;
                break;
            }

            if (i == 0)
            {
                // Set initial number of characters to display
                CharsToPrint = 0;

                // Set initial Spaces to insert
                SpacesToInsert = (U8)((((size * 2) + 1) * (16 / size)) + 3);

                PlxPrintf("%08lx: ", CurrAddr);
            }

            switch (size)
            {
                case sizeof(U8):
                    CurrValue = *(U8*)CurrAddr;
                    PlxPrintf("%02x ", (U8)CurrValue);
                    break;

                case sizeof(U16):
                    CurrValue = *(U16*)CurrAddr;
                    PlxPrintf("%04x ", (U16)CurrValue);
                    break;

                case sizeof(U32):
                    CurrValue = *(U32*)CurrAddr;
                    PlxPrintf("%08lx ", CurrValue);
                    break;

                case sizeof(ULONGLONG):
                    CurrValue = *(ULONGLONG*)CurrAddr;
                    PlxPrintf("%016qx ", (ULONGLONG)CurrValue);
                    break;
            }

            // Store value for printing
            for (x=0; x<size; x++)
            {
                buffer[i+x] = ((U8*)(&CurrValue))[x];

                // Verify printable character
                if ((buffer[i+x] != ' ') &&
                    (!isgraph(buffer[i+x])))
                {
                    buffer[i+x] = '.';
                }
            }

            i        += size;
            CurrAddr += size;

            // Adjust characters to print
            CharsToPrint += size;

            // Adjust spaces
            SpacesToInsert -= (size * 2) + 1;

            if (CurrAddr >= EndAddr)
            {
                Done         = TRUE;
                LineComplete = TRUE;
            }

            if (!LineComplete)
            {
                if (i == 8)
                {
                    SpacesToInsert -= 2;
                    PlxPrintf("- ");
                }
                else if (i == 16)
                {
                    i = 0;
                    LineComplete = TRUE;
                }
            }

            if (LineComplete)
            {
                LineComplete = FALSE;

                // Insert necessary spaces
                while (SpacesToInsert--)
                    PlxPrintf(" ");

                // Display characters at end of line
                for (x=0; x<CharsToPrint; x++)
                {
                    PlxPrintf("%c", buffer[x]);
                }

                PlxPrintf("\n");
            }
        }

        if (Repeat)
            PlxPrintf("\n");
    }
    while (Repeat);
}




/**********************************************************
 *
 * Function   :  Command_MemWrite
 *
 * Description:
 *
 *********************************************************/
void
Command_MemWrite(
    char *Args,
    char  size
    )
{
    U8         i;
    U8         Repeat;
    U32        Addr;
    char       buffer[20];
    char      *CurrArg;
    ULONGLONG  Value;


    do
    {
        i       = 0;
        Addr    = 0;
        Repeat  = 0;
        CurrArg = Args;

        do
        {
            GetNextArg(
                &CurrArg,
                buffer
                );

            if (!strlen(buffer))
            {
                if (i <= 1)
                    PlxPrintf("??? Error: missing argument(s)\n");
                break;
            }

            if (!strcmp(buffer, "r") || !strcmp(buffer, "R"))
                Repeat = 1;
            else
            {
                i++;
                if (i == 1)
                    Addr = (U32)htol(buffer);
                else
                {
                    Value = htol(buffer);
                    switch (size)
                    {
                        case sizeof(U8):
                            *(U8*)Addr = (U8)Value;
                            break;

                        case sizeof(U16):
                            *(U16*)Addr = (U16)Value;
                            break;

                        case sizeof(U32):
                            *(U32*)Addr = (U32)Value;
                            break;

                        case sizeof(ULONGLONG):
                            *(ULONGLONG*)Addr = Value;
                            break;
                    }

                    Addr += size;
                }
            }
        }
        while (CurrArg != NULL);
    }
    while (Repeat);
}




/**********************************************************
 *
 * Function   :  Command_IoRead
 *
 * Description:
 *
 *********************************************************/
void
Command_IoRead(
    char *Args,
    char  size
    )
{
    int         i;
    int         x;
    int         CharsToPrint;
    int         SpacesToInsert;
    char        buffer[20];
    char       *CurrArg;
    U8          Repeat;
    U32         EndAddr;
    U32         CurrValue;
    static U32  CurrAddr = 0;
    BOOLEAN     Done;
    BOOLEAN     LineComplete;


    do
    {
        i       = 0;
        Repeat  = 0;
        CurrArg = Args;
        EndAddr = CurrAddr + 0x80;

        do
        {
            GetNextArg(
                &CurrArg,
                buffer
                );

            if (!strlen(buffer))
                break;

            if ((buffer[0] == 'r') || (buffer[0] == 'R'))
            {
                Repeat = 1;
            }
            else
            {
                i++;
                if (i == 1)
                {
                    CurrAddr = (U32)htol(buffer);
                    EndAddr  = CurrAddr + 0x80;
                }
                else
                {
                    EndAddr = CurrAddr + (U32)htol(buffer);
                }
            }
        }
        while (CurrArg != NULL);

        i              = 0;
        Done           = FALSE;
        LineComplete   = FALSE;
        CharsToPrint   = 0;
        SpacesToInsert = 0;

        while (!Done)
        {
            if (Plx_kbhit())
            {
                // Clear the character
                Plx_getch();

                Repeat = 0;
                break;
            }

            if (i == 0)
            {
                // Set initial number of characters to display
                CharsToPrint = 0;

                // Set initial Spaces to insert
                SpacesToInsert = (U8)((((size * 2) + 1) * (16 / size)) + 3);

                PlxPrintf("%08x: ", (U32)CurrAddr);
            }

            switch (size)
            {
                case sizeof(U8):
                    PlxIoPortRead(
                        Gbl_hDevice,
                        CurrAddr,
                        BitSize8,
                        &CurrValue
                        );
                    PlxPrintf("%02x ", (U8)CurrValue);
                    break;

                case sizeof(U16):
                    PlxIoPortRead(
                        Gbl_hDevice,
                        CurrAddr,
                        BitSize16,
                        &CurrValue
                        );
                    PlxPrintf("%04x ", (U16)CurrValue);
                    break;

                case sizeof(U32):
                    PlxIoPortRead(
                        Gbl_hDevice,
                        CurrAddr,
                        BitSize32,
                        &CurrValue
                        );
                    PlxPrintf("%08x ", (U32)CurrValue);
                    break;
            }

            // Store value for printing
            for (x=0; x<size; x++)
            {
                buffer[i+x] = ((U8*)(&CurrValue))[x];

                // Verify printable character
                if ((buffer[i+x] != ' ') &&
                    (!isgraph(buffer[i+x])))
                {
                    buffer[i+x] = '.';
                }
            }

            i        += size;
            CurrAddr += size;

            // Adjust characters to print
            CharsToPrint += size;

            // Adjust spaces
            SpacesToInsert -= (size * 2) + 1;

            if (CurrAddr >= EndAddr)
            {
                Done         = TRUE;
                LineComplete = TRUE;
            }

            if (!LineComplete)
            {
                if (i == 8)
                {
                    SpacesToInsert -= 2;
                    PlxPrintf("- ");
                }
                else if (i == 16)
                {
                    i = 0;
                    LineComplete = TRUE;
                }
            }

            if (LineComplete)
            {
                LineComplete = FALSE;

                // Insert necessary spaces
                while (SpacesToInsert--)
                    PlxPrintf(" ");

                // Display characters at end of line
                for (x=0; x<CharsToPrint; x++)
                {
                    PlxPrintf("%c", buffer[x]);
                }

                PlxPrintf("\n");
            }
        }

        if (Repeat)
            PlxPrintf("\n");
    }
    while (Repeat);
}




/**********************************************************
 *
 * Function   :  Command_IoWrite
 *
 * Description:
 *
 *********************************************************/
void
Command_IoWrite(
    char *Args,
    char  size
    )
{
    U8    i;
    U8    Repeat;
    U32   Addr;
    char  buffer[20];
    char *CurrArg;
    U32   Value;


    do
    {
        i       = 0;
        Addr    = 0;
        Repeat  = 0;
        CurrArg = Args;

        do
        {
            GetNextArg(
                &CurrArg,
                buffer
                );

            if (!strlen(buffer))
            {
                if (i <= 1)
                    PlxPrintf("??? Error: missing argument(s)\n");
                break;
            }

            if (!strcmp(buffer, "r") || !strcmp(buffer, "R"))
                Repeat = 1;
            else
            {
                i++;
                if (i == 1)
                    Addr = (U32)htol(buffer);
                else
                {
                    Value = (U32)htol(buffer);
                    switch (size)
                    {
                        case sizeof(U8):
                            PlxIoPortWrite(
                                Gbl_hDevice,
                                Addr,
                                BitSize8,
                                &Value
                                );
                            break;

                        case sizeof(U16):
                            PlxIoPortWrite(
                                Gbl_hDevice,
                                Addr,
                                BitSize16,
                                &Value
                                );
                            break;

                        case sizeof(U32):
                            PlxIoPortWrite(
                                Gbl_hDevice,
                                Addr,
                                BitSize32,
                                &Value
                                );
                            break;
                    }

                    Addr += size;
                }
            }
        }
        while (CurrArg != NULL);
    }
    while (Repeat);
}
