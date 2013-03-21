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
 *	RTop.h (Remote Terminal operations header file)
 *
 *
 *
 *	Created 8/26/99 DL
 *
 *	
 * aceRTConfigure			
 * aceRTSetAddress			
 * aceRTDataBlkCreate		
 * aceRTDataBlkDelete		
 * aceRTDataBlkWrite		
 * aceRTDataBlkRead		
 * aceRTModeCodeIrqEnable	
 * aceRTModeCodeIrqDisable	
 * aceRTModeCodeIrqStatus	
 * aceRTModeCodeReadData	
 * aceRTModeCodeWriteData	
 * aceRTMsgLegalityEnable	
 * aceRTMsgLegalityDisable	
 * aceRTMsgLegalityStatus	
 * aceRTBusyBitsTblSet		
 * aceRTBusyBitsTblClear	
 * aceRTBusyBitsTblStatus	
 * aceRTStatusBitsSet		
 * aceRTStatusBitsClear	
 * aceRTStatusBitsStatus	
 * aceRTDataBlkMapToSA		
 * aceRTDataBlkUnmapFromSA	
 * aceRTBITWrdConfig		
 * aceRTBITWrdRead			
 * aceRTBITWrdWrite		
 * aceRTStart				
 * aceRTStop				
 * aceRTDecodeRawMsg		
 * aceRTGetStkMsgsRaw		
 * aceRTGetStkMsgDecoded	
 * aceRTInstallHBuf		
 * aceRTUninstallHBuf		
 * aceRTStkToHBuf			
 * aceRTGetHBufMsgCount	
 * aceRTGetHBufMsgsRaw		
 * aceRTGetHBufMsgDecoded	
 * aceRTDataBlkCircBufInfo	
 * aceRTCreateImageFiles	
 * aceRTSetRTSource
 * aceRTGetRTSource
 *
 */

#ifndef __RTOP_H__
#define __RTOP_H__

/* RT block status word bits */
#define ACE_RT_BSW_CMDERR			0x0001
#define ACE_RT_BSW_RTRTERR_2CMD		0x0002
#define ACE_RT_BSW_RTRTERR_GSA		0x0004
#define ACE_RT_BSW_INVWRD			0x0008
#define ACE_RT_BSW_BADSYNC			0x0010
#define ACE_RT_BSW_WCERR			0x0020
#define ACE_RT_BSW_ILLCMD			0x0040
#define ACE_RT_BSW_CIRCOVR			0x0080
#define ACE_RT_BSW_LPFAIL			0x0100
#define ACE_RT_BSW_NORES			0x0200
#define ACE_RT_BSW_FMTERR			0x0400
#define ACE_RT_BSW_RTRT				0x0800
#define ACE_RT_BSW_ERRFLG			0x1000
#define ACE_RT_BSW_CHNL				0x2000
#define ACE_RT_BSW_SOM				0x4000
#define ACE_RT_BSW_EOM				0x8000

/* Host Buffer Processing Modes */
#define ACE_RT_HBUFMODE_RT          0x0001
#define ACE_RT_HBUFMODE_BOTH        0x0002
#define ACE_RT_HBUFMODE_TMPRT       0x0003

/* wMsgLoc parameter can be any one of the following */
#define ACE_RT_MSGLOC_NEXT_PURGE	0	/* next unread msg, clr stk */
#define ACE_RT_MSGLOC_NEXT_NPURGE	1	/* next unread msg, leave stk */
#define ACE_RT_MSGLOC_LATEST_PURGE	2	/* latest msg, clear stk */
#define ACE_RT_MSGLOC_LATEST_NPURGE	3	/* latest msg, leave stk alone */

/* wCmdStkSize parameter can be any one of the following */
#define ACE_RT_CMDSTK_256	0x0000	/* Cmd Stacks are 256 words */
#define ACE_RT_CMDSTK_512	0x2000	/* Cmd Stacks are 512 words */
#define ACE_RT_CMDSTK_1K	0x4000	/* Cmd Stacks are 1K words */
#define ACE_RT_CMDSTK_2K	0x6000	/* Cmd Stacks are 2K words */

