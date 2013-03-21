
/*
{+D}
   SYSTEM:          Acromag IOS carrier device

   MODULE NAME:     devios.c

   VERSION:         F

   CREATION DATE:   04/01/09

   CODED BY:        FJM

   ABSTRACT:        IOS carrier device.

   CALLING
	SEQUENCE:

   MODULE TYPE:

   I/O RESOURCES:

   SYSTEM
	RESOURCES:

   MODULES
	CALLED:

   REVISIONS:

 DATE      BY       PURPOSE
-------- ----  ------------------------------------------------
06/02/10 FJM   Fedora13 update added #include <linux/wait.h> & <linux/sched.h>
08/17/10 FJM   Fixed idata.slot_mem_address passing bad
--/--/--       addresses also added IOS560.
01/17/11 FJM   Added IP57x.
05/25/11 FJM   Fedora 15 update.
01/23/13 FJM   Fedora 18 update, removed include of asm/system.h.

{-D}
*/


/* IOScarrier device */





#ifndef BUILDING_FOR_KERNEL
#define BUILDING_FOR_KERNEL	/* controls conditional inclusion in file ioscarrier.h */
#endif


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#include "../ioscarrier/ioscarrier.h"
#include "../ios220/ios220.h"
#include "../ios231/ios231.h"
#include "../ios320/ios320.h"
#include "../ios330/ios330.h"
#include "../ios341/ios341.h"

#include "../ios408/ios408.h"
#include "../ios409/ios409.h"
#include "../ios445/ios445.h"
#include "../ios470/ios470.h"
#include "../ios482/ios482.h"
#include "../ios520/ios520.h"
#include "../ios521/ios521.h"
#include "../iosep20x/iosep20x.h"
#include "../ios560/basicCAN/ios560b.h"
#include "../ios560/peliCAN/ios560p.h"
#include "../ios570/ios570.h"


#define DEVICE_NAME	"ioscarrier_"	/* the name of the device */
#define MAJOR_NUM	48


int open_dev[MAX_CARRIERS];
unsigned int board_irq[MAX_CARRIERS];
unsigned long carrier_address[MAX_CARRIERS];
unsigned long ios_mem_address[MAX_CARRIERS];
struct pci_dev *pIOSBoard[MAX_CARRIERS];
struct carrier_isr_data CARRIER_ISR_DATA[MAX_CARRIERS];           /* ISR routine handler structure */
int ret_val = 0;


/*
   Per 57x chip wait queues used for blocking.  This is an array of pointers of
   queues, flags and status which are used as parameters for the sleep and wake calls.
*/
wait_queue_head_t apWaitQueue[32];	/* wait queues for 57X modules */
int abWaitQueueFlag[32];		/* wait queues flags for 57X modules */
uint32_t adwIrqStatus[32];		/* individual device status */



static DECLARE_WAIT_QUEUE_HEAD(ios_queue);  /* wait queue for ios inputs */
static int wqc = 2;                         /* wait queue condition */


struct file;
struct inode;
struct pt_regs;



static int
open( struct inode *inode, struct file *fp )
{
   int minor;

   minor = inode->i_rdev & 0xf;
   if( minor > (MAX_CARRIERS - 1))
	   return( -ENODEV );
  
   if( open_dev[minor] )
	   return( -EBUSY );

   open_dev[minor] = 1;

   return( 0 );
}

static int
release( struct inode *inode, struct file *fp )
{
   int minor;

   minor = inode->i_rdev & 0xf;
   if( minor > (MAX_CARRIERS - 1))
	   return( -ENODEV );

   if( open_dev[minor] )
   {
	   open_dev[minor] = 0;
	   return( 0 );
   }
   return( -ENODEV );
}


