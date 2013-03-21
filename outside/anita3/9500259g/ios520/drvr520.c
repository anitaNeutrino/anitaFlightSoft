
#include <ctype.h>
#include "../ioscarrier/ioscarrier.h"
#include "ios520.h"
#include "portio.h"		/* serial port include file */

#define INTERRUPT_LEVEL 5   /* level or bus IRQ - may not be used on all systems */
#define VECTOR 192          /* interrupt vector - may not be used on all systems */

/*
{+D}
    SYSTEM:         Library Software

    FILENAME:       drvr520.c

    MODULE NAME:    main - main routine of example software.

    VERSION:        B

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

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
--------  ----   ------------------------------------------------
05/26/11  FJM    Fedora 15 update

{-D}
*/


/*
    MODULES FUNCTIONAL DETAILS:

	This module is the main routine for the example program
	which demonstrates how the Library is used.
*/


/*
    DECLARE EXTERNAL DATA AREAS:
*/

/*
    ASCII strings for error codes returned
*/

char *errs[] =
     {
       "",
       "Argument out of range",
       "Function not supported",
       "Hardware not supported",
       "Port is already open",
       "Port not found",
       ""                           /* array terminator */
     };

/*
    ASCII strings for FIFO control, baud rate, and data formats
*/

char *fcs[]  = { "Disabled", "Enabled"};
char *fts[]  = { "8 Bytes", "16 Bytes", "56 Bytes", "60 Bytes"};
char *sbs[]  = { "1", "1 1/2", "2"};
char *pars[] = { "None", "Odd", "Even", "Mark", "Space"};
char *dls[]  = { "5", "6", "7", "8"};
char *brs[]  = { "50", "75", "150", "300", "600", "1200", "2400", "3600", "4800", "7200", "9600",
		     "14400", "19200", "28800", "38400", "57600", "76800", "153600", "230400"};