/* RT Address Sources */
#define ACE_RT_INTERNAL_ADDR 0x0001
#define ACE_RT_EXTERNAL_ADDR 0x0000

/* wOptions parameter can be any of the following bit "OR'd" together */
#define ACE_RT_OPT_CLR_SREQ	0x00000001 /* Clear sreq after tx vector wrd*/
#define ACE_RT_OPT_LOAD_TT	0x00000002 /* Load Time Tag on sync MCODE */
#define ACE_RT_OPT_CLEAR_TT	0x00000004 /* Clear Time Tag on sync MCODE */
#define ACE_RT_OPT_OVR_DATA	0x00000008 /* Overwrite inv data -circ buf */
#define ACE_RT_OPT_OVR_MBIT	0x00000010 /* T/R*=0,MC MSB=0, ACE will resp*/
#define ACE_RT_OPT_ALT_STS	0x00000020 /* Use RT alternate status word */
#define ACE_RT_OPT_IL_RX_D	0x00000040 /* Illegal receive disable */
#define ACE_RT_OPT_BSY_RX_D	0x00000080 /* Busy receive disable */
#define ACE_RT_OPT_SET_RTFG	0x00000100 /* set flag if loopback tsts fail*/
#define ACE_RT_OPT_1553A_MC	0x00000200 /* 1553a mode codes enabled */
#define ACE_RT_OPT_MC_O_BSY	0x00000400 /* Busy bit and data word sent */
#define ACE_RT_OPT_BCST_DIS	0x00000800 /* Broadcast disabled */

/* wDataBlkType parameter can be any one of the following */
#define ACE_RT_DBLK_SINGLE		32	/* single msg data block 32 words */
#define ACE_RT_DBLK_DOUBLE		33	/* doubled buffered msg dblk 64 wds */
#define ACE_RT_DBLK_C_128		34	/* circular buffer 128 words long */
#define ACE_RT_DBLK_C_256		35	/* circular buffer 256 words long */
#define ACE_RT_DBLK_C_512		36	/* circular buffer 512 words long */
#define ACE_RT_DBLK_C_1K		37	/* circular buffer 1K words long */
#define ACE_RT_DBLK_C_2K		38	/* circular buffer 2K words long */
#define ACE_RT_DBLK_C_4K		39	/* circular buffer 4K words long */
#define ACE_RT_DBLK_C_8K		40	/* circular buffer 8K words long */
#define ACE_RT_DBLK_GBL_C_128	41	/* global circ buf 128 words long */
#define ACE_RT_DBLK_GBL_C_256	42	/* global circ buf 256 words long */
#define ACE_RT_DBLK_GBL_C_512	43	/* global circ buf 512 words long */
#define ACE_RT_DBLK_GBL_C_1K	44	/* global circ buf 1K words long */
#define ACE_RT_DBLK_GBL_C_2K	45	/* global circ buf 2K words long */
#define ACE_RT_DBLK_GBL_C_4K	46	/* global circ buf 4K words long */
#define ACE_RT_DBLK_GBL_C_8K	47	/* global circ buf 8K words long */


/* wModeCodeType parameter can be any one of the following */
#define ACE_RT_MCTYPE_RX_NO_DATA		0x0000 /* undefined mc's */
#define ACE_RT_MCTYPE_RX_DATA			0x0001 /* rx mc's w/ data */
#define ACE_RT_MCTYPE_TX_NO_DATA		0x0002 /* tx mc's w/o data */
#define ACE_RT_MCTYPE_TX_DATA			0x0003 /* tx mc's w/ data */
#define ACE_RT_MCTYPE_BCST_RX_NO_DATA	0x0004 /* undefined mc's */
#define ACE_RT_MCTYPE_BCST_RX_DATA		0x0005 /* bcst rx mc's w/ data */
#define ACE_RT_MCTYPE_BCST_TX_NO_DATA	0x0006 /* bcst tx mc's w/o data */
#define ACE_RT_MCTYPE_BCST_TX_DATA		0x0007 /* undefined/reserved mc */

