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
 *	Config.h (RTL device configuration/initialization module)
 *
 *
 *
 *	Created 03/21/00 DL
 *
 *	Function List:
 *		aceGetLibVersion
 *		aceGetMemAndRegInfo
 *		aceFree
 *		aceInitialize
 *		aceISQRead
 *		aceMemRead
 *		aceMemWrite
 *		aceRegRead
 *		aceRegWrite
 *		aceSetAddressMode
 *		aceSetClockFreq
 *		aceSetDecoderConfig
 *		aceSetIrqConditions
 *		aceSetIrqConfig
 *		aceSetTimeTagRes
 *		aceSetTimeTagValue
 *		aceGetTimeTagValue
 *		aceResetTimeTag
 *		aceSetRamParityChecking
 *		aceSetRespTimeOut
 *		aceSetWin32IrqConditions
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__


/* wAccess can be one of the following */
#define ACE_ACCESS_MAXNUM	3		/* 3 access types supported */
#define ACE_ACCESS_CARD		0		/* ACE card running on W32 OS */
#define ACE_ACCESS_SIM		1		/* simulated memory */
#define ACE_ACCESS_USR		2		/* user defined memory location */

/* wMode parameter can be one of the following */
#define ACE_MODE_MAXNUM		0x0005	/* 5 modes supported */
#define ACE_MODE_TEST		0x0000	/* self test mode */
#define ACE_MODE_BC			0x0001	/* 1553 bus controller mode */
#define ACE_MODE_RT			0x0002	/* 1553 remote terminal mode */
#define ACE_MODE_MT			0x0003	/* 1553 monitor terminal mode */
#define ACE_MODE_RTMT		0x0004	/* 1553 RT with monitoring */

/* the following values can be or'd in to the selected mode */
#define ACE_NO_TT_RESET		0x1000  /* or'd in value to not reset TT */
#define ACE_ADVANCED_MODE   0x2000  /* or'd in value to run in adv mode */

/* Software states that he hardware can be in */
#define ACE_STATE_RESET		0		/* Not initialized */
#define ACE_STATE_READY		1		/* Initialized in mode of operation*/
#define ACE_STATE_RUN		2		/* ACE mode is running */

/* The different ways to address hardware. A0-A0-> Incrementing the 
 * host address by 1 will access next word on the Mini-ACE. A1-A0-> 
 * Incrementing the host address by 2 will access the next word on
 * the Mini-ACE. A2-A0-> Incrementing the host address by 4 will 
 * access the next word on the Mini-ACE.
 */
#define ACE_ADDRMODE_A0_A0	0
#define ACE_ADDRMODE_A1_A0	1
#define ACE_ADDRMODE_A2_A0	2

/* wTTRes parameter can be one of the following */
#define ACE_TT_2US			0		/* 2us Time Tag resolution */
#define ACE_TT_4US			1		/* 4us Time Tag resolution */
#define ACE_TT_8US			2		/* 8us Time Tag resolution */
#define ACE_TT_16US			3		/* 16us Time Tag resolution*/
#define ACE_TT_32US			4		/* 32us Time Tag resolution*/
#define ACE_TT_64US			5		/* 64us Time Tag resolution*/
#define ACE_TT_TEST			6		/* Test Time Tag clock */
#define ACE_TT_EXT			7		/* Use external TT clock */

/* wRespTimeOut parameter can be one of the following */
#define ACE_RESPTIME_18US	0		/* 18.5us before reponse timout*/
#define ACE_RESPTIME_22US	1		/* 22.5us before reponse timout*/
#define ACE_RESPTIME_50US	2		/* 50.5us before reponse timout*/
#define ACE_RESPTIME_130US	3		/* 130us before reponse timout*/

/* wClockIn parameter can be one of the following */
#define ACE_CLOCK_16MHZ		0		/* 16MHz clock input */
#define ACE_CLOCK_12MHZ		1		/* 12MHz clock input */
#define ACE_CLOCK_20MHZ		2		/* 20MHz clock input */
#define ACE_CLOCK_10MHZ		3		/* 10MHz clock input */

/* wLvlOrPulse parameter can be one of the following */
#define ACE_IRQ_PULSE		0		/* Pulse interrupts */
#define ACE_IRQ_LEVEL		1		/* Level mode interrupts */

/* Interrupt status queue header can be one of the following */
#define ACE_ISQ_MSG_NONMSG			0x0001
/* Message based */
#define ACE_ISQ_TX_TIMEOUT			0x8000
#define ACE_ISQ_ILL_LKUP			0x4000
#define ACE_ISQ_MT_DSTK_50P_ROVR	0x2000
#define ACE_ISQ_MT_DSTK_ROVER		0x1000
#define ACE_ISQ_RT_DSTK_50P_ROVR	0x0800
#define ACE_ISQ_RT_DSTK_ROVER		0x0400
#define ACE_ISQ_MT_CSTK_50P_ROVR	0x0200
#define ACE_ISQ_MT_CSTK_ROVR		0x0100
#define ACE_ISQ_RT_CSTK_50P_ROVR	0x0080
#define ACE_ISQ_RT_CSTK_ROVR		0x0040
#define ACE_ISQ_HNDSHK_FAIL			0x0020
#define ACE_ISQ_FMT_ERR				0x0010
#define ACE_ISQ_MCODE				0x0008
#define ACE_ISQ_SA_EOM				0x0004
#define ACE_ISQ_EOM					0x0002
/* Non-Message Based */
#define ACE_ISQ_TIME_TAG			0x0010
#define ACE_ISQ_RT_ADDR_PAR			0x0008
#define ACE_ISQ_SELF_TEST			0x0004

