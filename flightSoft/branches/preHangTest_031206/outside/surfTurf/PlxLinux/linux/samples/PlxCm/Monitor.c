/******************************************************************************
 *
 * File Name:
 *
 *      Monitor.c
 *
 * Description:
 *
 *      Monitor main module
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ConsFunc.h"
#include "CommandLine.h"
#include "Monitor.h"
#include "MonitorCommands.h"
#include "PlxApi.h"
#include "RegisterDefs.h"




/**********************************************
 *                 Globals
 **********************************************/
char               CmdHistory[MAX_CMD_HISTORY][MAX_BUFFER_SZ];
HANDLE             Gbl_hDevice;
PCI_MEMORY         Gbl_PciBufferInfo;
DEVICE_NODE       *pDeviceSelected;
PLX_DEVICE_OBJECT  Gbl_DeviceObj;




/**********************************************************
 *
 * Function   :  main
 *
 * Description:
 *
 *********************************************************/
int
main(
     void
     )
{
    // Initialize the console
    ConsoleInitialize();

    // Make sure no device is selected
    pDeviceSelected = NULL;
    Gbl_hDevice     = INVALID_HANDLE_VALUE;

    // Clear DMA buffer information
    Gbl_PciBufferInfo.UserAddr     = (U32)-1;
    Gbl_PciBufferInfo.PhysicalAddr = (U32)-1;
    Gbl_PciBufferInfo.Size         = 0;


    // Build device list
    DeviceListCreate();

    // Start the monitor
    Monitor();

    // Free device list
    DeviceListFree();

    // Make sure device is closed
    if (Gbl_hDevice != INVALID_HANDLE_VALUE)
    {
        // Unmap PCI BAR spaces
        PciSpacesUnmap();

        // Unmap common buffer
        PlxPciCommonBufferUnmap(
            Gbl_hDevice,
            &Gbl_PciBufferInfo.UserAddr
            );

        PlxPciDeviceClose(
            Gbl_hDevice
            );
    }

    // Restore console
    ConsoleEnd();

    exit(0);
}




/**********************************************************
 *
 * Function   :  Monitor
 *
 * Description:
 *
 *********************************************************/
