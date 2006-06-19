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
 *      ConsFunc.c
 *
 * Description:
 *
 *      Provides a common layer to work with the console
 *
 * Revision History:
 *
 *      04-31-02 : PCI SDK v3.50
 *
 ******************************************************************************/


#include "ConsFunc.h"
#include "PlxTypes.h"




/******************************************************************
 *
 * Function   :  ConsoleInitialize
 *
 * Description:  Initialize the console
 *
 *****************************************************************/
void
ConsoleInitialize(
    void
    )
{
#if defined(PLX_LINUX)

    WINDOW *pActiveWin;


    // Initialize console
    pActiveWin = initscr();

    // Allow insert/delate of lines (for scrolling)
    idlok(
        pActiveWin,
        TRUE
        );

    // Allow console to scroll
    scrollok(
        pActiveWin,
        TRUE
        );

    // Disable buffering to provide input immediately to app
    cbreak();

    // getch will echo characters
    echo();

    // Enable blocking mode (for getch & scanw)
    nodelay(
        stdscr,
        FALSE
        );

#endif
}




/******************************************************************
 *
 * Function   :  ConsoleEnd
 *
 * Description:  Restore the console
 *
 *****************************************************************/
void
ConsoleEnd(
    void
    )
{
#if defined(PLX_LINUX)

    // Restore console
    endwin();

#endif
}




/*********************************************************************
 *
 * Function   :  OemIoWriteString
 *
 * Description:  Outputs a string
 *
 ********************************************************************/
void
OemIoWriteString(
    const char *pStr
    )
{
    while (*pStr != '\0')
    {
        OemIoWriteByte(
            *pStr
            );

        pStr++;
    }        
}




/*********************************************************************
 *
 * Function   :  PlxPrintf
 *
 * Description:  Outputs a formatted string
 *
 ********************************************************************/
void
PlxPrintf(
    const char *format,
    ...
    )
{
    int          i;
    int          Width;
    int          Precision;
    char         pFormatSpec[16];
    char         pOutput[MAX_DECIMAL_BUFFER_SIZE];
    va_list      pArgs;
    const char  *pFormat;
    signed char  Size;
    signed char  Flag_64_bit;
    signed char  FlagDirective;
    signed char  Flag_LeadingZero;
    ULONGLONG    value;


    // Initialize the optional arguments pointer
    va_start(
        pArgs,
        format
        );

    pFormat = format;

    while (*pFormat != '\0')
    {
        if (*pFormat != '%')
        {
            // Non-field, just print it
            OemIoWriteByte(
                *pFormat
                );
        }
        else
        {
            // Reached '%' character
            Size             = -1;
            Width            = -1;
            Precision        = -1;
            Flag_64_bit      = 0;
            FlagDirective    = -1;
            Flag_LeadingZero = -1;

            pFormat++;

            // Check for any flag directives
            if ( (*pFormat == '-') || (*pFormat == '+') ||
                 (*pFormat == '#') || (*pFormat == ' ') )
            {
                FlagDirective = *pFormat;

                pFormat++;
            }

            // Check for a width specification
            while ((*pFormat >= '0') && (*pFormat <= '9'))
            {
                // Check for leading 0
                if (Flag_LeadingZero == -1)
                {
                    if (*pFormat == '0')
                        Flag_LeadingZero = 1;
                    else
                        Flag_LeadingZero = 0;
                }

                if (Width == -1)
                    Width = 0;

                // Build the width
                Width = (Width * 10) + (*pFormat - '0');

                pFormat++;
            }

            // Check for a precision specification
            if (*pFormat == '.')
            {
                pFormat++;

                Precision = 0;

                // Get precision
                while ((*pFormat >= '0') && (*pFormat <= '9'))
                {
                    // Build the Precision
                    Precision = (signed char)(Precision * 10) + (*pFormat - '0');

                    pFormat++;
                }
            }

            // Check for size specification
            if ( (*pFormat == 'l') || (*pFormat == 'L') ||
                 (*pFormat == 'h') || (*pFormat == 'H') )
            {
                Size = *pFormat;

                pFormat++;
            }
            else if ((*pFormat == 'q') || (*pFormat == 'Q'))
            {
                // 64-bit precision requested
                Flag_64_bit = 1;
                Size        = 'l';

                pFormat++;
            }

            if (*pFormat == '\0')
                break;


            // Build the format specification
            i = 1;
            pFormatSpec[0] = '%';

            if (FlagDirective != -1)
            {
                pFormatSpec[i] = FlagDirective;
                i++;
            }

            if (Flag_LeadingZero == 1)
            {
                pFormatSpec[i] = '0';
                i++;
            }

            if ((Width != -1) && (Width != 0))
            {
                // Split up widths if 64-bit data
                if (Flag_64_bit)
                {
                    if (Width > 8)
                    {
                        Width = Width - 8;
                    }
                }

                sprintf(
                    &(pFormatSpec[i]),
                    "%d",
                    Width
                    );

                if (Width < 10)
                    i += 1;
                else if (Width < 100)
                    i += 2;
            }

            if ((Precision != -1) && (Precision != 0))
            {
                pFormatSpec[i] = '.';
                i++;

                sprintf(
                    &(pFormatSpec[i]),
                    "%d",
                    Precision
                    );

                if (Precision < 10)
                    i += 1;
                else if (Precision < 100)
                    i += 2;
            }

            if (Size != -1)
            {
                pFormatSpec[i] = Size;
                i++;
            }


            // Check type
            switch (*pFormat)
            {
                case 'd':
                case 'i':
                    pFormatSpec[i] = *pFormat;
                    i++;
                    pFormatSpec[i] = '\0';

                    // Convert value to string
                    sprintf(
                        pOutput,
                        pFormatSpec,
                        va_arg(pArgs, S32)
                        );

                    // Display output
                    OemIoWriteString(
                        pOutput
                        );
                    break;

                case 'x':
                case 'X':
                    pFormatSpec[i] = *pFormat;
                    i++;

                    pFormatSpec[i] = '\0';

                    // Display upper 32-bits if 64-bit data
                    if (Flag_64_bit)
                    {
                        // Get data from stack
                        value =
                            va_arg(
                                pArgs,
                                ULONGLONG
                                );

                        // Display value if not zero or a width is specified
                        if (((value >> 32) != 0) ||
                            ((Width != -1) && (Width != 0)))
                        {
                            // Prepare output string
                            sprintf(
                                pOutput,
                                pFormatSpec,
                                (U32)(value >> 32)
                                );

                            // Display upper 32-bits
                            OemIoWriteString(
                                pOutput
                                );

                            // Prepare format string for lower 32-bits
                            sprintf(
                                pFormatSpec,
                                "%%08%c",
                                *pFormat
                                );

                            // Clear upper 32-bits
                            value = (U32)value;
                        }
                    }
                    else
                    {
                        // Get data from stack
                        value =
                            va_arg(
                                pArgs,
                                U32
                                );
                    }

                    // Prepare output string
                    sprintf(
                        pOutput,
                        pFormatSpec,
                        (U32)value
                        );

                    // Display 32-bit value
                    OemIoWriteString(
                        pOutput
                        );
                    break;

                case 'c':
                    // Display the character
                    OemIoWriteByte(
                        (char) va_arg(pArgs, int)
                        );
                    break;

                case 'o':
                case 'u':
                    pFormatSpec[i] = *pFormat;
                    i++;
                    pFormatSpec[i] = '\0';

                    // Convert value to string
                    sprintf(
                        pOutput,
                        pFormatSpec,
                        va_arg(pArgs, unsigned int)
                        );

                    // Display output
                    OemIoWriteString(
                        pOutput
                        );
                    break;

                case 's':
                   OemIoWriteString(
                       va_arg(pArgs, char *)
                       );
                    break;

                default:
                    // Display the character
                    OemIoWriteByte(
                        *pFormat
                        );
                    break;
            }
        }

        pFormat++;
    }

    va_end(
        pArgs
        );
}




