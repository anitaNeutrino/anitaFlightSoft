//****************************************************************************
//*
//* Copyright (c) 2001 SBS Technologies, Inc.
//*
//* HARDWARE        : CR7
//*
//* PROGRAM NAME    : FlashLED
//* MODULE NAME     : FlashLED.c
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

#define LD_PROGRAM_NAME                     "FlashLED"
#define LD_PROGRAM_VERSION                  "v1.00"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

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
    struct timespec sleep_spec;
    int             led_status = 0;
    int             result;

    printf ("\n\n %s  %s "__DATE__" "__TIME__"   (%sBSP %s)\n",
            LD_PROGRAM_NAME, LD_PROGRAM_VERSION, GD_CR7_NAME, GD_CR7_RELEASE_VERSION);

    printf ("The status light should begin to blink.  Press ^C to exit...\n");

    sleep_spec.tv_sec = 0;
    sleep_spec.tv_nsec = 100000000; /* 100 ms */

    /*
     *  Make the pretty little lights blink!
     */

    while(1)
    {
      result = fn_ApiCr7SetStatusLEDState(led_status);

      if(fn_ApiCr7GetStatusLEDState() != led_status)
      {
	printf("Error!  LED Status does not match set state!\n");
      }

      led_status = led_status ? 0 : 1;

      nanosleep(&sleep_spec, NULL);
    }

    return 0;
}

