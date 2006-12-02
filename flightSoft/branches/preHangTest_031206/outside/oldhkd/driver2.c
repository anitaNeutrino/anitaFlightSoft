
#include "../carrier/apc8620.h"
#include "../ip320/ip320.h"

#define INTERRUPT_LEVEL 3   /* level or bus IRQ - may not be used on all systems */

struct scan_array s_array[SA_SIZE];

int main() {
    struct conf_blk_320 c_block_320;          /* configuration block */
    word az_data[SA_SIZE];            /* allocate data storage area */
    word cal_data[SA_SIZE];           /* allocate data storage area */
    word raw_data[SA_SIZE];           /* allocate data storage area */
    long cor_data[SA_SIZE];           /* allocate data storage area */
    long addr;                        /* board address */
    int channel;
    int i;
    float s, z;

    for( i = 0; i < SA_SIZE; i++ ) {
        s_array[i].gain = 1;            /*  gain=1 */
        s_array[i].chn = i;             /*  channel */
        az_data[i] = 0;                 /* clear auto zero buffer */
        cal_data[i] = 0;                /* clear calibration buffer */
        raw_data[i] = 0;                /* clear raw input buffer */
        cor_data[i] = 0;                /* clear corrected data buffer */
    }
  
    /* initialize the ip320 configuration block */
    c_block_320.range = RANGE_10TO10;       /* default +-5 V */
    c_block_320.trigger = STRIG;            /* 0 = software triggering */
    c_block_320.mode = SEI;                 /* mode */
    c_block_320.gain = GAIN_X1;             /* gain for analog input */
    c_block_320.average = 1;                /* number of samples to average */
    c_block_320.channel = 0;                /* default channel */
    c_block_320.data_mask = BIT12;          /* A/D converter data mask */
    c_block_320.bit_constant = CON12;       /* constant for data correction */
    c_block_320.s_raw_buf = &raw_data[0];   /* raw buffer start */
    c_block_320.s_az_buf = &az_data[0];     /* auto zero buffer start */
    c_block_320.s_cal_buf = &cal_data[0];   /* calibration buffer start */
    c_block_320.s_cor_buf = &cor_data[0];   /* corrected buffer start */
    c_block_320.sa_start = &s_array[0];     /* address of start of scan array */
    c_block_320.sa_end = c_block_320.sa_start;  /* address of end of scan array */
    c_block_320.bCarrier = FALSE;           /* indicate no carrier initialized and set up yet */
    c_block_320.bInitialized = FALSE;       /* indicate not ready to talk to IP */
    c_block_320.slotLetter = SLOT_C;
    c_block_320.nHandle = 0;                /* make handle to a closed carrier board */
    
    /* Check for Carrier Library */
    if(InitCarrierLib() != S_OK) {
        printf("\nUnable to initialize the carrier library. Exiting program.\n");
        exit(0);
    }
    
    /* Connect to Carrier */ 
    if(CarrierOpen(0, &c_block_320.nHandle) != S_OK) {
        printf("\nUnable to Open instance of carrier.\n");
        exit(0);
    } 

    /* Set up the board */
    GetCarrierAddress(c_block_320.nHandle, &addr);
    SetCarrierAddress(c_block_320.nHandle, addr);
    if(CarrierInitialize(c_block_320.nHandle) == S_OK) { /* Initialize Carrier */
       c_block_320.bCarrier = TRUE;
       SetInterruptLevel(c_block_320.nHandle, INTERRUPT_LEVEL);/* Set carrier interrupt level */
       printf("Setting board address to: %x \n", addr);
    } else {
        printf("\nUnable to Open initialize the carrier to address: %x.\n", addr);
        exit(0);
    }

    /* Set up the IP slot header */
    c_block_320.slotLetter = 'C';
    if(GetIpackAddress(c_block_320.nHandle, c_block_320.slotLetter, &addr) != S_OK) {
	printf("\nUnable to Get Ipack Address.\n");
        c_block_320.bInitialized = FALSE;
        exit(0);
    } else {
	printf("\nAble to Get Ipack Address for address: %c.\n", c_block_320.slotLetter);
	c_block_320.brd_ptr = (struct map320 *)addr;
        c_block_320.bInitialized = TRUE;
    }
   
    /* Acuire raw input data and print it out every sec */
    switch(c_block_320.range) {
        case RANGE_5TO5:
           z = -5.0000;
           s = 10.0000;
        break;

        case RANGE_0TO10:
           z =  0.0;
           s = 10.0000;
        break;

        default:        /*  RANGE_10TO10 */
           z = -10.0000;
           s =  20.0000;
        break;
    }

    ainmc320(&c_block_320);  /* acquire input data */
    
    while(1) {
       for(i = 0; i < SA_SIZE;i++)  {
          printf("raw data channel %d: %x ",i, raw_data[i]);
          printf("%2.3f Volts \n",((raw_data[i] >> 4) / (float)0xfff * s + z)); 
       }
       sleep(30); 
    }
}

