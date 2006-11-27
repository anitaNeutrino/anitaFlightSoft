//****************************************************************************
//*
//* Copyright (c) 2001 SBS Technologies, Inc.
//*
//* HARDWARE        : CR7
//*
//* PROGRAM NAME    : TestEEPROM
//* MODULE NAME     : TestEEPROM.c
//* VERSION         : 1.00
//* TOOL            : GNU Toolchain
//*
//* SOFTWARE DESIGN : Michael Zappe, modeled after Jan Allen's V5C BSP
//* BEGINNING DATE  : 01/31/01
//* DESCRIPTION     : 'C' Demo program to test CR7 EEPROM routines.
//*
//* NOTE:           : Users need to add code to make EEPROM accesses
//*                   thread/process safe.
//*
//*
//* REV   BY   DATE      CHANGE
//*---------------------------------------------------------------------------
//*
//****************************************************************************

#define LD_PROGRAM_NAME                     "TestEEPROM"
#define LD_PROGRAM_VERSION                  "v1.01"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>

#include "cr7.h"             //* CR7 Defines

#define LD_MENU_READ                        1
#define LD_MENU_WRITE                       2
#define LD_MENU_VERIFY                      3
#define LD_MENU_EXIT                        4

#define LD_TYPE_ALPHANUMERIC                1
#define LD_TYPE_DEC                         2
#define LD_TYPE_HEX                         3

#define LD_TO_READ                          1
#define LD_TO_WRITE                         2
#define LD_TO_OFFSET                        3

#define LD_PASS                             0
#define LD_FAIL                             1

#define LD_INVALID                          0
#define LD_VALID                            1

#define LD_RDWT_BUFFER_LEN                  512

#define LD_ONE_DIGIT                        1
#define LD_TWO_DIGITS                       2
#define LD_THREE_DIGITS                     3
#define LD_FOUR_DIGITS                      4

typedef int BOOL;

int   fn_BackupEEPROM        (void);
int   fn_RestoreEEPROM       (void);

int   fn_EnableEEPROM        (void);
int   fn_DisableEEPROM       (void);

int   fn_MapEEPROM           (void);
int   fn_UnMapEEPROM         (void);

int   fn_ReadEEPROM          (void);
int   fn_WriteEEPROM         (void);

void  fn_Delay               (unsigned char fd_uchChar_Parm, int fd_nLocn);
void  fn_GetEntry            (int fge_nLen_parm, int fge_nType_parm, int fge_nMin_parm, int fge_nMax_parm);
unsigned long fn_GetLength           (int fgl_nType_parm);
int   fn_GetPhysicalAddress  (void);


unsigned long                  ld_ulReadLength;          //* length (in bytes) to read.
unsigned long                  ld_ulWriteLength;         //* length (in bytes) to write.
int                    ld_nEntry;
unsigned char                  ld_uchaDataRead  [LD_RDWT_BUFFER_LEN+1];  //* destination for reads
unsigned char                  ld_uchaDataWrite [LD_RDWT_BUFFER_LEN+1];  //* source for writes.
char                   ld_chaUserInbuf  [100];
unsigned char                  ld_uchDummyBuffer[10001];

