
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
 *   BCDemo.c (Console App BC (Basic Operations) Demo)
 *  ----------------------------------------------------------------------
 *   This application runs the Enhanced-Mini-ACE in BC mode.  It shows how
 *   the user can operate the BC is basic operations.  It allows the user
 *  to output the stack in raw or decoded(text) format.
 *  ----------------------------------------------------------------------
 *
 *   Created  8/26/99 DL
 *   Modified 8/26/00 MR
 *   Updated 3/23/01 KWH
 *   Updated 8/03/05 ESK
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
 *      - aceBCGetHBufMsgDecoded
 *      - aceBCGetHBufMsgRaw
 *      - aceDataBlkCreate
 *      - aceBCMsgCreateBCtoRT
 *      - aceBCMsgCreateRTtoBC
 *      - aceBCMsgCreateRTtoRT
 *      - aceBCOpCodeCreate
 *      - aceBCFrameCreate
 *      - aceBCInstallHBuf
 *      - aceBCStart
 *      - aceBCStop
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

/* define data blocks */
#define DBLK1  1

/* define message constants */
#define MSG1   1

/* define opcodes */
#define OP1    1
#define OP2    2

/* define frame constants */
#define MNR1   1
#define MJR    2

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
Name:          Sleep()

Description:   Sleeps for 'dws' seconds.