/* wModeCodes parameter can be any of the following OR'd together */
/* the constants are followed by their valid types */
#define ACE_RT_MCIRQ_DYN_BUS_CTRL		0x0001 /* TX_NO_DATA */
#define ACE_RT_MCIRQ_SYNCHRONIZE		0x0002 /*(BCST_)(TX|RX)(_NO)_DATA*/
#define ACE_RT_MCIRQ_TRNS_STATUS		0x0004 /* TX_NO_DATA */
#define ACE_RT_MCIRQ_INIT_SELF_TEST		0x0008 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_TRNS_SHUTDOWN		0x0010 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_OVR_TRNS_SHUTDWN	0x0020 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_INH_TERM_FLAG		0x0040 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_OVR_INH_TERM_FLG	0x0080 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_RESET_REMOTE_TERM	0x0100 /* (BCST_)TX_NO_DATA */
#define ACE_RT_MCIRQ_TRNS_VECTOR		0x0001 /* TX_DATA */
#define ACE_RT_MCIRQ_TRNS_LAST_CMD		0x0004 /* TX_DATA */
#define ACE_RT_MCIRQ_TRNS_BIT			0x0008 /* TX_DATA */
#define ACE_RT_MCIRQ_SEL_TRNS_SHUTDWN	0x0010 /* (BCST_)RX_DATA */
#define ACE_RT_MCIRQ_OVR_SEL_TRNS_SHUTDWN 0x0020 /* (BCST_)RX_DATA */
#define ACE_RT_MCIRQ_RESERVED_BIT6		0x0040
#define ACE_RT_MCIRQ_RESERVED_BIT7		0x0080
#define ACE_RT_MCIRQ_RESERVED_BIT8		0x0100
#define ACE_RT_MCIRQ_RESERVED_BIT9		0x0200
#define ACE_RT_MCIRQ_RESERVED_BIT10		0x0400
#define ACE_RT_MCIRQ_RESERVED_BIT11		0x0800
#define ACE_RT_MCIRQ_RESERVED_BIT12		0x1000
#define ACE_RT_MCIRQ_RESERVED_BIT13		0x2000
#define ACE_RT_MCIRQ_RESERVED_BIT14		0x4000
#define ACE_RT_MCIRQ_RESERVED_BIT15		0x8000

/* wModeCode parameter can be any one of the following*/
#define ACE_RT_MCDATA_RX_SYNCHRONIZE		0x0001
#define ACE_RT_MCDATA_RX_SEL_T_SHUTDWN		0x0004
#define ACE_RT_MCDATA_RX_OVR_SEL_T_SHUTDWN	0x0005
#define ACE_RT_MCDATA_TX_TRNS_VECTOR		0x0010
#define ACE_RT_MCDATA_TX_TRNS_LAST_CMD		0x0012
#define ACE_RT_MCDATA_TX_TRNS_BIT			0x0013
#define ACE_RT_MCDATA_BCST_SYNCHRONIZE		0x0021
#define ACE_RT_MCDATA_BCST_SEL_T_SHUTDWN	0x0024
#define ACE_RT_MCDATA_BCST_OVR_SEL_T_SHUTDWN 0x0025

/* wOwnAddrOrBcst parameter can be any one of the following */
#define ACE_RT_OWN_ADDRESS				0x0001
#define ACE_RT_BCST_ADDRSS				0x0000


/* Use this constant in appropriate functions to request modification of 
 * all cases of a particular type.
 */
#define ACE_RT_MODIFY_ALL				0xFFFF

