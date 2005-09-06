/* PSA: Modified APC8620 to APC8620dos */
/* All the globals that might be affected by running in duplicate
   have "dos" prefixed.
   All the functions which would be affected by running in duplicate
   have apc8620 replaced with apc8620dos.
   The ISR408 code is probably fine, as it has no globals and is
   perfectly reentrant. Hell, it's pretty much inlinable. */
/* APC8620dos device */

#define __KERNEL__
#define MODULE

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/config.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/major.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/vt_kern.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include "../carrier/apc8620.h"
#include "../ip408/ip408.h"

/*
#define DEVICE_NAME	"apc8620dos"
#define MAJOR_NUM	46
*/
#define DEVICE_NAME	"apc8620dos"	/* the name of the device */
#define MAJOR_NUM	47

/*
int open_dev = 0;
int board_irq = 0;
int ret_val = 0;
u32 carrier_address = 0;
*/

int dos_open_dev = 0;
int dos_board_irq = 0;
int dos_ret_val = 0;
u32 dos_carrier_address = 0;



/* use mknod /dev/%s c %d <minor>", DEVICE_NAME, MAJOR_NUM */
/* to create a device file */



static int
open( struct inode *inode, struct file *fp )
{
   int minor;

   minor = ( inode->i_rdev >> 4 ) & 0xf;
   if( minor > 4 )
	   return( -ENODEV );
  
   if( dos_open_dev )
	   return( -EBUSY );

   dos_open_dev = 1;
   MOD_INC_USE_COUNT;
   return( 0 );
} 


static int
release( struct inode *inode, struct file *fp )
{
   int minor;

   minor = ( inode->i_rdev >> 4 ) & 0xf;
   if( minor > 4 )
	   return( -ENODEV );

   if( dos_open_dev )
   {
	   dos_open_dev = 0;
	   MOD_DEC_USE_COUNT;
	   return( 0 );
   }
   return( -ENODEV );
}


static ssize_t
read( struct file *fp, char *buf, size_t length, loff_t *offset )	
{ 
	 unsigned long adata;
	 unsigned short sdata;
	 unsigned char cdata;
	 unsigned long ldata;

 	 unsigned short *psdata;
	 unsigned char *pcdata;
	 unsigned long *pldata;

	switch( length )
	{
		case 1:	/* 8 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   pcdata = ( unsigned char * )adata;			/* get to correct pointer */
		   cdata = *pcdata;								/* pick up data */
		   ldata = ( unsigned long )cdata;				/* convert to long */
		   put_user( ldata, (unsigned long *)( buf + 4 ) );	/* update user data */
		break;
		case 2:	/* 16 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   psdata = ( unsigned short * )adata;			/* get to correct pointer */
		   sdata = *psdata;								/* pick up data */
		   ldata = ( unsigned long )sdata;				/* convert to long */
		   put_user( ldata, (unsigned long *)( buf + 4 ) );	/* update user data */
		break;
		case 4:	/* 32 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   pldata = ( unsigned long * )adata;			/* get to correct pointer */
		   ldata = *pldata;								/* pick up data */
		   put_user( ldata,(unsigned long *)( buf + 4 ) );	/* update user data */
		break;
		default:
			return( -EINVAL );
		break;
	}
    return( length );
}



static ssize_t 
write( struct file *fp, const char *buf, size_t length, loff_t *offset )	
{ 
	 unsigned long adata;
	 unsigned short sdata;
	 unsigned char cdata;
	 unsigned long ldata;

 	 unsigned short *psdata;
	 unsigned char *pcdata;
	 unsigned long *pldata;

	switch( length )
	{
		case 1:	/* 8 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   pcdata = ( unsigned char * )adata;			/* get to correct pointer */
		   get_user( ldata, (unsigned long *)( buf + 4 ) );	/* pickup data */
		   cdata = ( unsigned char )ldata;				/* data to correct type */
		   *pcdata = cdata;
		break;
		case 2:	/* 16 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   psdata = ( unsigned short * )adata;			/* get to correct pointer */
		   get_user( ldata, (unsigned long *)( buf + 4 ) );	/* pickup data */
		   sdata = ( unsigned short )ldata;				/* data to correct type */
		   *psdata = sdata;
		break;
		case 4:	/* 32 bit */
		   get_user( adata, (unsigned long *)buf );		/* pickup address */
		   pldata = ( unsigned long * )adata;			/* get to correct pointer */
		   get_user( ldata, (unsigned long *)( buf + 4 ) );	/* pickup data */
		   ldata = ( unsigned long )ldata;				/* data to correct type */
		   *pldata = ldata;
		break;
		default:
			return( -EINVAL );
		break;
	}
    return( length );
}


static int
ioctl( struct inode *inode, struct file *fp, unsigned int cmd, unsigned long arg)
{
    unsigned long ldata;

	switch( cmd )
	{
		case 0:	/* reserved for future use */
		case 1:
		case 2:
		case 3:
		case 4:
		break;
		case 5:/* return carrier address */
		   ldata = ( unsigned long )dos_carrier_address;	/* convert to long */
		   put_user( ldata, (unsigned long *)arg );	/* update user data */
		break;
		case 6:/* return IRQ number */
		   ldata = ( unsigned long )dos_board_irq;		/* convert IRQ to long */
		   put_user( ldata, (unsigned long *)arg );	/* update user data */
		break;
	}
	return( cmd );
}


