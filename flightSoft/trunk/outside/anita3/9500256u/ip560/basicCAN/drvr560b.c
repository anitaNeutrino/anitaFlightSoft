
#include <ctype.h>
#include "../../carrier/carrier.h"
#include "ip560b.h"


#define INTERRUPT_LEVEL 5	/* level or bus IRQ - may not be used on all systems */
#define VECTOR 192			/* interrupt vector - may not be used on all systems */


/*
{+D}
    SYSTEM:         Software for IP560

    FILENAME:       drvr560b.c

    MODULE NAME:    main - main routine of example software.

    VERSION:        C

    CREATION DATE:  03/19/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       This module is the main routine for the example program
                    which demonstrates how the IP560 Library is used.

    CALLING
        SEQUENCE:   

    MODULE TYPE:    void

    I/O RESOURCES:  

    SYSTEM
        RESOURCES:  

    MODULES
        CALLED:     

    REVISIONS:      

  DATE	     BY	    PURPOSE
  --------  ----    ------------------------------------------------
  04/01/11   FJM    Changed carrier include to carrier.h
  05/26/11   FJM    Fedora 15 update

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

    This module is the main routine for the example program
    which demonstrates how the IP560 Library is used.
*/



int main()

{
    

/*
    DECLARE LOCAL DATA AREAS:
*/

    char cmd_buff[40];   /* command line input buffer */
    int item;            /* menu item selection variable */
    int i;               /* loop counter */
    int status;          /* returned status of driver routines */
    unsigned finished;   /* flag to exit program */
    long addr, addr2;    /* hold boards addresses */
    long flag;           /* general flag for exiting loops */
    struct cblk560 c_block560[2]; /* configuration block */
    unsigned short s_mask; /* support memory mask */
    int current_channel;

/*
    ENTRY POINT OF ROUTINE:
    INITIALIZATION
*/
    flag = 0;	 	 /* indicate board address not yet assigned */
    finished = 0;	 /* indicate not finished with program */
    current_channel = 0;
/*
    Initialize the Configuration Parameter Blocks to default values.
*/
    memset(&c_block560[0], 0, sizeof(struct cblk560)); /* ch 0 */
    c_block560[0].acr = OURID;        /* Acceptance Code register */
    c_block560[0].amr = ACCEPTMASK;   /* Acceptance Mask Register */

/*
    Load the struct cblk560 btr0 and btr1 members with bit timming values from
    the requested baud index 6.
    500 Kbps   - 24MHz Fin, BRP of  2, sample point @ 83.33%, SJW of 3
*/
    sja1000_get_btrs(&c_block560[0], (unsigned int)6 );

    c_block560[0].cdr = 0x48;          /* Clock Divider register (CBP set, CLKOUT pin off) */
    c_block560[0].ocr = 0x1A;          /* Output Control register */
    c_block560[0].cr = 0x00;           /* Control register */
    c_block560[0].cmr = 0x0C;          /* Command register */
    c_block560[0].vector = VECTOR;     /* interrupt vector */
    c_block560[0].bCarrier = FALSE;	/* indicate no carrier initialized and set up yet */
    c_block560[0].bInitialized = FALSE;/* indicate not ready to talk to IP */
    c_block560[0].slotLetter = SLOT_A;
    c_block560[0].bus_clock = 1;       /* default 8MHz */
    c_block560[0].transceiver_enable = 1;   /* transceiver enabled */
    c_block560[0].transceiver_standby = 1;  /* transceiver active */
    c_block560[0].nHandle = -1;        /* make handle to a closed carrier board */

    c_block560[0].CAN_TxMsg.id = 0x555;	/* default ID */
    c_block560[0].CAN_TxMsg.len = 0x08;	/* default length */
    
    /* the remainder of ch 1 structure initialization is done at the end of menu selection 3 */
    memset(&c_block560[1], 0, sizeof(struct cblk560)); /* ch 1 */
    
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
    if(CarrierOpen(0, &c_block560[0].nHandle) != S_OK)
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

        printf("\nIP560 BasicCAN Library Demonstration  Version A\n");
        printf(" 1. Exit this Program\n");
        printf(" 2. Set Carrier Base Address\n");
        printf(" 3. Set IP Slot Letter\n");
        printf(" 4. Examine/Change CAN Channel\n");
        printf(" 5. Set Up Configuration Block Parameters\n");
        printf(" 6. Configure Board Command\n");
        printf(" 7. Read ID PROM\n");
        printf(" 8. Enable Interrupts\n");
        printf(" 9. Disable Interrupts\n");
        printf("10. Display Controller Registers\n");
        printf("11. Read Message\n");
        printf("12. Edit Transmit Buffer\n");
        printf("13. Transmit Message");

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
		  SetCarrierAddress(c_block560[0].nHandle, addr);	/* Set Carrier Address */
		}
		GetCarrierAddress(c_block560[0].nHandle, &addr);	/* Read back carrier address */
		printf("I/O Address: %lX\n",addr);
		printf("is this value correct(y/n)?: ");
		scanf("%s",cmd_buff);
		if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
		{
		  SetCarrierAddress(c_block560[0].nHandle, addr);	/* Set Carrier Address */
		  if(CarrierInitialize(c_block560[0].nHandle) == S_OK)/* Initialize Carrier */
		  {
		    c_block560[0].bCarrier = TRUE;
			SetInterruptLevel(c_block560[0].nHandle, INTERRUPT_LEVEL);/* Set carrier interrupt level */
		  }
		  flag = 1;
		}
		else
		  flag = 0;

	    }while( cmd_buff[0] != 'y' && cmd_buff[0] != 'Y' );

        /* Make sure this carrier supports IP memory space */

        GetCarrierID(c_block560[0].nHandle, &s_mask);	/* see if carrier can support memory I/O */
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

            /* Set IP Memory Address */
            SetMemoryAddress( c_block560[0].nHandle, (long)addr2);	/* Set IP Memory Address */
		  }
          GetMemoryAddress(c_block560[0].nHandle, (long*)&addr2);
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

        case 3: /* set IP Slot Letter */
            if(flag == 0 || c_block560[0].bCarrier == FALSE)
               printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
            {
               printf("\n\nEnter IP slot letter (A, B etc...): ");
               scanf("%s",cmd_buff);
               cmd_buff[0] = toupper(cmd_buff[0]);
               if(cmd_buff[0] < 'A' || cmd_buff[0] > GetLastSlotLetter())
               {
                  printf("\nInvalid Slot Letter!\n");
                  c_block560[0].bCarrier = FALSE;
               }
               else
               {
                  c_block560[0].slotLetter = cmd_buff[0];
/*
    Get the IPACK's base I/O address based on the Slot letter,
    and initialize the IPACK's data structure with the returned address
*/
                  if(GetIpackAddress(c_block560[0].nHandle, c_block560[0].slotLetter, &addr) != S_OK)
                  {
                     printf("\nUnable to Get Ipack I/O Address.\n");
                     c_block560[0].bInitialized = FALSE;
					 break;
                  }
                  else
                  {
                     c_block560[0].brd_ptr = (struct map560b *)addr;
                  }

/*
    Get the IPACK's memory base address based on the Slot letter,
    and initialize the IPACK's data structure with the returned address
*/
                  if(GetIpackMemoryAddress(c_block560[0].nHandle, c_block560[0].slotLetter, &addr2) != S_OK)
                  {
                     printf("\nUnable to Get Ipack Memory Address.\n");
                     c_block560[0].bInitialized = FALSE;
                  }
                  else
                  {
                     c_block560[0].brd_memptr = (struct mapmem560b *)addr2;
                     c_block560[0].bInitialized = TRUE;
                  }
               }
               /* complete the initialization of the ch 1 structure by copying the common items in ch0 to ch 1 structure */
               memcpy(&c_block560[1], &c_block560[0], sizeof(struct cblk560));
               c_block560[1].controller = 1;        /* mark as ch number 1 */
            }
        break;

        case 4: /* select channel */
            selectch560(&current_channel);
        break;      

        case 5: /* select configuration parameters */
            scfg560(&c_block560[current_channel]);

            /* insure module clock speed settings remain the same in both structures */
            c_block560[0].bus_clock = c_block560[current_channel].bus_clock;
            c_block560[1].bus_clock = c_block560[current_channel].bus_clock;

            /* insure module interrupt vectors remain the same in both structures */
            c_block560[0].vector = c_block560[current_channel].vector;
            c_block560[1].vector = c_block560[current_channel].vector;
        break;

        case 6: /* configure board command */
        
            if(!c_block560[current_channel].bInitialized)
                printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
            {
               status = sja1000init(&c_block560[current_channel]);  /* routine to init the board */
               if( status != 0)
                   printf("\nSJA1000 initialization error %d\n",status);
            }
        break;

        case 7: /* board id PROM */
        
              if(!c_block560[current_channel].bInitialized)
                  printf("\n>>> ERROR: BOARD NOT SET UP <<<\n");
              else
              {
                /* read board id information */
                ReadIpackID(c_block560[current_channel].nHandle, c_block560[current_channel].slotLetter,
				    &c_block560[current_channel].idProm[0], sizeof c_block560[current_channel].idProm / 2 ); /* read from carrier */

                printf("\nBoard ID Information\nIdentification:      ");
                for(i = 0; i < 4; i++)		/* identification */
                    printf("%c",c_block560[current_channel].idProm[i]);

                printf("\nManufacturer's ID:   %X",(byte)(c_block560[current_channel].idProm[4]));
                printf("\nIP Model Number:     %X",(byte)(c_block560[current_channel].idProm[5]));
                printf("\nRevision:            %X",(byte)(c_block560[current_channel].idProm[6]));
                printf("\nDriver I.D. (low):   %X",(byte)(c_block560[current_channel].idProm[8]));
                printf("\nDriver I.D. (high):  %X",(byte)(c_block560[current_channel].idProm[9]));
                printf("\nTotal I.D. Bytes:    %X",(byte)(c_block560[current_channel].idProm[10]));
                printf("\nCRC:                 %X\n",(byte)(c_block560[current_channel].idProm[11]));
              }
        break;

        case 8:     /* enable interrupts */
            EnableInterrupts( c_block560[current_channel].nHandle );
            c_block560[0].hflag = c_block560[1].hflag = 1;
        break;

        case 9: /* detach exception handlers */
            DisableInterrupts(c_block560[current_channel].nHandle);
            c_block560[0].hflag = c_block560[1].hflag = 0;
        break;

        case 10: /* controller register display */
            if(!c_block560[current_channel].bInitialized)
                  printf("\n>>> ERROR: BOARD NOT SET UP <<<\n");
            else
            {
                sja1000reg_dump(&c_block560[current_channel]);	/* controller register display */
            }
        break;

        case 11: /* Read message */

            if(!c_block560[current_channel].bInitialized)
                printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
                getMsg(&c_block560[current_channel]);
        break;

        case 12: /* Edit Transmit Buffer */
              edit_xmit_buffer(&c_block560[current_channel]);
        break;

        case 13: /* Write message */
            if(!c_block560[current_channel].bInitialized)
                printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
               sja1000_xmit (&c_block560[current_channel]);    /* call xmit routine */
        break;
            
        case 14: /* Silly message */           
            if(!c_block560[current_channel].bInitialized)
                printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
            else
                putMsg(&c_block560[current_channel]);
        break;           
        }   /* end of switch */
    }   /* end of while */
	
