#ifndef _CONSFUNC_H
#define _CONSFUNC_H

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
 *      ConsFunc.h
 *
 * Description:
 *
 *      Header file for the Console functions
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 ******************************************************************************/


#if defined(_WIN32)
    #include <stdio.h>
    #include <conio.h>
    #include <Windows.h>
#elif defined(PLX_LINUX)
    #include <stdlib.h>
    #include <curses.h>
    #include <unistd.h>
#endif




/*************************************
 *          Definitions
 ************************************/
#if defined(_WIN32)

    #define Cls                         ConsoleClear
    #define FlushInputBuffer()          FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE))
    #define OemIoWriteByte              _putch
    #define Plx_Sleep_ms                Sleep
    #define Plx_getch                   _getch
    #define Plx_kbhit()                 _kbhit()
    #define Plx_strncasecmp             _strnicmp
    #define Plx_scanf                   scanf

#elif defined(PLX_LINUX)

    #define Cls                         clear
    #define FlushInputBuffer()          flushinp()
    #define OemIoWriteByte              echochar
    #define Plx_Sleep_ms(arg)           usleep((arg) * 1000)
    #define Plx_strncasecmp             strncasecmp
    #define Plx_scanf                   scanw

    #if !defined(min)
        #define min(a, b)               (((a) < (b)) ? (a) : (b))
    #endif

#endif


/******************************************************************
 * A 64-bit HEX value (0xFFFF FFFF FFFF FFFF) requires 20 decimal
 * digits or 22 octal digits. The following constant defines the
 * buffer size used to hold an ANSI string converted from a
 * 64-bit HEX value.
 *****************************************************************/
#define MAX_DECIMAL_BUFFER_SIZE      30


#define _Pause                                              \
    do                                                      \
    {                                                       \
        PlxPrintf("  -- Press any key to continue --");     \
        Plx_getch();                                        \
        PlxPrintf("\r                                 \r"); \
    }                                                       \
    while(0)


#define _PauseWithExit                                                         \
    do                                                                         \
    {                                                                          \
        PlxPrintf("  -- Press any key to continue or ESC to exit --");         \
        if (Plx_getch() == 27)                                                 \
        {                                                                      \
            PlxPrintf("\r                                                \n"); \
            ConsoleEnd();                                                      \
            exit(0);                                                           \
        }                                                                      \
        PlxPrintf("\r                                                \r");     \
    }                                                                          \
    while(0)




/*************************************
 *            Functions
 ************************************/
void
ConsoleInitialize(
    void
    );

void
ConsoleEnd(
    void
    );

void
ConsoleClear(
    void
    );

void
SetCursorPosition(
    unsigned int xPos,
    unsigned int yPos
    );

void
OemIoWriteString(
    const char *pStr
    );

void
PlxPrintf(
    const char *format,
    ...
    );

#if defined(PLX_LINUX)
int
Plx_kbhit(
    void
    );

char
Plx_getch(
    void
    );
#endif




#endif