static ssize_t
read( struct file *fp, char *buf, size_t length, loff_t *offset )
{ 
	unsigned long adata, ldata, idata;
	unsigned short sdata;
	unsigned char cdata;

	get_user( adata, (unsigned long *)buf );	/* pickup address */
	switch( length )
	{
		case 1:	/* 8 bit */
		   cdata = readb( (void *) adata );
		   ldata = ( unsigned long )cdata;		/* convert to long */
		break;
		case 2:	/* 16 bit */
		   sdata = readw( (void *) adata );
		   ldata = ( unsigned long )sdata;		/* convert to long */
		break;
		case 4:	/* 32 bit */
		   ldata = readl( (void *) adata );
		break;

		/* 2x special serial I/O device reads */
		case 20:/* kiread */
		   cdata = kiread( ((adata >> 16) & 0xff), ((adata >> 8) & 0xff), (adata & 0xff));
		   ldata = ( unsigned long )cdata;		/* convert to long */
		break;
		case 21:/* kirstat */
		   cdata =  kirstat( ((adata >> 16) & 0xff), ((adata >> 8) & 0xff), (adata & 0xff));
		   ldata = ( unsigned long )cdata;		/* convert to long */
		break;
		case 22:/* kistat232 */
		   cdata =  kistat232( ((adata >> 16) & 0xff), ((adata >> 8) & 0xff), (adata & 0xff));
		   ldata = ( unsigned long )cdata;		/* convert to long */
		break;
		case 23:/* InitBuffer */
		   cdata =  InitBuffer( ((adata >> 16) & 0xff), ((adata >> 8) & 0xff), (adata & 0xff));
		   ldata = ( unsigned long )cdata;		/* convert to long */
		break;

		/* Read 32 bit from configuration space */
		case 0x40:	/* Read 32 bit from configuration space */
		   get_user( adata, (unsigned long *)adata );		/* pickup EE address */
		   get_user( idata, (unsigned long *)( buf + (sizeof(unsigned long)) ) );	/* pickup instance index */

		   if( pIOSBoard[idata] )
			pci_read_config_dword( pIOSBoard[idata], (int)adata, (u32*)&ldata ); /* read config space */
		   else
			ldata = 0;
		break;
		
		default:
		    cdata = sdata = adata = ldata = idata = 0;
		    return( -EINVAL );
		break;
	}
	put_user( ldata,(unsigned long *)( buf + (sizeof(unsigned long)) ) );	/* update user data */
	return( length );
}



static ssize_t
write( struct file *fp, const char *buf, size_t length, loff_t *offset )
{ 
	unsigned long adata, ldata, idata;

	get_user( adata, (unsigned long *)buf );								/* pickup address */
	get_user( ldata, (unsigned long *)( buf + (sizeof(unsigned long)) ) );	/* pickup data */
	switch( length )
	{
		case 1:	/* 8 bit */
		   writeb( (int)ldata, (void *)adata );
		break;
		case 2:	/* 16 bit */
		   writew( (int)ldata, (void *)adata );
		break;
		case 4:	/* 32 bit */
		   writel( (int)ldata, (void *)adata );
		break;
		case 8:	/* 16 bit blocking start convert */
		   wqc = 0;       /* indicate waiting */
		   writew( (int)ldata, (void *)adata );
		   wait_event_interruptible(ios_queue, wqc);
		   wqc = 2;       /* indicate complete */ 
/* printk("after wait_event_interruptible\n");*/
		break;
		   case 9: /* 8 bit blocking start convert */
		   wqc = 0;       /* indicate waiting */
		   writeb( (int)ldata, (void *)adata );
		   wait_event_interruptible(ios_queue, wqc);
		   wqc = 2;       /* indicate complete */ 
/* printk("after wait_event_interruptible\n");*/
		break;

		/* Write 32 bit to configuration space */
		case 0x40:	/* Write 32 bit to configuration space */
		   get_user( adata, (unsigned long *)adata );		/* pickup EE address */
		   get_user( idata, (unsigned long *)(buf+(2*(sizeof(unsigned long)))) );	/* pickup instance index */

		   if( pIOSBoard[idata] )
			pci_write_config_dword( pIOSBoard[idata], (int)adata, (u32)ldata ); /* write config space */
		break;

		default:
		    return( -EINVAL );
		break;
	}
    return( length );
}



/*  Used in versions FC14 and earlier */
/*
static int
ioctl( struct inode *inode, struct file *fp, unsigned int cmd, unsigned long arg)
*/

