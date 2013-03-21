
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
 *   MTIrq.c (Console App MT (Interrupt) Demo)
 *  ----------------------------------------------------------------------
 *  This application show the use of the AceXtreme in MT mode.
 *  It shows how using a host buffer, the number of messages lost is much
 *  less significant than polling the stack.  The messages are exported
 *  to a file is in raw or decoded format.
 *  ----------------------------------------------------------------------
 *
 *   Created  8/26/99 DL
 *   Modified 8/26/00 MR
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
 *      - aceMTStart
 *      - aceMTStop
 *      - aceMTGetInfo
 *      - aceMTInstallHBuf
 *      - aceMTGetHBufMsgsRaw
 *      - aceMTGetHBufMsgDecoded
 *      
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
 *
 * The following define is provided in case the target hardware does not
 * provide for a file system. In this case text output is directed to
 * stdout and binary output is omited.
 * 
 * If a file system is present, then comment out the "#define FILESYSTEMABSENT	1".
 * Text and binary output will be directed to the file system.
 */

#define FILESYSTEMABSENT	1


/* Include Files */
#include "libPrj/include/stdemace.h"
#include <stdio.h>
#include <sys/time.h> /* time */
#include <sys/select.h> /* select() */
#include <unistd.h>	/* sleep() */

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
         (wLibVer>>8),       /* Major */
         (wLibVer&0xF0)>>4,  /* Minor */
         (wLibVer&0xF));     /* Devel */
   if ((wLibVer&0xF)!=0)
   printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
   printf("Copyright (c) 2008 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("MTIrq.c MT operations [Interrupt Capturing] *\n");
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
Name:          TestMTHBufGetRawMsgs

Description:   This function creates an MT stack file using all msgs read
               from the ACE.

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetMTHBufRawMsgs(S16BIT DevNum)
{
   /* Variables */
   FILE      *pFile;
   S16BIT i             = 0x0000;
   S16BIT nResult       = 0x0000;
   U32BIT dwStkLost     = 0x00000000;
   U32BIT dwHBufLost    = 0x00000000;
   U32BIT dwMsgCount    = 0x00000000;
   U32BIT dwCurCount    = 0x00000000;
   MTINFO sMTInfo;
   U32BIT dwPrevCount   = 0x00000000;
   U16BIT wBuffer[400]  = { 0x00000000 };


   /* Open file */
#ifdef FILESYSTEMABSENT
	pFile = stdout;
#else
	pFile = fopen("mtstack.bin","w+b");
#endif /* FILESYSTEMABSENT */

   /* Copy dummy message to file so we know that it is an MT stack file*/
   memset(wBuffer,0,80);

   if((pFile != NULL) && (pFile != stdout))
      fwrite(wBuffer, 80, 1, pFile);

   /* Get & Display MT information */
   aceMTGetInfo(DevNum,&sMTInfo);
   printf("\n");
   printf("MT Stk Mode            = %s\n", sMTInfo.wStkMode?"DOUBLE":"SINGLE");
   printf("MT Cmd Stk Size(wrds)  = %d\n", sMTInfo.wCmdStkSize);
   printf("MT Data Stk Size(wrds) = %d\n", sMTInfo.wDataStkSize);

   /* Create Host Buffer */
   aceMTInstallHBuf(DevNum,((sMTInfo.wCmdStkSize/4)*40)*8 + 3);
   aceMTGetInfo(DevNum,&sMTInfo);
   printf("MT HBuf Size(wrds)     = %d\n", (int)sMTInfo.dwHBufSize);

   /* Start MT */
   aceMTStart(DevNum);

   /* Run Information */
   printf("%s%d","\n** MT Device Number ",DevNum);
   printf("%s"," outputting to 'mtstack.bin'");
   printf("%s",", press <ENTER> key to stop **\n\n");

   printf("\r");

   while(!kbhit())
   {
      /* Check Host Buffer for msgs */
      nResult = aceMTGetHBufMsgsRaw(DevNum,wBuffer,400,&dwMsgCount,&dwStkLost,
              &dwHBufLost);
      if(nResult)
      {
         printf("Error reading host buffer\n");
      }

      /* Msg Lost */
      if((dwStkLost > 0) || (dwHBufLost > 0))
         printf("Number of msgs lost %d\n",(int)(dwStkLost+dwHBufLost));

      /* Msg Found */
      if(dwMsgCount)
      {
         dwPrevCount = dwCurCount;
         dwCurCount += dwMsgCount;

         for(i=0; dwPrevCount<dwCurCount; dwPrevCount++,i++)
         {
            if((pFile != NULL) && (pFile != stdout))
               fwrite(&wBuffer[i*40], 80, 1, pFile);
         }
         printf("MT: Total msgs captured: %d\r", (int)dwCurCount);
      }
   }

   /* Throw away key */
   getchar();

   /* Stop MT */
   aceMTStop(DevNum);

   /* Close out of file */
   if((pFile != NULL) && (pFile != stdout))
	fclose(pFile);

   printf("\n");
}

/*-----------------------------------------------------------------------
Name:          OutputDecodedMsg

Description:   This function creates an MT stack file in ASCII text using
               all msgs read from the ACE.

In             DevNum    = the device number of the hardware to be accessed.
In             pFileName = pointer to a null terminated string containing file
                          name.
Out            none
------------------------------------------------------------------------*/
static void OutputDecodedMsg(U32BIT nMsgNum,MSGSTRUCT *pMsg,FILE *pFile)
{
   U16BIT i;
   char szBuffer[100];
   U16BIT wRT,wTR1,wTR2,wSA,wWC;

   /* Display message header info */
   fprintf(pFile,"MSG #%04d.  TIME = %08dus    BUS %c   TYPE%d: %s ",
      (int)nMsgNum,pMsg->wTimeTag*2,pMsg->wBlkSts&ACE_MT_BSW_CHNL?'B':'A',
      pMsg->wType, aceGetMsgTypeString(pMsg->wType));

   /* Display command word info */
   aceCmdWordParse(pMsg->wCmdWrd1,&wRT,&wTR1,&wSA,&wWC);
   sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR1?'T':'R',wSA,wWC);
   fprintf(pFile,"\n            CMD1 %04X --> %s",pMsg->wCmdWrd1,
      szBuffer);
   if(pMsg->wCmdWrd2Flg)
   {
      aceCmdWordParse(pMsg->wCmdWrd2,&wRT,&wTR2,&wSA,&wWC);
      sprintf(szBuffer,"%02d-%c-%02d-%02d",wRT,wTR2?'T':'R',wSA,wWC);
      fprintf(pFile,"\n            CMD2 %04X --> %s",pMsg->wCmdWrd2,
         szBuffer);
   }

   /* Display transmit status words */
   if((wTR1==1) || (pMsg->wBlkSts&ACE_MT_BSW_RTRT))
   {
      if(pMsg->wStsWrd1Flg)
         fprintf(pFile,"\n            STA1 %04X",pMsg->wStsWrd1);
   }

   /* Display Data words */
   for(i=0; i<pMsg->wWordCount; i++)
   {
      if(i==0)
         fprintf(pFile,"\n            DATA ");

      fprintf(pFile,"%04X  ",pMsg->aDataWrds[i]);

      if((i%8)==7)
         fprintf(pFile,"\n                 ");
   }

   /* Display receive status words */
   if((wTR1==0)  && !(pMsg->wBlkSts&ACE_MT_BSW_RTRT))
   {
      if(pMsg->wStsWrd1Flg)
         fprintf(pFile,"\n            STA1 %04X",pMsg->wStsWrd1);
   }
   if(pMsg->wStsWrd2Flg)
      fprintf(pFile,"\n            STA2 %04X",pMsg->wStsWrd2);

   /* Display Error information */
   if(pMsg->wBlkSts & ACE_MT_BSW_ERRFLG)
   {
      fprintf(pFile,"\n ERROR: %s",
         aceGetBSWErrString(ACE_MODE_MT,pMsg->wBlkSts));
   }
   fprintf(pFile,"\n---------------------------------------------------------\n");
}