/* wSAMask can be the any of the following "OR'd" together */
#define ACE_RT_SA_ALL		0xFFFFFFFF
#define ACE_RT_SA0			0x00000001
#define ACE_RT_SA1			0x00000002
#define ACE_RT_SA2			0x00000004
#define ACE_RT_SA3			0x00000008
#define ACE_RT_SA4			0x00000010
#define ACE_RT_SA5			0x00000020
#define ACE_RT_SA6			0x00000040
#define ACE_RT_SA7			0x00000080
#define ACE_RT_SA8			0x00000100
#define ACE_RT_SA9			0x00000200
#define ACE_RT_SA10			0x00000400
#define ACE_RT_SA11			0x00000800
#define ACE_RT_SA12			0x00001000
#define ACE_RT_SA13			0x00002000
#define ACE_RT_SA14			0x00004000
#define ACE_RT_SA15			0x00008000
#define ACE_RT_SA16			0x00010000
#define ACE_RT_SA17			0x00020000
#define ACE_RT_SA18			0x00040000
#define ACE_RT_SA19			0x00080000
#define ACE_RT_SA20			0x00100000
#define ACE_RT_SA21			0x00200000
#define ACE_RT_SA22			0x00400000
#define ACE_RT_SA23			0x00800000
#define ACE_RT_SA24			0x01000000
#define ACE_RT_SA25			0x02000000
#define ACE_RT_SA26			0x04000000
#define ACE_RT_SA27			0x08000000
#define ACE_RT_SA28			0x10000000
#define ACE_RT_SA29			0x20000000
#define ACE_RT_SA30			0x40000000
#define ACE_RT_SA31			0x80000000
			
			
/* wStatusBits can be any of the following OR'd together */
#define ACE_RT_STSBIT_DBCA				0x0800
#define ACE_RT_STSBIT_BUSY				0x0400
#define ACE_RT_STSBIT_SREQ				0x0200
#define ACE_RT_STSBIT_SSFLAG			0x0100
#define ACE_RT_STSBIT_RTFLAG			0x0080
/* wStatusBits when using the RT Alternate Status mode */
#define ACE_RT_STSBIT_S10				0x0800
#define ACE_RT_STSBIT_S09				0x0400
#define ACE_RT_STSBIT_S08				0x0200
#define ACE_RT_STSBIT_S07				0x0100
#define ACE_RT_STSBIT_S06				0x0080
#define ACE_RT_STSBIT_S05				0x0040
#define ACE_RT_STSBIT_S04				0x0020
#define ACE_RT_STSBIT_S03				0x0010
#define ACE_RT_STSBIT_S02				0x0008
#define ACE_RT_STSBIT_S01				0x0004
#define ACE_RT_STSBIT_S00				0x0002

/* wMsgType can be any of the following bitwize OR'd together */
#define ACE_RT_MSGTYPE_RX	0x0001	/* Rx msgs to SA will use data blk */
#define ACE_RT_MSGTYPE_TX	0x0002	/* Tx msgs to SA will use data blk */
#define ACE_RT_MSGTYPE_BCST	0x0004	/* Bcst msgs to SA will use data blk*/
#define ACE_RT_MSGTYPE_ALL	0x0007	/* Rx,Tx,Bcst msgs will use data blk*/

/* wIrqOptions can be 0 or any of the following "OR'd" together */
#define ACE_RT_DBLK_EOM_IRQ		0x0002 /* Irqs gen on end of msg */
#define ACE_RT_DBLK_CIRC_IRQ	0x0001 /* Irqs gen on circ buf rover */

/* wBITLoc can be one of the following */
#define ACE_RT_BIT_INTERNAL		0 /* (Default) use internal BIT wrd*/
#define ACE_RT_BIT_EXTERNAL		1 /* Use BIT from external RAM location */

/* wBITBusyInh can be one of the following */
#define ACE_RT_BIT_NO_INHIBIT	0 /* (Default) BIT wrd not inhibited */
#define ACE_RT_BIT_INHIBIT		1 /* BIT Wrd inhibited on RT busy */