/*
    disable interrupts from IP module
*/
    if(c_block560[current_channel].bInitialized)	  /* module address was set */
    {
       sja1000init(&c_block560[current_channel]);   /* routine to configure the board */
    }

    DisableInterrupts(c_block560[current_channel].nHandle);
    if(c_block560[current_channel].bCarrier)
       CarrierClose(c_block560[current_channel].nHandle);

    printf("\nEXIT PROGRAM\n");
    return(0);
}   /* end of main */



/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr560b.c

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

    FILENAME:       drvr560b.c

    MODULE NAME:    scfg560 - set configuration block contents.

    VERSION:        A

    CREATION DATE:  03/19/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine which is used to enter parameters into
		    the Configuration Block.
 
    CALLING
        SEQUENCE:   scfg560(c_block560)
                    where:
                        c_block560 (structure pointer)
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

void scfg560(struct cblk560 *c_blk)

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
        printf(" 2. Board I/O Pointer:  0x%lX\n",(unsigned long)c_blk->brd_ptr);
        printf("    Board MEM Pointer:  0x%lX\n",(unsigned long)c_blk->brd_memptr);
        printf(" 3. Message ID:         0x%03X\n",c_blk->CAN_TxMsg.id);
        printf(" 4. Message Length:     0x%02X\n",c_blk->CAN_TxMsg.len);
        printf(" 5. Acceptance Code:    0x%02X\n",c_blk->acr);
        printf(" 6. Acceptance Mask:    0x%02X\n",c_blk->amr);
        printf(" 7. Bus Timing 0:       0x%02X\n",c_blk->btr0);
        printf(" 8. Bus Timing 1:       0x%02X\n",c_blk->btr1);
        printf(" 9. Output Control:     0x%02X\n",c_blk->ocr);
        printf("10. Clock Divider:      0x%02X\n",c_blk->cdr);
        printf("11. Control:            0x%02X\n",c_blk->cr);
        printf("12. Command:            0x%02X\n",c_blk->cmr);
        printf("13. Interrupt Vector:   0x%02X\n",c_blk->vector);
        printf("14. Transceiver Enable: 0x%02X\n",c_blk->transceiver_enable);
        printf("15. Transceiver Active: 0x%02X\n",c_blk->transceiver_standby);
        printf("16. Module Clock Speed: 0x%02X\n",c_blk->bus_clock);
        status = GetIPClockControl(c_blk->nHandle, c_blk->slotLetter, &speed);
        printf("17. Carrier Clock Speed:0x%02X\n",speed);

        printf("\nSelect: ");
        scanf("%d",&item);

        switch(item){
        
        case 1: /* return to previous menu */
            finished++;
            break;

        case 2: /* board address */
            printf("BOARD ADDRESS CAN BE CHANGED ONLY IN THE MAIN MENU\n");
            break;

        case 3: /* message id */
        	c_blk->CAN_TxMsg.id = (word)get_param();
            break;

        case 4: /* Message Length */
        	c_blk->CAN_TxMsg.len = (byte)get_param();
            break;

        case 5: /* Acceptance Code */
            c_blk->acr = (byte)get_param();
            break;

        case 6: /* Acceptance Mask */
            c_blk->amr = (byte)get_param();
            break;

        case 7: /* Bus Timing 0 */
            c_blk->btr0 = (byte)get_param();
            break;

        case 8: /* Bus Timing 1 */
            c_blk->btr1 = (byte)get_param();
            break;

        case 9: /* Output Control */
            c_blk->ocr = (byte)get_param();
            break;

        case 10: /* Clock Divider */
            c_blk->cdr = (byte)get_param();
            break;

        case 11: /* Control */
            c_blk->cr = (byte)get_param();
            break;

        case 12: /* Command */
            c_blk->cmr = (byte)get_param();
            break;

        case 13: /* exception vector */
            c_blk->vector = (byte)get_param();
            break;
			
        case 14: /* transceiver enable enabled/disabled */
            c_blk->transceiver_enable = (byte)get_param();
            break;

        case 15: /* transceiver standby standby/active */
            c_blk->transceiver_standby = (byte)get_param();
            break;

		case 16: /* Module Clock Speed */
            printf("1 - 8MHZ Clock to Module\n");
            printf("2 - 32MHZ Clock to Module\n\n");
            c_blk->bus_clock = (byte)get_param();
            break;

		case 17: /* carrier clock select */
            /* call carrier API to get slot clock speed info */
            status = GetIPClockControl(c_blk->nHandle, c_blk->slotLetter, &speed);
            if( status )
              printf("\nSorry... Carrier can not change the module clock setting\n");
            else   /* call carrier API to set slot clock speed */
            {
              printf("0 - 8MHZ Module Clock From Carrier\n");
              printf("1 - 32MHZ Module Clock From Carrier\n\n");
              SetIPClockControl(c_blk->nHandle, c_blk->slotLetter, (int)get_param());
            }
            break;
        }
    }
}



