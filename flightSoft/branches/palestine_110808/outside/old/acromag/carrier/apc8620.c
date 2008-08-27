

/*
{+D}
    SYSTEM:         Acromag APC8620 Carrier Board

    FILENAME:	    APC8620.c

    MODULE NAME:    APC8620.c

    VERSION:	    V1.0

    CREATION DATE:  08/31/01

    CODED BY:	    FJM

    ABSTRACT:       This file contains the implementation of the
                    functions for the Acromag APC8620 carrier
                    board library.

    CALLING
	   SEQUENCE:

    MODULE TYPE:

    I/O 
	  RESOURCES:

    SYSTEM
	  RESOURCES:

    MODULES
	     CALLED:

    REVISIONS:

    DATE	 BY     PURPOSE
  --------   -----	---------------------------------------------------

{-D}
*/

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include "apc8620.h"


#define DEVNAME "/dev/apc8620"	/* name of device */


/*
	Global variables
*/
int	gNumberCarriers;		/*  Number of carrier boards that have been opened */

CARRIERDATA_STRUCT *gpCarriers[MAX_CARRIERS];	/* pointer to the carrier boards */



byte input_byte(int nHandle, byte *p)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return((byte)0);

	if(pCarrier->bInitialized == FALSE)
		return((byte)0);

	if( p )
	{
           /* place address to read byte from in data [0]; */
           data[0] = (unsigned long) p;
           /* pram3 = function: 1=read8bits,2=read16bits */
           read( pCarrier->nCarrierDeviceHandle, &data[0], 1 );
           return( (byte)data[1] );
	}
	return((byte)0);
}

word input_word(int nHandle, word *p)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return((word)0);

	if(pCarrier->bInitialized == FALSE)
		return((word)0);

	if( p )
	{
           /* place address to read word from in data [0]; */
           data[0] = (unsigned long) p;
           /* pram3 = function: 1=read8bits,2=read16bits */
           read( pCarrier->nCarrierDeviceHandle, &data[0], 2 );
           return( (word)data[1] );
	}
	return((word)0);
}

void output_byte(int nHandle, byte *p, byte v)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

	if(pCarrier->bInitialized == FALSE)
		return;

	if( p )
	{
		/* place address to write byte in data [0]; */
		data[0] = (unsigned long) p;
		/* place value to write @ address data [1]; */
		data[1] = (unsigned long) v;
		/* pram3 = function: 1=write8bits,2=write16bits */
		write( pCarrier->nCarrierDeviceHandle, &data[0], 1 );
	}
}

void output_word(int nHandle, word *p, word v)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

	if(pCarrier->bInitialized == FALSE)
		return;

	if( p )
	{
           /* place address to write word in data [0]; */
           data[0] = (unsigned long) p;
           /* place value to write @ address data [1]; */
           data[1] = (unsigned long) v;
           /* pram3 = function: 1=write8bits,2=write16bits */
           write( pCarrier->nCarrierDeviceHandle, &data[0], 2 );
	}
}

byte GetLastSlotLetter(void)
{
	/* This function returns the ASCII value of the Capital letter for the
		last slot letter that this carrier supports 
	*/

	return (byte)'E';
}

