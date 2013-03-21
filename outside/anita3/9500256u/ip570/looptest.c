
/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE 'C' Run Time Software Library
 *
 *			Copyright (c) 2007 by Data Device Corporation
 *			All Rights Reserved.
 *
 *	Created 7/28/04 RR
 *	Updated 8/03/05 ESK Updated for Integrity v4 and v5
 *
 *	looptest.c (ENHANCED MINI-ACE RTL Loop Back program)
 *
 *           Performs a loop around selftest of an Enhanced Mini-ACE using
 *           internal test mode registers. The test transmits a word on one
 *           channel and receives the transmitted word on the other channel.
 *           This test requires that a loop back cable be used which will
 *           connect channel A to channel B with the appropriate termination.
 *
 * Summary of Operation
 *           This test makes use of the Enhanced Mini-ACE's internal test mode
 *           registers. The encoder is controlled through the use
 *           of test register #1. The bit map for the purpose of
 *           this test are as follows:
 *
 *              msb bit 15: Encoder Channel A/B*
 *                      14: set to 0
 *                      13: Transmitter Enable
 *                      12: Encoder Enable
 *                      11: Command/Data* sync
 *                      10: set to 0
 *                       9: set to 0
 *                       8: set to 0
 *                       7: set to 0
 *                       6: set to 0
 *                       5: set to 1
 *                       4: set to 0
 *                       3: set to 0
 *                       2: set to 0
 *                       1: set to 0
 *               lsb bit 0: set to 0
 *
 *           The encoder is activated by asserting a Logic 1 to both
 *           encoder enable and transmitter enable. Once the device
 *           begins transmitting, encoder enable may be removed and
 *           the transmission will stop at the completion of the current
 *           word. Removing the transmitter enable signal once a
 *           transmission has begun will truncate the word immdediately.
 *           Bit 15 should be set to logic 1 to transmit on channel A or
 *           logic 0 to transmit on channel B. Bit 11 should be set to
 *           logic 1 to produce a command sync or a logic 0 to produce a
 *           data sync on the transmitted word. The transmitted word will
 *           be fetched from the BC Frame Time/RT Last Command/MT Trigger
 *           Word registers. Any 16 bit value may be placed into this
 *           register.
 *
 *           Once the transmission is started (by asserting transmitter
 *           enabled and encoder enable, encoder enable is removed
 *           immediately. This will ensure that only one word is transmitted
 *           (assuming that there is less than 20 usec between successive
 *           write transfers). Next the program waits for the word to be
 *           received by the monitor on the opposite channel or for a
 *           software timeout to occur. Note that the monitor will not
 *           store the word on the channel on which it is transmitting.
 *           The program polls the tag word of the first monitor stack
 *           entry. When a word is stored the monitor tag word will contain
 *           a non-zero value. The timeout count of 100 is chosen to
 *           guarantee at least a 20 usec delay to allow the word to be
 *           transfered (ACE access time is 200 ns * 100 transfers = 20 usec).
 *
 *           Once the word is detected by the monitor, then the tag word
 *           for the stored word is verified (ignoring the gap time) and
 *           the actual word received is verified.
 *
 *           This test is repeated for both channels transmitting.
 *           
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
 *
 * Field I/O Connections:
 *
 * CH0A_N is connected to CH0B_N and CH0A_P is connected to CH0B_P
 *
 * CH0_RT_PARITY is connected to GND.
 *
 * CH1A_N is connected to CH1B_N and CH1A_P is connected to CH1B_P
 *
 * CH1_RT_PARITY is connected to GND.
 */


#include "libPrj/include/stdemace.h"



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
	printf("***********************************************\n");
	printf("Enhanced Mini-ACE Integrated 1553 Terminal  *\n");
	printf("BU-69090 Runtime Library                    *\n");
	printf("Release Rev %d.%d.%d			    *\n",
		    (wLibVer>>8),			/* Major */
			(wLibVer&0xF0)>>4,		/* Minor */
			(wLibVer&0xF));			/* Devel */
	if ((wLibVer&0xF)!=0)
	printf("=-=-=-=-=-=-=-INTERIM VERSION-=-=-=-=-=-=-=-=-*\n");
	printf("Copyright (c) 2007 Data Device Corporation    *\n");
	printf("***********************************************\n");
	printf("LoopTest.c BC Loopback Test                   *\n");
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
Function Name:  looptest

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

int looptest(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, int hDevice, int slot, int chan)

{
	/* Variable Definition */
	S16BIT		DevNum      = 0x0000;
    S16BIT      nResult = 0;
    TESTRESULT  sTest;

	/* Print out sample header */
	PrintHeader();

    /*--- Get Logical Device # ---*/
	printf("\nSelect Looptest Logical Device Number (0-31):\n");
	printf("> ");

	scanf("%hd", &DevNum);

    printf("\n");

    /*--- Initialize Device as Test Mode ---*/
	nResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_TEST, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
	if(nResult)
	{
		printf("Initialize ");
		PrintOutError(nResult);
        PressAKey();
		return 0;
	}

    /*--- test ---*/
    nResult = aceTestLoopBack(DevNum, &sTest);

    /* Capture error */
	if(nResult)
	{
        aceFree(DevNum);
        printf("Loopback Test  ");
		PrintOutError(nResult);
        PressAKey();
		return 0;
	}

    /*--- check test result ---*/
    if(sTest.wResult == ACE_TEST_PASSED)
    {
        printf("Loopback Test Passed.\n");
    }
    else
    {
        printf("Loopback Test Failed, %d fails, expected data = %04x, actual data = %04x\n",
                                (int)sTest.wCount, (unsigned int)sTest.wExpData,(unsigned int)sTest.wActData);
    }

    /*--- Free ---*/
  nResult = aceFree(DevNum);
  if(nResult)
  {
	printf("Free ");
	PrintOutError(nResult);
      PressAKey();
	return 0;
  }

  /* Pause before program exit */
  PressAKey();

  return 0;
}