void
Monitor(
    void
    )
{
    U8      i;
    int     CurrCmd;
    int     FirstCmd;
    int     LastCmd;
    int     NextInp;
    char    buffer[MAX_BUFFER_SZ];
    BOOLEAN bFlag;


    // Select first PLX device
    pDeviceSelected =
        GetDevice(
            0,
            TRUE
            );

    if (pDeviceSelected == NULL)
    {
        PlxPrintf(
            "\n"
            "             Error: ** No PCI Devices found **\n"
            "\n"
            "                  Press any key to exit..."
            );

        // Wait for keypress
        Plx_getch();

        PlxPrintf("\n");

        return;
    }

    // Select the device
    PlxCm_DeviceOpen(
        pDeviceSelected
        );

    pDeviceSelected->bSelected = TRUE;

    // Get virtual addresses for valid PCI spaces
    PciSpacesMap();

    // Get Common buffer properties
    GetCommonBufferProperties();

    PlxPrintf(
        "\n\n"
        "                *************  NOTE  *************\n"
        "       This utility is provided as-is with no warranty or\n"
        "       support. It provides limited functionality.  The complete\n"
        "       source code is provided for those wishing to use it as\n"
        "       a reference or modify it to extend functionality.\n"
        "                **********************************\n"
        "\n"
        "                    Press any key to continue..."
        );

    // Wait for keypress
    Plx_getch();
    Cls();

    // Display devices
    DeviceListDisplay();

    PlxPrintf(
        "\n"
        "PLX Console Monitor, v%d.%d%d\n"
        "Copyright (c) 2003, PLX Technology, Inc.\n\n",
        MONITOR_VERSION_MAJOR,
        MONITOR_VERSION_MINOR,
        MONITOR_VERSION_REVISION
        );

    // Initialize variables
    i           = 0;
    CurrCmd     = 0;
    FirstCmd    = 0;
    LastCmd     = 0;

    memset(
        CmdHistory,
        '\0',
        sizeof(CmdHistory)
        );

    memset(
        buffer,
        '\0',
        MAX_BUFFER_SZ
        );

    PlxPrintf(MONITOR_PROMPT);

    do
    {
        // Wait for keypress
        NextInp = Plx_getch();

        switch (NextInp)
        {
            case 0:                 // Sporadic 0's sometimes appear, ignore
                break;

            case '\n':
            case '\r':
                PlxPrintf("\n");

                if (i != 0)
                {
                    strcpy(
                        CmdHistory[FirstCmd],
                        buffer
                        );

                    if (ProcessCommand(
                            CmdHistory[FirstCmd]
                            ) == FALSE)
                    {
                        // Exit selected
                        return;
                    }

                    FirstCmd = (FirstCmd + 1) % MAX_CMD_HISTORY;
                    CurrCmd  = FirstCmd;
                    if (LastCmd == FirstCmd)
                        LastCmd = (LastCmd + 1) % MAX_CMD_HISTORY;

                    CmdHistory[FirstCmd][0] = '\0';

                    memset(
                        buffer,
                        '\0',
                        i
                        );

                    i = 0;
                }

                PlxPrintf(MONITOR_PROMPT);
                break;

            case '\b':                // BackSpace
            case 127:
                if (i != 0)
                {
                    i--;
                    buffer[i] = '\0';
                    PlxPrintf("\b \b");
                }
                break;

            case '\t':
                // Ignore TAB
                break;

            case 27:                  // ESC

                // Delay for a bit to wait for additional keys
                Plx_Sleep_ms(100);

                // Process ESC only of no other key is pressed
                if (Plx_kbhit() == FALSE)
                {
                    // Clear the command-line
                    while (i)
                    {
                        PlxPrintf("\b \b");
                        i--;
                        buffer[i] = '\0';
                    }
                }
                break;

            case 0xe0:              // Extended key (Windows)
            case '[':               // Extended key (Linux)

                bFlag = TRUE;

                // Get extended code
                switch (Plx_getch())
                {
                    case 'A':            // Up arrow (Linux)
                    case 'H':            // Up arrow (Windows)
                        if (CurrCmd != LastCmd)
                        {
                            if (CurrCmd == 0)
                                CurrCmd = MAX_CMD_HISTORY - 1;
                            else
                                CurrCmd = CurrCmd - 1;
                        }
                        break;

                    case 'B':           // Down arrow (Linux)
                    case 'P':           // Down arrow (Windows)
                        if (CurrCmd != FirstCmd)
                        {
                            if (CurrCmd == MAX_CMD_HISTORY - 1)
                                CurrCmd = 0;
                            else
                                CurrCmd = CurrCmd + 1;
                        }
                        break;

                    case 'D':           // Left arrow (Linux)
                    case 'K':           // Left arrow (Windows)
                        // Not currently handled
                        bFlag = FALSE;
                        break;

                    case 'C':           // Right arrow (Linux)
                    case 'M':           // Right arrow (Windows)
                        // Not currently handled
                        bFlag = FALSE;
                        break;

                    default:
                        // Unsupported key, do nothing
                        bFlag = FALSE;
                        break;
                }

                if (bFlag)
                {
                    // Clear the command-line
                    while (i)
                    {
                        PlxPrintf("\b \b");
                        i--;
                        buffer[i] = '\0';
                    }

                    // Copy command from history
                    strcpy(
                        buffer,
                        CmdHistory[CurrCmd]
                        );

                    // Display new command
                    PlxPrintf(
                        buffer
                        );

                    i = (U8)strlen(buffer);
                }
                break;

            default:
                // Check for overflow
                if (i < (MAX_BUFFER_SZ - 1))
                {
                    buffer[i] = (char)NextInp;
                    PlxPrintf("%c", (char)NextInp);
                    i++;
                }
                break;
        }
    }
    while (1);
}




/**********************************************************
 *
 * Function   :  ProcessCommand
 *
 * Description:
 *
 *********************************************************/