int main(argc, argv)
int argc; char *argv[];
{

   extern int p_errno;          /* p_open error code */
   extern IO port0;             /* IO structure for port A */
   extern IO port1;             /* IO structure for port B */
   extern IO port2;             /* IO structure for port C */
   extern IO port3;             /* IO structure for port D */
   extern IO port4;             /* IO structure for port E */
   extern IO port5;             /* IO structure for port F */
   extern IO port6;             /* IO structure for port G */
   extern IO port7;             /* IO structure for port H */
   extern struct iostatus iolist[]; /* serial port management structure */

/*
    DECLARE LOCAL DATA AREAS:
*/
    char cmd_buff[128];         /* command line input buffer */
    int item;                   /* menu item selection variable */
    int finished, finished2;    /* flags to exit */
    long addr;                  /* long to hold board address */
    int flag;                   /* general flag for exiting loops */
    int hflag;                  /* interrupt handler installed flag */
    int oflag;                  /* port opened flag */
    IO *io, *itemp;             /* pointers to IO */
    int c;                      /* used by terminal emulator */
    struct int_source i_source; /* interrupt source structure */
    int hstatus;                /* interrupt attach status */
    int port;                   /* Serial IO port number */

/*
    ENTRY POINT OF ROUTINE:
    INITIALIZATION
*/
    if(argc == 2)
      port = atoi(argv[1]);

    if( port < 0 || port > 7)	/* check range of port number arg. */
	port = 0;		/* set default if needed */

    io = NULL;
    flag = 0;		/* indicate board address not yet assigned */
    finished = 0;	/* indicate not finished with program */
    hflag = 0;		/* indicate interrupt handler not installed */
    oflag = 0;		/* indicate port not opened */
    p_errno = 0;

    i_source.bCarrier = FALSE;	  /* indicate no carrier initialized and set up yet */
    i_source.bInitialized = FALSE;/* indicate not ready to talk */
    i_source.slotLetter = SLOT_A;
    i_source.nHandle = 0;	      /* make handle to a closed carrier board */

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
    if(CarrierOpen(0, &i_source.nHandle) != S_OK)
    {
	printf("\nUnable to Open instance of carrier.\n");
	finished = 1;	 /* indicate finished with program */
    }
    else
	flag = 1;

/*
    Enter main loop
*/
    
    while(!finished) {
	printf("\n\nIOS520 Library Demonstration  Version A\n\n");
	printf(" 1. Exit This Program\n");
	printf(" 2. Set IOS Module Base Address\n");
	printf(" 3. Set IOS Slot Letter\n");
	printf(" 4. Read ID PROM Command\n");
	printf(" 5. Attach Exception Handler\n");
	printf(" 6. Open Serial IO Port\n");
	printf(" 7. Set Data Format, Baud Rate\n");
	printf(" 8. RS-232 Control\n");
	printf(" 9. Transmit BREAK Signal\n");
	printf("10. FIFO Control\n");
	printf("11. Loopback Control\n");
	printf("12. Terminal Emulator\n");
	printf("13. Close Serial IO Port\n");
	printf("14. Detach Exception Handler\n");
	printf("\nSelect: ");
	scanf("%d",&item);

	switch(item) {

	case 1: /* exit program command */

	    printf("Exit program(y/n)?: ");
	    scanf("%s",cmd_buff);
	    if( cmd_buff[0] == 'y' || cmd_buff[0] == 'Y' )
		finished++;
	    break;

	case 2: /* set board address command */
		GetCarrierAddress(i_source.nHandle, &addr);	/* Read back carrier address */
		printf("address: %lX\n",addr);
		  if(CarrierInitialize(i_source.nHandle) == S_OK)/* Initialize Carrier */
		  {
		    i_source.bCarrier = TRUE;
		    SetInterruptLevel(i_source.nHandle, INTERRUPT_LEVEL);/* Set carrier interrupt level */
		    flag = 1;

		    /* Place interrupt source address in IO structure */

		    switch(port) /* select structure address */
		    {
		      case 1:
                port1.is_ptr = &i_source;
		      break;

		      case 2:
                port2.is_ptr = &i_source;
		      break;

		      case 3:
                port3.is_ptr = &i_source;
		      break;

		      case 4:
                port4.is_ptr = &i_source;
		      break;

		      case 5:
                port5.is_ptr = &i_source;
		      break;

		      case 6:
                port6.is_ptr = &i_source;
		      break;

		      case 7:
                port7.is_ptr = &i_source;
		      break;

		      default:
                port0.is_ptr = &i_source;
		      break;
		    }
		  }
		  else
		    flag = 0;
        break;

	case 3: /* set IOS Slot Letter */
		if(flag == 0 || i_source.bCarrier == FALSE)
			printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
		else
		{
			printf("\n\nEnter IOS slot letter (A, B etc...): ");
			scanf("%s",cmd_buff);
			cmd_buff[0] = (char)toupper((int)cmd_buff[0]);
			if(cmd_buff[0] < 'A' || cmd_buff[0] > GetLastSlotLetter())
			{
				printf("\nInvalid Slot Letter!\n");
				i_source.bCarrier = FALSE;
			}
			else
			{
				i_source.slotLetter = cmd_buff[0];

/*
	Get the base address based on the Slot letter,
	and initialize the data structure with the returned address
*/
				if(GetIOSAddress(i_source.nHandle, i_source.slotLetter, &addr) != S_OK)
				{
				  printf("\nUnable to Get Address.\n");
				  i_source.bInitialized = FALSE;
				}
				else
				{
				  /* Assign addresses to each of the IO pointers */

				  port0.base_addr = (byte *)addr;
				  port1.base_addr = (byte *)addr + 0x10;
				  port2.base_addr = (byte *)addr + 0x20;
				  port3.base_addr = (byte *)addr + 0x30;
				  port4.base_addr = (byte *)addr + 0x40;
				  port5.base_addr = (byte *)addr + 0x50;
				  port6.base_addr = (byte *)addr + 0x60;
				  port7.base_addr = (byte *)addr + 0x70;
				  i_source.bInitialized = TRUE;
				}
			}
		}
	    break;

	case 4:     /* read board I.D. command */

	    if(!i_source.bInitialized)
		printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
	    else
		psts520(&i_source); /* read board */
	    break;

	case 5:     /* attach exception handler */
	    if(!i_source.bInitialized || oflag == 1)
	       printf("\n>>> ERROR: ADDRESS NOT SET OR PORT ALREADY OPEN <<<\n");
	    else
	    {
	      finished2 = 0;
	      while(!finished2)
	      {
		printf("\n\nAttach Exception Handler\n\n");
		printf("1. Return to Previous Menu\n");
		printf("2. Attach Interrupt Handler\n");
		printf("\nSelect: ");
		scanf("%x",&item);

		switch(item){

		case 1:   /* return to previous menu */
		  finished2++;
		break;

		case 2:   /* attach handler */
		  if(hflag)
		     printf("\n>>> ERROR: HANDLERS ARE ATTACHED <<<\n");
		  else
		  {
		     /* attach the interrupt handler */
		     hstatus = EnableInterrupts(i_source.nHandle);
		     if(hstatus != S_OK)
		     {
			printf(">>> ERROR WHEN ENABLING INTERRUPTS <<<\n");
			hflag = 0;
		     }
		     else
		     {
			hflag = 1;
			printf("port %d Handlers are now attached\n",port);
		     }
 		  }
		break;
		} /* end of switch */
	      } /* end of while */
	    }
	    break;

	case 6: /* open IO Port */
	 if(!i_source.bInitialized)
	    printf("\n>>> ERROR: BOARD ADDRESS NOT SET <<<\n");
	 else
	 {
	   
/* 
   Interrupt handler installed? if so, set iolist to show 
   interrupts are supported 
*/

	   if(hflag != 0)
	     iolist[port].intok = TRUE;
	   
	   if((itemp = p_open(port)) == (void *)0)
	   {
	     printf("\n\nCannot open port number %d, error: %s \n"
		,port, errs[p_errno]);
	   }
	   else
	   {
	     io = itemp;
	     printf("\n\nPort number %d, successfully opened\n",port);
	     oflag = 1;
	   }
	 }
	break;

	case 7: /* setup data format baud, parity, stop bits */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else
	      setup(io);
	break;
	
	case 8: /* RS-232 output control */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else 
	      setup232(io);
	break;

	case 9: /* send BREAK */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else 
	      sendbrk(io);
	break;

	case 10: /* FIFO control */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else 
	      setupfifo(io);
	break;

	case 11: /* loopback control */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else 
	      setuploop(io);
	break;

	case 12: /* terminal emulator */
	    if(!i_source.bInitialized || oflag == 0)
	      printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	    else
	    {
	      finished2 = 0;
	      while(!finished2)
	      {
		printf("\n\n\nTerminal emulator\n");
		printf("\n1 Return to Previous Menu");
		printf("\n2 Transmit String");
		printf("\n3 Receive");
		printf("\nSelect: ");
		scanf("%d",&item);
		switch(item)
		{
		   case 1:		/* quit */
		    finished2++;
		   break;

		   case 2:		/* transmit a string */
gets( &cmd_buff[0] );
		    printf("\n\n\nSend String	<CR> to send\n\n\n");
/*		    scanf("%s",&cmd_buff[0]);*/
gets( &cmd_buff[0] );
/*		    fioRdString (STD_IN, &cmd_buff[0], sizeof cmd_buff - 1);
 */
		    handshake(io,&cmd_buff[0]);
		   break;

		   case 3:		/* receive */
		    printf("\n\n\nReceive	<Ctrl T> to exit\n\n\n");
		    while(1)		/* loop forever */
		    {
		      if((c = p_inchar(io)) != -1)    /* check for byte */
			printf("%c",c );

		      if(c == 20)	/* <Ctrl T> to quit */
			break;
		    }
		    break;
		}
	      }
	    }
	break;

	case 13: /* close IO Port */
	 if(!i_source.bInitialized || oflag == 0)
	   printf("\n>>> ERROR: ADDRESS NOT SET OR PORT NOT OPEN <<<\n");
	 else
	 {
	   if(( p_errno = p_close(io)) != 0)
	     printf("\n\nCannot close port number %d, error: %s \n",port, errs[p_errno]);
	   else
	     printf("\n\nPort number %d, successfully closed\n",port);
	 }
	 oflag = 0;
	break;
	
	case 14: /* detach exception handlers */
		hflag = 0;
		DisableInterrupts(i_source.nHandle);
	break;
	}   /* end of switch */
    }   /* end of while */
 
/*
   If not closed, close the port
*/
    if(oflag != 0)
      p_close(io);

    DisableInterrupts(i_source.nHandle);
    if(i_source.bCarrier)
	CarrierClose(i_source.nHandle);

    printf("\nEXIT PROGRAM\n");
    return(0);
}   /* end of main */