static long
device_ioctl( struct file *fp, unsigned int cmd, unsigned long arg)
{
    unsigned long ldata;
    unsigned long carrier, slot, chan, devnum;
    int i, status;

    status = (int)cmd;
    carrier = slot = chan = devnum = 0;
    switch( cmd )
    {
	case 0:	/* reserved for future use */
	case 1:
	case 2:
	case 3:
	case 4:/* return IOS MEM address */
            for(i = 0; i < MAX_CARRIERS; i++)                   /* get all boards */
            {
               ldata = ( unsigned long )ios_mem_address[i];      /* convert to long */
               put_user( ldata, (unsigned long *)(arg+(i*(sizeof(unsigned long)))) );	/* update user data */
            }    
	break;
	case 5:/* return IOS I/O address */
            for(i = 0; i < MAX_CARRIERS; i++)                    /* get all boards */
            {
               ldata = ( unsigned long )carrier_address[i];      /* convert to long */
               put_user( ldata, (unsigned long *)(arg+(i*(sizeof(unsigned long)))) );	/* update user data */
            }    
	break;
	case 6:/* return IRQ number */
            for(i = 0; i < MAX_CARRIERS; i++)                    /* get all boards */
            {
               ldata = ( unsigned long )board_irq[i];            /* convert IRQ to long */
               put_user( ldata, (unsigned long *)(arg+(i*(sizeof(unsigned long)))) );	/* update user data */
            }
	break;
	case 20: /* for 57x modules - set ISR info for carrier, slot, channel, and device number */
            get_user( carrier, (unsigned long *) arg );                               /* pickup carrier */
            get_user( slot,   (unsigned long *)(arg + (sizeof(unsigned long))) );     /* pickup slot */
            get_user( chan,   (unsigned long *)(arg + (2*(sizeof(unsigned long)))) ); /* pickup chan */
            get_user( devnum, (unsigned long *)(arg + (3*(sizeof(unsigned long)))) ); /* pickup devnum */
            /* set the device number in the carrier/slot ISR data structure */
            CARRIER_ISR_DATA[carrier].slot_isr_data[slot - SLOT_A].dev_num[chan] = devnum;
	break;
	case 21: /* for 57x modules - wake up blocked thread so it can terminate */
            get_user( ldata, (unsigned long *)arg  );	     /* pickup device number */
            abWaitQueueFlag[ldata] = 2; /* indicate this was a terminate wake-up and NOT an ISR wake up */
            wake_up_interruptible( &apWaitQueue[ldata] );
	break;
	case 22: /* for 57x modules - sleep waiting for the ISR or ISR clean-up to issue a wake up */
            get_user( ldata, (unsigned long *)arg  );	     /* pickup device number */
            if(abWaitQueueFlag[ldata] == 2)                  /* was this a SR clean-up wake-up? terminate */
            {
               abWaitQueueFlag[ldata] = 0;                    /* indicate clean-up action processed */
               ldata = 0;
               status = -1;  /* 'ioctl' returns -1 if interrupt processing is being terminated. */
            }
            else                                             /* normal interrupt processing. */
            {
              /* Sleep and wait for the 57x module ISR or ISR clean-up to wake us up */
              abWaitQueueFlag[ldata] = 0;                    /* indicate waiting */
              wait_event_interruptible( apWaitQueue[ldata], abWaitQueueFlag[ldata] );
              ldata = (unsigned long)adwIrqStatus[ldata];    /* return the interrupt status variable. */
              status = 0;  /* 'ioctl' returns  0 during normal interrupt processing. */
            }
            put_user( ldata,(unsigned long *) arg );	     /* overwrite device number value */
	break;
    }
/*  Used in versions FC14 and earlier */
/*	return( status ); */
	return((long)status);
}


static struct file_operations ioscarrier_ops = {
  .owner = THIS_MODULE, /* owner of the world */
  NULL,                 /* seek */
  .read = read,         /* read */
  .write = write,       /* write */
  NULL,                 /* readdir */
  NULL,                 /* poll */
/*  Used in versions FC14 and earlier */
/*  .ioctl = ioctl,       / * ioctl */
  .unlocked_ioctl = device_ioctl, /* unlocked_ioctl */
  NULL,                 /* mmap */
  .open = open,         /* open */
  NULL,                 /* flush */
  .release = release,   /* release */
  NULL,                 /* fsync */
  NULL,                 /* fasync */
  NULL,                 /* lock */ 
  NULL,                 /* readv */
  NULL,                 /* writev */
  NULL,                 /* sendpage */
  NULL                  /* get_unmapped_area */
};





/*
  IOS module interrupt service routines.

  Several IOS modules can generate an interrupt that can be used to indicate the
  competition of a data acquisition or some digital I/O or timer event.
  Module ISRs files are found in the 'devios' directory using the naming convention
  iosxxxisr.c (where xxx is the model number).  If you plan to generate interrupts
  with the IOS modules a call to the iosxxxisr() must be placed into the
  'ioscarrier_handler' function for the slot that corresponds to the IOS module.

  Any change to the 'devios.c' file requires the re-compilation of the device driver.
*/