/* A single entry from the RT/MT Interrupt Status Queue */
typedef struct ISQENTRY
{
	U16BIT wISQHeader;			/* gives information on what data is */
	U16BIT wISQData;			/* the information for the entry */
} ISQENTRY;

/*-----------------------------------------------------------------------
Struct:	HBUFMETRIC

Description:
	This structure defines valid performance information for a Host Buf
------------------------------------------------------------------------*/
typedef struct HBUFMETRIC
{
	U32BIT dwCount;		/* Number of Msgs in the host buffer */
	U32BIT dwLost;		/* Total number of msgs lost since install */
	U32BIT dwPctFull;	/* Current Percentage of HBuf used */
	U32BIT dwHighPct;	/* Highest Percentage of HBuf used */

} HBUFMETRIC;

/*-----------------------------------------------------------------------
Struct:	STKMETRIC

Description:
	This structure defines valid performance information for an RT CmdStk
------------------------------------------------------------------------*/
typedef struct STKMETRIC
{
	U32BIT dwLost;		/* Total number of msgs lost since RT start */
	U32BIT dwPctFull;	/* Current Percentage of Cmd Stk used */
	U32BIT dwHighPct;	/* Highest Percentage of Cmd Stk used */

} STKMETRIC;

/* dwIrqMask parameter can be the following OR'd together */
#define ACE_IMR1_EOM 				0x00000001
#define ACE_IMR1_BC_STATUS_SET		0x00000002
#define ACE_IMR1_RT_MODE_CODE		0x00000002
#define ACE_IMR1_MT_PATTERN_TRIG	0x00000002
#define ACE_IMR1_FORMAT_ERR 		0x00000004
#define ACE_IMR1_BC_END_OF_FRM		0x00000008
#define ACE_IMR1_BC_MSG_EOM 		0x00000010
#define ACE_IMR1_RT_SUBADDR_EOM 	0x00000010
#define ACE_IMR1_RT_CIRCBUF_ROVER	0x00000020
#define ACE_IMR1_TT_ROVER			0x00000040
#define ACE_IMR1_RT_ADDR_PAR_ERR	0x00000080
#define ACE_IMR1_BC_RETRY			0x00000100
#define ACE_IMR1_HSHAKE_FAIL		0x00000200
#define ACE_IMR1_MT_DATASTK_ROVER	0x00000400
#define ACE_IMR1_MT_CMDSTK_ROVER	0x00000800
#define ACE_IMR1_BCRT_CMDSTK_ROVER	0x00001000
#define ACE_IMR1_BCRT_TX_TIMEOUT	0x00002000
#define ACE_IMR1_RAM_PAR_ERR		0x00004000
#define ACE_IMR2_BIT_COMPLETE		0x00020000
#define ACE_IMR2_BC_UIRQ0			0x00040000
#define ACE_IMR2_BC_UIRQ1			0x00080000
#define ACE_IMR2_BC_UIRQ2			0x00100000
#define ACE_IMR2_BC_UIRQ3			0x00200000
#define ACE_IMR2_MT_DSTK_50P_ROVER	0x00400000
#define ACE_IMR2_MT_CSTK_50P_ROVER	0x00800000
#define ACE_IMR2_RT_CIRC_50P_ROVER	0x01000000
#define ACE_IMR2_RT_CSTK_50P_ROVER	0x02000000
#define ACE_IMR2_BC_TRAP			0x04000000
#define ACE_IMR2_BC_CALLSTK_ERR 	0x08000000
#define ACE_IMR2_GPQ_ISQ_ROVER		0x10000000
#define ACE_IMR2_RT_ILL_CMD 		0x20000000
#define ACE_IMR2_BC_OPCODE_PARITY	0x40000000

/* wAutoClear parameter can be one of the following */
#define ACE_IRQ_NO_AUTO_CLR 0		/* No auto clear irqs */
#define ACE_IRQ_AUTO_CLR	1		/* Auto clear irqs */

/* wDoubleOrSingle parameter can be one of the following */
#define ACE_SINGLE_ENDED	0		/* Decode singled ended input */
#define ACE_DOUBLE_ENDED	1		/* Decode double ended input */

/* wExpXingEnable parameter can be one of the following */
#define ACE_DISABLE_EXPANDED_XING 0	/* Sample on single clock edge */
#define ACE_ENABLE_EXPANDED_XING  1	/* Sample on both clock edges */