/*************************************************
 *
 *         Windows-specific functions
 *
 ************************************************/

#if defined(_WIN32)

/******************************************************************
 *
 * Function   :  ConsoleClear
 *
 * Description:  Clears the console window
 *
 *****************************************************************/
void
ConsoleClear(
    void
    )
{
    HANDLE                     hConsole;
    COORD                      CoordScreen;
    DWORD                      cCharsWritten;
    DWORD                      dwWinSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;


    // Get handle to the console
    hConsole =
        GetStdHandle(
            STD_OUTPUT_HANDLE
            );

    // Get the number of character cells in the current buffer
    GetConsoleScreenBufferInfo(
        hConsole,
        &csbi
        );

    dwWinSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Set screen coordinates
    CoordScreen.X = 0;
    CoordScreen.Y = 0;

    // Fill the entire screen with blanks
    FillConsoleOutputCharacter(
        hConsole,
        (TCHAR) ' ',
        dwWinSize,
        CoordScreen,
        &cCharsWritten
        );

    // Put the cursor at origin
    SetConsoleCursorPosition(
        hConsole,
        CoordScreen
        );
}

#endif // _WIN32




/*************************************************
 *
 *         Linux-specific functions
 *
 ************************************************/

#if defined(PLX_LINUX)

/******************************************************************
 *
 * Function   :  Plx_kbhit
 *
 * Description:  Determines if input is pending
 *
 *****************************************************************/
int
Plx_kbhit(
    void
    )
{
    char ch;


    // Put getch into non-blocking mode
    nodelay(
        stdscr,
        TRUE
        );

    // getch will not echo characters
    noecho();

    // Get character
    ch = getch();

    // Restore getch into blocking mode
    nodelay(
        stdscr,
        FALSE
        );

    // Restore getch echo of characters
    noecho();

    if (ch == ERR)
        return 0;

    // Put character back into input queue
    ungetch(
        ch
        );

    return 1;
}




/******************************************************************
 *
 * Function   :  Plx_getch
 *
 * Description:  Gets a character from the keyboard (with blocking)
 *
 *****************************************************************/
char
Plx_getch(
    void
    )
{
    char ch;


    // getch will not echo characters
    noecho();

    // Get character
    ch = getch();

    // Restore getch echo of characters
    echo();

    return ch;
}

#endif // PLX_LINUX