/**************** RT FUNCTIONS **************************/
_EXTERN S16BIT _DECL aceRTConfigure(S16BIT DevNum,U16BIT wCmdStkSize,
									U32BIT wOptions);

_EXTERN S16BIT _DECL aceRTSetAddress(S16BIT DevNum,U16BIT wRTAddress);

_EXTERN S16BIT _DECL aceRTGetAddress(S16BIT DevNum,U16BIT *pRTAddress);

_EXTERN S16BIT _DECL aceRTDataBlkCircBufInfo(S16BIT DevNum,
											 S16BIT nDataBlkID,
											 U16BIT *pUserRWOffset,
											 U16BIT *pAceRWOffset);

_EXTERN S16BIT _DECL aceRTDataBlkCreate(S16BIT DevNum,
										S16BIT nDataBlkID,
										U16BIT wDataBlkType,
										U16BIT *pBuffer,
										U16BIT wBufferSize);

_EXTERN S16BIT _DECL aceRTDataBlkDelete(S16BIT DevNum,
										S16BIT nDataBlkID);

_EXTERN S16BIT _DECL aceRTDataBlkWrite(S16BIT DevNum,
									   S16BIT nDataBlkID,
									   U16BIT *pBuffer,
									   U16BIT wBufferSize,
									   U16BIT wOffset);

_EXTERN S16BIT _DECL aceRTDataBlkRead(S16BIT DevNum,
									  S16BIT nDataBlkID,
									  U16BIT *pBuffer,
									  U16BIT wBufferSize,
									  U16BIT wOffset);

_EXTERN S16BIT _DECL aceRTDataBlkMapToSA(S16BIT DevNum,
										 S16BIT nDataBlkID,
										 U16BIT wSA,
										 U16BIT wMsgType,
										 U16BIT wIrqOptions,
										 U16BIT wLegalizeSA);

_EXTERN S16BIT _DECL aceRTDataBlkUnmapFromSA(S16BIT DevNum,
											 S16BIT nDataBlkID,
											 U16BIT wSA,
											 U16BIT wMsgType);

_EXTERN S16BIT _DECL aceRTModeCodeIrqEnable(S16BIT DevNum,
											U16BIT wModeCodeType,
											U16BIT wModeCodeIrq);

_EXTERN S16BIT _DECL aceRTModeCodeIrqDisable(S16BIT DevNum,
											 U16BIT wModeCodeType,
											 U16BIT wModeCodeIrq);

_EXTERN S16BIT _DECL aceRTModeCodeIrqStatus(S16BIT DevNum,
											U16BIT wModeCodeType,
											U16BIT *pwMCIrqStatus);

_EXTERN S16BIT _DECL aceRTModeCodeReadData(S16BIT DevNum,
										   U16BIT wModeCode,
										   U16BIT *pMCData);

_EXTERN S16BIT _DECL aceRTModeCodeWriteData(S16BIT DevNum,
											U16BIT wModeCode,
											U16BIT wMCData);

_EXTERN S16BIT _DECL aceRTSetAddrSource(S16BIT DevNum,
									    U16BIT wRTSource);

_EXTERN S16BIT _DECL aceRTGetAddrSource(S16BIT DevNum,
								  	    U16BIT *wRTSource);

_EXTERN S16BIT _DECL aceRTMsgLegalityEnable(S16BIT DevNum,
											U16BIT wOwnAddrOrBcst,
											U16BIT wTR,
											U16BIT wSA,
											U32BIT dwWC_MCMask);


_EXTERN S16BIT _DECL aceRTMsgLegalityDisable(S16BIT DevNum,
											 U16BIT wOwnAddrOrBcst,
											 U16BIT wTR,
											 U16BIT wSA,
											 U32BIT dwWC_MCMask);


_EXTERN S16BIT _DECL aceRTMsgLegalityStatus(S16BIT DevNum,
											U16BIT wOwnAddrOrBcst,
											U16BIT wTR,
											U16BIT wSA,
											U32BIT *pdwMsgLegality);

