/*	Data Device Corporation
 *	105 Wilbur Place
 *	Bohemia N.Y. 11716
 *	(631) 567-5600
 *
 *		ENHANCED MINI-ACE 'C' Run Time Software Library
 *
 *			Copyright (c) 1999 by Data Device Corporation
 *			All Rights Reserved.
 *
 *
 *	Errordef.h (RTL error definitions header file)
 *
 *
 *
 *	Created 8/26/99 DL
 *
 */


#ifndef __ERRORDEF_H__
#define __ERRORDEF_H__

#define ACE_ERR_SUCCESS		 	  0		/* No error occurred */

/* Config/Access Errors */
#define ACE_ERR_INVALID_DEVNUM	-50		/* Bad Card number */
#define ACE_ERR_INVALID_ACCESS	-51		/* Bad Access type */
#define ACE_ERR_INVALID_MODE	-52		/* Bad Mode type */
#define ACE_ERR_INVALID_STATE	-53		/* Bad device state */
#define ACE_ERR_INVALID_MEMSIZE	-54		/* Bad memory wrd size */
#define ACE_ERR_INVALID_ADDRESS	-55		/* Bad mem/reg address */
#define ACE_ERR_INVALID_OS		-56		/* Bad operating system */
#define ACE_ERR_INVALID_MALLOC	-57		/* Bad sim/win32 mem alloc */
#define ACE_ERR_INVALID_BUF		-58		/* Bad buffer */
#define ACE_ERR_INVALID_ADMODE	-59		/* Bad addressing mode */
#define ACE_ERR_SIMWRITEREG		-60		/* Bad simulated write reg */
#define ACE_ERR_TIMETAG_RES		-61		/* Bad time tag resolution */
#define ACE_ERR_RESPTIME		-62		/* Bad response timeout value*/
#define ACE_ERR_CLOCKIN			-63		/* Bad clock input value */
#define ACE_ERR_MSGSTRUCT		-64		/* Bad message structure */
#define ACE_ERR_PARAMETER		-65		/* Bad parameter */
#define ACE_ERR_INVALID_MODE_OP -66     /* Invalid Mode Options */
#define ACE_ERR_METRICS_NOT_ENA -67     /* Metrics not enabled */
#define ACE_ERR_NOT_SUPPORTED   -68     /* Function not supported */

/* Win32 Config/Access Errors */
#define ACE_ERR_REG_ACCESS		-80		/* Unable to access registry */
#define ACE_ERR_INVALID_CARD	-81		/* Not a valid card */
#define ACE_ERR_DRIVER_OPEN		-82		/* Unable to open device driver */
#define ACE_ERR_MAPMEM_ACC      -83     /* Error Accessing mapped memory */

/* Link list errors */
#define ACE_ERR_NODE_NOT_FOUND	-100	/* Could not find node in DLIST*/
#define ACE_ERR_NODE_MEMBLOCK	-101	/* Node was not a memory block*/
#define ACE_ERR_NODE_EXISTS		-102	/* Node ID is already in list */

/* Memory management errors */
#define ACE_ERR_MEMMGR_FAIL		-150	/* Could not alloc mem block */

/* Test mode errors */
#define ACE_ERR_TEST_BADSTRUCT	-200	/* Bad test result structure */
#define ACE_ERR_TEST_FILE		-201	/* Not a valid file */

/* MT mode errors */
#define ACE_ERR_MT_BUFTYPE		-300	/* Not a valid buffer mode */
#define ACE_ERR_MT_CMDSTK		-301	/* Not a valid cmd stack size */
#define ACE_ERR_MT_DATASTK		-302	/* Not a valid data stack size */
#define ACE_ERR_MT_FILTER_RT	-303	/* Not a valid RT address */
#define ACE_ERR_MT_FILTER_TR	-304	/* Not a valid TR bit */
#define ACE_ERR_MT_FILTER_SA	-305	/* Not a valid SA buffer */
#define ACE_ERR_MT_STKLOC		-306	/* Bad MT stack location */
#define ACE_ERR_MT_MSGLOC		-307	/* Bad MT msg location */
#define ACE_ERR_MT_HBUFSIZE		-308	/* Bad HBuf size */
#define ACE_ERR_MT_HBUF			-309	/* HBuf not allocated */
#define ACE_ERR_RTMT_COMBO_HBUF -310    /* RTMT Host Buffer used */
#define ACE_ERR_RTMT_HBUFSIZE   -311    /* Bad RTMT HBuf size */
#define ACE_ERR_RTMT_HBUF       -312    /* RTMT HBuf not allocated */
#define ACE_ERR_RTMT_MSGLOC     -313    /* Bad RTMT msg location */

/* RT mode errors */
#define ACE_ERR_RT_DBLK_EXISTS	-400	/* data block already exists */
#define ACE_ERR_RT_DBLK_ALLOC	-401	/* data block alloc failed */
#define ACE_ERR_RT_DBLK_MAPPED	-402	/* data block is linked to SA */
#define ACE_ERR_RT_DBLK_NOT_CB	-403	/* data block is not a circ buf */
#define ACE_ERR_RT_HBUF			-410	/* HBuf not allocated */


/* BC mode errors */
#define ACE_ERR_BC_DBLK_EXISTS	-500	/* data block already exists */
#define ACE_ERR_BC_DBLK_ALLOC	-501	/* data block alloc failed */
#define ACE_ERR_BC_DBLK_SIZE	-502	/* data block size incorrect */
#define ACE_ERR_UNRES_DATABLK	-503	/* data block not found */	
#define ACE_ERR_UNRES_MSGBLK	-504	/* msg block not found */
#define ACE_ERR_UNRES_FRAME		-505	/* frame block not found */
#define ACE_ERR_UNRES_OPCODE	-506	/* opcode block not found */
#define ACE_ERR_UNRES_JUMP		-507	/* jump address is out of frame */
#define ACE_ERR_FRAME_NOT_MAJOR	-508	/* Frame is not a major frame */

/* Host buffer errors */
#define ACE_ERR_HBUFSIZE		-600	/* HBuf size incompatible */
#define ACE_ERR_HBUF			-601	/* HBuf not allocated */

/* VxWorks errors */
#define ACE_ERR_TOO_MANY_DEVS	-700	/* max number of devs reached */

/* This function returns information describing a particular error */
_EXTERN S16BIT _DECL aceErrorStr(S16BIT nError,
								 char *pBuffer,
								 U16BIT wBufSize);

#endif
