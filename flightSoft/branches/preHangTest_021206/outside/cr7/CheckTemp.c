//****************************************************************************
//*
//* Copyright (c) 2001 SBS Technologies, Inc.
//*
//* HARDWARE        : CR7
//*
//* PROGRAM NAME    : CheckTemp
//* MODULE NAME     : CheckTemp.c
//* VERSION         : 1.00
//* TOOL            : GNU Toolchain
//*
//* SOFTWARE DESIGN : Michael Zappe, derived from Jan Allen's V5C BSP
//* BEGINNING DATE  : 01/31/01
//* DESCRIPTION     : 'C' Demo program to test CR7 BSP routines.
//*
//*
//* REV   BY   DATE      CHANGE
//*---------------------------------------------------------------------------
//*
//****************************************************************************

#define LD_PROGRAM_NAME                     "CheckTemp"
#define LD_PROGRAM_VERSION                  "v1.01"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "cr7.h"

#define LD_NBR_SAMPLES                      120
#define LD_INVALID                          0
#define LD_VALID                            1

void fn_GetEntry (void);
int  ld_nEntry;


//*****************************************************************************
//*
//* FUNCTION        : main
//*
//* DESCRIPTION     : Tests LM84 Temperature Sensor routines.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : 0
//*
//****************************************************************************
int main (void)
{
    int             main_nIndex_i;
    API_RESULT      main_Api_Result;
    int             main_chTemp1;
    int             main_chTemp2;

    main_chTemp1     = 0;
    main_chTemp2     = 0;

    printf ("\n\n %s  %s "__DATE__" "__TIME__"   (%sBSP %s)",
            LD_PROGRAM_NAME, LD_PROGRAM_VERSION, GD_CR7_NAME, GD_CR7_RELEASE_VERSION);

    printf ("\n\n Enter number of samples to display : ");
    fn_GetEntry ();

    //*------------------------------------------------------------------------
    //* read & display LOCAL high & low temp limits.
    //*------------------------------------------------------------------------
    printf ("\n\n TEMPERATURE LIMITS");

    main_Api_Result =
      fn_ApiCr7ReadLocalTHigh(&main_chTemp1);

    if (main_Api_Result == API_SUCCESS)
    {
        printf ("\n\n Local High :  %d", main_chTemp1);
    }
    else
    {
        printf ("\n %s: (2) Error reading local high temperature limit: \n %d %s\n",
                LD_PROGRAM_NAME, main_Api_Result, fn_ApiGetErrorMsg (main_Api_Result));
    }

    //*------------------------------------------------------------------------
    //* read & display REMOTE high & low temp limits.
    //*------------------------------------------------------------------------
    main_Api_Result =
      fn_ApiCr7ReadRemoteTHigh(&main_chTemp1);

    if (main_Api_Result == API_SUCCESS)
    {
        printf ("\n Remote High:  %d", main_chTemp1);
    }
    else
    {
        printf ("\n %s: (6) Error reading remote high temperature limit: \n %d %s\n",
                LD_PROGRAM_NAME, main_Api_Result, fn_ApiGetErrorMsg (main_Api_Result));
    }

    //*------------------------------------------------------------------------
    //* read & display Current Local & Remote temp settings.
    //*------------------------------------------------------------------------
    printf ("\n\n Current Local Temperature   Current Remote Temperature");

    for (main_nIndex_i = 0; main_nIndex_i < ld_nEntry; main_nIndex_i++)
    {
        main_Api_Result = 
	  fn_ApiCr7ReadLocalTemperature(&main_chTemp1);

        if (main_Api_Result == API_SUCCESS)
        {
            printf ("\n             %d", main_chTemp1);
        }
        else
        {
            printf ("\n %s: (10) Error reading current local temperature: \n %d %s\n",
                    LD_PROGRAM_NAME, main_Api_Result, fn_ApiGetErrorMsg (main_Api_Result));
            printf ("\n               ");
        }

        main_Api_Result = 
	  fn_ApiCr7ReadRemoteTemperature(&main_chTemp2);

        if (main_Api_Result == API_SUCCESS)
        {
            printf ("                        %d\n", main_chTemp2);
        }
        else
        {
            printf ("\n %s: (12) Error reading current remote temperature: \n %d %s\n",
                    LD_PROGRAM_NAME, main_Api_Result, fn_ApiGetErrorMsg (main_Api_Result));
        }

	sleep(1);
    }

    return 0;
}


//*****************************************************************************
//*
//* FUNCTION        : fn_GetEntry
//*
//* DESCRIPTION     : Gets user entry.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : none
//*
//****************************************************************************
void fn_GetEntry (void)
{
    int      fge_nData;
    int      fge_nResult;
    int      fge_nIndex_i;
    int      fge_nLen;
    char     fge_chaUserInbuf [100];



    ld_nEntry   = LD_NBR_SAMPLES;
    fge_nResult = LD_VALID;

	memset (fge_chaUserInbuf, 0, 100);
    fgets (fge_chaUserInbuf, 99, stdin);
	if (strchr (fge_chaUserInbuf, '\n') != NULL)
		*strchr (fge_chaUserInbuf, '\n') = '\0';

    if (fge_chaUserInbuf[0] == 0)
    {
        //* no error message, no entry provided, use default.
        return;
    }

    fge_nLen = strlen (fge_chaUserInbuf);

    for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
    {
         fge_nData   = (int)fge_chaUserInbuf[fge_nIndex_i];
         fge_nResult = isascii (fge_nData);

         if (fge_nResult == LD_INVALID)
         {
             return;                             //* use default.
         }
    }

    fge_nResult = LD_VALID;

    //*-----------------------------------------------------------
    //* Check for decimal entry.
    //*-----------------------------------------------------------
    for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
    {
         fge_nData   = (int)fge_chaUserInbuf[fge_nIndex_i];
         fge_nResult = isdigit (fge_nData);

         if (fge_nResult == LD_INVALID)
         {
             return;                             //* use default.
         }
    }

    if (fge_nResult != LD_INVALID)
    {
        ld_nEntry = atoi (fge_chaUserInbuf);
    }

    return;
//* END FUNCTION    : fn_GetEntry
}
