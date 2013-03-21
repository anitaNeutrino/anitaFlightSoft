
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
 *   BcdBuf.c (Console App BC (D-Buffer) Demo)
 *  ----------------------------------------------------------------------
 *   This application runs the Enhanced-Mini-ACE in BC mode.  It shows how
 *   using a double buffer for data can eliminate data loss.  The data is
 *   filled and read back dynamically allowing the user to manipulate the
 *   data.
 *  ----------------------------------------------------------------------
 *
 *   Created  8/26/99 DL
 *   Modified 8/26/00 MR
 *   Modified 4/13/04 RR
 *
 *   Function used in this example:
 *
 *      - aceInitialize
 *      - aceFree
 *      - aceGetLibVersion
 *      - aceErrorStr
 *      - aceGetMsgTypeString
 *      - aceCmdWordParse
 *      - aceGetBSWErrString
 *      - aceBCDataBlkRead
 *      - aceBCDataBlkWrite
 *      - aceBCGetHBufMsgDecoded
 *      - aceBCDataBlkCreate
 *      - aceBCMsgCreateBCtoRT
 *      - aceBCMsgCreateRTtoBC
 *      - aceBCMsgCreateRTtoRT
 *      - aceBCOpCodeCreate
 *      - aceBCFrameCreate
 *      - aceBCInstallHBuf
 *      - aceBCSetMsgRetry
 *      - aceBCCreateImageFiles
 *      - aceBCStart
 *      - aceBCStop
 *
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
 */


/* include files */
#include "libPrj/include/stdemace.h"
#include <stdio.h>
#include <sys/time.h> /* time */
#include <sys/select.h> /* select() */
#include <unistd.h>	/* sleep() */

#define HANDLE	U32BIT* 

/* define data blocks */
#define DBLK1  1

/* define message constants */
#define MSG1   1

/* define opcodes */
#define OP1    1
#define OP2    2

/* define frame constants */
#define MNR1   1
#define MJR    3

/* TT664 change */
static U16BIT wDataBlkLoc        = 0x0000;
static U16BIT wBlockData[2][32]  = {{0x0000, 0x0000},};


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
   printf("Copyright (c) 2007 Data Device Corporation  *\n");
   printf("*********************************************\n");
   printf("BcdBuf.c BC operations [Double Buffer Demo] *\n");
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
Name:          DisplayBCDataBlock

Description:   This function prints a data block to the screen.

               NOTE: added TT664 changes

In             nResult = The error number.
Out            none
------------------------------------------------------------------------*/
static void DisplayBCDataBlock(S16BIT DevNum,S16BIT nDataBlkID,HANDLE hConsole)
{
   /* Variables */
   U16BIT wBuffer[32]  = {0x0000};
   U16BIT i            =  0x0000;
   U16BIT j            =  0x0000;
   U16BIT loc          =  0x0000;
   U16BIT wDataCount;
   BOOL   bFound;

   /* Read data into local buffer */
   wDataCount = aceBCDataBlkRead(DevNum,nDataBlkID,wBuffer,32,0);

   /* search if we already have the data */
   bFound=FALSE;

   for (i=0; i<2; i++)
   {
      if (wBuffer[0]==wBlockData[i][0])
      {
         memcpy(wBlockData[i], wBuffer, 32*sizeof(U16BIT));
         bFound=TRUE;
         break;
      }
   }

   /* save it if we do not have the data yet */
   if (!bFound)
   {
      wDataBlkLoc++;
      wDataBlkLoc %=2;
      memcpy(wBlockData[wDataBlkLoc], wBuffer, 32*sizeof(U16BIT));
   }

   /* Display all data to screen */
   for (loc=0; loc<2; loc++)
   {
      printf("DB%d:", loc);
      for(j=0;j<4;j++)
      {
     	 if(j)
     		 printf("    ");
         for(i=0;i<8;i++)
         {
       		 printf("%04X  ",wBlockData[loc][i+(j*8)]);
         }
         printf("\n");
      }
      printf("\n");
   }
}