static irqreturn_t
ioscarrier_handler( int irq, void *did, struct pt_regs *cpu_regs )
{
   struct handler_data hdata;	/* wrapper structure for isr_data */
   volatile PCI_BOARD_MEMORY_MAP* pPCICard;
   int int_status;
   volatile unsigned short nValue;

/*
   The volatile unsigned short variable 'dummyWord' was created to force the compiler to
   generate code to read the interrupt select register on the carrier board, without it,
   the read instruction might be removed by a 'C' code optimizer.

   __attribute__ ((unused)) avoids compiler 'unused-but-set-variable' warning
*/

/* Used in versions FC14 and earlier */
/* volatile unsigned short dummyWord;*/

   volatile unsigned short dummyWord __attribute__ ((unused));




   int_status = 0;	/* indicate interrupt not handled */
   if( open_dev[0] )	/* Check open_dev[0] */
   {
     pPCICard = (PCI_BOARD_MEMORY_MAP*)carrier_address[0];
     nValue = readw((unsigned short*)&pPCICard->intPending);

     if( nValue & CARRIER_INT_MASK )/* non-zero if this carrier is interrupting */
     {
/*
printk("open_dev[0] pending %X\n",nValue);
*/
       /* Check each IOS slot for an interrupt pending */
       /* Call interrupt handler for any pending interrupts */

       if(nValue & IOSA_INT0_PENDING || nValue & IOSA_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_A - SLOT_A)];/* address of slot isr_data struct to handler data struct */

/*
         Place a call to one of the ISR routines listed below so
         it overwrites the line iosxxxisr( (void *)&hdata );

         ios330isr( (void *)&hdata );
         ios341isr( (void *)&hdata );
         ios408isr( (void *)&hdata );
         ios409isr( (void *)&hdata );
         ios470isr( (void *)&hdata );
         ios482isr( (void *)&hdata );
         ios52xisr( (void *)&hdata );
         iosep20xisr( (void *)&hdata );
         ios560bisr( (void *)&hdata );
         ios560pisr( (void *)&hdata );
         ios57xisr( (void *)&hdata );
*/

         /* Slot A ISR code goes in here */
         ios408isr( (void *)&hdata );				/* iosxxxisr( (void *)&hdata );*/

         /* read IOS A Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCICard->slotAInt0);
         dummyWord = readw((unsigned short*)&pPCICard->slotAInt1);
         int_status = 1;
       }			

       if(nValue & IOSB_INT0_PENDING || nValue & IOSB_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_B - SLOT_A)];/* address of slot isr_data struct to handler data struct */
/*
         Place a call to one of the ISR routines listed below so
         it overwrites the line iosxxxisr( (void *)&hdata );

         ios330isr( (void *)&hdata );
         ios341isr( (void *)&hdata );
         ios408isr( (void *)&hdata );
         ios409isr( (void *)&hdata );
         ios470isr( (void *)&hdata );
         ios482isr( (void *)&hdata );
         ios52xisr( (void *)&hdata );
         iosep20xisr( (void *)&hdata );
         ios560bisr( (void *)&hdata );
         ios560pisr( (void *)&hdata );
         ios57xisr( (void *)&hdata );
*/
         /* Slot B ISR code goes in here */
         /* iosxxxisr( (void *)&hdata );*/

         /* read IOS B Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCICard->slotBInt0);
         dummyWord = readw((unsigned short*)&pPCICard->slotBInt1);
         int_status = 1;
       }			

       if(nValue & IOSC_INT0_PENDING || nValue & IOSC_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_C - SLOT_A)];/* address of slot isr_data struct to handler data struct */
/*
         Place a call to one of the ISR routines listed below so
         overwrites the line iosxxxisr( (void *)&hdata );

         ios330isr( (void *)&hdata );
         ios341isr( (void *)&hdata );
         ios408isr( (void *)&hdata );
         ios409isr( (void *)&hdata );
         ios470isr( (void *)&hdata );
         ios482isr( (void *)&hdata );
         ios52xisr( (void *)&hdata );
         iosep20xisr( (void *)&hdata );
         ios560bisr( (void *)&hdata );
         ios560pisr( (void *)&hdata );
         ios57xisr( (void *)&hdata );
*/
         /* Slot C ISR code goes in here */
         /*     iosxxxisr( (void *)&hdata );*/

         /* read IOS C Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCICard->slotCInt0);
         dummyWord = readw((unsigned short*)&pPCICard->slotCInt1);
         int_status = 1;
       }			

       if(nValue & IOSD_INT0_PENDING || nValue & IOSD_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_D - SLOT_A)];/* address of slot isr_data struct to handler data struct */

/*
         Place a call to one of the ISR routines listed below so
         it overwrites the line iosxxxisr( (void *)&hdata );

         ios330isr( (void *)&hdata );
         ios341isr( (void *)&hdata );
         ios408isr( (void *)&hdata );
         ios409isr( (void *)&hdata );
         ios470isr( (void *)&hdata );
         ios482isr( (void *)&hdata );
         ios52xisr( (void *)&hdata );
         iosep20xisr( (void *)&hdata );
         ios560bisr( (void *)&hdata );
         ios560pisr( (void *)&hdata );
         ios57xisr( (void *)&hdata );
*/
         /* Slot D ISR code goes in here */
         /*     iosxxxisr( (void *)&hdata );*/

         /* read IOS D Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCICard->slotDInt0);
         dummyWord = readw((unsigned short*)&pPCICard->slotDInt1);
         int_status = 1;
       }
     }
   }

   if( int_status )
   {
      if( wqc == 0)            /* waiting for a blocked start convert AND an interrupt from the input board */
      {
         wqc = 1;               /* when received, change the condition to true... */
         wake_up_interruptible(&ios_queue);	/* ... and wake the blocked write */
      }
      return( IRQ_HANDLED);
   }
   else
      return( IRQ_NONE);
}



