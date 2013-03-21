
#include <string.h>
#include "../carrier/carrier.h"
#include "iplst.h"

/*
{+D}
    SYSTEM:         Software for IP modules

    FILENAME:       drvrlst.c

    MODULE NAME:    main - main routine of example software.

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       This module is the main routine for the example program
                    which demonstrates how the Library is used.

    CALLING
        SEQUENCE:   

    MODULE TYPE:    void

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:  

    MODULES
        CALLED:     

    REVISIONS:      

  DATE    BY        PURPOSE
-------  ----   ------------------------------------------------


{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

        This module is the main routine for the example program
        which demonstrates how the Library is used.
*/



int main(argc, argv)
int argc; char *argv[];
{
    
/*
    DECLARE LOCAL DATA AREAS:
*/

    unsigned char cmd_buff[40];     /* command line input buffer */
    unsigned char id_buff[80];      /* ID buffer */
    int item;                       /* menu item selection variable */
    unsigned finished;              /* flag to exit program */
    long addr;                      /* hold board address */
    struct sblklst s_blocklst;      /* allocate status param. blk */
    struct cblklst c_blocklst;      /* configuration block */
    int i;
    word idProm[12];                /* holds ID prom */
    int carrier_instance;

/*
    ENTRY POINT OF ROUTINE:
    INITIALIZATION
*/

    finished = 0;	 /* indicate not finished with program */

/*
    Initialize the Configuration Parameter Block to default values.
*/

    c_blocklst.bCarrier = FALSE;	/* indicate no carrier initialized and set up yet */
    c_blocklst.bInitialized = FALSE;/* indicate not ready to talk */
    c_blocklst.slotLetter = SLOT_A;
    c_blocklst.nHandle = 0;	/* make handle to a closed carrier board */
    c_blocklst.sblk_ptr = (struct sblklst*)&s_blocklst;

/*
	Initialize the Carrier library
*/
    if(InitCarrierLib() != S_OK)
    {
      printf("\nUnable to initialize the carrier library. Exiting program.\n");
      exit(0);
    }

/*
	Open an instance of a carrier device 
*/
    if(argc == 2) /* parse carrier instance from command line */
      carrier_instance = atoi(argv[1]);
    else
      carrier_instance = 0;

    if(CarrierOpen(carrier_instance, &c_blocklst.nHandle) != S_OK)
    {
      printf("\nUnable to open carrier.\n");
      finished = 1;	 /* indicate finished with program */
    }

/*
    Enter main loop
*/      

    while(!finished)
    {
      printf("\n\nList IP Modules  Version A\n\n");
      printf(" 1. Exit this Program\n");
      printf(" 2. List IP Modules\n");
      printf("\nSelect: ");
      scanf("%d",&item);

/*
    perform the menu item selected.
*/  
      switch(item)
      {

      case 1: /* exit program command */

          printf("Exit program(y/n)?: ");
          scanf("%s",cmd_buff);
          if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
              finished++;
      break;
        
      case 2: /* List IP Modules */
          GetCarrierAddress(c_blocklst.nHandle, &addr);	/* Read back carrier address */
          if(CarrierInitialize(c_blocklst.nHandle) == S_OK)/* Initialize Carrier */
          {
             c_blocklst.bCarrier = TRUE;

             for(c_blocklst.slotLetter = SLOT_A; c_blocklst.slotLetter <= GetLastSlotLetter(); c_blocklst.slotLetter += 1)
             {
                memset(idProm, 0, sizeof(idProm));		/* clear buffer */
                memset(id_buff, 0, sizeof(id_buff));		/* clear buffer */

                if(GetIpackAddress(c_blocklst.nHandle, c_blocklst.slotLetter, &addr) == S_OK)
                {
                  c_blocklst.brd_ptr = (struct maplst *)addr;
                  c_blocklst.bInitialized = TRUE;

                  /* read from carrier */
                  ReadIpackID(c_blocklst.nHandle, c_blocklst.slotLetter, &idProm[0], (sizeof(idProm) / 2));

                  for(i = 0; i < 12; i++ )
                     c_blocklst.sblk_ptr->id_prom[i] = (byte)idProm[i];

                  i = ExtractModel( c_blocklst.sblk_ptr->id_prom, id_buff );

                  printf("I/O Address %08X Slot %c Model: %s",(unsigned int)c_blocklst.brd_ptr, c_blocklst.slotLetter, id_buff);
                  if(i == 'H')		/* indicate a module that can operate at high speed */
                    printf("+\n");
                  else
                    printf("\n");
                }
                else
                  printf("\nUnable to Get IP Address.\n");
             }			
          }
          else
            printf("\nUnable to obtain carrier address.\n");

      break;
      }   /* end of switch */
    }   /* end of while */
	
    if(c_blocklst.bCarrier)
       CarrierClose(c_blocklst.nHandle);

    printf("\nEXIT PROGRAM\n");
    return(0);
}   /* end of main */



/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvrlst.c

    MODULE NAME:    ExtractModel - Extract the model number from the ID Prom string

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine which is used to Extract the model number from the ID string

    CALLING
        SEQUENCE:   int ExtractModel( unsigned char *rawid, unsigned char *model )
                    where:
                        unsigned char *rawid - The address of the raw ID string
						unsigned char *model - The extracted model number
                         
    MODULE TYPE:    void
    
    I/O RESOURCES:  
    
    SYSTEM
        RESOURCES:  
        
    MODULES
        CALLED:

    REVISIONS:      
    
  DATE    BY        PURPOSE
-------  ----   ------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:
    
*/




/* Extract the model number from the ID string. */