/*-----------------------------------------------------------------------
Name:          TestBCDoubleBuffer

Description:   This function test the BC double buffering feature.

In             DevNum    = The device number of the hardware to be accessed.
In             pFileName = Pointer to a null terminated string containing file
               name.
Out            none
------------------------------------------------------------------------*/
static void TestBCDoubleBuffer(S16BIT DevNum,HANDLE hConsole)
{
   /* Variables */
   U16BIT wBuffer[32] = {0x0000};
   U16BIT i           =  0x0000;
   U16BIT keyin       =  0x0000;

   /* Initialize Buffer */
   for(i=0;i<32;i++)
   {
      wBuffer[i]=1;
   }

   /* Start BC */
   aceBCStart(DevNum,MJR,-1);

   do{
      while(!kbhit())
      {
         /* Display current data */
         DisplayBCDataBlock(DevNum,DBLK1,hConsole);
         printf("Enter a number to fill the Rx buffer with (0 to exit).\n");
         printf("Current Number: %d\n",wBuffer[0]);
         Sleep(1);
      }

      /* Get Key Pressed */
      scanf("%hd", &keyin);
      
      /*check keyboad data*/
      if((keyin>0)&&(keyin<=9))
      {
         for(i=0;i<32;i++)
         {
            /* Fill buffer with key pressed */
            wBuffer[i]=(U16BIT)keyin;
         }

         /* Write modified buffer to data block */
         aceBCDataBlkWrite(DevNum,DBLK1,wBuffer,32,0);
      }

   }while(keyin!=0);

   printf("\n\n");

   /* Stop BC */
   aceBCStop(DevNum);
}

/*-----------------------------------------------------------------------
Name:	BcdBuf

Description:
	Program entry point.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void BcdBuf(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)
{
   /* Variables */
   S16BIT    DevNum       =  0x0000;
   S16BIT    wResult      =  0x0000;
   S16BIT    aOpCodes[10] = {0x0000};
   HANDLE    hConsole     =  NULL;
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
   printf("Select BC Logical Device Number (0-31):\n");
   printf("> ");
   scanf("%hd",&DevNum);

   /* Initialize Device */
   wResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_BC, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
   if(wResult)
   {
      printf("Initialize ");
      PrintOutError(wResult);
      PressAKey();
      return;
   }

   /* Get Bus selection */
   printf("\nSelect 1 For Bus A or 2 for Bus B\n");
   printf("> ");

   scanf("%ld", &u32BusSelection);
   
   if (u32BusSelection == 1) u32BusSelection = ACE_BCCTRL_CHL_A;
   else u32BusSelection = ACE_BCCTRL_CHL_B;
   
   /* Create double buffer data block */
   aceBCDataBlkCreate(DevNum,DBLK1,ACE_BC_DBLK_DOUBLE,wBuffer,32);

   /* Create message block using ACE_MSGOPT_DOUBLE_BUFFER option */
   aceBCMsgCreateBCtoRT(
      DevNum,  /* Device number                    */
      MSG1,    /* Message ID to create             */
      DBLK1,   /* Message will use this data block */
      5,       /* RT address                       */
      1,       /* RT subaddress                    */
      10,      /* Word count                       */
      0,       /* Default message timer            */
      u32BusSelection|ACE_MSGOPT_DOUBLE_BUFFER);  /* chl, msg double buf options */

   /* Create XQF opcode that will use msg block */
   aceBCOpCodeCreate(DevNum,OP1,ACE_OPCODE_XQF,ACE_CNDTST_ALWAYS,MSG1,0,0);

   /* Create CAL opcode that will call mnr frame from major */
   aceBCOpCodeCreate(DevNum,OP2,ACE_OPCODE_CAL,ACE_CNDTST_ALWAYS,MNR1,0,0);

   /* Create Minor Frame */
   aOpCodes[0] = OP1;
   aceBCFrameCreate(DevNum,MNR1,ACE_FRAME_MINOR,aOpCodes,1,0,0);

   /* Create Major Frame */
   aOpCodes[0] = OP2;
   aceBCFrameCreate(DevNum,MJR,ACE_FRAME_MAJOR,aOpCodes,1,10000,0);

   /* Install host buffer */
   aceBCInstallHBuf(DevNum,8*1024);

   /* Set Msg error retry scheme */
   aceBCSetMsgRetry(DevNum,ACE_RETRY_TWICE,ACE_RETRY_SAME,ACE_RETRY_ALT,0);

   /* create BC image file for export */
   /* aceBCCreateImageFiles(DevNum,MJR,"bcimage.bin","bcimage.h"); */

   wDataBlkLoc = 0x0000;

   /* Test Double Buffering */
   TestBCDoubleBuffer(DevNum,hConsole);

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
}
