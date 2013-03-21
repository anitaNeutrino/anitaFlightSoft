
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
 *   RTIrq.c (Console App RT (Interrupt) Demo)
 *  ----------------------------------------------------------------------
 *  This application shows the use of the Enhanced Mini-ACE in RT mode.
 *  It shows how using a host buffer, the number of messages lost is much
 *  less significant than polling the stack.  The messages are exported
 *  to a file is in raw or decoded format.
 *  ----------------------------------------------------------------------
 *
 *   Created 8/26/99 DL
 *   Created 8/26/00 MR
 *   Updated 3/23/01 KWH   Ported to Linux
 *   Updated 8/04/05 ESK   Ported to Integrity
 *
 *  Function used in this example:
 *
 *      - aceErrorStr
 *      - aceMTStart
 *      - aceMTStop
 *      - aceMTEnableRTFilter
 *      - aceMTGetStkMsgsRaw
 *      - aceMTDisableRTFilter
 *      - aceMTGetStkMsgDecoded
 *      - aceGetMsgTypeString
 *      - aceCmdWordParse
 *      - aceGetBSWErrString
 *      - aceGetLibVersion
 *      - aceInitialize
 *      - aceFree
 *      - aceRTInstallHBuf
 *      - aceRTStart
 *      - aceRTStop
 *      - aceRTGetHBufMsgDecoded
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


#define DBLK3    3

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
   printf("BU-69090 Runtime Library                    *\n");
   printf("Release Rev %d.%d.%d                           *\n",
         (wLibVer>>8),        /* Major */
         (wLibVer&0xF0)>>4,   /* Minor */
         (wLibVer&0xF));      /* Devel */
   if ((wLibVer&0xF)!=0)
   printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
   printf("Copyright (c) 2007 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("RTIrq.c RT operations [Message Interrupts]  *\n");
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
Name:          DisplayDecodedMsg

Description:   This function displays the information of a MSGSTRUCT to
               the screen.

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
Name:          GetRTHBufDecodedMsgs

Description:   This function creates an RT stack file in ASCII text using
               all msgs read from the ACE.

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetRTHBufDecodedMsgs(S16BIT DevNum)
{
   S16BIT nResult    = 0x0000;
   U32BIT dwStkLost  = 0x00000000;
   U32BIT dwHBufLost = 0x00000000;
   U32BIT dwMsgCount = 0x00000000;
   U32BIT dwCurCount = 0x00000000;
   MSGSTRUCT sMsg;

   /* Create Host Buffer */
   nResult = aceRTInstallHBuf(DevNum,(512*ACE_MSGSIZE_RT)*4 * 3);
   if(nResult)
   {
      PrintOutError(nResult);
      return;
   }

   /* Start RT */
   aceRTStart(DevNum);

   printf("\n** Monitoring Device Number %d, Press <ENTER> key to stop **\n\n",DevNum);

   while(!kbhit())
   {
      /* Check Host Buffer for msgs */
      nResult = aceRTGetHBufMsgDecoded(DevNum,
                               &sMsg,
                               &dwMsgCount,
                               &dwStkLost,
                               &dwHBufLost,
                               ACE_RT_MSGLOC_NEXT_PURGE);
      if(nResult)
      {
         PrintOutError(nResult);
         return;
      }

      /* Message Lost */
      if((dwStkLost > 0) || (dwHBufLost > 0))
      {
         printf("Number of msgs lost %d\n", (int)(dwStkLost+dwHBufLost));
      }

      /* Message Found */
      if(dwMsgCount)
      {
         ++dwCurCount;
         DisplayDecodedMsg((S16BIT)dwCurCount,&sMsg);
      }
   }

   /* Throw away key */
   getchar();

   /* Stop RT */
   aceRTStop(DevNum);
   printf("\n");
}


/*-----------------------------------------------------------------------
Name:          RtIrq

Description:   Program entry point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/
void RtIrq(U32BIT dwMemWrdSize, U32BIT dwRegAddr, U32BIT dwMemAddr, int hDevice, int slot, int chan)
{
   /* Variable Definition */
   S16BIT     DevNum      = 0x0000;
   S16BIT     wResult     = 0x0000;
   S16BIT     wRTAddress  = 0x0000;
   U16BIT     wBuffer[32] = {0x0001};

   /* Print out sample header */
   PrintHeader();

   /* Get Logical Device # */
   printf("\nSelect RT Logical Device Number (0-31):\n");
   printf("> ");

   scanf("%hd",&DevNum);

   /* Initialize Device */
   wResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_RT, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
   if(wResult)
   {
      printf("Initialize ");
      PrintOutError(wResult);
      PressAKey();
      return;
   }

   /* Get RT Address */
   printf("\nSelect RT Address (0-30):\n");
   printf("> ");

   scanf("%hd", &wRTAddress);

   /* Create DBLK3 for RT */
   aceRTDataBlkCreate(DevNum, DBLK3, ACE_RT_DBLK_SINGLE, wBuffer, 32);

   /* Set RT [or latch (BU-65565 only)] address */
   aceRTSetAddress(DevNum,wRTAddress);

   /* Map data block to given sub-address */
   aceRTDataBlkMapToSA(DevNum, DBLK3, wRTAddress, ACE_RT_MSGTYPE_ALL, 0, TRUE);

   /* Check Interrupts for msgs */
   GetRTHBufDecodedMsgs(DevNum);

   /* Free all resources */
   wResult = aceFree(DevNum);
   if(wResult)
   {
      printf("Free ");
      PrintOutError(wResult);
   }

   /* Pause before Program exit */
   PressAKey();
}

