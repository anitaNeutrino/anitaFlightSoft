
/*
{+D}
    SYSTEM:         Linux

    FILENAME:	    APCe8650.c

    MODULE NAME:    APCe8650.c

    VERSION:	    B

    CREATION DATE:  04/01/11

    CODED BY:	    FJM

    ABSTRACT:       This file contains the definitions, structures
                    and prototypes for APCe8650.

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
---------   -----	---------------------------------------------------
  02/14/12   FJM    CarrierOpen() now frees malloced memory if it fails & returns an error

{-D}
*/

#include "carrier.h"
#include <time.h>
#include <errno.h>


#define DEVNAME "/dev/apc8650_"	/* name of device */


#define MAX_TRIES		500


/*
	Global variables
*/
int	gNumberCarriers = -1;	/* Number of boards opened and/or flag = -1 if library... */
                            /* ... is uninitialized see function InitCarrierLib() */

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
		return((long)0);

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

long input_long_pci_config(int nHandle, long *p)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[3];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return((long)0);

        /* place address to read from in data[0]; */
        data[0] = (unsigned long) p;
        /* place device instance to read from @ data[1]; */
        data[1] = (unsigned long) pCarrier->nDevInstance;
        /* pram3 = function: 1=read8bits,2=read16bits,4=read32bits, 0x40=readconfig32bits */
        read( pCarrier->nCarrierDeviceHandle, &data[0], 0x40 );
        return(  SwapLong( (long)data[1] ) );
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

void output_long_pci_config(int nHandle, long *p, long v)
{
	CARRIERDATA_STRUCT* pCarrier;	/*  local carrier */
	unsigned long data[3];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == NULL)
		return;

        /* place address to write data[0]; */
        data[0] = (unsigned long) p;
        /* place value to write @ address data[1]; */
        data[1] = (unsigned long) SwapLong( v );
        /* place device instance  @ data[2]; */
        data[2] = (unsigned long) pCarrier->nDevInstance;
        /* pram3 = function: 1=write8bits,2=write16bits,4=write32bits, 0x40=writeconfig32bits */
        write( pCarrier->nCarrierDeviceHandle, &data[0], 0x40 );
}

void blocking_start_convert(int nHandle, word *p, word v)
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
           /* pram3 = function: 8=blocking_start_convert */
           write( pCarrier->nCarrierDeviceHandle, &data[0], 8 );
	}
}


void blocking_start_convert_byte(int nHandle, byte *p, byte v)
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
           /* pram3 = function: 9=blocking_start_convert_byte */
           write( pCarrier->nCarrierDeviceHandle, &data[0], 9 );
	}
}


byte GetLastSlotLetter(void)
{
	/* This function returns the ASCII value of the Capital letter for the
		last slot letter that this carrier supports 
	*/

	return (byte)'D';
}

CARSTATUS SetCarrierAddress(int nHandle, long lAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(lAddress != 0)
		pCarrier->lBaseAddress = lAddress;

	return (CARSTATUS)S_OK;
}

CARSTATUS GetCarrierAddress(int nHandle, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	*pAddress = pCarrier->lBaseAddress;
	return (CARSTATUS)S_OK;
}

CARSTATUS EnableInterrupts(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
    word nValue; 

	
	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	nValue = input_word(nHandle,(word*)&pPCIeCard->controlReg);
	nValue |= APC_INT_PENDING_CLEAR;	/*  Clear any pending interrupts */
	output_word(nHandle,(word*)&pPCIeCard->controlReg, nValue );

	nValue |= APC_INT_ENABLE;		/* Enable interrupts */
	output_word(nHandle,(word*)&pPCIeCard->controlReg, nValue );

	pCarrier->bIntEnabled = TRUE;	/*  Interrupts are Enabled */
	return (CARSTATUS)S_OK;
}

CARSTATUS DisableInterrupts(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
    word nValue; 

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	nValue = input_word(nHandle,(word*)&pPCIeCard->controlReg);
	nValue &= ~APC_INT_ENABLE;	/* Disable interrupts */
	output_word(nHandle,(word*)&pPCIeCard->controlReg, nValue );

	pCarrier->bIntEnabled = FALSE;

	return (CARSTATUS)S_OK;
}

CARSTATUS SetInterruptHandler(int nHandle, char chSlot, int nRequestNumber,
			int nVector, void(*pHandler)(void*), void* pData)
{
	return (CARSTATUS)S_OK;
}

