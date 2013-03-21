
/*
{+D}
   SYSTEM:          Acromag PCI carrier

   MODULE NAME:     dev8650.c

   VERSION:         C

   CREATION DATE:   04/01/11

   CODED BY:        FJM

   ABSTRACT:        8650 carrier device.

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
05/25/11 FJM   Fedora 15 update.
01/23/13 FJM   Fedora 18 update, removed include of asm/system.h.

{-D}
*/


/* APC8650 device */





#ifndef BUILDING_FOR_KERNEL
#define BUILDING_FOR_KERNEL	/* controls conditional inclusion in file carrier.h */
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


#include "../carrier/carrier.h"
#include "../ip1k100/ip1k100.h"
#include "../ip1k110/ip1k110.h"
#include "../ip220/ip220.h"
#include "../ip230/ip230.h"
#include "../ip231/ip231.h"
#include "../ip236/ip236.h"
#include "../ip320/ip320.h"
#include "../ip330/ip330.h"
#include "../ip340/ip340.h"
#include "../ip400/ip400.h"
#include "../ip405/ip405.h"
#include "../ip408/ip408.h"
#include "../ip409/ip409.h"
#include "../ip445/ip445.h"
#include "../ip470/ip470.h"
#include "../ip480/ip480.h"
#include "../ip482/ip482.h"
#include "../ip560/basicCAN/ip560b.h"
#include "../ip560/peliCAN/ip560p.h"
#include "../ip570/ip570.h"



#define DEVICE_NAME	"apc8650_"	/* the name of the device */
#define MAJOR_NUM	45


/* ///////////////////////////////////////////////////////////////// */
/* Message Signaled Interrupt (MSI) is a VECTOR based scheme and     */
/* needs to be enabled by setting CONFIG_PCI_MSI during kernel       */
/* config.                                                           */
/* Since the target of the inbound message is the local APIC,        */
/* providing CONFIG_X86_LOCAL_APIC must be enabled as well as        */
/* CONFIG_PCI_MSI.                                                   */
/* Use the following define to support Message Signaled Interrupt.   */
/* ///////////////////////////////////////////////////////////////// */

/*#define APCIE_USE_MSI_INT */





int open_dev[MAX_CARRIERS];
int msi_status[MAX_CARRIERS];
unsigned int board_irq[MAX_CARRIERS];
unsigned long carrier_address[MAX_CARRIERS];
unsigned long ip_mem_address[MAX_CARRIERS];
struct pci_dev *p86xxBoard[MAX_CARRIERS];
struct carrier_isr_data CARRIER_ISR_DATA[MAX_CARRIERS];           /* ISR routine handler structure */
int ret_val = 0;


/*
   Per 57x chip wait queues used for blocking.  This is an array of pointers of
   queues, flags and status which are used as parameters for the sleep and wake calls.
*/
wait_queue_head_t apWaitQueue[32];	/* wait queues for 57X modules */
int abWaitQueueFlag[32];		/* wait queues flags for 57X modules */
uint32_t adwIrqStatus[32];		/* individual device status */



static DECLARE_WAIT_QUEUE_HEAD(ain_queue);  /* wait queue for input boards */
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

	get_user( adata, (unsigned long *)buf );		/* pickup address */
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

		/* Read 32 bit from configuration space */
		case 0x40:	/* Read 32 bit from configuration space */
		   get_user( adata, (unsigned long *)adata );		/* pickup EE address */
		   get_user( idata, (unsigned long *)( buf + (sizeof(unsigned long)) ) );	/* pickup instance index */

		   if( p86xxBoard[idata] )
			pci_read_config_dword( p86xxBoard[idata], (int)adata, (u32*)&ldata ); /* read config space */
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

	get_user( adata, (unsigned long *)buf );				/* pickup address */
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
		   wait_event_interruptible(ain_queue, wqc);
		   wqc = 2;       /* indicate complete */ 
		   /* printk("after wait_event_interruptible\n");*/
		break;
		case 9: /* 8 bit blocking start convert */
		   wqc = 0;       /* indicate waiting */
		   writeb( (int)ldata, (void *)adata );
		   wait_event_interruptible(ain_queue, wqc);
		   wqc = 2;       /* indicate complete */ 
		   /* printk("after wait_event_interruptible\n");*/
		break;


		/* Write 32 bit to configuration space */
		case 0x40:	/* Write 32 bit to configuration space */
		   get_user( adata, (unsigned long *)adata );		/* pickup EE address */
		   get_user( idata, (unsigned long *)(buf+(2*(sizeof(unsigned long)))) );	/* pickup instance index */

		   if( p86xxBoard[idata] )
			pci_write_config_dword( p86xxBoard[idata], (int)adata, (u32)ldata ); /* write config space */
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
	case 4:/* return IP MEM address */
            for(i = 0; i < MAX_CARRIERS; i++)                   /* get all boards */
            {
               ldata = ( unsigned long )ip_mem_address[i];      /* convert to long */
               put_user( ldata, (unsigned long *)(arg+(i*(sizeof(unsigned long)))) );	/* update user data */
            }    
	break;
	case 5:/* return IP I/O address */
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
/*  return( status ); */
    return((long)status);
}


