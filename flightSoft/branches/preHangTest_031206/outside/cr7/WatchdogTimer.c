//****************************************************************************
//*
//* Copyright (c) 2001 SBS Technologies, Inc.
//*
//* HARDWARE        : CR7
//*
//* PROGRAM NAME    : WatchDogTimer
//* MODULE NAME     : WatchDogTimer.c
//* VERSION         : 1.00
//* TOOL            : GNU Toolchain
//*
//* SOFTWARE DESIGN : Michael Zappe, modeled after Jan Allen's V5C BSP
//* BEGINNING DATE  : 03/06/01
//* DESCRIPTION     : 'C' Demo program to test CR7 Watchdog timer routines.
//*
//*
//* REV   BY   DATE      CHANGE
//*---------------------------------------------------------------------------
//*
//****************************************************************************

#define LD_PROGRAM_NAME                     "WatchDogTimer"
#define LD_PROGRAM_VERSION                  "v1.01"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cr7.h"                //* V5x Defines

int fn_GetEntry    (void);
int fn_WdtExample1 (void);
int fn_WdtExample2 (void);
int fn_WdtExample3 (void);


#define LD_MENU_EXAMPLE_1                   1
#define LD_MENU_EXAMPLE_2                   2
#define LD_MENU_EXAMPLE_3                   3
#define LD_MENU_EXIT                        4

#define LD_INVALID                          0
#define LD_VALID                            1

#define LD_PASS                             0
#define LD_FAIL                             1

//*****************************************************************************
//*
//* FUNCTION        : main
//*
//* DESCRIPTION     : Demo program to test NT Watchdog timer dll routines.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : 0
//*
//****************************************************************************
int main (void)
{
    int         main_nData;
    int         main_nDone;
    int         main_nLen;
    int         main_nMenuOption;
    int         main_nResult;
    char        main_chaUserInbuf [100];
    API_RESULT  main_Api_Result;


    /* fn_InitV5x(); */

    printf ("\n\n %s  %s "__DATE__" "__TIME__"   (%sBSP %s)",
            LD_PROGRAM_NAME, LD_PROGRAM_VERSION, 
	    GD_CR7_NAME, GD_CR7_RELEASE_VERSION);

    main_nDone = FALSE;

    do
    {
        printf ("\n\n %d - disable watchdog reset after 25 refreshes",LD_MENU_EXAMPLE_1);
        printf ("\n %d - trigger watchdog reset after 25 refreshes",  LD_MENU_EXAMPLE_2);
        printf ("\n %d - trigger watchdog reset after 500 milliseconds",     LD_MENU_EXAMPLE_3);
        printf ("\n %d - Exit",                                       LD_MENU_EXIT);
        printf ("\n\n Enter option [1-%d] : ",                        LD_MENU_EXIT);


        //*-----------------------------------------------------------
        //* Get and validate user's menu option.
        //*-----------------------------------------------------------
        main_nMenuOption = 0;                    //* make compiler happy.
        main_nResult = LD_VALID;

        memset (main_chaUserInbuf, 0, 100);
        fgets (main_chaUserInbuf, 99, stdin);
        if (strchr (main_chaUserInbuf, '\n') != NULL)
            *strchr (main_chaUserInbuf, '\n') = '\0';

        if (main_chaUserInbuf[0] == 0)
        {
            //* no error message, no entry provided.
            continue;
        }

        main_nLen = strlen (main_chaUserInbuf);

        if (main_nLen != 1)
        {
            printf ("\n Error, too many characters entered.");
            continue;
        }

        main_nData   = (int)main_chaUserInbuf[0];
        main_nResult = isascii (main_nData);

        if (main_nResult == LD_INVALID)
        {
            printf ("\n Error, non ascii characters entered.");
            continue;
        }

        main_nResult = LD_VALID;
        main_nData   = (int)main_chaUserInbuf[0];
        main_nResult = isdigit (main_nData);

        if (main_nResult == LD_INVALID)
        {
            printf ("\n Error, non numeric characters entered.");
            continue;
        }

        if (main_nResult != LD_INVALID)
        {
            main_nMenuOption = atoi (main_chaUserInbuf);

            if ((main_nMenuOption < LD_MENU_EXAMPLE_1) ||
                (main_nMenuOption > LD_MENU_EXIT))
            {
                printf ("\n Error, numeric value out of range.");
                continue;
            }
        }


        //*-----------------------------------------------------------
        //* Execute the selected menu option.
        //*-----------------------------------------------------------
        switch (main_nMenuOption)
        {
            case LD_MENU_EXAMPLE_1:
                 //*-----------------------------------------------------------
                 //* disable watchdog reset after 25 refreshes.
                 //*-----------------------------------------------------------
                 main_nResult = fn_WdtExample1 ();

                 if (main_nResult == LD_PASS)
                 {
                     printf ("\n\n Watchdog Timer test complete.");
                 }
                 else
                 {
                     printf ("\n\n Error, Watchdog Timer test failed.");
                 }

                 break;


            case LD_MENU_EXAMPLE_2:
                 //*-----------------------------------------------------------
                 //* trigger watchdog reset after 25 refreshes.
                 //*-----------------------------------------------------------
                 main_nResult = fn_WdtExample2 ();

                 if (main_nResult == LD_PASS)
                 {
                     printf ("\n\n Watchdog Timer should soon invoke a system reset.");
                 }
                 else
                 {
                     printf ("\n\n Error, Watchdog Timer test failed.");
                 }

                 break;


            case LD_MENU_EXAMPLE_3:
                 //*-----------------------------------------------------------
                 //* trigger watchdog reset after 7 seconds
                 //*-----------------------------------------------------------
                 main_nResult = fn_WdtExample3 ();

                 if (main_nResult == LD_PASS)
                 {
                     printf ("\n\n Watchdog Timer should soon invoke a system reset.");
                 }
                 else
                 {
                     printf ("\n\n Error, Watchdog Timer test failed.");
                 }

                 break;


            case LD_MENU_EXIT:
                 //*-----------------------------------------------------------
                 //* Exit
                 //*-----------------------------------------------------------
                 main_nDone = TRUE;
                 break;


            default:
                 //*-----------------------------------------------------------
                 //* Invalid entry
                 //*-----------------------------------------------------------
                 printf ("\n\n Entry out of range.\n");
                 break;
        }
    } while (!main_nDone);
        
    return 0;
//* END FUNCTION    : main
}


