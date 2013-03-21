
/*   Data Device Corporation
 *   105 Wilbur Place
 *   Bohemia N.Y. 11716
 *   (631) 567-5600
 *
 *    AceXtreme 'C' Run Time Library Toolbox
 *
 *       Copyright (c) 2008 Data Device Corporation
 *       All Rights Reserved.
 *
 *
 *   MTPoll.c (Console App MT (Poll) Demo)
 *  ----------------------------------------------------------------------
 *   This application runs the AceXtreme in MT mode.  Using simple
 *   polling, the stack is queried for messages and read off as found. The
 *   application also allows filtering of messages to aid in a more
 *   focused report.
 *  ----------------------------------------------------------------------
 *
 *   Created  8/26/99 DL
 *   Modified 8/26/00 MR
 *   Updated  3/23/01 KWH
 *   Updated 12/13/04 ESK
 *   Updated  2/25/05 ESK
 *   Updated  8/04/05 ESK
 *
 *  Function used in this example:
 *
 *      - aceInitialize
 *      - aceFree
 *      - aceGetLibVersion
 *   - aceErrorStr
 *      - aceGetMsgTypeString
 *      - aceCmdWordParse
 *      - aceGetBSWErrString
 *      - aceMTStart
 *      - aceMTStop
 *      - aceMTEnableRTFilter
 *      - aceMTDisableRTFilter
 *      - aceMTGetStkMsgsRaw
 *      - aceMTGetStkMsgDecoded
 *      
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules. 
 */

/* Include Files */
#include "libPrj/include/stdemace.h"
#include <stdio.h>
#include <sys/time.h> /* time */
#include <sys/select.h> /* select() */
#include <unistd.h>	/* sleep() */

