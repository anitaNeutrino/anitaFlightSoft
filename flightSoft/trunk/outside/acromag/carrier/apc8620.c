
/*
{+D}
    SYSTEM:			Linux

    FILENAME:	    APC8620.c

    MODULE NAME:    APC8620.c

    VERSION:	    C

    CREATION DATE:  08/31/01

    CODED BY:	    FJM

    ABSTRACT:       This file contains the definitions, structures
                    and prototypes for APC8620A.
                    The APC8620A is a enhanced version of the APC8620.
                    This library may be used with both enhanced (APC8620A)
                    and non-enhanced (APC8620) carriers.
                    Calling the enhanced functions will not harm existing
                    non-enhanced (APC8620) hardware.
                    The enhanced functions check to be sure of the carrier's
                    capabilities.
                    CarrierInitialize() must be called before calling any
                    enhanced function.
                    Enhanced functions will return "E_NOT_IMPLEMENTED" for
                    a non-enhanced (APC8620) carrier.

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
  12/22/04   FJM    Fedora core FC3 update.
  01/20/05   FJM    Extended carrier function library interface.

{-D}
*/

#include "apc8620.h"

#define DEVNAME "/dev/apc8620_"	/* name of device */


/*
	Global variables
*/
int	gNumberCarriers;		/*  Number of carrier boards that have been opened */

CARRIERDATA_STRUCT *gpCarriers[MAX_CARRIERS];	/* pointer to the carrier boards */





/*
        Some systems can resolve BIG_ENDIAN/LITTLE_ENDIAN data transfers in hardware.
        If the system is resolving BIG_ENDIAN/LITTLE_ENDIAN data transfers in hardware
        the SWAP_ENDIAN define should be commented out.

        When resolving the BIG_ENDIAN/LITTLE_ENDIAN data transfers in hardware is not
        possible or desired the SWAP_ENDIAN define is provided.

        Define SWAP_ENDIAN to enable software byte swapping for word and long transfers
*/

/* #define SWAP_ENDIAN		/ * SWAP_ENDIAN enables software byte swapping for word and long transfers */


word SwapBytes( word v )             
{
#ifdef SWAP_ENDIAN		/* endian correction if needed */
  word  Swapped;

  Swapped = v << 8;
  Swapped |= ( v >> 8 );
  return( Swapped );
#else				/* no endian correction needed */
  return( v );
#endif /* SWAP_ENDIAN */

}


long SwapLong( long v )             
{
#ifdef SWAP_ENDIAN		/* endian correction if needed */
 word Swap1, Swap2;
 long Swapped; 

  Swap1 = (word)(v >> 16);
  Swap1 = SwapBytes( Swap1 );             

  Swap2 = (word)v & 0xffff;
  Swap2 = SwapBytes( Swap2 );

  Swapped = (long)(Swap2 << 16); 
  Swapped |= (long)(Swap1 & 0xffff);
  return( Swapped );
#else				/* no endian correction needed */
  return( v );
#endif /* SWAP_ENDIAN */
 
}





byte input_byte(int nHandle, byte *p)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

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
		return;

	if( p )
	{
           /* place address to read word from in data [0]; */
           data[0] = (unsigned long) p;
           /* pram3 = function: 1=read8bits,2=read16bits */
           read( pCarrier->nCarrierDeviceHandle, &data[0], 2 );
           return(  SwapBytes( (word)data[1] ) );
	}
	return((word)0);
}

long input_long(int nHandle, long *p)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

	if( p )
	{
           /* place address to read word from in data [0]; */
           data[0] = (unsigned long) p;
           /* pram3 = function: 1=read8bits,2=read16bits,4=read32bits */
           read( pCarrier->nCarrierDeviceHandle, &data[0], 4 );
           return(  SwapLong( (long)data[1] ) );
	}
	return((long)0);
}

void output_byte(int nHandle, byte *p, byte v)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
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

	if( p )
	{
           /* place address to write word in data [0]; */
           data[0] = (unsigned long) p;
           /* place value to write @ address data [1]; */
           data[1] = (unsigned long) SwapBytes( v );
           /* pram3 = function: 1=write8bits,2=write16bits */
           write( pCarrier->nCarrierDeviceHandle, &data[0], 2 );
	}
}