/*-----------------------------------------------------------------------
Name:          TestMTHBufGetDecodedMsgs

Description:   This function creates an MT stack file in ASCII text using
               all msgs read from the ACE.

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetMTHBufDecodedMsgs(S16BIT DevNum)
{
   FILE      *pFile;
   S16BIT    wResult    = 0x0000;
   U32BIT    dwStkLost  = 0x00000000;
   U32BIT    dwHBufLost = 0x00000000;
   U32BIT    dwMsgCount = 0x00000000;
   U32BIT    dwCurCount = 0x00000000;
   MSGSTRUCT sMsg;
   MTINFO    sMTInfo;



   /* Open file */
#ifdef FILESYSTEMABSENT
	pFile = stdout;
#else
	pFile = fopen("mtstack.txt","w+");
#endif /* FILESYSTEMABSENT */

   /* Display MT info to screen */
   aceMTGetInfo(DevNum,&sMTInfo);
   printf("\nMT Stk Mode            = %s\n", sMTInfo.wStkMode?"DOUBLE":"SINGLE");
   printf("MT Cmd Stk Size(wrds)  = %d\n", sMTInfo.wCmdStkSize);
   printf("MT Data Stk Size(wrds) = %d\n", sMTInfo.wDataStkSize);
   aceMTInstallHBuf(DevNum,((sMTInfo.wCmdStkSize/4)*40)*8);
   aceMTGetInfo(DevNum,&sMTInfo);
   printf("MT HBuf Size(wrds)     = %d\n", (int)sMTInfo.dwHBufSize);

   /* Run Information */
   printf("%s%d","\n** MT Device Number ",DevNum);
   printf("%s"," outputting to 'mtstack.txt'");
   printf("%s",", press <ENTER> key to stop **\n\n");

   printf("\r");

   /* Start MT */
   aceMTStart(DevNum);

   while(!kbhit())
   {
      /* Check host buffer for msgs */
      wResult = aceMTGetHBufMsgDecoded(DevNum,&sMsg,&dwMsgCount,&dwStkLost,
               &dwHBufLost,ACE_MT_MSGLOC_NEXT_PURGE);
      if(wResult)
      {
         printf("Error reading host buffer\n");
      }

      /* Msg lost */
      if((dwStkLost > 0) || (dwHBufLost > 0))
         printf("Number of msgs lost %d\n", (int)dwStkLost);

      /* Msg found */
      if(dwMsgCount)
      {
         ++dwCurCount;
         OutputDecodedMsg(dwCurCount,&sMsg,pFile);
         printf("MT: Total msgs captured: %d\r", (int)dwCurCount);
      }
   }

   /* Throw away key */
   getchar();
   printf("\n");

   /* Stop MT */
   aceMTStop(DevNum);

   /* Close out of file */
   if((pFile != NULL) && (pFile != stdout))
	fclose(pFile);
}

/*-----------------------------------------------------------------------
Name:          MtIrq

Description:   Program entry point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/
void MtIrq(U32BIT dwMemWrdSize, U32BIT dwRegAddr, U32BIT dwMemAddr, int hDevice, int slot, int chan)
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
      
   /* Select Raw or Decoded Msgs */
   printf("\nSelect stack output format:\n");
   printf("1. Text\n");
   printf("2. Binary\n");
   printf("> ");

   scanf("%hd", &wSelection);

   switch(wSelection)
   {
      case 1:
         GetMTHBufDecodedMsgs(DevNum);
      break;

      case 2:
         GetMTHBufRawMsgs(DevNum);
      break;
   }

   /* Free all resources */
   wResult = aceFree(DevNum);
   if(wResult)
   {
      printf("Free ");
      PrintOutError(wResult);
      PressAKey();
      return;
   }

   /* Pause before program exit */
   PressAKey();
}