CARSTATUS GetIpackAddress(int nHandle, char chSlot, long* pAddress)
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
	default:
		*pAddress = 0;
		return E_INVALID_SLOT;
		break;
	}
	return (CARSTATUS)S_OK;
}

CARSTATUS ReadIpackID(int nHandle, char chSlot, word* pWords, int nWords)
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
	default:
		pWord = 0;
		return E_INVALID_SLOT;
		break;
	}

	for(i = 0; i < nWords; i++, pWords++, pWord++)
		*pWords	= input_word(nHandle, pWord );

	return (CARSTATUS)S_OK;
}


CARSTATUS SetInterruptLevel(int nHandle, word uLevel)
{
	return (CARSTATUS)S_OK;
}

CARSTATUS GetInterruptLevel(int nHandle, word* pLevel)
{
	return (CARSTATUS)S_OK;
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


CARSTATUS InitCarrierLib(void)
{
	int i;                      /* General purpose index */

	if( gNumberCarriers == -1)  /* first time used - initialize */
	{
	  gNumberCarriers = 0;      /* Initialize number of carriers to 0 */

	  /* initialize the pointers to the Carrier data structure */
	  for(i = 0; i < MAX_CARRIERS; i++)
		gpCarriers[i] = 0;      /* set to a NULL pointer */
	}
	return (CARSTATUS)S_OK;
}


CARSTATUS CarrierOpen(int nDevInstance, int* pHandle)
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
		return (CARSTATUS)E_OUT_OF_MEMORY;
	}

	/* initialize new data structure */
	pCarrier->nHandle = -1;	
	pCarrier->bInitialized = FALSE;
	pCarrier->bIntEnabled = FALSE;
	pCarrier->uCarrierID = 0;
	pCarrier->lMemBaseAddress = 0;
	pCarrier->nDevInstance = -1;	/* Device Instance */

	memset( &pCarrier->devname[0], 0, sizeof(pCarrier->devname));
	memset( &devnamebuf[0], 0, sizeof(devnamebuf));
	memset( &devnumbuf[0], 0, sizeof(devnumbuf));
	strcpy(devnamebuf, DEVNAME);
	sprintf(&devnumbuf[0],"%d",nDevInstance);
	strcat(devnamebuf, devnumbuf);

  	pCarrier->nCarrierDeviceHandle = open( devnamebuf, O_RDWR );

	if( pCarrier->nCarrierDeviceHandle < 0 )
	{
	   free((void*)pCarrier);
   	   *pHandle = -1;	/* set handle to an invalid value */
	   return (CARSTATUS)ERROR;
	}

	strcpy(&pCarrier->devname[0], &devnamebuf[0]);	/* save device name */
	pCarrier->nDevInstance = nDevInstance;	/* Device Instance */

	/* Get Base IP memory (if present) Address  */
	memset( &data[0], 0, sizeof(data)); /* no IP mem if data[x] returns 0 from ioctl() */
	ioctl( pCarrier->nCarrierDeviceHandle, 4, &data[0] );/* get IP mem address cmd */
	pCarrier->lMemBaseAddress = data[nDevInstance];

	/* Get Base Address of Carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 5, &data[0] );/* get address cmd */
	pCarrier->lBaseAddress = data[nDevInstance];

	if( pCarrier->lBaseAddress == 0 )/* no carrier if data[x] returns 0 from ioctl() */
	{
	   free((void*)pCarrier);
	   *pHandle = -1;	/* set handle to an invalid value */
	   return (CARSTATUS)ERROR;
	}

	/* Get IRQ Number from carrier */
	ioctl( pCarrier->nCarrierDeviceHandle, 6, &data[0] );/* get IRQ cmd */
	pCarrier->nIntLevel = ( int )( data[nDevInstance] & 0xFF );

	AddCarrier(pCarrier);	/*  call function to add carrier to array and set handle */
	*pHandle = pCarrier->nHandle;	/* return the new handle */
	return (CARSTATUS)S_OK;
}

CARSTATUS CarrierClose(int nHandle)
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
	return (CARSTATUS)S_OK;
}

