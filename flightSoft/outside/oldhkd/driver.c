
#include "../carrier/apc8620.h"
#include "../ip220/ip220.h"

#define INTERRUPT_LEVEL 5   /* level or bus IRQ - may not be used on all systems */

word convert_volt_to_hex(float volts);

int main() {
    struct conf_blk_220 c_block_220;          /* configuration block */
    char off_data[A_SIZE];            /* allocate data storage area */
    char gain_data[A_SIZE];           /* allocate data storage area */
    word ideal_data[A_SIZE];          /* allocate data storage area */
    word cor_data[A_SIZE];            /* allocate data storage area */    
    long addr;                        /* board address */
    int channel;
  
    /* initialize the ip220 configuration block */
    c_block_220.mode = TM;                  /* mode */
    c_block_220.bit_constant = CON12;       /* constant for data correction */
    c_block_220.ideal_buf = &ideal_data[0]; /* ideal value */
    c_block_220.off_buf = &off_data[0];     /* offset buffer start */
    c_block_220.gain_buf = &gain_data[0];   /* gain buffer start */
    c_block_220.cor_buf = &cor_data[0];     /* corrected */
    c_block_220.bCarrier = FALSE;           /* indicate no carrier initialized and set up yet */
    c_block_220.bInitialized = FALSE;       /* indicate not ready to talk to IP */
    c_block_220.slotLetter = SLOT_A;
    c_block_220.nHandle = 0;                /* make handle to a closed carrier board */
    
    /* Check for Carrier Library */
    if(InitCarrierLib() != S_OK) {
        printf("\nUnable to initialize the carrier library. Exiting program.\n");
        exit(0);
    }
    
    /* Connect to Carrier */ 
    if(CarrierOpen(0, &c_block_220.nHandle) != S_OK) {
        printf("\nUnable to Open instance of carrier.\n");
        exit(0);
    } 

    /* Set up the board */
    GetCarrierAddress(c_block_220.nHandle, &addr);
    SetCarrierAddress(c_block_220.nHandle, addr);
    if(CarrierInitialize(c_block_220.nHandle) == S_OK) { /* Initialize Carrier */
       c_block_220.bCarrier = TRUE;
       SetInterruptLevel(c_block_220.nHandle, INTERRUPT_LEVEL);/* Set carrier interrupt level */
       printf("Setting board address to: %x \n", addr);
    } else {
        printf("\nUnable to Open initialize the carrier to address: %x.\n", addr);
        exit(0);
    }

    /* Set up the IP slot header */
    c_block_220.slotLetter = 'D';
    if(GetIpackAddress(c_block_220.nHandle, c_block_220.slotLetter, &addr) != S_OK) {
	printf("\nUnable to Get Ipack Address.\n");
        c_block_220.bInitialized = FALSE;
        exit(0);
    } else {
	printf("\nAble to Get Ipack Address for address: %c.\n", c_block_220.slotLetter);
	c_block_220.brd_ptr = (struct map220 *)addr;
        c_block_220.bInitialized = TRUE;
    }

    /* Read in calibration constants
    rccid220(c_blk);

    /* for every channel send out a 273 * channel in BOB format*/
     for (channel = 0; channel < A_SIZE; channel++) {
         ideal_data[channel] =  convert_volt_to_hex(-0.05);
         printf("Channel %d, BOB: %x Volts: %f\n", channel, ideal_data[channel], (float)ideal_data[channel]/4095*20 - 10);
         wro220(&c_block_220, channel, (ideal_data[channel] << 4));
         sleep(1);
     }
    sleep(10000);
}

word convert_volt_to_hex(float volts) {
   float hex;

   hex = 4095.0 * ((volts + 10.0)/20.0); 
   return (word) hex;
}

