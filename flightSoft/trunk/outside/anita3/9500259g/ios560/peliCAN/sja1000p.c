
#include <stdio.h>
#include <unistd.h>
#include "../../ioscarrier/ioscarrier.h"
#include "ios560p.h"


/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	sja1000init()

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:       The module is used to initialize the board for peliCAN mode I/O.
                        
    CALLING
	    SEQUENCE:   status = sja1000init(struct cblk560 *c_blk);
                          where:
                            status (int)
                              The returned status of the function, 0 on success
							  -1 on PLL lock error, -2 on reset mode error, -3 hardware error.
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

    The module is used to initialize the board for peliCAN mode I/O.
    The returned status of the function, 0 on success -1 on PLL lock error,
    -2 on reset mode error , -3 hardware error.
*/



int sja1000init(struct cblk560 *c_blk)
{
   int status;
   int wait;
   int offset;
   byte temp;

   if( c_blk->controller & 1) /* determine current CAN controller channel */
      offset = 0x200;         /* offset to channel 1 */
   else
      offset = 0;             /* offset to channel 0 */

   /* Disable the controller's interrupts */
   output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_IER, IER_DIS_INT);

   /* Select the bus clock frequency for the module 8/32MHz */
   output_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister1, (c_blk->bus_clock & 3));

   status = -1;	/* indicate PLL lock incomplete */

   /* Read lock bit, waiting for the PLL to lock */
   for( wait = 75; wait; wait-- )
   {
      if ( input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + StatusRegister) & 0x10)
          break;

      usleep((unsigned long) 1000); /* 1000uS */
   }

   if( wait )     /* obtained PLL lock */
   {
     /* Software reset controller mode bit */
     output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_MOD, MOD_RM);

     status = -2;	/* indicate reset incomplete */
   
     /* Read reset mode bit, waiting for the device to enter reset mode */
     for( wait = 75; wait; wait-- )
     {
        if ( input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_MOD) & MOD_RM)
            break;

		usleep((unsigned long) 1000); /* 1000uS */
     }

     if( wait )     /* entered reset mode update registers */
     {
       /* Clock Divider register */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_CDR, c_blk->cdr);

       /* Output Control register */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_OCR, c_blk->ocr);

       /* Acceptance Code Registers */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ACR0, c_blk->acr[0]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ACR1, c_blk->acr[1]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ACR2, c_blk->acr[2]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ACR3, c_blk->acr[3]);

       /* Acceptance Mask Registers */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_AMR0, c_blk->amr[0]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_AMR1, c_blk->amr[1]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_AMR2, c_blk->amr[2]);
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_AMR3, c_blk->amr[3]);

       /* RX Error Code register */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_RXERR, 0);

       /* TX Error Code register  */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_TXERR, 0);

       /* Bus Timing register 0 */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_BTR0, c_blk->btr0);

       /* Bus Timing register 1 */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_BTR1, c_blk->btr1);

       /* Read to clear Error Code Capture register */
       input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ECC);

       if( c_blk->controller & 1)
       {
         /* Interrupt Vector register B */
         output_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + IntVectBRegister, c_blk->vector);

         /* get transceiver B control bits */
         temp = input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister0);
         /* AND off only the transceiver B standby, transceiver B enable, and interrupt enable bits */
         temp &= 0x0E;

         temp |= ( c_blk->transceiver_standby << 5);    /* insert transceiver B standby bit */
         temp |= ( c_blk->transceiver_enable << 6);     /* insert transceiver B enable bit */
         temp |= ( 1 << 7 );                            /* force on interrupt enable bit */
         output_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister0, temp);

         /* Read back the interrupt vector register, the value is checked later... */ 
         temp = input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + IntVectBRegister);
       }
       else
       {
         /* Interrupt Vector register A */
         output_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + IntVectARegister, c_blk->vector);

         /* get transceiver A control bits */
         temp = input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister0);
         /* AND off only the transceiver A standby, transceiver A enable, and interrupt enable bits */
         temp &= 0xE0;

         temp |= ( c_blk->transceiver_standby << 1);    /* insert transceiver A standby bit */
         temp |= ( c_blk->transceiver_enable << 2);     /* insert transceiver A enable bit */
         temp |= ( 1 << 3 );                            /* force on interrupt enable bit */
         output_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister0, temp);

         /* Read back the interrupt vector register, the value is checked later... */ 
         temp = input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + IntVectARegister);
       }

       /* update any mode bits while still in reset mode */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_MOD, (c_blk->mr | CR_RR));

       /* Return to operating mode */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_MOD, (c_blk->mr & (~CR_RR)));

       /* Check the interrupt vector register value... this is to catch the empty slot condition */
       if( temp != c_blk->vector)
           status = -3;
       else
           status = 0;     /* indicate success */
     }
   }
   return(status);
}