/*
{+D}
    SYSTEM:         Library Software

    FILENAME:       drvr520.c

    MODULE NAME:    psts520 - print board information

    VERSION:        A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Routine which is used to cause the "Read Board I.D."
                    command to be executed and to print out the results to
                    the console.

    CALLING
	SEQUENCE:   psts520(is)
		    where:
			is(structure pointer)
			  The address of the status param. block

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

void psts520(is)
struct int_source *is;
{

/*
    DECLARE LOCAL DATA AREAS:
*/
    int item;           /* menu item variable */
    int i;              /* loop index */
    unsigned finished;  /* flags to exit loops */

/*
    ENTRY POINT OF ROUTINE:
*/

    finished = 0;
    while(!finished)
    {
	rsts520(is);	      /* Read Command */
	printf("\n\nBoard I.D. Information\n\n");
	printf("Identification:         ");
	for(i = 0; i < 4; i++)          /* identification */
	   printf("%c",is->id_prom[i]);
	printf("\nManufacturer's I.D.:	  %X",(byte)is->id_prom[4]);
	printf("\nIOS Model Number:	  %X",(byte)is->id_prom[5]);
	printf("\nRevision:		  %X",(byte)is->id_prom[6]);
	printf("\nReserved:		  %X",(byte)is->id_prom[7]);
	printf("\nDriver I.D. (low):	  %X",(byte)is->id_prom[8]);
	printf("\nDriver I.D. (high):	  %X",(byte)is->id_prom[9]);
	printf("\nTotal I.D. Bytes:	  %X",(byte)is->id_prom[10]);
	printf("\nCRC:			  %X",(byte)is->id_prom[11]);

	printf("\n\n1 Return to Previous Menu\n2 Read Again\n");
	printf("\nselect: ");
	scanf("%d",&item);

	switch(item){

	case 1: /* return to previous menu */
	    finished++;
	    break;
	}
    }
}