CARSTATUS CarrierInitialize(int nHandle)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	/*  initialize carrier */
	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

	/* now reset the carrier */
	output_word(nHandle, (word*)&pPCIeCard->controlReg, SOFTWARE_RESET );

	/* pCarrier->uCarrierID value is saved in the carrier structure for later use */
	/* see include file apce8650.h for carrier attributes */
	pCarrier->uCarrierID = PCI_CARRIER;	/* attributes PCI */

	pCarrier->uCarrierID |= CARRIER_CLK; /* ths carrier supports clock */
	pCarrier->uCarrierID |= CARRIER_MEM; /* ths carrier supports memory */
	
	pCarrier->bInitialized = TRUE;	/*  Carrier is now initialized */
	return (CARSTATUS)S_OK;
}


/* carrier enhancements */
CARSTATUS SetMemoryAddress(int nHandle, long lAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(lAddress != 0)
		pCarrier->lMemBaseAddress = lAddress;

	return (CARSTATUS)S_OK;
}


CARSTATUS GetMemoryAddress(int nHandle, long* pAddress)
{
	CARRIERDATA_STRUCT* pCarrier;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	*pAddress = pCarrier->lMemBaseAddress;
	return (CARSTATUS)S_OK;
}


CARSTATUS GetIpackMemoryAddress(int nHandle, char chSlot, long* pAddress)
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
		default:
			*pAddress = 0;
			return E_INVALID_SLOT;
			break;
		}
		return (CARSTATUS)S_OK;
	}
	return (CARSTATUS)E_NOT_IMPLEMENTED;
}


CARSTATUS GetCarrierID(int nHandle, word* pCarrierID)
{
	CARRIERDATA_STRUCT* pCarrier;
	
	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	*pCarrierID = pCarrier->uCarrierID;
	return (CARSTATUS)S_OK;
}


CARSTATUS SetIPClockControl(int nHandle, char chSlot, word uControl)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
	word nValue;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	/* check carrier ID to see if 32MHZ IP clocking is supported */
	if( pCarrier->uCarrierID & CARRIER_CLK )	/* nonzero can support 32MHZ IP clock */
	{
		pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	    nValue = input_word(nHandle, (word*)&pPCIeCard->IPClockControl);

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
				nValue |= 8;		/* make slot D IP clock 32MHZ */
			break;
		default:
			return E_INVALID_SLOT;
			break;
		}
		output_word(nHandle, (word*)&pPCIeCard->IPClockControl, nValue ); /* write value */
		return (CARSTATUS)S_OK;
	}
	return (CARSTATUS)E_NOT_IMPLEMENTED;
}


CARSTATUS GetIPClockControl(int nHandle, char chSlot, word* pControl)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
	word nValue;

	*pControl = 0;	/* default */

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	if(pCarrier->bInitialized == FALSE)
		return E_NOT_INITIALIZED;

	/* check carrier ID to see if 32MHZ IP clocking is supported */
	if( pCarrier->uCarrierID & CARRIER_CLK )	/* nonzero can support 32MHZ IP clock */
	{
		pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	    nValue = input_word(nHandle, (word*)&pPCIeCard->IPClockControl);

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
		default:
			return E_INVALID_SLOT;
			break;
		}
		*pControl = nValue;			/* write value */
		return (CARSTATUS)S_OK;
	}
	return (CARSTATUS)E_NOT_IMPLEMENTED;
}

CARSTATUS SetAutoAckDisable(int nHandle, word uState)
{
	return (CARSTATUS)E_NOT_IMPLEMENTED;
}

CARSTATUS GetAutoAckDisable(int nHandle, word* pState)
{
	*pState = FALSE;
	return (CARSTATUS)E_NOT_IMPLEMENTED;
}

CARSTATUS GetTimeOutAccess(int nHandle, word* pState)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
	word nValue;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	/* get control register */
	nValue = input_word(nHandle, (word*)&pPCIeCard->controlReg);

	if(nValue & 0x0010)	/* mask for IP timeout status */
	  *pState = TRUE;
	else
	  *pState = FALSE;

	return (CARSTATUS)S_OK;
}

CARSTATUS GetIPErrorBit(int nHandle, word* pState)
{
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;
	word nValue;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		
	/* get control register */
	nValue = input_word(nHandle, (word*)&pPCIeCard->controlReg);

	if( nValue & 0x0001 )	/* mask for IP error bit */
	  *pState = TRUE;
	else
	  *pState = FALSE;

	return (CARSTATUS)S_OK;
}