/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	void sja1000reg_dump()

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:       The module provides a diagnostic display of the sja1000
                    CAN controllers registers.
                        
    CALLING
	    SEQUENCE:   void sja1000reg_dump(struct cblk560 *c_blk);
                          where:
                            void
                                ptr (pointer to structure)
                                    Pointer to the configuration block structure.
                            
    MODULE TYPE:    void

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

    The module provides a diagnostic display of the sja1000 CAN controllers registers.
*/

void sja1000reg_dump(struct cblk560 *c_blk)
{
   byte sr;
   int offset;
   int item;            /* menu item selection variable */
   unsigned finished;   /* flag to exit program */


    if( c_blk->controller & 1)  /* determine current CAN controller channel */
	   offset = 0x200;         /* offset to channel 1 */
    else
      offset = 0;              /* offset to channel 0 */

    finished = 0;	 /* indicate not finished with loop */

    while(!finished) {
      printf("\nModule Status Registers:\n");
      printf("MSR  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + StatusRegister));
      printf("CR0  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister0));
      printf("CR1  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_ptr + ControlRegister1));
      printf("SJA1000 peliCAN Operating Mode Registers:\n");
      printf("MOD  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_MOD));
      sr = input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_SR);
      printf("SR   = 0x%02X", sr);
      if (sr & SR_RBS) printf (" rxfull");
      if (sr & SR_DOS) printf (" overrun");
      if (sr & SR_TBS) printf (" txready");
      if (sr & SR_TCS) printf (" txdone");
      if (sr & SR_RS)  printf (" rxbusy");
      if (sr & SR_TS)  printf (" txbusy");
      if (sr & SR_ES)  printf (" error");
      if (sr & SR_BS)  printf (" busoff");
      printf("\nIR   = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)(c_blk->brd_memptr + offset + CAN_IR)));
      printf("BTR0 = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_BTR0));
      printf("BTR1 = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_BTR1));
      printf("OCR  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_OCR));
      printf("ALC  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ALC));
      printf("ECC  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ECC));
      printf("EWL  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_EWL));
      printf("RXEC = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_RXERR));
      printf("TXEC = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_TXERR));
      printf("CDR  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_CDR));
      printf("FIR  = 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_FIR));
      printf("ID   = 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID1));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID2));
      printf("\nSFFD = 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(0)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(2)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(4)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(6)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(8)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(10)));
      printf(" 0x%02X", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(12)));
      printf(" 0x%02X\n", input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(14)));
      
      printf(" 1. Return to Previous Menu\n");
      printf("\nSelect: ");
      scanf("%d",&item);

      if( item == 1)
          finished++;
    }
}



/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	sja1000_xmit()

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:       This module is used to transmit a message in peliCAN mode.
                        
    CALLING
	    SEQUENCE:   status = sja1000_xmit (struct cblk560 *c_blk);
                          where:
                            status (int)
                              The returned status of the function, 0 on success.
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

    This module is used to transmit a message in peliCAN mode.
*/



int sja1000_xmit (struct cblk560 *c_blk)
{
  int i, x, status = -1;
  int offset;
  int wait;

  if( c_blk->controller & 1)
    offset = 0x200;
  else
    offset = 0;

  /* Check the transmit buffer status */
  if (input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_SR) & SR_TBS)
  {
    /* Ok to write a message into the transmit buffer... limit message length to 8 or less */
    if (c_blk->CAN_TxMsg.len > 8)
        c_blk->CAN_TxMsg.len = 8;

    /* update id1 & id2 registers */
    output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID1,
	       (c_blk->CAN_TxMsg.id >> 3));

    output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID2,
	       (c_blk->CAN_TxMsg.id << 5));

    /* write the message into the xmit registers */
    for (i = 0, x = 0; i < c_blk->CAN_TxMsg.len; i++, x+=2)
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(x), c_blk->CAN_TxMsg.data[i]);
  
	/* Write frame information register */
    output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_FIR, c_blk->CAN_TxMsg.len);

    /* If interrupts are enabled, interrupt drive the transmitter */
    if(c_blk->hflag)
    {
       /* Enable the controller's transmit interrupt... OR in the IER_TIE bit */
       output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_IER,
		(input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_IER) | IER_TIE));

      /* send the message... blocking until the message is gone... the ISR will wake us up */
      blocking_start_convert_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_CMR, CMR_TR );

      /* Disable the controller's transmit interrupt... AND off the IER_TIE bit */
      output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_IER,
		(input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_IER) & (~IER_TIE)));
    }
    else	/* Interrupts are not enabled, poll for transmission complete bit */
    {
      /* send the message */
        output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_CMR, CMR_TR );

      /* Wait for the transmission to complete */
      for( wait = 2000; wait; wait-- )
      {
        if ( input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_SR) & SR_TCS)
           break;

        usleep((unsigned long) 1000); /* 1000uS */
      }
    }

	/* Check the transmission complete status bit... if set indicate success */
    if (input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_SR) & SR_TCS)
		status = 0;
	else
		status = -2;	/* indicate an unsuccessful message transmission */
  }
  return(status);
}



/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	sja1000_recv()

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:       This module is used to receive a message in peliCAN mode.
                        
    CALLING
	    SEQUENCE:   status = sja1000_recv (struct cblk560 *c_blk);
                    where:
                      status (int)
                        The returned status of the function, 0 on success -1 on no
						message available, and -2 on extended frame not supported
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

    This module is used to receive a message in peliCAN mode.
	-1 = no message to read
	-2 = extended frame messages not supported

*/

int sja1000_recv (struct cblk560 *c_blk)
{
  int i, x, status = -1;
  byte fir;
  int offset;


  if( c_blk->controller & 1)
   offset = 0x200;
  else
    offset = 0;

  /* Check the Receive Buffer Status */
  if (input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_SR) & SR_RBS)
  {
      /* Read frame information register */
      fir = input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_FIR);

      /* Extract frame format 0 - STANDARD, 1- EXTENDED IDENTIFIER */
      c_blk->CAN_RxMsg.format = fir >> 7;

	  /* extended frame messages not supported */
      if (c_blk->CAN_RxMsg.format)
          return(-2);

      /* Extract data length */
      c_blk->CAN_RxMsg.len = fir & 0x0f;
	  
      /* If data length exceeds 8 bytes adjust it to 8 */
      if (c_blk->CAN_RxMsg.len > 8)
          c_blk->CAN_RxMsg.len = 8;

	  /* update id */
	  c_blk->CAN_RxMsg.id = ((unsigned int)input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID1) << 3)
			+ (input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_ID2) >> 5);

      /* Check for a remote transmission request */
      if (fir & 0x40)
      {
		 c_blk->CAN_RxMsg.id |= 0x40;       /* indicate a remote transmission request */
      }
      else
      {
         /* Read message out of the receive registers */
	     for (i = 0, x = 0; i < c_blk->CAN_RxMsg.len; i++, x+=2)
              c_blk->CAN_RxMsg.data[i] = input_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_DATA_SFF(x));
	  }

      /* Release Receive Buffer */
	  output_byte(c_blk->nHandle, (byte*)c_blk->brd_memptr + offset + CAN_CMR, CMR_RRB);
	  status = 0;		/* indicate success */
  }
  return(status);
}