BOOLEAN
ProcessCommand(
    char *buffer
    )
{
    U8   i;
    char Cmd[MAX_CMD_SZ];


    GetNextArg(
        &buffer,
        Cmd
        );

    // Convert command to lower-case
    i=0;
    while (Cmd[i] != '\0')
    {
        Cmd[i] = (char)tolower(Cmd[i]);
        i++;
    }

    if (!strcmp(Cmd, "cls") || !strcmp(Cmd, "clear"))
    {
        Cls();
    }
    else if (!strcmp(Cmd, "exit") ||
             !strcmp(Cmd, "quit") ||
             !strcmp(Cmd, "q"))
    {
        return FALSE;
    }
    else if (!strcmp(Cmd, "ver"))
    {
        Command_Version();
    }
    else if (!strcmp(Cmd, "?") || !strcmp(Cmd, "help"))
    {
        Command_Help();
    }
    else if (!strcmp(Cmd, "dev"))
    {
        Command_Dev(
            buffer
            );
    }
    else if (!strcmp(Cmd, "vars"))
    {
        Command_Variables();
    }
    else if (!strcmp(Cmd, "buff"))
    {
        Command_DisplayCommonBufferProperties();
    }
    else if (!strcmp(Cmd, "scan"))
    {
        Command_Scan();
    }
    else if (!strcmp(Cmd, "reset"))
    {
        Command_Reset();
    }
    else if (!strcmp(Cmd, "pcr"))
    {
        Command_PciRegs(
            buffer
            );
    }
    else if (!strcmp(Cmd, "lcr"))
    {
        Command_Regs(
            REGS_LCR,
            buffer
            );
    }
    else if (!strcmp(Cmd, "rtr"))
    {
        Command_Regs(
            REGS_RTR,
            buffer
            );
    }
    else if (!strcmp(Cmd, "dma"))
    {
        Command_Regs(
            REGS_DMA,
            buffer
            );
    }
    else if (!strcmp(Cmd, "mqr"))
    {
        Command_Regs(
            REGS_MQR,
            buffer
            );
    }
    else if (!strcmp(Cmd, "eep"))
    {
        Command_Eep(
            buffer
            );
    }
    else if (!strcmp(Cmd, "db"))
    {
        Command_MemRead(
            buffer,
            sizeof(U8)
            );
    }
    else if (!strcmp(Cmd, "dw"))
    {
        Command_MemRead(
            buffer,
            sizeof(U16)
            );
    }
    else if (!strcmp(Cmd, "dl"))
    {
        Command_MemRead(
            buffer,
            sizeof(U32)
            );
    }
    else if (!strcmp(Cmd, "dq"))
    {
        Command_MemRead(
            buffer,
            sizeof(ULONGLONG)
            );
    }
    else if (!strcmp(Cmd, "eb"))
    {
        Command_MemWrite(
            buffer,
            sizeof(U8)
            );
    }
    else if (!strcmp(Cmd, "ew"))
    {
        Command_MemWrite(
            buffer,
            sizeof(U16)
            );
    }
    else if (!strcmp(Cmd, "el"))
    {
        Command_MemWrite(
            buffer,
            sizeof(U32)
            );
    }
    else if (!strcmp(Cmd, "eq"))
    {
        Command_MemWrite(
            buffer,
            sizeof(ULONGLONG)
            );
    }
    else if (!strcmp(Cmd, "ib"))
    {
        Command_IoRead(
            buffer,
            sizeof(U8)
            );
    }
    else if (!strcmp(Cmd, "iw"))
    {
        Command_IoRead(
            buffer,
            sizeof(U16)
            );
    }
    else if (!strcmp(Cmd, "il"))
    {
        Command_IoRead(
            buffer,
            sizeof(U32)
            );
    }
    else if (!strcmp(Cmd, "ob"))
    {
        Command_IoWrite(
            buffer,
            sizeof(U8)
            );
    }
    else if (!strcmp(Cmd, "ow"))
    {
        Command_IoWrite(
            buffer,
            sizeof(U16)
            );
    }
    else if (!strcmp(Cmd, "ol"))
    {
        Command_IoWrite(
            buffer,
            sizeof(U32)
            );
    }
    else if (!strcmp(Cmd, "mf"))
    {
        PlxPrintf(" ** Not implemented yet **\n\n");
    }
    else if (!strcmp(Cmd, "ms"))
    {
        PlxPrintf(" ** Not implemented yet **\n\n");
    }
    else
    {
        PlxPrintf(
            "\"%s\" is not a valid command. Type "
            "\"help\" for more information.\n\n",
            buffer-strlen(Cmd)
            );
    }

    return TRUE;
}




/**********************************************************
 *
 * Function   :  PciSpacesMap
 *
 * Description:
 *
 *********************************************************/
void
PciSpacesMap(
    void
    )
{
    U8 i;


    for (i=0; i<6; i++)
    {
        // Map buffer into user space
        PlxPciBarMap(
            Gbl_hDevice,
            i,
            &(pDeviceSelected->Va_PciBar[i])
            );
    }
}




/**********************************************************
 *
 * Function   :  PciSpacesUnmap
 *
 * Description:
 *
 *********************************************************/
void
PciSpacesUnmap(
    void
    )
{
    U8 i;


    for (i=0; i<6; i++)
    {
        // Map buffer into user space
        PlxPciBarUnmap(
            Gbl_hDevice,
            &(pDeviceSelected->Va_PciBar[i])
            );
    }
}




/**********************************************************
 *
 * Function   :  GetCommonBufferProperties
 *
 * Description:
 *
 *********************************************************/
void
GetCommonBufferProperties(
    void
    )
{
    // Get PCI buffer properties
    PlxPciCommonBufferProperties(
        Gbl_hDevice,
        &Gbl_PciBufferInfo
        );

    // Map buffer into user space
    PlxPciCommonBufferMap(
        Gbl_hDevice,
        &Gbl_PciBufferInfo.UserAddr
        );
}
