
/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE Software Toolbox
 *
 *			Copyright (c) 2002 Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	RTMTCmbDemo.c (Console App RT/Monitor Combination Demo)
 *  -----------------------------------------------------------------------
 *	This application runs the Enhanced-Mini-ACE in RTMT mode.  The chip
 *  is configured to monitor all messages, plus act as an RT by the user
 *  specified settings.  All messages monitored, included emulated RT
 *  messages are captured by the RTMT Host Buffer. All MT and RT operations 
 *  are valid in this mode.
 *  -----------------------------------------------------------------------
 *
 *	Created 4/24/02 MR
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

/* Global Variables */
U32BIT dwRTStkLost	= 0x00000000;
U32BIT dwMTStkLost	= 0x00000000;
U32BIT dwRTHBufLost     = 0x00000000;
U32BIT dwRTMTHBufLost   = 0x00000000;
U32BIT dwNumOfMsgs	= 0x00000000;

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
Name:	PrintHeader

Description:
	Prints the sample program header.

In		none
Out		none
------------------------------------------------------------------------*/
static void PrintHeader()
{
	U16BIT     wLibVer;
	
	wLibVer = aceGetLibVersion();
	printf("*********************************************\n");
	printf("Enhanced Mini-ACE Integrated 1553 Terminal  *\n");
    printf("BU-69090 Runtime Library                    *\n");
	printf("Release Rev %d.%d.%d                           *\n",
		    (wLibVer>>8),			
			(wLibVer&0xF0)>>4,		
			(wLibVer&0xF));			
	if ((wLibVer&0xF)!=0)
	printf("=-=-=-=-=-=-DEVELOPMENT VERSION-=-=-=-=-=-=-*\n");
	printf("Copyright (c) 2000 Data Device Corporation  *\n");
	printf("*********************************************\n");
	printf("RTMTCmbDemo.c RT/MT operations Demo         *\n");
	printf("*********************************************\n");
	printf("%s\n",GetLibraryRevisionInfo());
}

/*-----------------------------------------------------------------------
Name:	PrintOutError

Description:
	This function prints out errors returned from library functions.

In		nResult = The error number.
Out		none
------------------------------------------------------------------------*/
static void PrintOutError(S16BIT nResult)
{
	char buf[80];
	aceErrorStr(nResult,buf,80);
	printf("RTL Function Failure-> %s.\n",buf);
}