/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	sja1000_get_btrs()

    VERSION:		A

    CREATION DATE:	04/12/10

    CODED BY:		FJM

    ABSTRACT:       Load the struct cblk560 btr0 and btr1 members with bit timming values
                    for the requested baud index.
                        
    CALLING
	    SEQUENCE:   void = sja1000_get_btrs(struct cblk560 *c_blk, unsigned int index );
                          where:
                          	ptr (pointer to structure)
							  Pointer to the configuration block structure.
                          	index (unsigned int)
							  Baud rate index.
                            
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

    Bus Timing parameters:

    Sampling Mode (SAM)
	In general, the single sample mode is adequate for most applications.
	A sample mode of three reduces the amount of allowable system
	propagation delay by approximately two time quanta, use the single
	sample mode for high-speed networks.
	SAM = 0 = 1-Sample Mode, SAM = 1 = 3-Sample Mode.

	Synchronization Jump Width
	It is recommended that a large Synchronization Jump Width (SJW) be used.
	The allowable range of Synchronization Jump Width is between 1 and 4.

	Example: Compute the bit timming values for a 24 MHz oscillator signal
	fed to the SJA1000, we want a bit rate of 250 kbit/s, with a sampling
	point close to 81.25% of the whole bit, and a Synchronization Jump
	Width (SJW) of 3 quanta.

  	Oscillator Frequency = 24MHz
	Synchronization Jump Width = 3
	SAM = 0 = 1-Sample Mode

	1. Compute the Bit Rate Prescaler (BRP).
	The CAN Bit Rate Prescaler (BRP) is used to divide the oscillator
	frequency into a suitable CAN clock rate. The CAN clock rate establishes
	the time quantum value. The allowable range of Bit Rate Prescaler is
	between 1 and 64.

	Note: Time quantum values should be as small as practical, providing better
          resolution in selecting the location of the sample point in the bit
          period as well as the size of the Synchronization Jump Width (SJW).

	Determine an initial prescale value, select an initial time
	quantum value in the range 250e-9 to 500e-9 sec.

	BRP = initial time quantum value /  2 * (1 / Oscillator Frequency)
	BRP = 250e-9 /  2 * (1 / 24e6 Hz) = 3

	Bit Rate Prescaler = 3

	Time Quantum = 2 * Bit Rate Prescaler / Oscillator Frequency
	Time Quantum = 2 * 3 / 24e6 Hz = 250e-9 sec

	2. Compute bit time.
	The desired bit rate is 250K BPS (Bits Per Second).
	Bit Time = 1 / bit rate
	Bit Time = 1 / 250e3 BPS = 4e-6 sec

	3. Compute the number of time quanta required. The number of time
	quanta must be an integer value.  The allowable range of time quanta
	is typically between 8 and 25, select larger values when possible.

	Number of Time Quanta Required = Bit Time / Time Quantum
	Number of Time Quanta Required = 4e-6 sec / 250e-9 sec = 16

	4. Establish the Sample Point, Time Segment1 and Time Segment2 values.
	Always use integer values, a good general rule is to set the sample
	point at 80% or above. Sample points in the 80% - 90% range are
	required for industry standards J1939 (250K BPS) and J2284 (500K BPS).
	The allowable range of Time Segment1 is between 1 and 16.
	The allowable range of Time Segment2 is between 1 and 8.

	Sample Point Location = 81.25% = 81.25/100 = 0.8125

	Number of Time Quanta Required = 16 = Sync Segment + Time Segment1 + Time Segment2
	Note: The Sync Segment time quantum value is fixed at one.
	       
	Placing the sample point at 81.25%

	Time Segment1 = Number of Time Quanta Required * 81.25%
	Time Segment1 = 16 * 0.8125 = 13 (integer values only)

	Time Segment2 = Number of Time Quanta Required - Time Segment1
	Time Segment2 = 16 - 13 = 3

	Time Segment1 = 13	(integer values only)
	Time Segment2 =  3	(integer values only)

	Check Sample Point = (Time Segment1) / (Time Segment1 + Time Segment2)
	Check Sample Point = 13 / (13 + 3) = 0.8125 and 0.8125 * 100 = 81.25%

	SJA 1000 CAN Bit Timing Control Registers
	-----------------------------------------------------------------------
	| bit#|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
	-----------------------------------------------------------------------
	| btr0|	SJW1  | SJW0  |	BRP5  | BRP4  | BRP3  |	BRP2  | BRP1  | BRP0  |
	-----------------------------------------------------------------------
	| btr1| SAM   |TSEG22 |TSEG21 |TSEG20 |TSEG13 |TSEG12 |TSEG11 |TSEG10 |
	-----------------------------------------------------------------------
	Note: Registers containing the bit timing parameters are implemented as
	      binary values "one less" than the calculated values.

	btr0 = (Synchronization Jump Width - 1) * 64 + (Bit Rate Prescaler - 1) 
	btr0 = (3 - 1) * 64 + (3 - 1) = 130 = 0x82
    
	btr1 = SAM * 128 + (Time Segment2 - 1) * 16 + (Time Segment1 - 1 - Sync Segment)
	btr1 = 0 * 128 + (3 - 1) * 16 + (13 - 1 - 1) = 43 = 0x2b