In             dws' seconds
Out            none
------------------------------------------------------------------------*/
static void Sleep(unsigned long int dws)
{
  	sleep((unsigned int) dws);
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
static void PrintHeader(void)
{
   U16BIT     wLibVer;

   wLibVer = aceGetLibVersion();
   printf("*********************************************\n");
   printf("Enhanced Mini-ACE Integrated 1553 Terminal  *\n");
   printf("BU-69090 Runtime Library                    *\n");
   printf("Release Rev %d.%d.%d                           *\n",
         (wLibVer>>8),        /* Major*/
         (wLibVer&0xF0)>>4,   /* Minor*/
         (wLibVer&0xF));      /* Devel*/
   if ((wLibVer&0xF)!=0)
   printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
   printf("Copyright (c) 2007 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("BCDemo.c BC operations [Basic Op Demo]      *\n");
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
Name:          OutputDecodedMsg

Description:   This function creates an BC stack file in ASCII text using
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
      (int)nMsgNum,pMsg->wTimeTag*2,pMsg->wBlkSts&ACE_BC_BSW_CHNL?'B':'A',
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
   if((wTR1==1) || (pMsg->wBCCtrlWrd&ACE_BCCTRL_RT_TO_RT))
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
   if((wTR1==0)  && !(pMsg->wBCCtrlWrd&ACE_BCCTRL_RT_TO_RT))
   {
      if(pMsg->wStsWrd1Flg)
         fprintf(pFile,"\n            STA1 %04X",pMsg->wStsWrd1);
   }
   if(pMsg->wStsWrd2Flg)
      fprintf(pFile,"\n            STA2 %04X",pMsg->wStsWrd2);

   /* Display Error information */
   if(pMsg->wBlkSts & 0x170f)
      fprintf(pFile,
      "\n ERROR: %s",aceGetBSWErrString(ACE_MODE_BC,pMsg->wBlkSts));

   fprintf(pFile,"\n---------------------------------------------------------\n");
}

/*-----------------------------------------------------------------------
Name:          GetBCHBufRawMsgs

Description:   This function creates an BC stack file in ASCII text using
               all msgs read from the ACE and outputs to a file.

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetBCHBufRawMsgs(S16BIT DevNum)
{
   /* Variables */
   FILE   *pFile;
   U16BIT    i             =  0x0000;
   U16BIT    wMsgCount     =  0x0000;
   U32BIT    dwMsgCount    =  0x00000000;
   U32BIT    dwMsgLost     =  0x00000000;
   U32BIT    dwPrevCount   =  0x00000000;
   U32BIT    dwCurCount    =  0x00000000;
   U16BIT    wRepeatCount  =  20;


   U16BIT wBuffer[64] =
   {  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
      0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
      0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
      0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
   };

   /* Open file */
#ifdef FILESYSTEMABSENT
	pFile = stdout;
#else
	pFile = fopen("bcstack.bin","wb+");
#endif /* FILESYSTEMABSENT */

   printf("\n** The BC will now send %d messages",wRepeatCount);

   /* Start BC */
   aceBCStart(DevNum,MJR,wRepeatCount);

   /* Run Information */
   printf("%s%d%s%s",
          "\n** BC Device Number ",
         DevNum,
         ", outputting to 'bcstack.bin'",
         ", press <ENTER> to stop **\n\n"
        );

   PressAKey();
   printf("\n\n");

   while(!kbhit())
   {
      /* Check host buffer for msgs */
      aceBCGetHBufMsgsRaw(DevNum,wBuffer,ACE_MSGSIZE_BC,
                        &dwMsgCount,&dwMsgLost);

      /* Msg found */
      if(dwMsgCount)
      {
         dwPrevCount += dwMsgLost;
         dwCurCount += dwMsgCount;

         for(i=0; dwPrevCount<dwCurCount; dwPrevCount++,i++)
         {
        	 if((pFile != NULL) && (pFile != stdout))
        	    fwrite(&wBuffer[i*ACE_MSGSIZE_BC], 84, 1, pFile);
         }
         printf("BC: Total msgs captured: %d                  \r",
                (int)(dwCurCount+dwMsgLost));

         wMsgCount++;
      }
      Sleep(1);
   }

   /* Throw away key */
   getchar();

   /* Close out of file */
   if((pFile != NULL) && (pFile != stdout))
	fclose(pFile);

   /* Stop BC */
   aceBCStop(DevNum);

   printf("\n");
}

/*-----------------------------------------------------------------------
Name:          GetBCHBufDecodedMsgs

Description:   This function creates an BC stack file in ASCII text using
               all msgs read from the ACE and outputs to a file.

In             DevNum = the device number of the hardware to be accessed.
Out            none
------------------------------------------------------------------------*/
static void GetBCHBufDecodedMsgs(S16BIT DevNum)
{
   FILE   *pFile;
   S16BIT nResult       = 0x0000;
   U32BIT dwHBufLost    = 0x00000000;
   U32BIT dwMsgCount    = 0x00000000;
   U32BIT dwCurCount    = 0x00000000;
   U16BIT wRepeatCount  =  20;
   MSGSTRUCT sMsg;

#ifdef FILESYSTEMABSENT
	pFile = stdout;
#else
	pFile = fopen("bcstack.txt","w+t");
#endif /* FILESYSTEMABSENT */

   printf("\n** The BC will now send %d messages",wRepeatCount);

   /* Start BC */
   aceBCStart(DevNum,MJR,wRepeatCount);

   /* Run Information */
   printf("%s%d%s%s",
          "\n** BC Device Number ",
         DevNum,
         ", outputting to 'bcstack.txt'",
         ", press <ENTER> to stop **\n\n"
        );

   printf("** Messages will also be output to the screen\n");

   PressAKey();
   printf("\n\n");

   while(!kbhit())
   {
      /* Check host buffer for msgs */
      nResult = aceBCGetHBufMsgDecoded(DevNum,&sMsg,&dwMsgCount,
               &dwHBufLost,ACE_BC_MSGLOC_NEXT_PURGE);
      if(nResult)
      {
         printf("Error reading host buffer\n");
      }

      /* Display msg lost count */
      if(dwHBufLost > 0)
         printf("Number of msgs lost %u\n",(int)dwHBufLost);

      /* Message found */
      if(dwMsgCount)
      {
         ++dwCurCount;
         OutputDecodedMsg(dwCurCount,&sMsg,pFile);
         printf("BC: Total msgs captured: %d                \r",(int)dwCurCount);
      }
      if((pFile != NULL) && (pFile != stdout))
		  DisplayDecodedMsg((U16BIT)dwCurCount,&sMsg);

      Sleep(1);
   }

   /* Throw out key */
   getchar();

   /* Close out of file */
   if((pFile != NULL) && (pFile != stdout))
	fclose(pFile);

   /* Stop BC */
   aceBCStop(DevNum);

   printf("\n");
}

/*-----------------------------------------------------------------------
Name:          BcDemo

Description:   Program entry point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/
void BcDemo(U32BIT dwMemWrdSize, U32BIT dwRegAddr, U32BIT dwMemAddr, int hDevice, int slot, int chan)
{
   /* Variables */
   S16BIT    DevNum       =  0x0000;
   S16BIT    wResult      =  0x0000;
   S16BIT    wSelection   =  0x0000;
   S16BIT    aOpCodes[10] = {0x0000};
   U32BIT    u32BusSelection =  1;

   U16BIT wBuffer[64] =
   {  0x1111,0x2222,0x3333,0x4444,0x1111,0x2222,0x3333,0x4444,
      0x1111,0x2222,0x3333,0x4444,0x1111,0x2222,0x3333,0x4444,
      0x1111,0x2222,0x3333,0x4444,0x1111,0x2222,0x3333,0x4444,
      0x1111,0x2222,0x3333,0x4444,0x1111,0x2222,0x3333,0x4444
   };


   /* Print out sample header */
   PrintHeader();

   /* Get Logical Device # */
   printf("\nSelect BC Logical Device Number (0-31):\n");
   printf("> ");

   scanf("%hd", &DevNum);

   /* Get Logical Device # */
   printf("\nSelect 1 For Bus A or 2 for Bus B\n");
   printf("> ");

   scanf("%ld", &u32BusSelection);

   /* Initialize Device */
   wResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_BC, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);

   if(wResult)
   {
      printf("Initialize ");
      PrintOutError(wResult);
      PressAKey();
      return;
   }

   /* Create data block */
   aceBCDataBlkCreate(DevNum,DBLK1,32,wBuffer,32);

   if (u32BusSelection == 1) u32BusSelection = ACE_BCCTRL_CHL_A;
   else u32BusSelection = ACE_BCCTRL_CHL_B;

   /* Create message block */
   aceBCMsgCreateBCtoRT(
      DevNum,              /* Device number                    */
      MSG1,                /* Message ID to create             */
      DBLK1,               /* Message will use this data block */
      1,                   /* RT address                       */
      1,                   /* RT subaddress                    */
      10,                  /* Word count                       */
      0,                   /* Default message timer            */
      u32BusSelection);   /* use chl A options                */

   /* Create XEQ opcode that will use msg block */
   aceBCOpCodeCreate(DevNum,OP1,ACE_OPCODE_XEQ,ACE_CNDTST_ALWAYS,MSG1,0,0);

   /* Create CAL opcode that will call mnr frame from major */
   aceBCOpCodeCreate(DevNum,OP2,ACE_OPCODE_CAL,ACE_CNDTST_ALWAYS,MNR1,0,0);

   /* Create Minor Frame */
   aOpCodes[0] = OP1;
   aceBCFrameCreate(DevNum,MNR1,ACE_FRAME_MINOR,aOpCodes,1,0,0);

   /* Create Major Frame */
   aOpCodes[0] = OP2;
   aceBCFrameCreate(DevNum,MJR,ACE_FRAME_MAJOR,aOpCodes,1,1000,0);

   /* Create Host Buffer */
   aceBCInstallHBuf(DevNum,32*1024);

   /* Display messages as Raw or Decoded Formats */
   printf("\nWould you like messages displayed as:\n");
   printf("1. Raw\n");
   printf("2. Decoded\n");
   printf(">  ");
   scanf("%hd", &wSelection);

   switch(wSelection)
   {
      case 1: GetBCHBufRawMsgs(DevNum);      break;
      case 2: GetBCHBufDecodedMsgs(DevNum);  break;
      default:GetBCHBufDecodedMsgs(DevNum);  break;
   }

   /* Free all resources */
   wResult = aceFree(DevNum);
   if(wResult)
   {
      printf("Free ");
      PrintOutError(wResult);
      return;
   }

   /* Pause before program exit */
   PressAKey();

   return;
}

