
/*   Data Device Corporation
 *   105 Wilbur Place
 *   Bohemia N.Y. 11716
 *   (631) 567-5600
 *
 *    ENHANCED MINI-ACE Software Toolbox
 *
 *       Copyright (c) 2007 Data Device Corporation
 *       All Rights Reserved.
 *
 *
 *   RTMode.c (Console App RT (Mode Code) Demo)
 *  ----------------------------------------------------------------------
 *  ----------------------------------------------------------------------
 *
 *   Created 8/26/99 DL
 *   Created 8/26/00 MR
 *   Updated 3/23/01 KWH
 *   Updated 2/25/05 ESK
 *   Updated 8/04/05 ESK
 *
 *  Function used in this example:
 *
 *      - aceInitialize
 *      - aceFree
 *      - aceGetLibVersion
 *      - aceErrorStr
 *      - aceGetMsgTypeString
 *      - aceCmdWordParse
 *      - aceGetBSWErrString
 *
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
 *
 */

/* Include Files */
#include "libPrj/include/stdemace.h"
#include <stdio.h>
#include <sys/time.h> /* time */
#include <sys/select.h> /* select() */
#include <unistd.h>	/* sleep() */


#define RT_BUF_SIZE 256

/*-----------------------------------------------------------------------
Name:          PressAKey()

Description:   Allows application to pause to allow screen contents to be read.

In             none
Out            none
------------------------------------------------------------------------*/
static void PressAKey(void)
{
   printf("\nPress <ENTER> to continue...");
   getchar();
}

/*-----------------------------------------------------------------------
Name:          kbhit()

Description:   Check for a keyboard hit.

In             none
Out            none
------------------------------------------------------------------------*/
static int kbhit()
{
	struct timeval	tv;
	fd_set		read_fd;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&read_fd);
	FD_SET(0, &read_fd);
	usleep(1);
	if( select(1, &read_fd, NULL, NULL, &tv) == -1 )
		return 0;

	if( FD_ISSET(0, &read_fd) )
		return 1;

	return 0;
}
/*-----------------------------------------------------------------------
Name:          PrintHeader

Description:   Prints the sample program header.

In             none
Out            none
------------------------------------------------------------------------*/
static void PrintHeader()
{
   U16BIT     wLibVer;

   wLibVer = aceGetLibVersion();
   printf("*********************************************\n");
   printf("Enhanced Mini-ACE Integrated 1553 Terminal  *\n");
   printf("BU-69090 1553 Runtime Library               *\n");
   printf("Release Rev %d.%d.%d                           *\n",
         (wLibVer>>8),        /* Major */
         (wLibVer&0xF0)>>4,   /* Minor */
         (wLibVer&0xF));      /* Devel */
   if ((wLibVer&0xF)!=0)
   printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
   printf("Copyright (c) 2009 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("RTMode.c RT operations (Mode Code)          *\n");
   printf("*********************************************\n");
   printf("%s\n",GetLibraryRevisionInfo());
}

/*-----------------------------------------------------------------------
Name:          PrintOutError

Description:   This function prints out errors returned from library functions.

In             nResult = The error number.
Out            none
------------------------------------------------------------------------*/
static void PrintOutError(S16BIT nResult)
{
   char buf[80];
   aceErrorStr(nResult,buf,80);
   printf("RTL Function Failure-> %s.\n",buf);
}

/*-----------------------------------------------------------------------
Name:          GetRTModeCodeData

Description:   This function reads the data associated with the mode code
               receive command SYNCHRONIZE (with data).

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetRTModeCodeData(S16BIT DevNum)
{
    S16BIT s16Result;
    U16BIT s16Buf[RT_BUF_SIZE];
    U16BIT s16BufWdSize = RT_BUF_SIZE;
    U16BIT wData = 0x0000;

   /* Start RT */
   aceRTStart(DevNum);

   printf("\nUse TestSim/BusTracer to send mode code, Synchronize with Data\n");
   printf("\nPress <ENTER> to abort\n\n");

   while (!kbhit())
   {
       /* Retrieve message */
      s16Result = aceRTGetStkMsgsRaw(DevNum, s16Buf, s16BufWdSize);
      
      if(s16Result > 0)
      {
            printf("**aceRTGetStkMsgsRaw**\nBuffer Dump (NumWds:%d) status:%d\n", s16BufWdSize, s16Result);
            printf("%04x %04x %04x %04x %04x %04x %04x %04x\n",
                  s16Buf[0], s16Buf[1], s16Buf[2], s16Buf[3],
                  s16Buf[4], s16Buf[5], s16Buf[6], s16Buf[7]);
            printf("%04x %04x %04x %04x %04x %04x %04x %04x\n\n",
                  s16Buf[8], s16Buf[9], s16Buf[10], s16Buf[11],
                  s16Buf[12], s16Buf[13], s16Buf[14], s16Buf[15]);

            /* Read Mode Code Word */
            s16Result = aceRTModeCodeReadData(DevNum,ACE_RT_MCDATA_RX_SYNCHRONIZE,&wData);

          if (s16Result < 0)
          {
              printf("aceRTModeCodeReadData function failed to execute!\n");
              break;
          }
          else
          {
              /* Display Mode Code Data */
              printf("Current Data for Synchronize Mode Code Table: 0x%04x.\r",wData);
              if(wData == 0x11)
              {
                printf("\nTest Passed.\n");
                break;
              }
              else
              {
                printf("\nTest Failed.\n");
                break;
              }
          }
      }
   }

   /* Stop RT */
   aceRTStop(DevNum);

   printf("\n");
}

/*-----------------------------------------------------------------------
Name:       RtMode

Description:
	Program Entry Point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void RtMode(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)
{
   S16BIT DevNum          = 0x0000;
   S16BIT nResult         = 0x0000;
   S16BIT wRTAddress      = 0x0000;

   /* Print out sample header */
   PrintHeader();

   /* Get Logical Device # */
   printf("\nSelect RT Logical Device Number (0-31):\n");
   printf("> ");

   scanf("%hd", &DevNum);

   /* Initialize card and check for failure */
   nResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_RT, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);

   if(nResult)
   {
      printf("Initialize ");
      PrintOutError(nResult);
      return;
   }

   /* Select RT Address */
   printf("\nSelect RT Address (0-30):\n");
   printf("> ");

   scanf("%hd", &wRTAddress);

   /* Set RT Address */
   aceRTSetAddress(DevNum,wRTAddress);
   
   /* Legalize msgs received by the RT own address and mode code 17 (Synchronize with Data */
   aceRTMsgLegalityEnable(DevNum,ACE_RT_MODIFY_ALL,ACE_RT_MODIFY_ALL,
                            ACE_RT_MODIFY_ALL, ACE_RT_SA17);

   /* Checks Mode Code Data */
   GetRTModeCodeData(DevNum);

   /* Free all resources */
   nResult = aceFree(DevNum);
   if(nResult)
   {
      printf("Free ");
      PrintOutError(nResult);
      return;
   }

   /* Pause before Program exit */
   PressAKey();
}