/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setup

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Display/change current settings.

    CALLING
	SEQUENCE:   void setup(io)
			Where:
				io (pointer*) to struct IO

    MODULE TYPE:

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


void setup(io)

IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   int choice, errcode;

/*
    ENTRY POINT OF ROUTINE:
*/

   while(1)
   {
     printf("\n\n\nDATA FORMAT MENU\n\n");
     printf("1. Exit This Menu\n2. Read Again\n");
     printf("3. Select Baud Rate: %s\n", brs[getbaud(io)]);
     printf("4. Select Parity:    %s\n", pars[getparity(io)]);
     printf("5. Select Stop Bits: %s\n", sbs[getstops(io)]);
     printf("6. Select Data Bits: %s\n", dls[getdl(io)]);
     printf("\nSelect: ");
     scanf("%d",&choice);
     switch(choice)
     {
       case 1:
	  return;

       case 2:
	  continue;

       case 3:
	  printf("\n\n");
	  printf("0 =    50 baud  10 =   9600 baud\n");
	  printf("1 =    75 baud  11 =  14400 baud\n");
	  printf("2 =   150 baud  12 =  19200 baud\n");
	  printf("3 =   300 baud  13 =  28800 baud\n");
	  printf("4 =   600 baud  14 =  38400 baud\n");
	  printf("5 =  1200 baud  15 =  57600 baud\n");
	  printf("6 =  2400 baud  16 =  76800 baud\n");
	  printf("7 =  3600 baud  17 = 153600 baud\n");
	  printf("8 =  4800 baud  18 = 230400 baud\n");
	  printf("9 =  7200 baud\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setbaud(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 4:
	  printf("\n\n0 = NONE\n1 = ODD\n2 = EVEN\n3 = MARK\n4 = SPACE\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setparity(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 5:
	  printf("\n\n0 = 1 STOP\n1 = 1 1/2 STOP\n2 = 2 STOP\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setstops(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 6:
	  printf("\n\n0 = 5 BITS\n1 = 6 BITS\n2 = 7 BITS\n3 = 8 BITS\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setdatalen(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;
       
       default:
	  printf("\n\nInvalid entry\n");
       break;
     }
   }
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setup232

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Display/change current settings.

    CALLING
	SEQUENCE:   void setup232(io)
			Where:
				io (pointer*) to struct IO

    MODULE TYPE:

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


void setup232(io)

IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   static char *stats[] = {"OFF", "ON", "   "};
   int choice, errcode, rtsstat, dtrstat, gpo1stat, gpo2stat;

/*
    ENTRY POINT OF ROUTINE:
*/

   rtsstat = getrts(io);
   dtrstat = getdtr(io);
   gpo1stat = getgpo1(io);
   gpo2stat = getgpo2(io);

   while(1)
   {
     printf("\n\n\nCURRENT RS-232 INPUT STATUS:");
     printf("\n\nCTS  DSR  DCD  RI");
     printf("\n%-3.3s  %-3.3s  %-3.3s  %-3.3s\n\n",
	stats[getcts(io)], stats[getdsr(io)], 
	stats[getdcd(io)], stats[getri(io)]);

     printf("1. Exit This Menu\n2. Read Again\n");
     printf("3. Toggle RTS:  %s\n", stats[rtsstat]);
     printf("4. Toggle DTR:  %s\n", stats[dtrstat]);
     printf("5. Toggle GPO1: %s\n", stats[gpo1stat]);
     printf("6. Toggle GPO2: %s\n", stats[gpo2stat]);
     printf("\nSelect: ");
     scanf("%d",&choice);
     switch(choice)
     {
       case 1:
	  return;

       case 2:
	  continue;

       case 3:
	  if(rtsstat < 2) 
	    rtsstat = !rtsstat;
	  if((errcode = setrts(io,rtsstat)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 4:
	  if(dtrstat < 2) 
	     dtrstat = !dtrstat;
	  if((errcode = setdtr(io,dtrstat)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 5:
	  if(gpo1stat < 2) 
	    gpo1stat = !gpo1stat;
	  if((errcode = setgpo1(io,gpo1stat)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 6:
	  if(gpo2stat < 2) 
	    gpo2stat = !gpo2stat;
	  if((errcode = setgpo2(io,gpo2stat)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       default:
	  printf("\n\nInvalid entry\n");
       break;
     }
   }
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setupfifo

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Display/change current settings.

    CALLING
	SEQUENCE:   void setupfifo(io)
			Where:
				io (pointer*) to struct IO

    MODULE TYPE:

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


void setupfifo(io)

IO *io;                        /* pointer to IO structure */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   int choice, errcode;

/*
    ENTRY POINT OF ROUTINE:
*/

   while(1)
   {
     printf("\n\n\nCURRENT SETTINGS:\n\n");
     printf("1. Exit This Menu\n2. Read Again\n");
     printf("3. Select FIFO Control:       %s\n", fcs[getfifocontrol(io)]);
     printf("4. Select FIFO Trigger Point: %s\n", fts[getfifotrigger(io)]);
     printf("\nEnter selection: ");
     scanf("%d",&choice);
     switch(choice)
     {
       case 1:
	  return;

       case 2:
       continue;

       case 3:
	  printf("\n\n0 = Disabled\n1 = Enabled\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setfifocontrol(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       case 4:
	  printf("\n\n0 = Trigger on  8 Bytes\n1 = Trigger on 16 Bytes");
	  printf("\n2 = Trigger on 56 Bytes\n3 = Trigger on 60 Bytes\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setfifotrigger(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;
       
       default:
	  printf("\n\nInvalid entry\n");
       break;
     }
   }
}



/*
{+D}
    SYSTEM:         Serial Port Software

    MODULE NAME:    setuploop

    VERSION:        V1.0

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:       FM

    ABSTRACT:       Display/change current settings.

    CALLING
	SEQUENCE:   void setuploop(io)
			Where:
				io (pointer*) to struct IO

    MODULE TYPE:

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


void setuploop(io)

IO *io;                         /* pointer to IO structure */

{

/*
    DECLARE LOCAL DATA AREAS:
*/

   int choice, errcode;

/*
    ENTRY POINT OF ROUTINE:
*/

   while(1)
   {
     printf("\n\n\nCURRENT SETTINGS:\n\n");
     printf("1. Exit This Menu\n2. Read Again\n");
     printf("3. Select Loopback Control: %s\n", fcs[getloopback(io)]);
     printf("\nEnter selection: ");
     scanf("%d",&choice);
     switch(choice)
     {
       case 1:
	  return;

       case 2:
       continue;

       case 3:
	  printf("\n\n0 = Disabled\n1 = Enabled\n");
	  printf("\nEnter code for new value and press return: ");
	  scanf("%d",&choice);
	  if((errcode = setloopback(io,choice)) != 0)
	     printf("\nERROR:  %s\n",errs[errcode]);
       break;

       default:
	  printf("\n\nInvalid entry\n");
       break;
     }
   }
}


/*
{+D}
    SYSTEM:	    Library Software

    FILENAME:	    drvr520.c

    MODULE NAME:    handshake - RTS/CTS IO handshaking

    VERSION:	    A

    CREATION DATE:  04/01/09

    DESIGNED BY:    FM

    CODED BY:	    FM

    ABSTRACT:

    CALLING
	SEQUENCE:   handshake(io,c)
		    where:
			io (pointer*) to struct IO
			c (byte) character to send

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

void handshake(io,c)

IO *io;
byte *c;

{

/*
    DECLARE LOCAL DATA AREAS:
*/
    int i;		/* loop index */
    int status;
    int count;

/*
    ENTRY POINT OF ROUTINE:
*/

    setrts(io,1);		/* assert RTS */
    i = 100;			/* timeout counter */
    do
    {
      i--;
      status = getcts(io);	/* read CTS state */
      if(status == TRUE)
	break;
      delay(1); 		/* 55mS delay */
    }while(status != TRUE && i != 0);

    if(status == TRUE && i != 0) /* if ready send */
    {

      count = strlen((char *)c);
      for( i = 0; i < count && count < 128; i ++)
	    p_putc(io ,c[i]);	/* output */

      p_putc(io ,0xd);		/* output <CR> */

      waitsre(io);		/* wait for TSRE */
    }
    setrts(io,0);		/* negate RTS */
 }