/* wRamParityEnable parameter can be one of the following */
#define ACE_DISABLE_PARITY_CHECK 0	/* Dont display failures in ISR */
#define ACE_ENABLE_PARITY_CHECK  1	/* Display failures in ISR */


/*---------------------- FUNCTION PROTOYPES -------------------------*/

/*-----------------------------------------------------------------------
Function:	aceGetLibVersion

Description:
	This function returns an unsigned word containing the version
	information of this library. The high byte contains the major version
	and the low byte contains the minor version to 2 decimal places. 
	(Example: 0x0101-> Version 1.01, 0x022C-> Version 2.44)

Inputs:
	----------------+--------------------------------------+--------------
	Parameter       | Description						   | Valid values
	----------------+--------------------------------------+--------------
	none            |                                      |
	----------------+--------------------------------------+--------------

Return = 16-Bits of version information
------------------------------------------------------------------------*/
_EXTERN U16BIT _DECL aceGetLibVersion(void);
_EXTERN U16BIT _DECL aceGetCoreVersion(void);

/*-----------------------------------------------------------------------
Function:	aceGetMemRegInfo

Description:
	This function returns an unsigned word containing the version
	information of this library. The high byte contains the major version
	and the low byte contains the minor version to 2 decimal places. 
	(Example: 0x0101-> Version 1.01, 0x022C-> Version 2.44)

Inputs:
	----------------+--------------------------------------+--------------
	Parameter       | Description						   | Valid values
	----------------+--------------------------------------+--------------
	DevNum          | # associated with hardware access    | 0..7
	----------------+--------------------------------------+--------------
	pRegAddr        | address of a U32BIT variable         | 0-4GB
	----------------+--------------------------------------+--------------
	pMemAddr        | address of a U32BIT variable         | 0-4GB
	----------------+--------------------------------------+--------------

Return:  error condition -> ACE_ERR_SUCCESS
------------------------------------------------------------------------*/
_EXTERN S16BIT _DECL aceGetMemRegInfo(	S16BIT DevNum,
						U32BIT* pRegAddr,
						U32BIT* pMemAddr);

_EXTERN S16BIT _DECL aceInitialize(S16BIT DevNum,
						U16BIT wAccess,
						U16BIT wMode,
						U32BIT dwMemWrdSize,
						U32BIT dwRegAddr,
						U32BIT dwMemAddr,
						int    hDevice,
                                                int slot,
                                                int chan);

_EXTERN S16BIT _DECL aceFree(S16BIT DevNum);

_EXTERN U16BIT _DECL aceRegRead(S16BIT DevNum,U16BIT wRegister);
_EXTERN S16BIT _DECL aceRegWrite(S16BIT DevNum,U16BIT wRegAddr,
								 U16BIT wValue);
_EXTERN U16BIT _DECL aceMemRead(S16BIT DevNum,U16BIT wMemAddr);
_EXTERN S16BIT _DECL aceMemWrite(S16BIT DevNum,U16BIT wMemAddr,
								 U16BIT wValue);
_EXTERN S16BIT _DECL aceSetAddressMode(S16BIT DevNum,U16BIT wAddrMode);

_EXTERN S16BIT _DECL aceSetTimeTagRes(S16BIT DevNum,U16BIT wTTRes);
_EXTERN S16BIT _DECL aceSetTimeTagValue(S16BIT DevNum,U16BIT wTTValue);
_EXTERN S16BIT _DECL aceGetTimeTagValue(S16BIT DevNum,U16BIT* wTTValue);
_EXTERN S16BIT _DECL aceResetTimeTag(S16BIT DevNum);

_EXTERN S16BIT _DECL aceSetRespTimeOut(S16BIT DevNum,U16BIT wRespTimeOut);
_EXTERN S16BIT _DECL aceSetClockFreq(S16BIT DevNum,U16BIT wClockIn);
_EXTERN S16BIT _DECL aceSetIrqConfig(S16BIT DevNum,U16BIT wLvlOrPulse,
									 U16BIT wAutoClear);
_EXTERN S16BIT _DECL aceSetDecoderConfig(S16BIT DevNum,
						 U16BIT wDoubleOrSingle,
						 U16BIT wExpXingEnable);

_EXTERN S16BIT _DECL aceSetIrqConditions(	S16BIT DevNum,
						U16BIT bEnable,
						U32BIT dwIrqMask,
void(_DECL *funcExternalIsr)(S16BIT DevNum, U32BIT dwIrqStatus));

_EXTERN S16BIT _DECL aceSetRamParityChecking(S16BIT DevNum,
						 U16BIT wRamParityEnable);

_EXTERN S16BIT _DECL aceISQRead(S16BIT DevNum, ISQENTRY *pISQEntry);

_EXTERN S16BIT _DECL aceISQClear(S16BIT DevNum);

_EXTERN S16BIT _DECL aceSetMetrics(S16BIT DevNum, U16BIT bEnable);

_EXTERN U8BIT* _DECL GetLibraryRevisionInfo(void);	/* FJM 01/11/11 */

#endif