/*-----------------------------------------------------------------------
Name:	DisplayDecodedMsg

Description:
	This function displays the information of a MSGSTRUCT to the screen.

In		nMsgNum = The number of the message.
In		pMsg    = The relavant information of the message.
Out		none
------------------------------------------------------------------------*/
static void DisplayDecodedMsg(U32BIT nMsgNum,MSGSTRUCT *pMsg)
{
	U16BIT i;
	char szBuffer[100];
	U16BIT wRT,wTR1,wTR2,wSA,wWC;
	
	/* Display message header info */
	printf("MSG #%04ld.  TIME = %08dus    BUS %c   TYPE%d: %s ",nMsgNum,
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
Name:	ProcessHBufMsgs

Description:
	This function will remove messages from the RT and RTMT Host 
	Buffer and process them (print to screen).

In		DevNum = The logical device number of the RTMT device.
Out		none
------------------------------------------------------------------------*/
void ProcessHBufMsgs(S16BIT DevNum)
{
	MSGSTRUCT sMsg;
	U32BIT	  dwMsgCount=0x00000000;
	
		/* Get RT Messages */
		aceRTGetHBufMsgDecoded( DevNum,
					&sMsg,
					&dwMsgCount,
					&dwRTStkLost,
					&dwRTHBufLost,
					ACE_RT_MSGLOC_NEXT_PURGE);
		if(dwMsgCount)
		{
			++dwNumOfMsgs;
			printf("-RT Message-\n");
			DisplayDecodedMsg(dwNumOfMsgs,&sMsg);
		}

		/* Get RTMT Messages */
		aceRTMTGetHBufMsgDecoded( DevNum,
					  &sMsg,
					  &dwMsgCount,
					  &dwRTStkLost,
					  &dwMTStkLost,
					  &dwRTMTHBufLost,
 					  ACE_MT_MSGLOC_NEXT_PURGE);

		/* Msg found */
		if(dwMsgCount)
		{
			++dwNumOfMsgs;
			printf("-RTMT Message-\n");
			DisplayDecodedMsg(dwNumOfMsgs,&sMsg);
		}
}

/*-----------------------------------------------------------------------
Name:	GetRTMTHBufDecodedMsgs

Description:
	This function gets the set RT messages or global MT messages.

In		DevNum = The logical device number of the RTMT device.
Out		none
------------------------------------------------------------------------*/
void GetRTMTHBufDecodedMsgs( S16BIT DevNum,
						     S16BIT wRTAddr,
						     S16BIT wRTSubAddr)
{
	S16BIT    nResult = 0x0000;
	
	/* Start RT/MT Operations */
	nResult = aceRTMTStart(DevNum);

	if(nResult)
	{
		printf("RTMT Start ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	printf("\n** Monitoring Device # %d as an MT and RT Addr#%d/SA#",DevNum,wRTAddr);
	printf("%d, Press <ENTER> to stop **\n\n",wRTSubAddr);

	/* Start processing messages waiting for user to stop */
	while(!kbhit())
	{
		ProcessHBufMsgs(DevNum);
	}
	
	/* Throw away key */
    getchar();

	/* Stop RT/MT Operations */
	aceRTMTStop(DevNum);
}

/*-----------------------------------------------------------------------
Function Name:  RtMtCmbDemo

Description:
	This program runs the card in combined RT/MT mode.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void RtMtCmbDemo(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)
{
	MTINFO sMTInfo;
	S16BIT DevNum;
	S16BIT nResult         = 0x0000;
	S16BIT wRTAddress      = 0x0001;
	S16BIT wRTSubAddress   = 0x0001;
	U32BIT dwRTHBufSize    = 0x00000000;
	U32BIT dwMTHBufSize    = 0x00000000;
	U32BIT dwRTMTHBufSize  = 0x00000000;

	/* Initialize Global Variables */
	dwNumOfMsgs 		= 0x00000000;
	dwRTStkLost		= 0x00000000;
	dwMTStkLost		= 0x00000000;
	dwRTHBufLost     	= 0x00000000;
	dwRTMTHBufLost   	= 0x00000000;

	/* Print out sample header */
	PrintHeader();
	
	/* Get Logical Device # */
	printf("\nSelect RT/MT Logical Device Number (0-31):\n");
	printf("> ");
	scanf("\n%hd", &DevNum);
	getchar();
	
	/* Initialize Device */
	nResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_RTMT, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
	if(nResult)
	{
		printf("Initialize ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	/* Configure RTMT for combination monitoring */
	nResult = aceRTMTConfigure( DevNum,
						 	    ACE_RT_CMDSTK_2K,
								ACE_MT_SINGLESTK,
								ACE_MT_CMDSTK_4K,
								ACE_MT_DATASTK_32K,
		  						ACE_RTMT_COMBO_HBUF);
	if(nResult)
	{
		printf("Configure ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	/* Install RT Host Buffer */
	nResult = aceRTInstallHBuf(DevNum,(512*ACE_MSGSIZE_RT)*4 + 3);
	if(nResult)
	{
		printf("RT HBuf ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	/* Install RTMT Host Buffer */
	aceMTGetInfo(DevNum,&sMTInfo);

	/* Calc HBuf sizes */
	dwRTHBufSize     = (U32BIT)((2048/4)*ACE_MSGSIZE_RTMT);
	dwMTHBufSize     = (U32BIT)((4096/4)*ACE_MSGSIZE_RTMT);

	dwRTMTHBufSize   = (U32BIT)((dwRTHBufSize+dwMTHBufSize)*20);
		
	nResult = aceRTMTInstallHBuf(DevNum,dwRTMTHBufSize);
	if(nResult)
	{
		printf("RTMT HBuf ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	/* Select RT Address */
	printf("\nSelect RT Address (0-30):\n");
	printf("> ");
	scanf("\n%hd", &wRTAddress);
	getchar();

	/* Set RT Address */
	aceRTSetAddress(DevNum,wRTAddress);

	/* Create data block for RT use */
	aceRTDataBlkCreate(DevNum,1,ACE_RT_DBLK_SINGLE,NULL,0);

	/* Select RT Sub-Address */
	printf("\nSelect Destination Sub-Address (1-30):\n");
	printf("> ");
	scanf("\n%hd", &wRTSubAddress);
	getchar();
	
	/* Map data block to given sub-address */	
	aceRTDataBlkMapToSA(DevNum,
					    1,
						wRTSubAddress,
						ACE_RT_MSGTYPE_RX | ACE_RT_MSGTYPE_TX,
						0,
						TRUE);
	
	/* Check for RT/MT messages */
	GetRTMTHBufDecodedMsgs(DevNum,wRTAddress,wRTSubAddress);
	
	/* Free all resources */
	nResult = aceFree(DevNum);
	
	if(nResult)
	{
		printf("Free ");
		PrintOutError(nResult);
		PressAKey();
		return;
	}

	/* Pause before program exit */
	PressAKey();
	return;
}