/*-----------------------------------------------------------------------
Name:	PressAKey()

Description:
	Allows application to pause to allow screen contents to be read.

In		none
Out		none
------------------------------------------------------------------------*/
static void PressAKey()
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
   printf("AceXtreme Integrated 1553 Terminal          *\n");
   printf("BU-69090 Runtime Library                    *\n");
   printf("Release Rev %d.%d.%d                           *\n",
         (wLibVer>>8),       /* Major */
         (wLibVer&0xF0)>>4,  /* Minor */
         (wLibVer&0xF));     /* Devel */
   if ((wLibVer&0xF)!=0)
   printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
   printf("Copyright (c) 2007 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("MTPoll.c MT operations [Poll Capturing]     *\n");
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
Name:          GetMTRawMsgs

Description:   This function gets messages in raw (binary) format off the MT stack.

In             DevNum = The logical device number of the MT device.
Out            none
------------------------------------------------------------------------*/
static void GetMTStkRawMsgs(S16BIT DevNum)
{
   int i,j;
   S16BIT nResult=0;
   U16BIT wBuffer[400];
   S16BIT nMsgNum;

   aceMTStart(DevNum);

   printf("\n** Monitoring Device Number %d, Press <ENTER> to stop **\n\n",DevNum);

   /* Poll Msgs */
   while(!kbhit())
   {
      /* Increment Msg Count */
      nMsgNum = nResult;
      nResult += aceMTGetStkMsgsRaw(DevNum,wBuffer,400,ACE_MT_STKLOC_ACTIVE);

      if(nMsgNum != nResult)
      {
         for(i=0;(nMsgNum+1)<=nResult; nMsgNum++,i++)
         {
            printf("\n\n-----Msg # %d-----", nMsgNum+1);
            for(j=0; j<40; j++)
            {
               if((j%8) == 0)
                  printf("\n");

               printf(" %04X  ",wBuffer[(i*40)+j]);

            }
         }
         printf("\nMT: Total Number of Messages %d\n",nResult);
      }
   }

   /* Throw away key */
   getchar();

   aceMTStop(DevNum);
}

/*-----------------------------------------------------------------------
Name:          DisplayDecodedMsg

Description:   This function displays the information of a MSGSTRUCT to the screen.

In             nMsgNum = The number of the message.
In             pMsg    = The relavant information of the message.
Out            none
------------------------------------------------------------------------*/
static void DisplayDecodedMsg(S16BIT nMsgNum,MSGSTRUCT *pMsg)
{
   U16BIT i;
   char szBuffer[100];
   U16BIT wRT,wTR1,wTR2,wSA,wWC;

   /* Display message header info */
   printf("MSG #%04d.  TIME = %08dus    BUS %c   TYPE%d: %s ",nMsgNum,
      pMsg->wTimeTag*2,pMsg->wBlkSts&ACE_MT_BSW_CHNL?'B':'A',
      pMsg->wType, aceGetMsgTypeString(pMsg->wType));

   /* Display command word info */
   aceCmdWordParse(pMsg->wCmdWrd1,&wRT,&wTR1,&wSA,&wWC);
   sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR1?'T':'R',wSA,wWC);
   printf("\n            CMD1 %04X --> %s",pMsg->wCmdWrd1,szBuffer);
   if(pMsg->wCmdWrd2Flg)
   {
      aceCmdWordParse(pMsg->wCmdWrd2,&wRT,&wTR2,&wSA,&wWC);
      sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR2?'T':'R',wSA,wWC);
      printf("\n        CMD2 %04X --> %s",pMsg->wCmdWrd2,szBuffer);
   }

   /* Display transmit status words */
   if((wTR1==1) || (pMsg->wBlkSts&ACE_MT_BSW_RTRT))
   {
      if(pMsg->wStsWrd1Flg)
         printf("\n            STA1 %04X",pMsg->wStsWrd1);
   }

   /* Display Data words */
   for(i=0; i<pMsg->wWordCount; i++)
   {
      if(i==0)
         printf("\n            DATA ");

      printf("%04X  ",pMsg->aDataWrds[i]);

      if((i%8)==7)
         printf("\n                 ");
   }

   /* Display receive status words */
   if((wTR1==0)  && !(pMsg->wBlkSts&ACE_MT_BSW_RTRT))
   {
      if(pMsg->wStsWrd1Flg)
         printf("\n            STA1 %04X",pMsg->wStsWrd1);
   }
   if(pMsg->wStsWrd2Flg)
      printf("\n            STA2 %04X",pMsg->wStsWrd2);

   /* Display Error information */
   if(pMsg->wBlkSts & ACE_MT_BSW_ERRFLG)
      printf("\n ERROR: %s",aceGetBSWErrString(ACE_MODE_MT,pMsg->wBlkSts));
   printf("\n\n");
}

/*-----------------------------------------------------------------------
Name:          ConfigfureRTFiltering

Description:   This function allows the user to filter specific RT information.

In             DevNum = The logical device number of the MT device.
Out            none
------------------------------------------------------------------------*/
static void ConfigureRTFiltering(S16BIT DevNum)
{
   S16BIT wResult     = 0x0000;
   U16BIT wRTAddress  = 0x0000;
   U16BIT wTxRx       = 0x0000;
   U16BIT wSubAddr    = 0x0000;
   U16BIT wSelection  = 0x0000;
   U32BIT wSAMask     = 0x00000001;

   printf("\n** Configure RT Filtering **\n\n");

   /* Select filtered RT address */
   printf("Filter which RT address (0-30, All = -1)?\n");
   printf("> ");

   scanf("%hd", &wRTAddress);

   /* Select RT trasmit or receive filtering */
   printf("\nFilter Transmit or Receive (All = -1)?\n");
   printf("1. Transmit\n");
   printf("2. Receive\n");
   printf("> ");

   scanf("%hd", &wTxRx);

   switch(wTxRx)
   {
      case 1:
         wTxRx = ACE_MT_FILTER_TX;
      break;

      case 2:
         wTxRx = ACE_MT_FILTER_RX;
      break;

      default:
         wTxRx = ACE_MT_FILTER_ALL;
      break;
   }

   /* Select filtered sub-address */
   printf("\nFilter which sub-address (1-30, All = -1)?\n");
   printf("> ");

   scanf("%hd", &wSubAddr);

   /* Shift to mask select SA */
   if (wSubAddr != 0xffff)
   {
      wSAMask <<= wSubAddr;
   }else
   {
      wSAMask = 0xffffffff;
   }

   printf("\nEnable or Disable RT Monitoring using above configuration?\n");
   printf("1. Enable RT Monitoring\n");
   printf("2. Disable RT Monitoring\n");
   printf("> ");

   scanf("%hd", &wSelection);

   switch(wSelection)
   {
      case 1:
         /* must disable all filters first */
         wResult = aceMTDisableRTFilter(DevNum,ACE_MT_FILTER_ALL,ACE_MT_FILTER_ALL,ACE_MT_FILTER_SA_ALL);
         wResult = aceMTEnableRTFilter(DevNum,wRTAddress,wTxRx,wSAMask);
      break;

      case 2:
         wResult = aceMTDisableRTFilter(DevNum,wRTAddress,wTxRx,wSAMask);
      break;
   }
   if(wResult)
   {
      printf("\nFree ");
      PrintOutError(wResult);
   }
}