*/


/*
    Baud rate table, values for btr0 and btr1 for each baud rate
    Baud rates tend to be one of the following: 10 Kbps, 20 Kbps, 50 Kbps, 100 Kbps,
    125 Kbps, 250 Kbps, 500 Kbps (Most common), 800 Kbps, and 1,000 Kbps.
    The baud rates are indexed as follows:

         sja1000_bauds[baud rate index 0 thru 8] [ 0 = btr0, 1 = btr1 value]

         sja1000_bauds[5][0] = the btr0 value for 250 Kbps
         sja1000_bauds[5][1] = the btr1 value for 250 Kbps
*/



static byte sja1000_bauds[9] [2] = {
	{ 0xbb, 0x3e },   /* 10 Kbps    - 24MHz Fin, BRP of 60, sample point @ 80.00%, SJW of 3 */ 
	{ 0x9d, 0x3e },   /* 20 Kbps    - 24MHz Fin, BRP of 30, sample point @ 80.00%, SJW of 3 */ 
	{ 0x8e, 0x2b },   /* 50 Kbps    - 24MHz Fin, BRP of 15, sample point @ 81.25%, SJW of 3 */ 
	{ 0x85, 0x3e },   /* 100 Kbps   - 24MHz Fin, BRP of  6, sample point @ 80.00%, SJW of 3 */
	{ 0x85, 0x2b },   /* 125 Kbps   - 24MHz Fin, BRP of  6, sample point @ 81.25%, SJW of 3 */
	{ 0x82, 0x2b },   /* 250 Kbps   - 24MHz Fin, BRP of  3, sample point @ 81.25%, SJW of 3 */
	{ 0x81, 0x18 },   /* 500 Kbps   - 24MHz Fin, BRP of  2, sample point @ 83.33%, SJW of 3 */
	{ 0x80, 0x2a },   /* 800 Kbps   - 24MHz Fin, BRP of  1, sample point @ 80.00%, SJW of 3 */
	{ 0x80, 0x18 }    /* 1,000 Kbps - 24MHz Fin, BRP of  1, sample point @ 83.33%, SJW of 3 */
};