int
init_module( void ) 
{ 
  extern struct pci_dev *pIOSBoard[MAX_CARRIERS];
  struct pci_dev *pioscarrier;
  int i,j,k,l;
  char devnamebuf[32];
  char devnumbuf[8];

  memset( &pIOSBoard[0], 0, sizeof(pIOSBoard));
  memset( &open_dev[0], 0, sizeof(open_dev));
  memset( &board_irq[0], 0, sizeof(board_irq));
  memset( &ios_mem_address[0], 0, sizeof(ios_mem_address));
  memset( &carrier_address[0], 0, sizeof(carrier_address));
  memset( &abWaitQueueFlag[0], 0, sizeof(abWaitQueueFlag));
  memset( CARRIER_ISR_DATA, 0, sizeof(CARRIER_ISR_DATA));


  pioscarrier = NULL;
  for( i = 0, j = 0; i < MAX_CARRIERS; i++ )
  {
/*  Use pci_find_device() for earlier versions FC3/4/5/6 */
/*  pioscarrier = ( struct pci_dev *)pci_find_device( 0x10B5, 0x1024, pioscarrier ); */
    pioscarrier = ( struct pci_dev *)pci_get_device( 0x10B5, 0x1024, pioscarrier );
    if( pioscarrier )
    {
      pIOSBoard[i] = pioscarrier;
      carrier_address[i] = (unsigned long)pioscarrier->resource[2].start;
      carrier_address[i]= (unsigned long)ioremap_nocache( carrier_address[i], 1024); /* no cache! */

      if( carrier_address[i] )
      {
        memset( &devnamebuf[0], 0, sizeof(devnamebuf));
        memset( &devnumbuf[0], 0, sizeof(devnumbuf));
        strcpy(devnamebuf, DEVICE_NAME);
        sprintf(&devnumbuf[0],"%d",i);
        strcat(devnamebuf, devnumbuf);
        ret_val = pci_enable_device(pioscarrier);
        board_irq[i] = pioscarrier->irq;

/*  Used for earlier versions FC3/4/5/6 */
/*	ret_val = request_irq ( board_irq[i], ioscarrier_handler, SA_INTERRUPT | SA_SHIRQ, devnamebuf, ( void *)carrier_address[i] ); */
/*  Used for FC7 */
/*	ret_val = request_irq ( board_irq[i], (irq_handler_t)ioscarrier_handler, SA_INTERRUPT | SA_SHIRQ, devnamebuf, ( void *)carrier_address[i] );*/
/*  Used for FC8/9/10 */
/*	ret_val = request_irq ( board_irq[i], (irq_handler_t)ioscarrier_handler, IRQF_DISABLED | IRQF_SHARED, devnamebuf, ( void *)carrier_address[i] );*/

      	ret_val = request_irq ( board_irq[i], (irq_handler_t)ioscarrier_handler, IRQF_SHARED, devnamebuf, ( void *)carrier_address[i] );
        printk("%s mapped   I/O=%08lX IRQ=%02X Rv=%X\n",devnamebuf,(unsigned long)carrier_address[i], board_irq[i],ret_val);

      	/* if the additional region is present map it into memory */
        ios_mem_address[i] = (unsigned long)pioscarrier->resource[3].start;	/* get IOS mem region if present */
        if( ios_mem_address[i] )
        {
/*  Used in earlier versions FC3/4/5/6/7/8 */
/*        ios_mem_address[i] = (unsigned long)__ioremap( ios_mem_address[i], 0x4000000, _PAGE_PCD ); / * no cache!  PPC use _PAGE_NO_CACHE */
	  ios_mem_address[i] = (unsigned long)ioremap_nocache( ios_mem_address[i], 0x4000000 ); /* no cache! */

          if( ios_mem_address[i] )
              printk("%s mapped   MEM=%08lX\n",devnamebuf, (unsigned long)ios_mem_address[i]);

        }
        j++;
      }
    }
    else
	break;
  }
  if( j )	/* found at least one device */
  {
	ret_val = register_chrdev ( MAJOR_NUM, DEVICE_NAME, &ioscarrier_ops );

	if( ret_val < 0)
	{
		printk(DEVICE_NAME);
	    printk(" Failed to register error = %d\n", ret_val);
	}
	else
    {
      /* initialize wait queues for 57X modules */
      for( k = 0; k < 32; k++ )
	init_waitqueue_head(&apWaitQueue[k]);

      /* initialize the interrupt handler data structure */
      for( k = 0; k < MAX_CARRIERS; k++ )
      {
        for( l = 0; l < MAX_SLOTS; l++ )
        {
           /* initialize the interrupt data structure with the device number array set to unused for this slot */
           memset( &CARRIER_ISR_DATA[k].slot_isr_data[l].dev_num[0], 0xFF, sizeof(CARRIER_ISR_DATA[k].slot_isr_data[l].dev_num));
           switch((l+SLOT_A))
           {
             case SLOT_A:
               /* initialize the interrupt data structure with the slot I/O address, the slot memory address and the slot letter */
               CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_A;
               if( carrier_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_A_IO_OFFSET];/* slot I/O addr */
               if( ios_mem_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ios_mem_address[k])[SLOT_A_MEM_OFFSET];/* slot mem addr */
             break;
             case SLOT_B:
               CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_B;
               if( carrier_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_B_IO_OFFSET];/* slot I/O addr */
               if( ios_mem_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ios_mem_address[k])[SLOT_B_MEM_OFFSET];/* slot mem addr */
              break;
              case SLOT_C:
                CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_C;
               if( carrier_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_C_IO_OFFSET];/* slot I/O addr */
               if( ios_mem_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ios_mem_address[k])[SLOT_C_MEM_OFFSET];/* slot mem addr */
              break;
              default:	/* case SLOT_D: */
                CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_D;
               if( carrier_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_D_IO_OFFSET];/* slot I/O addr */
               if( ios_mem_address[k] )
                 CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ios_mem_address[k])[SLOT_D_MEM_OFFSET];/* slot mem addr */
              break;
           }
        }
      }
    }
    return( 0 );
  }
  return( -ENODEV );
}


void
cleanup_module( void ) 
{
  char devnamebuf[32];
  char devnumbuf[8];
  int i;


  if( ret_val >= 0 )
  {
    unregister_chrdev( MAJOR_NUM, DEVICE_NAME );
    for( i = 0; i < MAX_CARRIERS; i++ )
    {
      if( carrier_address[i] )
      {
        memset( &devnamebuf[0], 0, sizeof(devnamebuf));
        memset( &devnumbuf[0], 0, sizeof(devnumbuf));
        strcpy(devnamebuf, DEVICE_NAME);
        sprintf(&devnumbuf[0],"%d",i);
        strcat(devnamebuf, devnumbuf);

        free_irq( board_irq[i], (void *)carrier_address[i] );
        iounmap( (void *)carrier_address[i] );
        printk("%s unmapped I/O=%08lX IRQ=%02X\n",devnamebuf,(unsigned long)carrier_address[i], board_irq[i]);

        if( ios_mem_address[i] )
        {
           iounmap( (void *)ios_mem_address[i] );
           printk("%s unmapped MEM=%08lX\n",devnamebuf,(unsigned long)ios_mem_address[i]);
	}
      }
    }
  }
}

MODULE_LICENSE("GPL and additional rights");