/*-----------------------------------------------------------------------
Name:          GetMTStkDecodedMsgs

Description:   This function gets messages in a decoded structure off the stack.

In             DevNum = The logical device number of the MT device.
Out            none
------------------------------------------------------------------------*/
static void GetMTStkDecodedMsgs(S16BIT DevNum)
{
   S16BIT nResult;
   MSGSTRUCT sMsg;
   S16BIT nMsgNum = 0;

   aceMTStart(DevNum);

   printf("\n** Monitoring Device Number %d, Press <ENTER> to stop **\n\n",DevNum);

   /* Poll Msgs */
   while(!kbhit())
   {
      nResult = aceMTGetStkMsgDecoded(DevNum,&sMsg,
         ACE_MT_MSGLOC_NEXT_PURGE, ACE_MT_STKLOC_ACTIVE);
      if(nResult==1)
      {
         ++nMsgNum;
         DisplayDecodedMsg(nMsgNum,&sMsg);
      }
   }

   /* Throw away key */
   getchar();

   aceMTStop(DevNum);
}

/*-----------------------------------------------------------------------
Name:          MtPoll

Description:   Program entry point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void MtPoll(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)
{
   /* Variable Definition */
   S16BIT     DevNum      = 0x0000;
   S16BIT     wResult     = 0x0000;
   S16BIT     wSelection  = 0x0000;

   /* Print out sample header */
   PrintHeader();

   /* Get Logical Device # */
   printf("\nSelect MT Logical Device Number (0-31):\n");
   printf("> ");

   scanf("%hd", &DevNum);

   /* Initialize Device */
   wResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_MT, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
   if(wResult)
   {
      printf("Initialize ");
      PrintOutError(wResult);
      PressAKey();
      return;
   }

   /* RT Filtering Selection */
   printf("\nWould you like to use RT Filtering?\n");
   printf("1. Yes\n");
   printf("2. No\n");
   printf("> ");

   scanf("%hd", &wSelection);

   switch(wSelection)
   {
      case 1:
         /* Setup simple RT Filter */
         ConfigureRTFiltering(DevNum);
      break;

      default:
      break;
   }

   /* Display messages as Raw or Decoded Formats */
   printf("\nWould you like messages displayed as:\n");
   printf("1. Raw\n");
   printf("2. Decoded\n");
   printf(">  ");

   scanf("%hd", &wSelection);

   switch(wSelection)
   {
      case 1:  GetMTStkRawMsgs(DevNum);      break;
      case 2:  GetMTStkDecodedMsgs(DevNum);  break;
      default: GetMTStkDecodedMsgs(DevNum);  break;
   }

   /* Free Ace Memory and Device */
   wResult = aceFree(DevNum);
   if(wResult)
   {
      printf("Free ");
      PrintOutError(wResult);
   }

   /* Pause before program exit */
   PressAKey();
}
