
/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE Software Toolbox
 *
 *			Copyright (c) 2007 Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	RTdBuf.c (Console App RT (D-Buffer) Demo)
 *  ----------------------------------------------------------------------
 *  This application runs the Enhanced Mini-ACE in RT mode.  Using two
 *  double-buffered data blocks, the RT sub-addresses are filled with
 *  user-selected data.  This will aid in the avoiding of lost messages.
 *  ----------------------------------------------------------------------
 *
 *	Created  8/26/99 DL
 *	Modified 8/26/00 MR
 *
 *  Function used in this example:
 *
 *	- aceInitialize
 *	- aceFree
 *	- aceGetLibVersion
 *	    - aceErrorStr
 *	- aceGetMsgTypeString
 *	- aceCmdWordParse
 *	- aceGetBSWErrString
 *	- aceRTDataBlkRead
 *	- aceRTDataBlkWrite
 *	- aceRTStart
 *	- aceRTStop
 *	- aceRTSetAddress
 *	- aceRTDataBlkCreate
 *	- aceRTDataBlkMapToSA
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


#define HANDLE	U32BIT*


#define DATA_BLOCK_ID_TX	1
#define DATA_BLOCK_ID_RX	2

/*-----------------------------------------------------------------------
Name:	       PressAKey()

Description:   Allows application to pause to allow screen contents to be read.

In	       none
Out	       none
------------------------------------------------------------------------*/
static void PressAKey(void)
{
   printf("\nPress <ENTER> to continue...");
   getchar();
}

/*-----------------------------------------------------------------------
Name:	       Sleep()

Description:   Sleeps for 'dws' seconds.

In	       dws' seconds
Out	       none
------------------------------------------------------------------------*/
static void Sleep(unsigned long int dws)
{
	sleep((unsigned int) dws);
}
/*-----------------------------------------------------------------------
Name:	       kbhit()

Description:   Check for a keyboard hit.

In	       none
Out	       none
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
	U16BIT	   wLibVer;

	wLibVer = aceGetLibVersion();
	printf("*********************************************\n");
	printf("Enhanced Mini-ACE Integrated 1553 Terminal  *\n");
	printf("BU-69090 1553 Runtime Library		    *\n");
	printf("Release Rev %d.%d.%d			       *\n",
		    (wLibVer>>8),			/* Major */
			(wLibVer&0xF0)>>4,		/* Minor */
			(wLibVer&0xF));			/* Devel */
	if ((wLibVer&0xF)!=0)
	printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-*\n");
	printf("Copyright (c) 2007 Data Device Corporation  *\n");
	printf("*********************************************\n");
	printf("RTDBuf.c RT operations [Double Bufferring]  *\n");
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
Name:	DisplayRTDataBlock

Description:
	This function prints a data block to the screen.

    NOTE: added TT664 changes

In		nResult = The error number.
Out		none
------------------------------------------------------------------------*/
static void DisplayRTDataBlock(S16BIT DevNum,S16BIT nDataBlkID,HANDLE hConsole)
{
	/* Variables */
	U16BIT wBuffer[32];
	U16BIT i, j;

	/* Init buffer */
	for(i=0;i<32;i++)
	{
	  wBuffer[i]=3;
	}

	/* Read data into local buffer */
	aceRTDataBlkRead(DevNum,nDataBlkID,wBuffer,32,0);

    /* Print Labels */
	if (nDataBlkID == DATA_BLOCK_ID_RX)
    {
	printf("\nRX:");
    }
	else
    {
	printf("\nTX:");
    }

	/* Display data to screen */
	for(j=0;j<4;j++)
	{
	  if(j)
	printf("   ");
		for(i=0;i<8;i++)
		{
			printf("%04X  ",wBuffer[i+(j*8)]);
		}
		printf("\n");
	}
}

/*-----------------------------------------------------------------------
Name:	TestRTDoubleBuffer

Description:
	This function creates an RT stack file in ASCII text using all msgs
	read from the ACE.

In		DevNum = the device number of the hardware to be accessed.
In		pFileName = pointer to a null terminated string containing file
				name.
Out		none
------------------------------------------------------------------------*/
static void TestRTDoubleBuffer(S16BIT DevNum,HANDLE hConsole)
{
	/* Variables */
	U16BIT keyin       =  0x0000;
	S16BIT  i	    =  0x0000;
	U16BIT  wBuffer[32];

	/* Init buffer */
	for(i=0;i<32;i++)
	{
	  wBuffer[i]=1;
	}

	/* Set RT address to 5 */
	aceRTSetAddress(DevNum,5);

	/* Create a Tx Double Buffer data block and map to SA 1 */
	aceRTDataBlkCreate(DevNum,DATA_BLOCK_ID_TX,ACE_RT_DBLK_DOUBLE,wBuffer,32);
	aceRTDataBlkMapToSA(DevNum,DATA_BLOCK_ID_TX,1,ACE_RT_MSGTYPE_TX,0,TRUE);

	/* Create a Rx Double Buffer data block and map to SA 2 */
	aceRTDataBlkCreate(DevNum,DATA_BLOCK_ID_RX,ACE_RT_DBLK_DOUBLE,NULL,0);
	aceRTDataBlkMapToSA(DevNum,DATA_BLOCK_ID_RX,2,ACE_RT_MSGTYPE_RX,0,TRUE);

	/* Start RT */
	aceRTStart(DevNum);

	do{
		while(!kbhit())
		{
			/* Display Data in RX Data Block */
			DisplayRTDataBlock(DevNum,DATA_BLOCK_ID_RX,hConsole);
			DisplayRTDataBlock(DevNum,DATA_BLOCK_ID_TX,hConsole);
			printf("Enter a number to fill the Tx Buffer with (0 to exit).\n");
			printf("Current Number: %d\n",wBuffer[0]);
			Sleep(1);
		}

	    /* Get Key Pressed */
	    scanf("%hd", &keyin);

	    /*check keyboad data*/
	    if((keyin>0)&&(keyin<=9))
		{
			/* Fill buffer with KeyPress */
			for(i=0;i<32;i++)
			{
				wBuffer[i]=(U16BIT)keyin;
			}

			/* Write Data to TX Address */
			aceRTDataBlkWrite(DevNum,DATA_BLOCK_ID_TX,wBuffer,32,0);
		}

	}while(keyin!=0);

	printf("\n\n");
	PressAKey();

	/* Stop RT */
	aceRTStop(DevNum);
}

/*-----------------------------------------------------------------------
Name:	RtdBuf

Description:
	Program Entry Point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In		hDevice     -> Handle to an open carrier
In		slot	    -> Module Slot letter
In		chan	    -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void RtdBuf(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)

{
	/* Variable Definition */
	S16BIT	   DevNum	 = 0x0000;
	S16BIT	   wResult	 = 0x0000;
	HANDLE	   hConsole	 =  NULL;

	/* Print out sample header */
	PrintHeader();

	/* Get Logical Device # */
	printf("Select RT Logical Device Number (0-31):\n");
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

	/* Test RT D-Buffering */
	TestRTDoubleBuffer(DevNum,hConsole);

	/* Free all resources */
	wResult = aceFree(DevNum);
	if(wResult)
	{
		printf("Free ");
		PrintOutError(wResult);
		return;
	}
	return;
}