static struct file_operations apc8650_ops = {
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
  Module interrupt service routines.

  Several modules can generate an interrupt that can be used to indicate the
  competition of a data acquisition or some digital I/O or timer event.
  Module ISRs files are found in the 'isr' directory using the naming convention
  isr_xxx.c (where xxx is the model number).  If you plan to generate interrupts
  with the modules a call to the isrxxx() must be placed into the
  'apc8650_handler' function for the slot that corresponds to the module.

  Any change to the 'dev8650.c' file requires the re-compilation of the device driver.
*/




static irqreturn_t
apc8650_handler( int irq, void *did, struct pt_regs *cpu_regs )
{ 
   struct handler_data hdata;	/* wrapper structure for isr_data */
   volatile PCIe_BOARD_MEMORY_MAP* pPCIeCard;
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




   int_status = 0;              /* indicate interrupt not handled */
   if( open_dev[0] )            /* Check the 1st carrier board open_dev[0] */
   {
     pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)carrier_address[0];
     nValue = readw((unsigned short*)&pPCIeCard->intPending);

     if( nValue & CARRIER_INT_MASK )/* non-zero if this carrier is interrupting */
     {
/*
printk("\nopen_dev[0] pending %X",nValue);
*/
       /* Check each slot for an interrupt pending */
       /* Call the SLOT_A interrupt handler for any pending interrupts */
       if(nValue & IPA_INT0_PENDING || nValue & IPA_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_A - SLOT_A)];/* address of slot isr_data struct to handler data struct */
/*
         Place a call to one of the ISR routines listed below so
         it overwrites the line isr_xxx( (void *)&hdata );

         isr_330( (void *)&hdata );
         isr_340( (void *)&hdata );
         isr_400( (void *)&hdata );
         isr_408( (void *)&hdata );
         isr_409( (void *)&hdata );
         isr_470( (void *)&hdata );
         isr_482( (void *)&hdata );
         isr_560b( (void *)&hdata );
         isr_560p( (void *)&hdata );
         isr_57x( (void *)&hdata );
         isr_1k110( (void *)&hdata );
*/

         /* Slot A ISR code goes in here */
         isr_408( (void *)&hdata );

         /* read IP A Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt1);
         int_status = 1;
       }			

       if(nValue & IPB_INT0_PENDING || nValue & IPB_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_B - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot B ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP B Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt1);
         int_status = 1;
       }

       if(nValue & IPC_INT0_PENDING || nValue & IPC_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_C - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot C ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP C Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt1);
         int_status = 1;
       }			

       if(nValue & IPD_INT0_PENDING || nValue & IPD_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[0].slot_isr_data[(SLOT_D - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot D ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP D Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt1);
         int_status = 1;
       }			
     }
   }



   if( open_dev[1] )		/* Check the 2nd carrier board open_dev[1] */
   {
     pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)carrier_address[1];
     nValue = readw((unsigned short*)&pPCIeCard->intPending);

     if( nValue & CARRIER_INT_MASK )/* non-zero if this carrier is interrupting */
     {
/*
printk("\nopen_dev[1] pending %X",nValue);
*/
       /* Check each IP slot for an interrupt pending */
       /* Call interrupt handler for any pending interrupts */
       if(nValue & IPA_INT0_PENDING || nValue & IPA_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[1].slot_isr_data[(SLOT_A - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot A ISR code goes in here */
         isr_408( (void *)&hdata );

         /* read IP A Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt1);
         int_status = 1;
       }			

       if(nValue & IPB_INT0_PENDING || nValue & IPB_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[1].slot_isr_data[(SLOT_B - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot B ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/
         /* read IP B Interrupt Select Space */

         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt1);
         int_status = 1;
       }

       if(nValue & IPC_INT0_PENDING || nValue & IPC_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[1].slot_isr_data[(SLOT_C - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot C ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP C Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt1);
         int_status = 1;
       }			

       if(nValue & IPD_INT0_PENDING || nValue & IPD_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[1].slot_isr_data[(SLOT_D - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot D ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP D Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt1);
         int_status = 1;
       }			
     }
   }



   if( open_dev[2] )		/* Check the 3rd carrier board open_dev[2] */
   {
     pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)carrier_address[2];
     nValue = readw((unsigned short*)&pPCIeCard->intPending);

     if( nValue & CARRIER_INT_MASK )/* non-zero if this carrier is interrupting */
     {
/*
printk("\nopen_dev[2] pending %X",nValue);
*/
       /* Check each IP slot for an interrupt pending */
       /* Call interrupt handler for any pending interrupts */

       if(nValue & IPA_INT0_PENDING || nValue & IPA_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[2].slot_isr_data[(SLOT_A - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot A ISR code goes in here */
         isr_408( (void *)&hdata );

         /* read IP A Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt1);
         int_status = 1;
       }			

       if(nValue & IPB_INT0_PENDING || nValue & IPB_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[2].slot_isr_data[(SLOT_B - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot B ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP B Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt1);
         int_status = 1;
       }

       if(nValue & IPC_INT0_PENDING || nValue & IPC_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[2].slot_isr_data[(SLOT_C - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot C ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP C Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt1);
         int_status = 1;
       }			

       if(nValue & IPD_INT0_PENDING || nValue & IPD_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[2].slot_isr_data[(SLOT_D - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot D ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP D Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt1);
         int_status = 1;
       }			
     }
   }



   if( open_dev[3] )		/* Check the 4th carrier board open_dev[3] */
   {
     pPCIeCard = (PCIe_BOARD_MEMORY_MAP*)carrier_address[3];
     nValue = readw((unsigned short*)&pPCIeCard->intPending);

     if( nValue & CARRIER_INT_MASK )/* non-zero if this carrier is interrupting */
     {
/*
printk("\nopen_dev[3] pending %X",nValue);
*/
       /* Check each IP slot for an interrupt pending */
       /* Call interrupt handler for any pending interrupts */

       if(nValue & IPA_INT0_PENDING || nValue & IPA_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[3].slot_isr_data[(SLOT_A - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot A ISR code goes in here */
         isr_408( (void *)&hdata );

         /* read IP A Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotAInt1);
         int_status = 1;
       }			

       if(nValue & IPB_INT0_PENDING || nValue & IPB_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[3].slot_isr_data[(SLOT_B - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot B ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP B Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotBInt1);
         int_status = 1;
       }

       if(nValue & IPC_INT0_PENDING || nValue & IPC_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[3].slot_isr_data[(SLOT_C - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot C ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP C Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotCInt1);
         int_status = 1;
       }			

       if(nValue & IPD_INT0_PENDING || nValue & IPD_INT1_PENDING)
       {
         hdata.hd_ptr = (char *)&CARRIER_ISR_DATA[3].slot_isr_data[(SLOT_D - SLOT_A)];/* address of slot isr_data struct to handler data struct */

         /* Slot D ISR code goes in here */
         /*     isr_xxx( (void *)&hdata );*/

         /* read IP D Interrupt Select Space */
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt0);
         dummyWord = readw((unsigned short*)&pPCIeCard->slotDInt1);
         int_status = 1;
       }			
     }
   }

   if( int_status )
   {
       if( wqc == 0)            /* waiting for a blocked start convert AND an interrupt from the input board */
       {
         wqc = 1;               /* when received, change the condition to true... */
         wake_up_interruptible(&ain_queue);	/* ... and wake the blocked write */
       }
       return( IRQ_HANDLED);
   }
   else
       return( IRQ_NONE);
}



int
init_module( void ) 
{
  extern struct pci_dev *p86xxBoard[MAX_CARRIERS];
  struct pci_dev *p8650;
  int i,j,k,l;
  char devnamebuf[32];
  char devnumbuf[8];

  memset( &p86xxBoard[0], 0, sizeof(p86xxBoard));
  memset( &open_dev[0], 0, sizeof(open_dev));
  memset( &msi_status[0], 0, sizeof(msi_status));
  memset( &board_irq[0], 0, sizeof(board_irq));
  memset( &ip_mem_address[0], 0, sizeof(ip_mem_address));
  memset( &carrier_address[0], 0, sizeof(carrier_address));
  memset( &abWaitQueueFlag[0], 0, sizeof(abWaitQueueFlag));
  memset( CARRIER_ISR_DATA, 0, sizeof(CARRIER_ISR_DATA));

  p8650 = NULL;
  for( i = 0, j = 0; i < MAX_CARRIERS; i++ )
  {
    p8650 = ( struct pci_dev *)pci_get_device( 0x16D5, 0x5901, p8650 );
    if( p8650 )
    {
      p86xxBoard[i] = p8650;
      carrier_address[i] = (unsigned long)p8650->resource[2].start;
      carrier_address[i]= (unsigned long)ioremap_nocache( carrier_address[i], 0x4000000); /* no cache! */

      if( carrier_address[i] )
      {
        memset( &devnamebuf[0], 0, sizeof(devnamebuf));
        memset( &devnumbuf[0], 0, sizeof(devnumbuf));
        strcpy(devnamebuf, DEVICE_NAME);
        sprintf(&devnumbuf[0],"%d",i);
        strcat(devnamebuf, devnumbuf);
        ret_val = pci_enable_device(p8650);

#ifdef APCIE_USE_MSI_INT
			msi_status[i] = pci_enable_msi(p86xxBoard[i]);	/* Use Message Signaled Interrupts (MSI) */
#endif

        board_irq[i] = p8650->irq;

        ret_val = request_irq ( board_irq[i], (irq_handler_t)apc8650_handler, IRQF_SHARED, devnamebuf, ( void *)carrier_address[i] );
        printk("%s mapped   I/O=%08lX IRQ=%02X Rv=%X",devnamebuf,(unsigned long)carrier_address[i], board_irq[i],ret_val);

#ifdef APCIE_USE_MSI_INT
			printk(" MSI=%X", msi_status[i]);
#endif
			printk("\n");

        ip_mem_address[i] = carrier_address[i];	/* IP mem region same as carrier */
        j++;
      }
    }
    else
       break;
  }
  if( j )	/* found at least one device */
  {
    ret_val = register_chrdev ( MAJOR_NUM, DEVICE_NAME, &apc8650_ops );

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
               if( ip_mem_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ip_mem_address[k])[SLOT_A_MEM_OFFSET];/* slot mem addr */
             break;
             case SLOT_B:
               CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_B;
               if( carrier_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_B_IO_OFFSET];/* slot I/O addr */
               if( ip_mem_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ip_mem_address[k])[SLOT_B_MEM_OFFSET];/* slot mem addr */
              break;
              case SLOT_C:
                CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_C;
                if( carrier_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_C_IO_OFFSET];/* slot I/O addr */
                if( ip_mem_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ip_mem_address[k])[SLOT_C_MEM_OFFSET];/* slot mem addr */
              break;
              case SLOT_D:
                CARRIER_ISR_DATA[k].slot_isr_data[l].slot_letter = (unsigned long)SLOT_D;
                if( carrier_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_io_address = (unsigned long)&((char *)carrier_address[k])[SLOT_D_IO_OFFSET];/* slot I/O addr */
                if( ip_mem_address[k] )
                  CARRIER_ISR_DATA[k].slot_isr_data[l].slot_mem_address = (unsigned long)&((char *)ip_mem_address[k])[SLOT_D_MEM_OFFSET];/* slot mem addr */
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
#ifdef APCIE_USE_MSI_INT
		if(msi_status[i] == 0)			/* if enabled, remove Message Signaled Interrupts (MSI) */
			pci_disable_msi(p86xxBoard[i]);
#endif
        iounmap( (void *)carrier_address[i] );
        printk("%s unmapped I/O=%08lX IRQ=%02X\n",devnamebuf,(unsigned long)carrier_address[i], board_irq[i]);
	  }
	}
  }
}

MODULE_LICENSE("GPL and additional rights");