ACRO_CSTATUS SetCarrierAddress(int nHandle, long lAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(lAddress != 0)
		pCarrier->lBaseAddress = lAddress;

	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS GetCarrierAddress(int nHandle, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	*pAddress = pCarrier->lBaseAddress;
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS EnableInterrupts(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
        word nValue; 

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	nValue = input_word(nHandle,(word*)&pPCICard->controlReg);
	nValue |= APC_INT_PENDING_CLEAR;	/*  Clear any pending interrupts */
	output_word(nHandle,(word*)&pPCICard->controlReg, nValue );

	nValue |= APC_INT_ENABLE;		/* Enable interrupts */
	output_word(nHandle,(word*)&pPCICard->controlReg, nValue );

	pCarrier->bIntEnabled = TRUE;	/*  Interrupts are Enabled */
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS DisableInterrupts(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
        word nValue; 

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	nValue = input_word(nHandle,(word*)&pPCICard->controlReg);
	nValue &= ~APC_INT_ENABLE;	/* Disable interrupts */
	output_word(nHandle,(word*)&pPCICard->controlReg, nValue );

	pCarrier->bIntEnabled = FALSE;

	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS SetInterruptHandler(int nHandle, char chSlot, int nRequestNumber,
			int nVector, void(*pHandler)(void*), void* pData)
{
	int slot;
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

 	slot = (int)( toupper( chSlot ) - 0x41);

	/* Set user data area pointer for slot handler into device driver */
	ioctl( pCarrier->nCarrierDeviceHandle, slot, pData );
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS GetIpackAddress(int nHandle, char chSlot, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;
	
	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	switch(chSlot)
	{
	case SLOT_A:
		*pAddress = pCarrier->lBaseAddress + SLOT_A_IO_OFFSET;
		break;
	case SLOT_B:
		*pAddress = pCarrier->lBaseAddress + SLOT_B_IO_OFFSET;
		break;
	case SLOT_C:
		*pAddress = pCarrier->lBaseAddress + SLOT_C_IO_OFFSET;
		break;
	case SLOT_D:
		*pAddress = pCarrier->lBaseAddress + SLOT_D_IO_OFFSET;
		break;
	case SLOT_E:
		*pAddress = pCarrier->lBaseAddress + SLOT_E_IO_OFFSET;
		break;
	default:
		*pAddress = 0;
		return E_INVALID_SLOT;
		break;
	}
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS ReadIpackID(int nHandle, char chSlot, word* pWords, int nWords)
{
	int i;		/* 	local index */	
	word* pWord;	/*  local variable */
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	switch(chSlot)
	{
	case SLOT_A:
		pWord = (word *)(pCarrier->lBaseAddress + SLOT_A_ID_OFFSET);
		break;
	case SLOT_B:
		pWord = (word *)(pCarrier->lBaseAddress + SLOT_B_ID_OFFSET);
		break;
	case SLOT_C:
		pWord = (word *)(pCarrier->lBaseAddress + SLOT_C_ID_OFFSET);
		break;
	case SLOT_D:
		pWord = (word *)(pCarrier->lBaseAddress + SLOT_D_ID_OFFSET);
		break;
	case SLOT_E:
		pWord = (word *)(pCarrier->lBaseAddress + SLOT_E_ID_OFFSET);
		break;
	default:
		pWord = 0;
		return E_INVALID_SLOT;
		break;
	}


	for(i = 0; i < nWords; i++, pWords++, pWord++)
		*pWords	= input_word(nHandle, pWord );

	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS SetInterruptLevel(int nHandle, word uLevel)
{
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS GetInterruptLevel(int nHandle, word* pLevel)
{
	return (ACRO_CSTATUS)S_OK;
}

void AddCarrier(CARRIERDATA_STRUCT* pCarrier)
{
	int	i, j;	/* general purpose index */
	BOOL bFound;	/* general purpose BOOL */

	/* Determine a handle for this carrier */
	for(i = 0; i < MAX_CARRIERS; i++)
	{
		bFound = TRUE;
		for(j = 0; j < gNumberCarriers; j++)
		{
			if(i == gpCarriers[j]->nHandle)
			{
				bFound = FALSE;
				break;
			}
		}
		
		if(bFound)
			break;				
	}
	
	/* set new handle */
	pCarrier->nHandle = i;

	/* add carrier to array */
	gpCarriers[gNumberCarriers] = pCarrier;
	gNumberCarriers++;	/* increment number of carriers */
}

void DeleteCarrier(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	int i;

	if(gNumberCarriers == 0)
		return;

	pCarrier = 0;	/* initialize pointer to null */

	/* Find carrier that has this handle */
	for(i = 0; i < gNumberCarriers; i++)
	{
		if(nHandle == gpCarriers[i]->nHandle)
		{
			pCarrier = gpCarriers[i];
			break;
		}
	}

	/* return if no carrier has been found */
	if(pCarrier == 0)
		return;

	/* delete the memory for this carrier */
	free((void*)pCarrier);

	/* Rearrange carrier array */
	gpCarriers[i] = gpCarriers[gNumberCarriers - 1];
	gpCarriers[gNumberCarriers - 1] = 0;		
	
	/* decrement carrier count */
	gNumberCarriers--;
}

CARRIERDATA_STRUCT* GetCarrier(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	int i;		/*  General purpose index */

	/* Find carrier that has this handle */
	for(i = 0; i < gNumberCarriers; i++)
	{
		if(nHandle == gpCarriers[i]->nHandle)
		{
			pCarrier = gpCarriers[i];
			return pCarrier;
		}
	}

	return (CARRIERDATA_STRUCT*)0;	/* return null */	
}

ACRO_CSTATUS InitCarrierLib(void)
{
	int i;	/* General purpose index */
		
	gNumberCarriers = 0;	/* Initialize number of carriers to 0 */

	/* initialize the pointers to the Carrier data structure */
	for(i = 0; i < MAX_CARRIERS; i++)
	{
		gpCarriers[i] = 0;	/* set to a NULL pointer */
	}

	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS CarrierOpen(int nDevInstance, int* pHandle)
{
  return CarrierOpenDev(nDevInstance, pHandle, DEVNAME);
}

ACRO_CSTATUS CarrierOpenDev(int nDevInstance, int* pHandle, char *devname)
{
	CARRIERDATA_STRUCT* pCarrier;	/* local carrier pointer */
        unsigned long data[2];

	if(gNumberCarriers == MAX_CARRIERS)
		return E_OUT_OF_CARRIERS;

	/* Allocate memory for a new Carrier structure */
	pCarrier = (CARRIERDATA_STRUCT*)malloc(sizeof(CARRIERDATA_STRUCT));
	
	if(pCarrier == 0)
	{
		*pHandle = -1;	/* set handle to an invalid value */
		return (ACRO_CSTATUS)E_OUT_OF_MEMORY;
	}

	/* initialize new data structure */
	pCarrier->nHandle = -1;	
	pCarrier->bInitialized = FALSE;
	pCarrier->bIntEnabled = FALSE;
  	pCarrier->nCarrierDeviceHandle = open( devname, O_RDWR );

	if( pCarrier->nCarrierDeviceHandle < 0 )
		return (ACRO_CSTATUS)ERROR;

	/* Get Base Address of Carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 5, &data[0] );/* get address cmd */
	pCarrier->lBaseAddress = data[0];

	/* Get IRQ Number from carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 6, &data[1] );/* get IRQ cmd */
	pCarrier->nIntLevel = ( int )( data[1] & 0xFF );
	AddCarrier(pCarrier);	/*  call function to add carrier to array and set handle */
	*pHandle = pCarrier->nHandle;	/* return the new handle */
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS CarrierClose(int nHandle)
{
	/*  Delete the carrier with the provided handle */
	CARRIERDATA_STRUCT* pCarrier;	/* local carrier pointer */

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

  	close( pCarrier->nCarrierDeviceHandle );
  	pCarrier->nCarrierDeviceHandle = -1;
	DeleteCarrier(nHandle);		
	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS CarrierInitialize(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
        word nValue; 

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
	{
		pCarrier->bInitialized = FALSE;
		return E_INVALID_HANDLE;
	}

	/*  initialize carrier */
	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	nValue = input_word(nHandle,(word*)&pPCICard->controlReg);
	nValue |= SOFTWARE_RESET;	 /* Perform Software reset */
	output_word(nHandle,(word*)&pPCICard->controlReg, nValue );
	pCarrier->bInitialized = TRUE;	/*  Carrier is now initialized */
	return (ACRO_CSTATUS)S_OK;
}

