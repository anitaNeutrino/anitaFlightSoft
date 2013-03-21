/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE 'C' Run Time Library
 *
 *			Copyright (c) 1999 by Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	Testop.h (RTL test operations header file)
 *
 *
 *
 *	Created 8/26/99 DL
 *
 * 09/23/10 - With the permission of Data Device Corporation, this software
 * has been modified by Acromag, Inc. for use with Ip57x and Ios57x modules.
 */


#ifndef __TESTOP_H__
#define __TESTOP_H__

/*-------------------------------------------------------------
Struct:	TESTOPERATION

Description:
	This structure completely describes Test mode of operation.
---------------------------------------------------------------*/
typedef struct TESTOPERATION
{
	U32BIT  dwOptions;		/* Test Options */	

} TESTOPERATION;

/* Test result structure */
typedef struct TESTRESULT
{
	U16BIT wResult;		/* pass or fail code     */
	U16BIT wExpData;	/* expected data on fail */
	U16BIT wActData;	/* actual data on fail   */
	U16BIT wAceAddr;	/* address of failure    */
	U16BIT wCount;		/* test count index      */

} TESTRESULT;

/* All possible result codes returned in test structure */
#define ACE_TEST_PASSED 			0x00
#define ACE_TEST_NOT_PERFORMED		0x01
#define ACE_TEST_FAILED_RAM 		0x02
#define ACE_TEST_FAILED_REGISTER	0x03
#define ACE_TEST_FAILED_PROTOCOL	0x04
#define ACE_TEST_FAILED_INTERRUPT	0x05
#define ACE_TEST_FAILED_MVECTOR	 	0x06
#define ACE_TEST_FAILED_RVECTOR 	0x07
#define ACE_TEST_FAILED_NOTOPEN 	0x08
#define ACE_TEST_FAILED_LOOPBACK    0x0A			/* FJM 04/19/10 */

_EXTERN S16BIT _DECL aceTestConfigure(S16BIT DevNum,U32BIT dwOptions);

_EXTERN S16BIT _DECL aceTestRegisters(S16BIT DevNum,TESTRESULT *pTest);

_EXTERN S16BIT _DECL aceTestMemory(S16BIT DevNum,TESTRESULT *pTest,
								   U16BIT wValue);

_EXTERN S16BIT _DECL aceTestProtocol(S16BIT DevNum,TESTRESULT *pTest);

_EXTERN S16BIT _DECL aceTestIrqs(S16BIT DevNum,TESTRESULT *pTest);

_EXTERN S16BIT _DECL aceTestVectors(S16BIT DevNum,TESTRESULT *pTest, char *pMem);/* FJM 04/19/10 */

_EXTERN S16BIT _DECL aceTestLoopBack(S16BIT DevNum, TESTRESULT *pTest); /* FJM 04/19/10 */
#endif
