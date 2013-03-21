
/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE 'C' Run Time Software Library
 *
 *			Copyright (c) 2000 by Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	acetest.c (ENHANCED MINI-ACE Tester program)
 *
 *
 *
 *	Created 05/2/00 DL
 *
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
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
	printf("aceTest.c Test ACE operations Demo          *\n");
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
Name:	RegTest

Description:
	Tests register operation on an Enhanced Mini-ACE.

In		DevNum = ID associated with the hardware being accessed
In		sTest = TestResult structure.
Out		none
------------------------------------------------------------------------*/
void RegTest(S16BIT DevNum,TESTRESULT *pTest)
{
	S16BIT nResult;

	printf("Testing...");
	nResult = aceTestRegisters(DevNum,pTest);
	if(nResult)
	{
		PrintOutError(nResult);
		return;
	}

	if(pTest->wResult == ACE_TEST_PASSED)
	{
		printf("Registers Passed test.\n");
	}else
	{
		printf("Register Failed test, expected=%04x read=%04x!!!\n",
			pTest->wExpData,pTest->wActData);
/*		exit(-1);*//* FJM 04/19/10 */
	}
}

/*-----------------------------------------------------------------------
Name:	RamTest

Description:
	Tests hardware memory on an Enhanced Mini-ACE.

In		DevNum = ID associated with the hardware being accessed
In		sTest = TestResult structure.
In		wValue = the value to written and verified.
Out		none
------------------------------------------------------------------------*/
void RamTest(S16BIT DevNum,TESTRESULT *pTest,U16BIT wValue)
{
	S16BIT nResult;

	printf("Testing...");
	nResult = aceTestMemory(DevNum,pTest,wValue);
	if(nResult)
	{
		PrintOutError(nResult);
		return;
	}

	if(pTest->wResult == ACE_TEST_PASSED)
	{
		printf("Ram Passed %04x test.\n",wValue);
	}else
	{
		printf("Ram Failed %04x test, data read = %04x addr = %d!!!\n",
			wValue,pTest->wActData,pTest->wAceAddr);

	}
}

/*-----------------------------------------------------------------------
Name:	InterruptTest

Description:
	Tests hardware interrupts on an Enhanced Mini-ACE.

In		DevNum = ID associated with the hardware being accessed
In		sTest = TestResult structure.
Out		none
------------------------------------------------------------------------*/
void InterruptTest(S16BIT DevNum,TESTRESULT *pTest)
{
	printf("Testing...");

	aceTestIrqs(DevNum,pTest);
	if(pTest->wResult == ACE_TEST_PASSED)
	{
		printf("Interrupt Occurred, Passed test.\n");
	}else
	{
		printf("Interrupt Test Failure, %s %s!!!\n",
			(pTest->wCount&1)?"NO TimeTag Rollover":"",
			(pTest->wCount&2)?"NO IRQ":"");
	}

}

/*-----------------------------------------------------------------------
Name:	TestProtocol

Description:
	Tests hardware protocol on an Enhanced Mini-ACE.

In		DevNum  = ID associated with the hardware being accessed (0-31).
In		sTest   = TestResult structure.
Out		none
------------------------------------------------------------------------*/
void TestProtocol(S16BIT DevNum,TESTRESULT *pTest)
{
	
	S16BIT nResult = 0x0000;

	printf("Testing...");

	/* Test card Protocol */
	nResult = aceTestProtocol(DevNum,pTest);
	
	/* Capture error */
	if(nResult)
	{
		PrintOutError(nResult);
		return;
	}
	
	/* Display test results */
	if(pTest->wResult == ACE_TEST_PASSED)
	{
		printf("Protocol Unit Passed test.\n");
	}else
	{
		printf("Protocol Unit Failed test, expected=%04x read=%04x addr=%04x!!!\n",
			    pTest->wExpData,pTest->wActData,pTest->wAceAddr);
	}
}


/*-----------------------------------------------------------------------
Name:	TestVectors

Description:
	Tests the Enhanced Mini-ACE using a Test Vector File called Testvec.h.

In		DevNum  = ID associated with the hardware being accessed (0-31).
In		pTest   = TestResult structure.
In      pMem    = Address of memory resident test vector data. 	
Out		none
------------------------------------------------------------------------*/
void TestVectors(S16BIT DevNum,TESTRESULT *pTest, char *pMem)
{
	S16BIT nResult = 0x0000;

	printf("Testing...");
	
	/* Test card vectors  NOTE: Must have file 'Testvec.h' */
	nResult = aceTestVectors(DevNum,pTest,pMem);
	
	/* Capture error */
	if(nResult)
	{
		PrintOutError(nResult);
		return;
	}

	/* Display test results */
	if(pTest->wResult == ACE_TEST_PASSED)
	{
		printf("Vectors Passed, EOF at line #%d.\n",pTest->wCount);
	}else
	{
		printf("Vectors Failed!\n");
		printf("Failure...at line #%d\n",pTest->wCount);
		printf("          location=%s\n",
  			  (pTest->wResult==ACE_TEST_FAILED_MVECTOR)?"memory":"register");
		printf("          address=%04x\n",pTest->wAceAddr);
		printf("          expected=%04x\n",pTest->wExpData);
		printf("          actual=%04x\n",pTest->wActData);
	}
}


/*-----------------------------------------------------------------------
Name:	aceTest

Description:
	This program tests an Enhanced Mini-ACE.

In		wMemWrdSize -> Amount of addressable memory in words
In		wRegAddr    -> The location where ACE registers start
In		wMemAddr    -> The location where ACE memory starts
In		pMemTestVec -> Address of memory resident test vector data
In              hDevice     -> Handle to an open carrier
In              slot        -> Module Slot letter
In              chan        -> Mini-ACE Channel number
Out		none
------------------------------------------------------------------------*/

void aceTest(unsigned long dwMemWrdSize, unsigned long dwRegAddr, unsigned long dwMemAddr, char *pMemTestVec, int hDevice, int slot, int chan)
{
	S16BIT nResult;
	TESTRESULT sTest;
	S16BIT DevNum;

	/* Print out sample header */
	PrintHeader();
	
	/* Get Logical Device # */
	printf("\nSelect aceTest Logical Device Number (0-31):\n");
	printf("> ");
	scanf("\n%hd", &DevNum);
	getchar();
	
	printf("\n****Testing: Device Number %d****\n",DevNum);

    nResult = aceInitialize(DevNum,ACE_ACCESS_CARD,ACE_MODE_TEST, dwMemWrdSize, dwRegAddr, dwMemAddr, hDevice, slot, chan);
	if(nResult)
	{
		printf("Initialize ");
		PrintOutError(nResult);
		return;
	}

 	RegTest(DevNum,&sTest);
 	RamTest(DevNum,&sTest,0xaaaa);
 	RamTest(DevNum,&sTest,0xaa55);
 	RamTest(DevNum,&sTest,0x55aa);
 	RamTest(DevNum,&sTest,0x5555);
	RamTest(DevNum,&sTest,0xffff);
	RamTest(DevNum,&sTest,0x1111);
	RamTest(DevNum,&sTest,0x8888);
	RamTest(DevNum,&sTest,0x0000);
	InterruptTest(DevNum,&sTest);
	TestProtocol(DevNum,&sTest);
	TestVectors(DevNum,&sTest,pMemTestVec);
	
	nResult = aceFree(DevNum);
	if(nResult)
	{
		printf("Free ");
		PrintOutError(nResult);
		return;
	}
}