void output_long(int nHandle, long *p, long v)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[2];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

	if( p )
	{
           /* place address to write word in data [0]; */
           data[0] = (unsigned long) p;
           /* place value to write @ address data [1]; */
           data[1] = (unsigned long) SwapLong( v );
           /* pram3 = function: 1=write8bits,2=write16bits,4=write32bits */
           write( pCarrier->nCarrierDeviceHandle, &data[0], 4 );
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
	CARRIERDATA_STRUCT* pCarrier;	/* local carrier pointer */
    unsigned long data[MAX_CARRIERS];
	char devnamebuf[32];
	char devnumbuf[8];


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
	pCarrier->uCarrierID = 0;
	pCarrier->lMemBaseAddress = 0;
    memset( &pCarrier->devname[0], 0, sizeof(pCarrier->devname));

    memset( &devnamebuf[0], 0, sizeof(devnamebuf));
    memset( &devnumbuf[0], 0, sizeof(devnumbuf));
    strcpy(devnamebuf, DEVNAME);
    sprintf(&devnumbuf[0],"%d",nDevInstance);
    strcat(devnamebuf, devnumbuf);

  	pCarrier->nCarrierDeviceHandle = open( devnamebuf, O_RDWR );

	if( pCarrier->nCarrierDeviceHandle < 0 )
		return (ACRO_CSTATUS)ERROR;

	strcpy(&pCarrier->devname[0], &devnamebuf[0]);	/* save device name */

	/* Get Base IP memory (if present) Address  */
    memset( &data[0], 0, sizeof(data)); /* no IP mem if data[x] returns 0 from ioctl() */
	ioctl( pCarrier->nCarrierDeviceHandle, 4, &data[0] );/* get IP mem address cmd */
	pCarrier->lMemBaseAddress = data[nDevInstance];

	/* Get Base Address of Carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 5, &data[0] );/* get address cmd */
	pCarrier->lBaseAddress = data[nDevInstance];

	/* Get IRQ Number from carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 6, &data[0] );/* get IRQ cmd */
	pCarrier->nIntLevel = ( int )( data[nDevInstance] & 0xFF );
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

  	close( &pCarrier->devname[0] );
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

	/* determine the carrier type and initialize */
	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	/* now reset the carrier */
	output_word(nHandle, (word*)&pPCICard->controlReg, SOFTWARE_RESET );

	/* following a software reset 8620a type products will return either */
	/* 0xAyyy or 0xByyyy values in the MS nibble of the control register (yyy = dont care)*/
	/* original 8620 carriers will return 0x0yyy in the MS nibble of the control register */
	/* 0x0yyy = original 8620 board - no extended features */
	/* 0xAyyy = 8620a board with 32MHZ IP clock without extra memory space */
	/* 0xByyy = 8620a board with 32MHZ IP clock and extra memory space */
	nValue = input_word( nHandle,(word*)&pPCICard->controlReg);	/* read again */

	/* pCarrier->uCarrierID value is saved in the carrier structure for later use */
	/* see include file apc8620.h for carrier attributes */

	pCarrier->uCarrierID = PCI_CARRIER;	/* attributes PCI */

	/* test the result */
	if((nValue & 0xF000) == 0xA000)
		pCarrier->uCarrierID |= CARRIER_CLK;

	if((nValue & 0xF000) == 0xB000)
	{
		pCarrier->uCarrierID |= CARRIER_CLK;
		pCarrier->uCarrierID |= CARRIER_MEM;
	}		
	pCarrier->bInitialized = TRUE;	/*  Carrier is now initialized */

	return (ACRO_CSTATUS)S_OK;
}


/* carrier enhancements */
ACRO_CSTATUS SetMemoryAddress(int nHandle, long lAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(lAddress != 0)
		pCarrier->lMemBaseAddress = lAddress;

	return (ACRO_CSTATUS)S_OK;
}


ACRO_CSTATUS GetMemoryAddress(int nHandle, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	*pAddress = pCarrier->lMemBaseAddress;
	return (ACRO_CSTATUS)S_OK;
}


ACRO_CSTATUS GetIpackMemoryAddress(int nHandle, char chSlot, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;
	
	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	if( pCarrier->lMemBaseAddress )	/* nonzero if MEM space is supported */
	{
		switch(chSlot)
		{
		case SLOT_A:
			*pAddress = pCarrier->lMemBaseAddress + SLOT_A_MEM_OFFSET;
			break;
		case SLOT_B:
			*pAddress = pCarrier->lMemBaseAddress + SLOT_B_MEM_OFFSET;
			break;
		case SLOT_C:
			*pAddress = pCarrier->lMemBaseAddress + SLOT_C_MEM_OFFSET;
			break;
		case SLOT_D:
			*pAddress = pCarrier->lMemBaseAddress + SLOT_D_MEM_OFFSET;
			break;
		case SLOT_E:
			*pAddress = pCarrier->lMemBaseAddress + SLOT_E_MEM_OFFSET;
			break;
		default:
			*pAddress = 0;
			return E_INVALID_SLOT;
			break;
		}
		return (ACRO_CSTATUS)S_OK;
	}
	return (ACRO_CSTATUS)E_NOT_IMPLEMENTED;
}


ACRO_CSTATUS GetCarrierID(int nHandle, word* pCarrierID)
{
	CARRIERDATA_STRUCT* pCarrier;
	
	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	*pCarrierID = pCarrier->uCarrierID;
	return (ACRO_CSTATUS)S_OK;
}