void sja1000_get_btrs(struct cblk560 *c_blk, unsigned int index )
{
	unsigned int i;

    i = sizeof (sja1000_bauds) >> 1;
    if (index >= i)
        index = (i--);

    c_blk->btr0 = sja1000_bauds[index][0]; /* btr0 value for this baud */
    c_blk->btr1 = sja1000_bauds[index][1]; /* btr1 value for this baud */
}




/*
{+D}
    SYSTEM:         Acromag IOS560 CAN I/O Board

    FILENAME:		sja1000p.c

    MODULE NAME:	getMsg()

    VERSION:		A

    CREATION DATE:	03/19/10

    CODED BY:		FJM

    ABSTRACT:       This module is used to receive and display a message in peliCAN mode.
                        
    CALLING
	    SEQUENCE:   void getMsg(struct cblk560 *c_blk);
                      where:
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

    This module is used to receive and display a message in peliCAN mode.
*/



void getMsg(struct cblk560 *c_blk)
{
  int i,j;

  i = sja1000_recv (c_blk);	/* check for and read if message is present */

  if ( i == 0 )                  /* message was present and read */
  {
    printf("\nRXID  = 0x%03X\n", c_blk->CAN_RxMsg.id);
    printf("RXLEN = 0x%02X\n", c_blk->CAN_RxMsg.len);
    printf("RXBUF =");
	for (j = 0; j < c_blk->CAN_RxMsg.len; j++)
        printf(" 0x%02X", c_blk->CAN_RxMsg.data[j]);

    printf("\n");
  }
  else
    printf("\nno messages!\n");
}





void putMsg(struct cblk560 *c_blk)
{
  int i;

    /* setup ID & length */
    c_blk->CAN_TxMsg.id = (word)0x555;
    c_blk->CAN_TxMsg.len = (byte)8;

    /* load the xmit message buffer */
    c_blk->CAN_TxMsg.data[0] = 'P';
    c_blk->CAN_TxMsg.data[1] = 'O';
    c_blk->CAN_TxMsg.data[2] = 'O';
    c_blk->CAN_TxMsg.data[3] = 'D';
    c_blk->CAN_TxMsg.data[4] = 'L';
    c_blk->CAN_TxMsg.data[5] = 'E';
    c_blk->CAN_TxMsg.data[6] = 'S';
    c_blk->CAN_TxMsg.data[7] = '!';

    /* call xmit routine */
    i = sja1000_xmit (c_blk);
    if ( i )   /* message xmit error */
        printf("message xmit error 0x%02X\n", i);
}
