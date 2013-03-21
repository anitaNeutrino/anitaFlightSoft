
/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE Software Toolbox
 *
 *			Copyright (c) 2000 Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	RTMTDemo.c (Console App RT/Monitor Demo)
 *  ----------------------------------------------------------------------
 *	This application runs the Enhanced-Mini-ACE in RTMT mode.  The chip
 *  is configured to monitor all messages, plus act as an RT by the user
 *  specified settings.  All MT and RT operations are valid in this mode.
 *  ----------------------------------------------------------------------
 *
 *	Created 8/26/99 DL
 *	Created 8/26/00 MR
 *	Updated 3/23/01 KWH
 *
 *  Function used in this example:
 *
 *      - aceInitialize
 *      - aceFree
 *      - aceGetLibVersion
 *	    - aceErrorStr
 *      - aceGetMsgTypeString
 *      - aceCmdWordParse
 *      - aceGetBSWErrString
 *   	- aceRTMTStart
 *   	- aceRTMTStop
 *      - aceRTSetAddress
 *      - aceRTDataBlkCreate
 *      - aceRTDataBlkMapToSA
 *      - aceMTGetStkMsgDecoded
 *      - aceRTGetStkMsgDecoded
 *      
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
	printf("RTMTDemo.c RT/MT operations Demo            *\n");
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
void PrintOutError(S16BIT nResult)
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
void DisplayDecodedMsg(S16BIT nMsgNum,MSGSTRUCT *pMsg)
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
Name:	GetRTMTStkDecodedMsgs

Description:
	This function gets the set RT messages or global MT messages.

In		DevNum = The logical device number of the MT device.
Out		none
------------------------------------------------------------------------*/
void GetRTMTStkDecodedMsgs(S16BIT DevNum,
						   S16BIT wRTAddr,
						   S16BIT wRTSubAddr)
{
	S16BIT    nResult = 0x0000;
	S16BIT    nMsgNum = 0x0000;
	MSGSTRUCT sMsg;
	
	/* Start RT/MT Operations */
	aceRTMTStart(DevNum);

	printf("\n** Polling Device # %d as an MT and RT Addr#%d/SA#",
		   DevNum,wRTAddr);
	printf("%d, Press <ENTER> to stop **\n\n",wRTSubAddr);

	/* Poll Msgs */
	while(!kbhit())
	{
		/* Get MT Messages */
		nResult = aceMTGetStkMsgDecoded(DevNum,&sMsg,
										ACE_MT_MSGLOC_NEXT_PURGE,
										ACE_MT_STKLOC_ACTIVE);
		/* Msg found */
		if(nResult==1)
		{
			++nMsgNum;
			printf("-MT Message-\n");
			DisplayDecodedMsg(nMsgNum,&sMsg);
		}

		/* Reset error */
		nResult = 0;

		/* Check for RT messages */
		nResult = aceRTGetStkMsgDecoded(DevNum,&sMsg,
										ACE_RT_MSGLOC_NEXT_PURGE);
		/* Msg found */
		if(nResult==1)
		{
			++nMsgNum;
			printf("-RT Message-\n");
			DisplayDecodedMsg(nMsgNum,&sMsg);
		}
	}

	/* Throw away key */
    getchar();

	/* Stop RT/MT Operations */
	aceRTMTStop(DevNum);
}

/*-----------------------------------------------------------------------
Function Name:  RtMtDemo

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

void RtMtDemo(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)
{
	S16BIT DevNum        = 0x0000;
	S16BIT nResult       = 0x0000;
	S16BIT wRTSubAddress = 0x0000;
	S16BIT wRTAddress    = 0x0000;

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
	GetRTMTStkDecodedMsgs(DevNum,wRTAddress,wRTSubAddress);
	
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