/*
   Read/write ID Register.
   Address - contains the address of serial EEPROM to read or write
   Data - data only for a write cycle
   Cycle - cycle type read or write

   The ID Register is used to store and retrieve a 16 bit non-volatile
   identifier that can be used to uniquely identify a particular carrier board.
   Only the low 16 bits are active.
*/


void GuaranteedSleep(uint32_t msec)
{
  struct timespec timeout0, timeout1;
  struct timespec *tmp, *t0, *t1;

  t0 = &timeout0;
  t1 = &timeout1;
  t0->tv_sec = msec / 1000;
  t0->tv_nsec = (msec % 1000) * (1000 * 1000);

  while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR))
  {
    tmp = t0;
    t0 = t1;
    t1 = tmp;
  }
}



CARSTATUS AccessVPD( int nHandle, ULONG Address, ULONG* Data, ULONG Cycle )
{
    ULONG i;
	CARRIERDATA_STRUCT* pCarrier;
	PCIe_BOARD_MEMORY_MAP* pPCIeCard;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

	pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)pCarrier->lBaseAddress;		

    if( Cycle )					/* if write cycle... */
    {

      /* Poll FLASH_BUSY bit it should read logic low */
      for(i = 0; i < (long)MAX_TRIES; i++ )
      {
        if( (input_word( nHandle,(word*)&pPCIeCard->controlReg) & (word)FLASH_BUSY) == 0 )
		break;

        GuaranteedSleep((uint32_t)6);
      }

      if( i >= (long)MAX_TRIES )
         return((int) -1);		/* before write loop timeout error */

      /* write the value to the ID register */
      output_word( nHandle,(word*)&pPCIeCard->ID_Register, (word)(*Data));

      /* Poll FLASH_BUSY bit until it reads as a logic low.*/
      for(i = 0; i < (long)MAX_TRIES; i++ )
      {
        if( (input_word( nHandle,(word*)&pPCIeCard->controlReg) & (word)FLASH_BUSY) == 0 )
           break;

        GuaranteedSleep((uint32_t)6);
      }

      if( i >= (long)MAX_TRIES )
         return((int) -2);		/* after write loop timeout error */
    }
    else				/* else a read cycle... */
    {
      /* Poll FLASH_BUSY bit until it reads as a logic low.*/
      for(i = 0; i < (long)MAX_TRIES; i++ )
      {
        if( (input_word( nHandle,(word*)&pPCIeCard->controlReg) & (word)FLASH_BUSY) == 0 )
           break;

        GuaranteedSleep((uint32_t)6);
      }

      if( i >= (long)MAX_TRIES )
         return((int) -3);		/* read loop timeout error */

      /* read the data value from the ID_Register */
      *Data = (long)input_word( nHandle,(word*)&pPCIeCard->ID_Register);
	}
    return((int) 0);
}

CARSTATUS WakeUp57x(int nHandle, int DevNum)
{
	CARRIERDATA_STRUCT* pCarrier;
	unsigned long data;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

        data = (unsigned long)DevNum;
	ioctl( pCarrier->nCarrierDeviceHandle, 21, &data );/* get Wake57x cmd */
	
	return (CARSTATUS)S_OK;
}

unsigned int Block57x(int nHandle, int DevNum, long unsigned int *dwStatus)
{
	CARRIERDATA_STRUCT* pCarrier;
	unsigned long data;
	unsigned int cmd_status;

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

        data = (unsigned long)DevNum;
	cmd_status = ioctl( pCarrier->nCarrierDeviceHandle, 22, &data );/* get Block57x cmd */
        *dwStatus = (long unsigned int)data;
	
	return (cmd_status);
}

CARSTATUS SetInfo57x(int nHandle, int slot, int channel, int DevNum)
{
	CARRIERDATA_STRUCT* pCarrier;
	unsigned long data[4];

	pCarrier = GetCarrier(nHandle);
	if(pCarrier == 0)
		return E_INVALID_HANDLE;

        data[0] = (unsigned long)pCarrier->nDevInstance;
        data[1] = (unsigned long)slot;
        data[2] = (unsigned long)channel;
        data[3] = (unsigned long)DevNum;

	ioctl( pCarrier->nCarrierDeviceHandle, 20, &data[0] );/* get SetInfo57x cmd */
	
	return (CARSTATUS)S_OK;
}