static struct file_operations apc8620dos_ops = {
		THIS_MODULE,/* owner of the world */
		NULL,		/* seek     */
		read,		/* read     */
		write,		/* write    */
		NULL,		/* readdir  */
		NULL,		/* poll     */
		ioctl,		/* ioctl    */
		NULL,		/* mmap     */
		open,		/* open     */
		NULL,		/* flush    */
		release,	/* release  */
		NULL,		/* fsync    */
		NULL,		/* fasync   */
		NULL,		/* lock     */
		NULL,		/* readv    */
		NULL,		/* writev   */
		NULL,		/* sendpage */
		NULL		/* get_unmapped_area */
};


static void
apc8620dos_handler( int irq, void *did, struct pt_regs *cpu_regs )
{ 
  volatile unsigned short dummyWord, nValue;
  volatile PCI_BOARD_MEMORY_MAP* pPCICard;


   pPCICard = (PCI_BOARD_MEMORY_MAP*)dos_carrier_address;
   nValue = readw((unsigned short*)&pPCICard->intPending);
   /* Call interrupt handler for any pending interrupts */

   if(nValue & IPA_INT0_PENDING || nValue & IPA_INT1_PENDING)
   {
      /* Slot A ISR code goes in here */
      if( dos_carrier_address )
          isr_408( (void *)dos_carrier_address + SLOT_A_IO_OFFSET );

	/* read IP A Interrupt Select Space */
	dummyWord = readw((unsigned short*)&pPCICard->slotAInt0);
	dummyWord = readw((unsigned short*)&pPCICard->slotAInt1);
   }			

   if(nValue & IPB_INT0_PENDING || nValue & IPB_INT1_PENDING)
   {
     /* Slot B ISR code goes in here */
/*
      if( dos_carrier_address )
          isr_xxx( dos_carrier_address + SLOT_B_IO_OFFSET );
*/
	/* read IP B Interrupt Select Space */
	dummyWord = readw((unsigned short*)&pPCICard->slotBInt0);
	dummyWord = readw((unsigned short*)&pPCICard->slotBInt1);
   }			

   if(nValue & IPC_INT0_PENDING || nValue & IPC_INT1_PENDING)
   {
 	/* Slot C ISR code goes in here */
/*
      if( dos_carrier_address )
          isr_xxx( dos_carrier_address + SLOT_C_IO_OFFSET );
*/
	/* read IP C Interrupt Select Space */
	dummyWord = readw((unsigned short*)&pPCICard->slotCInt0);
	dummyWord = readw((unsigned short*)&pPCICard->slotCInt1);
   }			

   if(nValue & IPD_INT0_PENDING || nValue & IPD_INT1_PENDING)
   {
	/* Slot D ISR code goes in here */
/*
      if( dos_carrier_address )
          isr_xxx( dos_carrier_address + SLOT_D_IO_OFFSET );
*/
	/* read IP D Interrupt Select Space */
	dummyWord = readw((unsigned short*)&pPCICard->slotDInt0);
	dummyWord = readw((unsigned short*)&pPCICard->slotDInt1);
   }			

   if(nValue & IPE_INT0_PENDING || nValue & IPE_INT1_PENDING)
   {
	/* Slot E ISR code goes in here */
/*
      if( dos_carrier_address )
          isr_xxx( dos_carrier_address + SLOT_E_IO_OFFSET );
*/
	/* read IP E Interrupt Select Space */
	dummyWord = readw((unsigned short*)&pPCICard->slotEInt0);
	dummyWord = readw((unsigned short*)&pPCICard->slotEInt1);
   }			
}



int
init_module( void ) 
{ 
  struct pci_dev *p8620;

  if(!pci_present())   /* No PCI */
     return( -ENODEV );

  p8620 = NULL;
  p8620 = pci_find_device( 0x10B5, 0x1024, p8620 );
  /* 
     PSA:
     This is APC8620dos: which means find the second APC8620. So
     we repeat the search. p8620 is not null now, so this counts
     as a "from" pointer.
  */
  p8620 = pci_find_device( 0x10B5, 0x1024, p8620 );
  if( p8620 )
  { 
    dos_carrier_address = (u32)p8620->resource[2].start;
    printk("\nVID = 0x10B5, DID = 0x1024 physical addr = %X",dos_carrier_address );
    dos_carrier_address = (u32)__ioremap( dos_carrier_address, 4096, _PAGE_PCD ); /* no cache */

    if( dos_carrier_address == 0  )
        return( -ENODEV );
    else
        printk(" mapped addr = %X\n",dos_carrier_address);

    dos_ret_val = register_chrdev ( MAJOR_NUM, DEVICE_NAME , &apc8620dos_ops );
    if( dos_ret_val < 0)
    {
       printk("Failed to register error = %d\n",dos_ret_val);
       return( dos_ret_val );
    }

    dos_board_irq = p8620->irq;
    request_irq (  dos_board_irq, apc8620dos_handler, SA_INTERRUPT | SA_SHIRQ, DEVICE_NAME, ( void *)dos_carrier_address );
    return( 0 );
  }
  return( -ENODEV );
}


void
cleanup_module( void ) 
{
   if( dos_ret_val >= 0 )
   {
      unregister_chrdev( dos_ret_val, DEVICE_NAME );
      free_irq( dos_board_irq, (void *)dos_carrier_address );
      iounmap( (void *)dos_carrier_address );
   }
}

























































