ACRO_CSTATUS SetIPClockControl(int nHandle, char chSlot, word uControl)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
	word nValue;
	ACRO_CSTATUS result;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	/* check carrier ID to see if 32MHZ IP clocking is supported */
	if( pCarrier->uCarrierID & CARRIER_CLK )	/* nonzero can support 32MHZ IP clock */
	{
		pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	    nValue = input_word(nHandle, (word*)&pPCICard->IPClockControl);

		switch(chSlot)
		{
		case SLOT_A:
			nValue &= 0x00FE;		/* default force bit 0 = 0 = 8MHZ IP clock */
			if( uControl )			/* does caller want 32MHZ? */
				nValue |= 1;		/* make slot A IP clock 32MHZ */
			break;
		case SLOT_B:
			nValue &= 0x00FD;		/* default force bit 1 = 0 = 8MHZ IP clock */
			if( uControl )			/* does caller want 32MHZ? */
				nValue |= 2;		/* make slot B IP clock 32MHZ */
			break;
		case SLOT_C:
			nValue &= 0x00FB;		/* default force bit 2 = 0 = 8MHZ IP clock */
			if( uControl )			/* does caller want 32MHZ? */
				nValue |= 4;		/* make slot C IP clock 32MHZ */
			break;
		case SLOT_D:
			nValue &= 0x00F7;		/* default force bit 3 = 0 = 8MHZ IP clock */
			if( uControl )			/* does caller want 32MHZ? */
			nValue |= 8;			/* make slot D IP clock 32MHZ */
			break;
		case SLOT_E:
			nValue &= 0x00EF;		/* default force bit 4 = 0 = 8MHZ IP clock */
			if( uControl )			/* does caller want 32MHZ? */
				nValue |= 0x10;		/* make slot E IP clock 32MHZ */
			break;
		default:
			return E_INVALID_SLOT;
			break;
		}
		output_word(nHandle, (word*)&pPCICard->IPClockControl, nValue ); /* write value */
		return (ACRO_CSTATUS)S_OK;
	}
	return (ACRO_CSTATUS)E_NOT_IMPLEMENTED;
}


ACRO_CSTATUS GetIPClockControl(int nHandle, char chSlot, word* pControl)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
	word nValue;
	ACRO_CSTATUS result;

	*pControl = 0;	/* default */

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	/* check carrier ID to see if 32MHZ IP clocking is supported */
	if( pCarrier->uCarrierID & CARRIER_CLK )	/* nonzero can support 32MHZ IP clock */
	{
		pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	    nValue = input_word(nHandle, (word*)&pPCICard->IPClockControl);

		switch(chSlot)
		{
		case SLOT_A:
			if( nValue & 1 )	/* test IP clock bit 0 = 8MHZ, 1 = 32MHZ */
				nValue = 1;
			else
				nValue = 0;
			break;
		case SLOT_B:
			if( nValue & 2 )	/* bit 1 */
				nValue = 1;
			else
				nValue = 0;
			break;
		case SLOT_C:
			if( nValue & 4 )	/* bit 2 */
				nValue = 1;
			else
				nValue = 0;
			break;
		case SLOT_D:
			if( nValue & 8 )	/* bit 3 */
				nValue = 1;
			else
				nValue = 0;
			break;
		case SLOT_E:
			if( nValue & 0x10 )	/* bit 4 */
				nValue = 1;
			else
				nValue = 0;
			break;
		default:
			return E_INVALID_SLOT;
			break;
		}
		*pControl = nValue;			/* write value */
		return (ACRO_CSTATUS)S_OK;
	}
	return (ACRO_CSTATUS)E_NOT_IMPLEMENTED;
}

ACRO_CSTATUS SetAutoAckDisable(int nHandle, word uState)
{
	return (ACRO_CSTATUS)E_NOT_IMPLEMENTED;
}

ACRO_CSTATUS GetAutoAckDisable(int nHandle, word* pState)
{
	*pState = FALSE;
	return (ACRO_CSTATUS)E_NOT_IMPLEMENTED;
}

ACRO_CSTATUS GetTimeOutAccess(int nHandle, word* pState)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
	word nValue;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	/* get control register */
	nValue = input_word(nHandle, (word*)&pPCICard->controlReg);

	if(nValue & 0x0010)	/* mask for IP timeout status */
	  *pState = TRUE;
	else
	  *pState = FALSE;

	return (ACRO_CSTATUS)S_OK;
}

ACRO_CSTATUS GetIPErrorBit(int nHandle, word* pState)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCI_BOARD_MEMORY_MAP* pPCICard;
	word nValue;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	pPCICard = (PCI_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	/* get control register */
	nValue = input_word(nHandle, (word*)&pPCICard->controlReg);

	if( nValue & 0x0001 )	/* mask for IP error bit */
	  *pState = TRUE;
	else
	  *pState = FALSE;

	return (ACRO_CSTATUS)S_OK;
}
