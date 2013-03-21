
#include <ctype.h>		/* toupper() */
#include <sys/types.h>		/* getpid() */
#include <unistd.h>		/* sleep() */
#include "../ioscarrier/ioscarrier.h"
#include "ios570.h"
#include "Testvec.h"		/* Memory resident test vector data */

#define INTERRUPT_LEVEL 5	/* level or bus IRQ - may not be used on all systems */
#define VECTOR			192	/* interrupt vector - may not be used on all systems */

/*
{+D}
    SYSTEM:         Software for IOS570

    FILENAME:       drvr570.c

    MODULE NAME:    main - main routine of example software.

    VERSION:        A

    CREATION DATE:  03/19/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       This module is the main routine for the example program
                    which demonstrates how the IOS570 Library is used.

    CALLING
        SEQUENCE:   

    MODULE TYPE:    void

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:  

    MODULES
        CALLED:     

    REVISIONS:      

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module is the main routine for the example program
    which demonstrates how the IOS570 Library is used.
*/



int main(argc, argv)
int argc; char *argv[];
{

/*
    DECLARE LOCAL DATA AREAS:
*/

    char cmd_buff[40];   /* command line input buffer */
    int item;            /* menu item selection variable */
    int i;               /* loop counter */
    int status;          /* returned status of routines */
    unsigned finished;   /* flag to exit program */
    unsigned finished1;   /* flag to exit program */
    long addr, addr2;    /* hold boards addresses */
    long flag;           /* general flag for exiting loops */
    struct cblk570 c_block570[MAX_CH]; /* configuration block */
    unsigned short s_mask; /* support memory mask */
    int ch_number;
    int carrier_instance = 0;
/*
    ENTRY POINT OF ROUTINE:
    INITIALIZATION
*/


    if(argc == 2)
      carrier_instance = atoi(argv[1]);

    ch_number = 0;   /* assume single channel device */
    flag = 0;	 	 /* indicate board address not yet assigned */
    finished = 0;	 /* indicate not finished with program */

/*
    Initialize the Configuration Parameter Block to default values.
*/

    memset(&c_block570[0], 0, sizeof(c_block570[MAX_CH]));

    c_block570[0].vector = VECTOR;			/* interrupt vector */
    c_block570[0].bCarrier = FALSE;			/* indicate no carrier initialized and set up yet */
    c_block570[0].bInitialized = FALSE;		/* indicate not ready to talk */
    c_block570[0].slotLetter = SLOT_A;
    c_block570[0].RemoteTerminalAddressLatch = 1; /* default settings */
    c_block570[0].InterruptEnable = 1;		/* enabled */
    c_block570[0].bus_clock = 1;			/* default 8MHz */
    c_block570[0].nHandle = -1;				/* make handle to a closed carrier board */

    /* the remainder of ch 1 structure initialization is done during menu selection 3 */
    
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
    if(CarrierOpen(carrier_instance, &c_block570[0].nHandle) != S_OK)
    {
      printf("\nUnable to Open instance of carrier.\n");
      exit(0);
    }
    else
      flag = 1;

/*
    Enter main loop
    */
    while(!finished) {
    	printf("\nIOS570 Library Demonstration  Version A\n");
        printf(" 1. Exit this Program\n");
        printf(" 2. Set Carrier Base Address\n");
        printf(" 3. Set IOS Slot Letter\n");
        printf(" 4. Examine/Change Current Channel\n");
        printf(" 5. Set Up Configuration Block Parameters\n");
        printf(" 6. Configure Board Command\n");
        printf(" 7. Read ID PROM\n");
        printf(" 8. Enable Interrupts\n");
        printf(" 9. Disable Interrupts\n");
        printf("10. 1553 Functions\n");
        printf("\nSelect: ");
        scanf("%d",&item);

/*
    perform the menu item selected.
*/  
        switch(item) {

        case 1: /* exit program command */

            printf("Exit program(y/n)?: ");
            scanf("%s",cmd_buff);
            if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
                finished++;
        break;

        case 2: /* set board address command */
	    do 
	    {
		if(flag == 0)
		{
		  printf("\n\nenter carrier board base I/O address in hex: ");
		  scanf("%lx",&addr);
		  /* Set Carrier Address for Open Carrier Device */
		  SetCarrierAddress(c_block570[0].nHandle, addr);	/* Set Carrier Address */
		}
		GetCarrierAddress(c_block570[0].nHandle, &addr);	/* Read back carrier address */
		printf("I/O Address: %lX\n",addr);
		printf("is this value correct(y/n)?: ");
		scanf("%s",cmd_buff);
		if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
		{
		  SetCarrierAddress(c_block570[0].nHandle, addr);	/* Set Carrier Address */
		  if(CarrierInitialize(c_block570[0].nHandle) == S_OK)/* Initialize Carrier */
		  {
		    c_block570[0].bCarrier = TRUE;
			SetInterruptLevel(c_block570[0].nHandle, INTERRUPT_LEVEL);/* Set carrier interrupt level */
		  }
		  flag = 1;
		}
		else
		  flag = 0;

	    }while( cmd_buff[0] != 'y' && cmd_buff[0] != 'Y' );

        /* Make sure this carrier supports memory space */
        GetCarrierID(c_block570[0].nHandle, &s_mask);	/* see if carrier can support memory I/O */
        if( ( s_mask & CARRIER_MEM ) == 0)
        {
          printf("\nSorry... this carrier does not support memory space\n");
          finished = 1;	 /* indicate finished with program */
		  break;
        }

        flag = 1;         /* indicate memory address not yet assigned */
        do 
		{
          if(flag == 0)
		  {
            printf("\n\nenter carrier memory base address in hex: ");
            scanf("%lx",&addr2);

            /* Set IOS Memory Address */
            SetMemoryAddress( c_block570[0].nHandle, (long)addr2);	/* Set IOS Memory Address */
		  }
          GetMemoryAddress(c_block570[0].nHandle, (long*)&addr2);
          printf("Memory Address: %lX\n",addr2);
          printf("is this value correct(y/n)?: ");
          scanf("%s",cmd_buff);
          if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
		  {
            flag = 1;
		  }
          else
            flag = 0;
	    }while( cmd_buff[0] != 'y' && cmd_buff[0] != 'Y' );

        break;

        case 3: /* set IOS Slot Letter */
            if(flag == 0 || c_block570[0].bCarrier == FALSE)
               printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
            {
               printf("\n\nEnter IOS slot letter (A, B etc...): ");
               scanf("%s",cmd_buff);
               cmd_buff[0] = toupper(cmd_buff[0]);
               if(cmd_buff[0] < 'A' || cmd_buff[0] > GetLastSlotLetter())
               {
                  printf("\nInvalid Slot Letter!\n");
                  c_block570[0].bCarrier = FALSE;
               }
               else
               {
                  c_block570[0].slotLetter = cmd_buff[0];
/*
    Get the IOS base I/O address based on the Slot letter,
    and initialize the IOS data structure with the returned address
*/
                  if(GetIOSAddress(c_block570[0].nHandle, c_block570[0].slotLetter, &addr) != S_OK)
                  {
                     printf("\nUnable to Get IOS I/O Address.\n");
                     c_block570[0].bInitialized = FALSE;
					 break;
                  }
                  else
                  {
                     c_block570[0].brd_ptr = (struct map570 *)addr;
                  }

/*
    Get the IOS memory base address based on the Slot letter,
    and initialize the IOS data structure with the returned address
*/
                  if(GetIOSMemoryAddress(c_block570[0].nHandle, c_block570[0].slotLetter, &addr2) != S_OK)
                  {
                     printf("\nUnable to Get IOS Memory Address.\n");
                     c_block570[0].bInitialized = FALSE;
                  }
                  else
                  {
                     c_block570[0].brd_memptr = (byte*)addr2;/* ch 0 addr */
                     c_block570[0].bInitialized = TRUE;

                     /* read board id information */
                     ReadIOSID(c_block570[0].nHandle, c_block570[0].slotLetter,
      				    &c_block570[0].idProm[0], sizeof c_block570[0].idProm / 2 ); /* read from carrier */
 
                     /* initialize the remaining config block using c_block570[0] as the source */
                     memcpy(&c_block570[1], &c_block570[0], sizeof(struct cblk570)); /* ch 1 */
                     c_block570[1].brd_memptr = (byte*)(addr2 + CH1_REG_OFFSET);/* ch 1 addr */
                     c_block570[1].controller = 1;        /* mark as ch number 1 */
                  }
               }
            }
        break;
        
        case 4: /* Examine/Change Current Channel */
        	selectch570( &ch_number);
        break;
        
        case 5: /* select configuration parameters */
            scfg570(&c_block570[ch_number]);

            /* insure module clock speed settings remain the same in both structures */
            c_block570[0].bus_clock = c_block570[1].bus_clock = c_block570[ch_number].bus_clock;
            /* insure module TagClockSource settings remain the same in both structures */
            c_block570[0].TagClockSource = c_block570[1].TagClockSource = c_block570[ch_number].TagClockSource;
            /* insure module interrupt vectors remain the same in both structures */
            c_block570[0].vector = c_block570[1].vector = c_block570[ch_number].vector;
        break;

        case 6: /* configure board command */
            if(!c_block570[ch_number].bInitialized)
                printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
            {
                status = init570(&c_block570[ch_number]); /* routine to init the board */
                if( status != 0)
                    printf("\nInitialization error %d\n",status);
            }
        break;

        case 7: /* board id PROM */
              if(!c_block570[ch_number].bInitialized)
                  printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
              else
              {
                /* read board id information */
                ReadIOSID(c_block570[ch_number].nHandle, c_block570[ch_number].slotLetter,
				    &c_block570[ch_number].idProm[0], sizeof c_block570[ch_number].idProm / 2 ); /* read from carrier */

                printf("\n\nBoard ID Information");
                printf("\nIdentification:              ");
                for(i = 0; i < 4; i++)		/* identification */
                    printf("%c",c_block570[ch_number].idProm[i]);

                printf("\nManufacturer's ID:           %X",(byte)(c_block570[ch_number].idProm[4]));
                printf("\nIOS Model Number:             %X",(byte)(c_block570[ch_number].idProm[5]));
                printf("\nRevision:                    %X",(byte)(c_block570[ch_number].idProm[6]));
/*                printf("\nReserved:                    %X",(byte)(c_block570[ch_number].idProm[7]));
                printf("\nDriver I.D. (low):           %X",(byte)(c_block570[ch_number].idProm[8]));
                printf("\nDriver I.D. (high):          %X",(byte)(c_block570[ch_number].idProm[9]));
*/                printf("\nTotal I.D. Bytes:            %X",(byte)(c_block570[ch_number].idProm[10]));
                printf("\nCRC:                         %X\n",(byte)(c_block570[ch_number].idProm[11]));
              }
        break;

        case 8: /* enable interrupts */
        	/*    attach handler */

	        EnableInterrupts( c_block570[0].nHandle );
        break;

        case 9: /* detach exception handlers */
            DisableInterrupts(c_block570[0].nHandle);
        break;

        case 10: /* 1553 functions */
            finished1 = 0;	 /* indicate not finished */
            while(!finished1) {
                printf("\n1553 Functions\n");
                printf(" 1. Return to Previous Menu\n");
                printf(" 2. aceTest\n");
                printf(" 3. Loopback Test\n");
                printf(" 4. BcDemo\n");
                printf(" 5. BcdBuf\n");
                printf(" 6. RtMode\n");
                printf(" 7. RtPoll\n");
                printf(" 8. RtIrq\n");
                printf(" 9. RtdBuf\n");
                printf("10. MtPoll\n");
                printf("11. MtIrq\n");        
                printf("12. RtMtDemo\n");
                printf("13. RtMtCmbDemo\n");
                printf("Select: ");
                scanf("%d",&item);

                /* perform the menu item selected. */  
                switch(item) {
                case 1: /* exit program command */
                    printf("Return to Previous Menu(y/n)?: ");
                    scanf("%s",cmd_buff);
                    if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
                        finished1++;
                break;

                case 2:	/* aceTest */
                    if(!c_block570[ch_number].bInitialized)
                       printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                       aceTest((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               (char*)&IPIOS57X, c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 3: /* looptest */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        looptest((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                                 (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                                 c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 4: /* BcDemo */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        BcDemo((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr +  MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 5: /* BcdBuf */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        BcdBuf((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 6: /* RtMode */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        RtMode((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 7: /* RtPoll */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        RtPoll((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 8: /* RtIrq */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        RtIrq((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                              (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                              c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 9: /* RtdBuf */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                    	RtdBuf((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                              (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 10: /* MtPoll */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        MtPoll((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                               (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                               c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;
                
                case 11: /* MtIrq */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        MtIrq((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                              (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                              c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 12: /* RtMtDemo */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        RtMtDemo((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                                 (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                                 c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;

                case 13: /* RtMtCmbDemo */
                    if(!c_block570[ch_number].bInitialized)
                        printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
                    else
                        RtMtCmbDemo((unsigned long) 0x10000, (unsigned long) c_block570[ch_number].brd_memptr,
                                    (unsigned long) ((char*)c_block570[ch_number].brd_memptr + MEM_OFFSET),
                                    c_block570[ch_number].nHandle, (int)c_block570[ch_number].slotLetter, ch_number );
                break;
                }
            }
        break;
        }   /* end of switch */
    }   /* end of while */
	
/*
    disable interrupts from IOS module
*/

    DisableInterrupts(c_block570[0].nHandle);
    if(c_block570[0].bCarrier)
	CarrierClose(c_block570[0].nHandle);

    printf("\nEXIT PROGRAM\n");
    return(0);
}   /* end of main */



/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr570.c

    MODULE NAME:    get_param - get a parameter from the console

    VERSION:        A

    CREATION DATE:  01/05/95

    DESIGNED BY:    RTL

    CODED BY:       RTL

    ABSTRACT:       Routine which is used to get parameters

    CALLING
        SEQUENCE:   get_param();

    MODULE TYPE:    long

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:  

    MODULES
        CALLED:     

    REVISIONS:      

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


long get_param()
{

/*
    declare local storage.
*/

    int temp;

    printf("enter hex parameter: ");
    scanf("%x",&temp);
    printf("\n");
    return((long)temp);
}


/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr570.c

    MODULE NAME:    selectch570 - Select channel.

    VERSION:	    A

    CREATION DATE:  03/22/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine selects current channel on the 570 module.

    CALLING
        SEQUENCE:   selectch570(&current_channel)
		    where:
			current_channel (pointer)
			  The address of the current_channel variable to update

    MODULE TYPE:    void

    I/O RESOURCES:

    SYSTEM
	RESOURCES:

    MODULES
	CALLED:

    REVISIONS:

  DATE	  BY	    PURPOSE
-------  ----	------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:
*/

void selectch570(int *current_channel)

{

/*
    DECLARE LOCAL DATA AREAS:
*/
    int item, cntr;             /* menu item variable */
    unsigned finished;          /* flag to exit loop */

/*
    ENTRY POINT OF ROUTINE:
*/
    finished = 0;
    while(!finished)
    {
      printf("\n\nCurrent Channel: %x\n\n",*current_channel);
      printf(" 1. Return to Previous Menu\n");
      printf(" 2. Change Channel\n");
      printf("\nselect: ");
      scanf("%d",&item);
      switch(item)
      {
        case 1: /* return to previous menu */
	        finished++;
	    break;

        case 2: /* Select channel */
            printf("Enter New Channel Number (0 - 1): ");
            scanf("%x",&cntr);
            *current_channel = (cntr & 1);
	    break;
     }
  }
}

/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr570.c

    MODULE NAME:    scfg570 - set configuration block contents.

    VERSION:        A

    CREATION DATE:  03/19/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine which is used to enter parameters into
		    the Configuration Block.
 
    CALLING
        SEQUENCE:   scfg570(c_block570)
                    where:
                        c_block570 (structure pointer)
                          The address of the configuration param. block

    MODULE TYPE:    void

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:  

    MODULES
        CALLED:

    REVISIONS:      

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:
*/

void scfg570(struct cblk570 *c_blk)

{

/*
    DECLARE LOCAL DATA AREAS:
*/
    int item;                   /* menu item variable */
    unsigned finished;		    /* flags to exit loops */
    int status;
    word speed; 				/* storage for retreived data */
    
/*
    ENTRY POINT OF ROUTINE:
*/
    finished = 0;
    while(!finished)
    {
        printf("\n\nConfiguration Block Parameters For Channel %X\n\n",c_blk->controller);
        printf(" 1. Return to Previous Menu\n");
        printf(" 2. Board I/O Pointer:     0x%lX\n",(unsigned long)c_blk->brd_ptr);
        printf("    Board MEM Pointer:     0x%lX\n",(unsigned long)c_blk->brd_memptr);
        printf(" 3. Transceiver Inhibit A: 0x%02X\n",c_blk->TransceiverInhibitA);
        printf(" 4. Transceiver Inhibit B: 0x%02X\n",c_blk->TransceiverInhibitB);
        printf(" 5. Master Clear:          0x%02X\n",c_blk->MasterClear);
        printf(" 6. Self Test Enable:      0x%02X\n",c_blk->SelfTestEnable);
        printf(" 7. RT Address Latch:      0x%02X\n",c_blk->RemoteTerminalAddressLatch);
        printf(" 8. Tag Clock Source:      0x%02X\n",c_blk->TagClockSource);
        printf(" 9. Interrupt Enable:      0x%02X\n",c_blk->InterruptEnable);
        printf("10. Interrupt Vector:      0x%02X\n",c_blk->vector);
        printf("11. Module Clock Speed:    0x%02X\n",c_blk->bus_clock);
        status = GetIOSClockControl(c_blk->nHandle, c_blk->slotLetter, &speed);
        printf("12. Carrier Clock Speed:   0x%02X\n",speed);
        printf("\nSelect: ");
        scanf("%d",&item);

        switch(item){
        
        case 1: /* return to previous menu */
            finished++;
            break;

        case 2: /* board address */
            printf("BOARD ADDRESS CAN BE CHANGED ONLY IN THE MAIN MENU\n");
            break;
            
        case 3: /* Transceiver Inhibit A */
        	c_blk->TransceiverInhibitA = (byte)get_param();
            break;
			
        case 4: /* Transceiver Inhibit B */
        	c_blk->TransceiverInhibitB = (byte)get_param();
            break;
			
        case 5: /* Master Clear */
        	c_blk->MasterClear = (byte)get_param();
            break;
			
        case 6: /* Built In Self Test Enable */
        	c_blk->SelfTestEnable = (byte)get_param();
            break;

        case 7: /* Remote Terminal Address Latch */
        	c_blk->RemoteTerminalAddressLatch = (byte)get_param();
            break;

        case 8: /* Tag Clock Source */
            c_blk->TagClockSource = (byte)get_param();
            break;

        case 9: /* Interrupt Enable */
        	c_blk->InterruptEnable = (byte)get_param();
            break;

        case 10: /* Interrupt Vector */
        	c_blk->vector = (byte)get_param();
            break;
            
		case 11: /* Module Clock Speed */
            printf("1 - 8MHZ Clock to Module\n");
            printf("2 - 32MHZ Clock to Module\n\n");
            c_blk->bus_clock = (byte)get_param();
            break;

		case 12: /* carrier clock select */
            /* call carrier API to get slot clock speed info */
            status = GetIOSClockControl(c_blk->nHandle, c_blk->slotLetter, &speed);
            if( status )
              printf("\nSorry... Carrier can not change the module clock setting\n");
            else   /* call carrier API to set slot clock speed */
            {
              printf("0 - 8MHZ Module Clock From Carrier\n");
              printf("1 - 32MHZ Module Clock From Carrier\n\n");
              SetIOSClockControl(c_blk->nHandle, c_blk->slotLetter, (int)get_param());
            }
            break;
        }
    }
}

/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr570.c

    MODULE NAME:    init570()

    VERSION:        A

    CREATION DATE:  03/19/10

    CODED BY:       FJM

    ABSTRACT:       The module is used to initialize the board.
                        
    CALLING
	    SEQUENCE:   status = init570(init570(struct cblk570 *c_blk);
                          where:
                            status (int)
                              The returned status of the function, 0 on success
							  -1 on PLL lock error, -2 hardware error.
							ptr (pointer to structure)
							  Pointer to the configuration block structure.
                            
    MODULE TYPE:    int

    I/O RESOURCES:

    SYSTEM RESOURCES:

    MODULES CALLED:

    REVISIONS:

    DATE	   BY    	PURPOSE
    --------   -----	---------------------------------------------------

{-D}
*/



/*  
    MODULES FUNCTION DETAILS

    The module is used to initialize the board.
    The returned status of the function, 0 on success -1 on PLL lock error,
    -2 hardware error.
*/

int init570(struct cblk570 *c_blk)
{
   int status;
   int wait;
   word temp;

   /* Select the bus clock frequency for the module 8MHz/32MHz */
   temp = input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg);
   temp &= 0x3FFF;  /* clear control register bits 15 & 14 */

   /* write the cleared bits back to the board to stop the bus clock before changing to the new clock value */
   output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg, temp);
   temp |= (word)((c_blk->bus_clock & 3) << 14);   /* OR in the bus clock bits */
   output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg, temp);
    
   status = -1;	/* indicate PLL lock incomplete */

   /* Read lock bit, waiting for the PLL to lock */
   for( wait = 75; wait; wait-- )
   {
      if ( input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->StatusReg) & 0x8000)
          break;

      usleep((unsigned long) 1000); /* 1000uS */
   }

   if( wait )     /* obtained PLL lock */
   {
           if( c_blk->controller & 1)	/* second channel */
	   {
		   /* Interrupt vector B */
           output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->IntVectBReg, (word)c_blk->vector);
 
           temp = input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg);
           temp &= 0xC03F;  /* clear bits 13 thru 6 */
           /* Transceiver Inhibit A */
           temp |= (c_blk->TransceiverInhibitA & 1) << 13;  /* OR in bit 13 setting */          
           /* Transceiver Inhibit B */
           temp |= (c_blk->TransceiverInhibitB & 1) << 12;  /* OR in bit 12 setting */         
           /* Master Clear */
           temp |= (c_blk->MasterClear & 1) << 11;			/* OR in bit 11 setting */         
           /* Built In Self Test Enable */            
           temp |= (c_blk->SelfTestEnable & 1) << 10;		/* OR in bit 10 setting */   	
           /* Remote Terminal Address Latch */
           temp |= (c_blk->RemoteTerminalAddressLatch & 1) << 9;/* OR in bit 9 setting */
           /* Interrupt Enable */
           temp |= (c_blk->InterruptEnable & 1) << 8;		/* OR in bit 8 setting */
           /* Tag Clock Source */
           temp |= (c_blk->TagClockSource & 1) << 7;		/* OR in bit 7 setting */
           output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg, (word)temp);
           /* Read back the interrupt vector register, the value is checked later... */ 
           temp = input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->IntVectBReg);
	   }
	   else
	   {
		   /* Interrupt vector A */
           output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->IntVectAReg, (word)c_blk->vector);        

           temp = input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg);
           temp &= 0xFF00;	/* clear bits 7 thru 0 */
           /* Tag Clock Source */
           temp |= (c_blk->TagClockSource & 1) << 7;/* OR in bit 7 setting */
           /* Transceiver Inhibit A */
           temp |= (c_blk->TransceiverInhibitA & 1) << 5;  /* OR in bit 5 setting */          
           /* Transceiver Inhibit B */
           temp |= (c_blk->TransceiverInhibitB & 1) << 4;  /* OR in bit 4 setting */         
           /* Master Clear */
           temp |= (c_blk->MasterClear & 1) << 3;			/* OR in bit 3 setting */         
           /* Built In Self Test Enable */            
           temp |= (c_blk->SelfTestEnable & 1) << 2;		/* OR in bit 2 setting */   	
           /* Remote Terminal Address Latch */
           temp |= (c_blk->RemoteTerminalAddressLatch & 1) << 1;/* OR in bit 1 setting */
           /* Interrupt Enable */
           temp |= (c_blk->InterruptEnable & 1);			/* OR in bit 0 setting */          
           output_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->ControlReg, (word)temp);
           /* Read back the interrupt vector register, the value is checked later... */ 
           temp = input_word(c_blk->nHandle, (word*)&c_blk->brd_ptr->IntVectAReg);
	   }      
         
      /* Check the interrupt vector register value... this is to catch the empty slot condition */
      if( (byte)temp != c_blk->vector)
           status = -2;
      else
           status = 0;     /* indicate success */
   }
   return(status);
}