_EXTERN S16BIT _DECL aceRTBusyBitsTblSet(S16BIT DevNum,
										 U16BIT wOwnAddrOrBcst,
										 U16BIT wTR,
										 U32BIT dwSAMask);


_EXTERN S16BIT _DECL aceRTBusyBitsTblClear(S16BIT DevNum,
										   U16BIT wOwnAddrOrBcst,
										   U16BIT wTR,
										   U32BIT dwSAMask);

_EXTERN S16BIT _DECL aceRTBusyBitsTblStatus(S16BIT DevNum,
											U16BIT wOwnAddrOrBcst,
											U16BIT wTR,
											U32BIT *pdwSABusyBits);

_EXTERN S16BIT _DECL aceRTStatusBitsSet(S16BIT DevNum,
										U16BIT wStatusBits);

_EXTERN S16BIT _DECL aceRTStatusBitsClear(S16BIT DevNum,
										  U16BIT wStatusBits);

_EXTERN S16BIT _DECL aceRTStatusBitsStatus(S16BIT DevNum,
										   U16BIT *pwStatusBits);


_EXTERN S16BIT _DECL aceRTBITWrdConfig(S16BIT DevNum,U16BIT wBITLoc,
									   U16BIT wBITBusyInh);

_EXTERN S16BIT _DECL aceRTBITWrdRead(S16BIT DevNum,U16BIT wBITLoc,
									 U16BIT *pBITWrd);

_EXTERN S16BIT _DECL aceRTBITWrdWrite(S16BIT DevNum,U16BIT wBITWrd);


_EXTERN S16BIT _DECL aceRTStart(S16BIT DevNum);

_EXTERN S16BIT _DECL aceRTStop(S16BIT DevNum);

_EXTERN S16BIT _DECL aceRTGetStkMsgsRaw(S16BIT DevNum,
										U16BIT *pBuffer,
										U16BIT wBufferSize);

_EXTERN S16BIT _DECL aceRTDecodeRawMsg(S16BIT DevNum,U16BIT *pBuffer,
									   MSGSTRUCT *pMsg);

_EXTERN S16BIT _DECL aceRTGetStkMsgDecoded(S16BIT DevNum,
										   MSGSTRUCT *pMsg,
										   U16BIT wMsgLoc);

_EXTERN S16BIT _DECL aceRTInstallHBuf(S16BIT DevNum,
									  U32BIT dwHBufSize);

_EXTERN S16BIT _DECL aceRTUninstallHBuf(S16BIT DevNum);

_EXTERN S16BIT _DECL aceRTStkToHBuf(S16BIT DevNum);

_EXTERN S16BIT _DECL aceRTGetHBufMsgCount(S16BIT DevNum);

_EXTERN S16BIT _DECL aceRTGetHBufMsgsRaw(S16BIT DevNum,
										 U16BIT *pBuffer,
										 U16BIT wBufferSize,
										 U32BIT *pdwMsgCount,
										 U32BIT *pdwMsgLostStk,
										 U32BIT *pdwMsgLostHBuf);

_EXTERN S16BIT _DECL aceRTGetHBufMsgDecoded(S16BIT DevNum,
											MSGSTRUCT *pMsg,
											U32BIT *pdwMsgCount,
											U32BIT *pdwMsgLostStk,
											U32BIT *pdwMsgLostHBuf,
											U16BIT wMsgLoc);

_EXTERN S16BIT _DECL aceRTCreateImageFiles(S16BIT DevNum,
										   char *pszIFile,
										   char *pszHFile);

_EXTERN S16BIT _DECL aceRTGetHBufMetric( S16BIT DevNum,
										 HBUFMETRIC *pMetric,
										 U16BIT bReset);

_EXTERN S16BIT _DECL aceRTGetStkMetric( S16BIT DevNum,
										STKMETRIC *pMetric,
										U16BIT bReset);
#endif