//*****************************************************************************
//*
//* FUNCTION        : fn_WdtExample1
//*
//* DESCRIPTION     : This example, enables the watchdog timer, and sets it to
//*                   expire in 4*1 sec.  The watchdog is kept from expiring by
//*                   periodically calling the Api SetWdtRefresh function.
//*                   The system is then kept from resetting by disabling
//*                   the watchdog timer.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : pass/fail
//*
//*****************************************************************************
int fn_WdtExample1 (void)
{
    int         fwe1_nIndex_i;
    API_RESULT  fwe1_Api_Result;
    struct timespec sleep_req;

    fwe1_Api_Result = fn_ApiCr7EnableWatchdog(1);

    if (fwe1_Api_Result != API_SUCCESS)
    {
        printf ("\n %s: (6) Error enabling Watchdog Timer: \n %d %s\n",
                LD_PROGRAM_NAME, fwe1_Api_Result, fn_ApiGetErrorMsg (fwe1_Api_Result));
        return LD_FAIL;
    }

    sleep_req.tv_sec = 0;
    sleep_req.tv_nsec = 400000000;

    //* Refresh the timer to prevent timeout and reset
    for  (fwe1_nIndex_i=0; fwe1_nIndex_i<25; fwe1_nIndex_i++)
    {
      nanosleep(&sleep_req, NULL);

      fwe1_Api_Result = fn_ApiCr7PingWatchdog ();  
    }

    //* Now disable the watchdog timer.
    fwe1_Api_Result = fn_ApiCr7EnableWatchdog(0);

    if (fwe1_Api_Result != API_SUCCESS)
    {
        return LD_FAIL;
    }

    //* System should not reset in 4*1 sec
    return LD_PASS;
}


//*****************************************************************************
//*
//* FUNCTION        : fn_WdtExample2
//*
//* DESCRIPTION     : This example, enables the watchdog timer, and sets it to
//*                   expire in 4*1 sec.  The watchdog is kept from expiring by
//*                   periodically calling the ApiSetWdtRefresh function.
//*                   After 25 times the watchdog is left to expire.  A system
//*                   reset should occur at that time.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : pass/fail
//*
//*****************************************************************************
int fn_WdtExample2 (void)
{
    int         fwe2_nIndex_i;
    API_RESULT  fwe2_Api_Result;
    struct timespec sleep_req;

    fwe2_Api_Result = fn_ApiCr7EnableWatchdog(1);

    if (fwe2_Api_Result != API_SUCCESS)
    {
        printf ("\n %s: (6) Error enabling Watchdog Timer: \n %d %s\n",
                LD_PROGRAM_NAME, fwe2_Api_Result, fn_ApiGetErrorMsg (fwe2_Api_Result));
        return LD_FAIL;
    }

    sleep_req.tv_sec = 0;
    sleep_req.tv_nsec = 400000000;

    //* Refresh the timer to prevent timeout and reset
    for  (fwe2_nIndex_i=0; fwe2_nIndex_i<25; fwe2_nIndex_i++)
    {
      nanosleep(&sleep_req, NULL);

      fwe2_Api_Result = fn_ApiCr7PingWatchdog();  
    }

    //* System should reset in .5 sec
    return LD_PASS;
}


//*****************************************************************************
//*
//* FUNCTION        : fn_WdtExample3
//*
//* DESCRIPTION     : This example, enables the watchdog timer, and sets it to
//*                   expire in 7*1 sec.  A board reset will occur.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : pass/fail
//*
//*****************************************************************************
int fn_WdtExample3 (void)
{
    API_RESULT  fwe2_Api_Result;

    fwe2_Api_Result = fn_ApiCr7EnableWatchdog(1);

    if (fwe2_Api_Result != API_SUCCESS)
    {
        printf ("\n %s: (6) Error enabling Watchdog Timer: \n %d %s\n",
                LD_PROGRAM_NAME, fwe2_Api_Result, fn_ApiGetErrorMsg (fwe2_Api_Result));
        return LD_FAIL;
    }

    //* System should reset in .5 sec
    return LD_PASS;
}