int ExtractModel( unsigned char *rawid, unsigned char *model )
{
    unsigned char x1, x2, x3, x4;
    int mfg, ip;

    x1 = (unsigned char)rawid[0];
    x2 = (unsigned char)rawid[1];
    x3 = (unsigned char)rawid[2];
	/* x4 contains the module clock speed flag 'C' = 8MHz, 'H' = 32MHz and is returned to the caller */
    x4 = (unsigned char)rawid[3];
    mfg = (int)rawid[4];
    ip = (int)rawid[5];

    /* check first 3 bytes VITA type II == 'VIT' for valid module */
    if( (unsigned char)x1 == 'V' && (unsigned char)x2 == 'I' && (unsigned char)x3 == 'T' )
    {
	strcpy((char*)model,"IO_UNKNOWN");	/* not one of Acromags.*/
	return((int)x4);
    }
    else
    {
      /* check first 3 bytes VITA type I should == ASCII 'IPA' for valid module */
      if( (unsigned char)x1 == 'I' && (unsigned char)x2 == 'P' && (unsigned char)x3 == 'A' )
      {
        if( mfg == 0xA3 ) /* one of Acromags */
        {
          switch( ip )
          {
            case 0x01:
                strcpy((char*)model,"IP405");
                return((int)x4);
            case 0x02:
                strcpy((char*)model,"IP400");
                return((int)x4);
            case 0x03:
                strcpy((char*)model,"IP408");
                return((int)x4);
            case 0x04:
                strcpy((char*)model,"IP500");
                return((int)x4);
            case 0x05:
                strcpy((char*)model,"IP501-X");
                return((int)x4);
            case 0x06:
                strcpy((char*)model,"IP502");
                return((int)x4);
            case 0x07:
                strcpy((char*)model,"IP503");
                return((int)x4);
            case 0x08:
                strcpy((char*)model,"IP470");
                return((int)x4);
            case 0x09:
                strcpy((char*)model,"IP445");
                return((int)x4);
            case 0x10:
                strcpy((char*)model,"IP440-X");
                return((int)x4);
            case 0x11:
                strcpy((char*)model,"IP330");
                return((int)x4);
            case 0x12:
                strcpy((char*)model,"IP235-8");
                return((int)x4);
            case 0x13:
                strcpy((char*)model,"IP235-4");
                return((int)x4);
            case 0x14:
                strcpy((char*)model,"IP511-X");
                return((int)x4);
            case 0x15:
                strcpy((char*)model,"IP512-X");
                return((int)x4);
            case 0x16:
                strcpy((char*)model,"IP480-6");
                return((int)x4);
            case 0x17:
                strcpy((char*)model,"IP480-2");
                return((int)x4);
            case 0x18:
                strcpy((char*)model,"IP230-8");
                return((int)x4);
            case 0x19:
                strcpy((char*)model,"IP230-4");
                return((int)x4);
            case 0x20:
                strcpy((char*)model,"IP409");
                return((int)x4);
            case 0x22:
                strcpy((char*)model,"IP220-16");
                return((int)x4);
            case 0x23:
                strcpy((char*)model,"IP220-8");
                return((int)x4);
            case 0x24:
                strcpy((char*)model,"IP520");
                return((int)x4);
            case 0x25:
                strcpy((char*)model,"IP521");
                return((int)x4);
            case 0x26:
                strcpy((char*)model,"IP236-8");
                return((int)x4);
            case 0x27:
                strcpy((char*)model,"IP236-4");
                return((int)x4);
            case 0x28:
                strcpy((char*)model,"IP340");
                return((int)x4);
            case 0x29:
                strcpy((char*)model,"IP341");
                return((int)x4);
            case 0x32:
                strcpy((char*)model,"IP320");
                return((int)x4);
            case 0x33:
                strcpy((char*)model,"IP231-8");
                return((int)x4);
            case 0x34:
                strcpy((char*)model,"IP231-16");
                return((int)x4);
            case 0x40:
                strcpy((char*)model,"IP1K100C"); /* C = configuration mode */
                return((int)x4);
            case 0x41:
                strcpy((char*)model,"IP1K100U"); /* U = user mode */
                return((int)x4);
            case 0x42:
                strcpy((char*)model,"IP1K110C"); /* C = configuration mode */
                return((int)x4);
            case 0x43:
                strcpy((char*)model,"IP1K110U"); /* U = user mode */
                return((int)x4);				
            case 0x44:
                strcpy((char*)model,"IP1K125");
                return((int)x4);
            case 0x45:
                strcpy((char*)model,"IP482");
                return((int)x4);
            case 0x46:
                strcpy((char*)model,"IP483");
                return((int)x4);
            case 0x47:
                strcpy((char*)model,"IP484");
                return((int)x4);
            case 0x48:
                strcpy((char*)model,"IPEP20XC"); /* C = configuration mode */
                return((int)x4);
            case 0x49:
                strcpy((char*)model,"IPEP20XU"); /* U = user mode */
                return((int)x4);
            case 0x50:
                strcpy((char*)model,"IP560");
                return((int)x4);
            case 0x51:
                strcpy((char*)model,"IP560-I");
                return((int)x4);
            case 0x52:
                strcpy((char*)model,"IP571");
                return((int)x4);
            case 0x53:
                strcpy((char*)model,"IP572");
                return((int)x4);
            case 0xAA:
                strcpy((char*)model,"IPTEST");   /* factory test module */
                return((int)x4);
            default:
                strcpy((char*)model,"IPGENERIC");
                return((int)x4);
          }
        }
        else
        {
          strcpy((char*)model,"IPUNKNOWN");
          return((int)x4);
        }
      }
      strcpy((char*)model,"IPVACANT");
      return((int)x4);
    }
}