//*****************************************************************************
//*
//* FUNCTION        : main
//*
//* DESCRIPTION     : Demo program to test NT EEPROM dll routines.
//*
//* INPUTS          : none
//*
//* OUTPUTS         : LD_PASS = passed
//*                   LD_FAIL = failed
//*
//****************************************************************************
int main (void)
{
  unsigned char       *main_uchReadPointer;
  unsigned char       *main_uchWritePointer;
  unsigned long       main_ulLength;
  unsigned long       main_ulIndex_i;
  int         main_nMenuOption;
  int         main_nResult;
  int         main_nDone;

  printf ("\n\n %s  %s "__DATE__" "__TIME__"   (%sBSP %s)",
	  LD_PROGRAM_NAME, LD_PROGRAM_VERSION, GD_CR7_NAME, GD_CR7_RELEASE_VERSION);
  main_nDone = FALSE;
    
  do
  {
    //*--------------------------------------------------------------------
    //* Get test type.
    //*--------------------------------------------------------------------
    printf ("\n\n\n  %d - Read",              LD_MENU_READ);
    printf ("\n  %d - Write",                 LD_MENU_WRITE);
    printf ("\n  %d - Write, Read & Verify",  LD_MENU_VERIFY);
    printf ("\n  %d - Exit",                  LD_MENU_EXIT);
    printf ("\n\n Enter option [1-%d] : ",    LD_MENU_EXIT);
    
    fn_GetEntry (LD_TWO_DIGITS, LD_TYPE_DEC, 1, LD_MENU_EXIT);
    main_nMenuOption = ld_nEntry;
      
    switch (main_nMenuOption)
    {
    case LD_MENU_READ:
      //*--------------------------------------------------------
      //* Read EEPROM
      //*--------------------------------------------------------
      main_nResult = fn_ReadEEPROM();
      
      if (main_nResult != LD_PASS)
      {
	break;
      }
      
      //*--------------------------------------------------------
      //* display data read from EEPROM.
      //*--------------------------------------------------------
      main_uchReadPointer = ld_uchaDataRead;
      printf ("\n\n EEPROM Data Read: \n");
      
      for (main_ulIndex_i = 0;
	   main_ulIndex_i < ld_ulReadLength;
	   main_ulIndex_i++)
      {
	printf (" %2.2x", *main_uchReadPointer++);
      }
	
      break;
      
      
    case LD_MENU_WRITE:
      //*--------------------------------------------------------
      //* Write EEPROM
      //*--------------------------------------------------------
      main_nResult = fn_WriteEEPROM();
      break;
      
      
    case LD_MENU_VERIFY:
      //*--------------------------------------------------------
      //* Write EEPROM, then read it and compare to what was written.
      //*--------------------------------------------------------
      main_nResult = fn_WriteEEPROM();
      
      if (main_nResult != LD_PASS)
      {
	break;
      }
      
      //*--------------------------------------------------------
      //* Read EEPROM
      //*--------------------------------------------------------
      main_nResult = fn_ReadEEPROM();
      
      if (main_nResult != LD_PASS)
      {
	break;
      }
      
      //*--------------------------------------------------------
      //* compare results.
      //*--------------------------------------------------------
      main_nResult  = LD_PASS;
      main_ulLength = ld_ulWriteLength;
      
      if (ld_ulReadLength < ld_ulWriteLength)
      {
	main_ulLength = ld_ulReadLength;
      }
	
      main_uchReadPointer  = ld_uchaDataRead;
      main_uchWritePointer = ld_uchaDataWrite;
      
      for (main_ulIndex_i = 0;
	   main_ulIndex_i < main_ulLength;
	   main_ulIndex_i++)
      {
	if (*main_uchReadPointer++ != *main_uchWritePointer++)
	{
	  main_nResult = LD_FAIL;
	  printf ("\n %s: (2) Error, invalid data at offset 0x%lx.",
		  LD_PROGRAM_NAME, main_ulIndex_i);
	}
      }
							
      if (main_nResult == LD_PASS)
      {
	printf ("\n Verified EEPROM write and read successful.");
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
      printf ("\n Invalid menu option.");
      break;
    }
  } while (!main_nDone);
  
  return LD_PASS;
  //* END PROGRAM     : main
}

//*****************************************************************************
//*
//* FUNCTION        : fn_GetEntry
//*
//* DESCRIPTION     : Obtains input from the user.
//*
//* INPUTS          : maiximum valid length of entry expected.
//*                   type of entry expected.
//*                   minimum value.
//*                   maximum value.
//*
//* OUTPUTS         : none
//*
//****************************************************************************
void  fn_GetEntry (int fge_nLen_parm, int fge_nType_parm, int fge_nMin_parm, int fge_nMax_parm)
{
    int      fge_nData;
    int      fge_nResult;
    int      fge_nIndex_i;
    int      fge_nLen;
		
		
		
    for (;;)
    {
      memset (ld_chaUserInbuf, 0, 100);
      fgets (ld_chaUserInbuf, 99, stdin);
      if (strchr (ld_chaUserInbuf, '\n') != NULL)
        *strchr (ld_chaUserInbuf, '\n') = '\0';
      
      if (ld_chaUserInbuf[0] == 0)
      {
	printf ("\n Error, entry required. : ");
	continue;
      }
      
      fge_nLen = strlen (ld_chaUserInbuf);
      
      if (fge_nLen > fge_nLen_parm)
      {
	if ((fge_nType_parm     == LD_TYPE_HEX) &&
	    (ld_chaUserInbuf[0] == '0')         &&
	    (ld_chaUserInbuf[1] == 'x'))
	{
	  printf ("\n Enter hex data without a leading '0x' : ");
	}
	else
	{
	  printf ("\n Error, too many characters entered. : ");
	}
	
	continue;
      }
      
      fge_nResult = LD_VALID;
      
      for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
      {
	fge_nData   = (int)ld_chaUserInbuf[fge_nIndex_i];
	fge_nResult = isascii (fge_nData);
	
	if (fge_nResult == LD_INVALID)
	{
	  printf ("\n Error, non-ascii characters entered. : ");
	  break;
	}
      }
      
      if (fge_nResult == LD_INVALID)
      {
	continue;
      }
      
      switch (fge_nType_parm)
      {
      case LD_TYPE_DEC:
	//*-----------------------------------------------------------
	//* Check for decimal entry.
	//*-----------------------------------------------------------
	for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
	{
	  fge_nData   = (int)ld_chaUserInbuf[fge_nIndex_i];
	  fge_nResult = isdigit (fge_nData);
	  
	  if (fge_nResult == LD_INVALID)
	  {
	    printf ("\n Error, non numeric characters entered. : ");
	    break;
	  }
	}
	
	if (fge_nResult != LD_INVALID)
	{
	  ld_nEntry = atoi (ld_chaUserInbuf);
	  
	  if ((ld_nEntry < fge_nMin_parm) ||
	      (ld_nEntry > fge_nMax_parm))
	  {
	    fge_nResult = LD_INVALID;
	    printf ("\n Error, numeric value out of range. : ");
	  }
	}

	break;
	
      case LD_TYPE_HEX:
	//*-----------------------------------------------------------
	//* Check for hex entry.
	//*-----------------------------------------------------------
	for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
	{
	  fge_nData   = (int)ld_chaUserInbuf[fge_nIndex_i];
	  fge_nResult = isxdigit (fge_nData);
	  
	  if (fge_nResult == LD_INVALID)
	  {
	    if ((ld_chaUserInbuf[0] == '0') &&
		(ld_chaUserInbuf[1] == 'x'))
	    {
	      printf ("\n Enter hex data without a leading '0x' : ");
	    }
	    else
	    {
	      printf ("\n Error, non hex characters entered. : ");
	    }
	    
	    break;
	  }
	}
	
	break;
	
      case LD_TYPE_ALPHANUMERIC:
	//*-----------------------------------------------------------
	//* Check for alphanumeric entry.
	//*-----------------------------------------------------------
	for (fge_nIndex_i=0; fge_nIndex_i < fge_nLen; fge_nIndex_i++)
	{
	  fge_nData   = (int)ld_chaUserInbuf[fge_nIndex_i];
	  fge_nResult = isalnum (fge_nData);
	  
	  if (fge_nResult == LD_INVALID)
	  {
	    printf ("\n Error, non-alpha/numeric characters entered. : ");
	    break;
	  }
	}

	break;

      default:
	//*-------------------------------------------------------
	//* Invalid entry
	//*-------------------------------------------------------
	fge_nResult = LD_INVALID;
	printf ("\n Error, unknown data type. : ");
	break;
      }
      
      if (fge_nResult != LD_INVALID)
      {
	//* valid is anything other than 0.
	break;                               //* done
      }
    }
    
    return;
//* END FUNCTION    : fn_GetEntry
}


//*****************************************************************************
//*
//* FUNCTION        : fn_GetLength
//*
//* DESCRIPTION     : Gets a length from the user.
//*
//* INPUTS          : descriptive type
//*
//* OUTPUTS         : length
//*
//****************************************************************************
unsigned long fn_GetLength (int fgl_nType_parm)
{
    unsigned long    fgl_ulLength;

    //*------------------------------------------------------------------------
    //* Get the number of bytes.
    //*------------------------------------------------------------------------
    printf ("\n Enter the number of bytes");

    switch (fgl_nType_parm)
    {
    case LD_TO_READ:
      printf (" to read (hex) : ");
      break;
      
    case LD_TO_WRITE:
      printf (" to write (hex) : ");
      break;
      
    case LD_TO_OFFSET:
      printf (" offset from the start of EEPROM (hex) : ");
      break;

    default:
      printf (" (hex) : ");
      break;
    }

    fgl_ulLength = 0;
    fn_GetEntry (8, LD_TYPE_HEX, 0, 0);
    sscanf (ld_chaUserInbuf, "%08lx", &fgl_ulLength);

    if (fgl_ulLength > (unsigned long)GD_CR7_EEPROM_DEFAULT_LENGTH)
    {
      fgl_ulLength = (unsigned long)GD_CR7_EEPROM_DEFAULT_LENGTH;
      printf ("\n Length set to maximum, ox%lx.", fgl_ulLength);
    }

    return fgl_ulLength;
//* END FUNCTION    : fn_GetLength
}

//*****************************************************************************
//*
//* FUNCTION        : fn_ReadEEPROM
//*
//* DESCRIPTION     : Reads the EEPROM.
//*
//* INPUTS          : none.
//*
//* OUTPUTS         : LD_PASS = passed
//*                   LD_FAIL = failed
//*
//****************************************************************************
int  fn_ReadEEPROM (void)
{
    unsigned char       *frn_pchMemAddress;
    unsigned long       frn_ulLengthToRead;
    unsigned long       frn_ulOffset;
    unsigned long       frn_ulIndex_i;
    int                 frn_nResult;

    //*------------------------------------------------------------------------
    //* get the offset and length to read.
    //*------------------------------------------------------------------------
    frn_ulLengthToRead   = fn_GetLength (LD_TO_READ);
    frn_ulOffset = fn_GetLength (LD_TO_OFFSET);

    if ((frn_ulLengthToRead + frn_ulOffset) > (unsigned long)GD_CR7_EEPROM_DEFAULT_LENGTH)
    {
        printf ("\n %s: (34) Error, combined offset and length to read exceed length of EEPROM.",
                LD_PROGRAM_NAME);
        return LD_FAIL;
    }

    //*------------------------------------------------------------------------
    //* init local read buffer.
    //*------------------------------------------------------------------------
    for (frn_ulIndex_i = 0;
         frn_ulIndex_i < LD_RDWT_BUFFER_LEN;
         frn_ulIndex_i++)
    {
         ld_uchaDataRead[frn_ulIndex_i] = 0x00;
    }

    //*------------------------------------------------------------------------
    //* Read EEPROM
    //*------------------------------------------------------------------------
    frn_pchMemAddress    = ld_uchaDataRead;
    ld_ulReadLength      = frn_ulLengthToRead;

    frn_nResult = fn_ApiCr7SerialEEPROMRead(frn_ulOffset, ld_uchaDataRead, ld_ulReadLength);

    return (frn_nResult >= 0) ? LD_PASS : LD_FAIL;
//* END FUNCTION    : fn_ReadEEPROM
}

//*****************************************************************************
//*
//* FUNCTION        : fn_WriteEEPROM
//*
//* DESCRIPTION     : Writes to EEPROM.
//*
//* INPUTS          : none.
//*
//* OUTPUTS         : LD_PASS = passed
//*                   LD_FAIL = failed
//*
//****************************************************************************
int  fn_WriteEEPROM (void)
{
    unsigned long  fwn_ulIndex_i;
    unsigned long  fwn_ulLengthToWrite;
    unsigned long  fwn_ulOffset;
    int    fwn_nResult;
    unsigned char  fwn_uchValue;

    //*------------------------------------------------------------------------
    //* get the offset, length to write, and a 2 digit value to write.
    //*------------------------------------------------------------------------
    fwn_ulLengthToWrite  = fn_GetLength (LD_TO_WRITE);
    fwn_ulOffset = fn_GetLength (LD_TO_OFFSET);

    if ((fwn_ulLengthToWrite + fwn_ulOffset) >
        (unsigned long)GD_CR7_EEPROM_DEFAULT_LENGTH)
    {
        printf ("\n %s: (40) Error, combined offset and length to write exceed length of EEPROM.", LD_PROGRAM_NAME);
        return LD_FAIL;
    }

    printf ("\n Enter a two-digit hex value to write : ");
    fn_GetEntry (LD_TWO_DIGITS, LD_TYPE_HEX, 0, 0);
    sscanf (ld_chaUserInbuf, "%x", &fwn_nResult);
    fwn_uchValue = (unsigned char)fwn_nResult;

    //*------------------------------------------------------------------------
    //* init a local write buffer with a test pattern.
    //*------------------------------------------------------------------------
    for (fwn_ulIndex_i = 0; fwn_ulIndex_i < fwn_ulLengthToWrite; fwn_ulIndex_i++)
    {
         ld_uchaDataWrite[fwn_ulIndex_i] = fwn_uchValue;
    }

    for (; fwn_ulIndex_i < LD_RDWT_BUFFER_LEN; fwn_ulIndex_i++)
    {
         ld_uchaDataWrite[fwn_ulIndex_i] = 0;
    }

    //*------------------------------------------------------------------------
    //* Write data to EEPROM.
    //*------------------------------------------------------------------------
    ld_ulWriteLength     = fwn_ulLengthToWrite;

    fwn_nResult = fn_ApiCr7SerialEEPROMWrite(fwn_ulOffset, ld_uchaDataWrite, fwn_ulLengthToWrite);

    return (fwn_nResult >= 0) ? LD_PASS : LD_FAIL;
//* END FUNCTION    : fn_WriteEEPROM
}