/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr560b.c

    MODULE NAME:    edit_xmit_buffer

    VERSION:        A

    CREATION DATE:  03/22/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine which is used to do edit the xmit buffer.

    CALLING
	SEQUENCE:   edit_xmit_buffer(&c_block560)
                    where:
                       c_block560 (structure pointer)
                          The address of the configuration/status param. block

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




int ishexdigit(c)
{
  return( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') );
}





void edit_xmit_buffer(struct cblk560 *c_blk)

{


/*
    DECLARE LOCAL DATA AREAS:
*/
    int item;           /* menu item variable */
    unsigned finished;  /* flags to exit loops */
    byte *mptr;		/* pointer to current memory location */
    int i, j;
    char buf[64];

/*
    ENTRY POINT OF ROUTINE:
*/

    finished = 0;
    while(!finished)
    {
	printf("\nEdit Transmit Buffer Menu\n\n");
	printf(" 1. Return to Previous Menu\n");
	printf(" 2. Fill Buffer\n");
	printf(" 3. Edit Buffer\n");
	printf(" 4. View Buffer\n");
	printf("\nSelect: ");
	scanf("%d",&item);

	switch(item)
        {

	case 1: /* return to previous menu */
	    finished++;
	    break;

	case 2: /* Fill buffer */
        printf("Constant=1 or Incrementing=2 Pattern: ");
	    scanf("%x",&i);
	    if( i == 2 )
	    {
	      for(i = 0; i >= 0 && i < 8; i++)
              c_blk->CAN_TxMsg.data[i] = i;
	    }
	    else
	    {
	      printf("Enter fill value in hex: ");
	      scanf("%x",&i);
	      memset(&c_blk->CAN_TxMsg.data[0], i, 8);
	    }
	    break;

	case 3: /* Edit buffer */
            memset(&buf[0], 0, sizeof(buf));
            printf("\nUse '.' Exit, '+' Next, '-' Previous\n\n");
	    mptr = &c_blk->CAN_TxMsg.data[0];
            i = 0;
eloop:
            while( i >= 0 && i < 8)
            {
	      printf("%08lX: %02X ", (unsigned long)&mptr[i],mptr[i] );/* print address & byte */
	      scanf("%s",&buf[0]);
	      buf[2] = 0;

              if( buf[0] == '+' )
              {
                i++;
                goto eloop;
              }
              if( buf[0] == '-' )
              {
                i--;
                goto eloop;
              }
              if( ishexdigit(buf[0]) )
              {
                if( !ishexdigit(buf[1]) )
		   buf[1] = 0;

		sscanf(buf,"%x", (int*)&j);
		mptr[i++] = (byte)j;
	      }
	      else
		break;
            }
	break;

	case 4: /* View buffer */
            i = 0;
            mptr = &c_blk->CAN_TxMsg.data[0];

            while( i >= 0 && i < 8)
            {
               printf("\n%08lX: ", (unsigned long)&mptr[i] );/* print starting address of line */

               for( j = 8; j; j-- )          /* print line of bytes in hex */
                  printf( "%02X ", mptr[i++] );

            }
        break;
	}
    }
}


/*
{+D}
    SYSTEM:         Software

    FILENAME:       drvr560b.c

    MODULE NAME:    selectch560 - Select channel.

    VERSION:	    A

    CREATION DATE:  03/22/10

    DESIGNED BY:    FJM

    CODED BY:       FJM

    ABSTRACT:       Routine selects current channel on the 560 module.

    CALLING
        SEQUENCE:   selectch560(&current_channel)
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

void selectch560(int *current_channel)

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
