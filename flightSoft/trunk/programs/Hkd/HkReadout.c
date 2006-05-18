/* Quick stupid program to read out analog voltages. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "anitaStructures.h"
/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip320/ip320.h"
#define INTERRUPT_LEVEL 10
#define NUM_ACRO_CHANS 40

//#define USE_RANGE RANGE_5TO5

const char carrierDefault[]="/dev/apc8620";

void dumpArrayValues();
void acromagSetup(unsigned int ip320BoardLocation, char *ip320carrier);
int ip320_Setup();
int ip320_GetCal();
int ip320_Read();
void dumpArrayValues();

/* global variables for acromag control */
int carrierHandle;
struct conf_blk_320 cblk_320;
struct scan_array ip320_s_array[NUM_ACRO_CHANS]; 
word ip320_az_data[NUM_ACRO_CHANS];        
word ip320_cal_data[NUM_ACRO_CHANS];       
word ip320_raw_data[NUM_ACRO_CHANS];       
long ip320_cor_data[NUM_ACRO_CHANS];
int useRange=5;


/* Prettyprinting stuff */
typedef struct {
  char *description;
  float conversion;
  float offset;
  int active;
} ip320_desc; /* IP320 descriptor structure */
ip320_desc desc[NUM_ACRO_CHANS];
char *header = NULL;
int prettyFormat = 0;
/* Advanced prettyprinting stuff */
#define MAX_COLUMN 5
#define MAX_ROW 20
int numRows = 0;
int rowaddr[MAX_ROW][MAX_COLUMN];
void prettyprint();

int main (int argc, char **argv)
{
  int i;

  int status;
  char *eString;
  char ip320carrier[FILENAME_MAX];
  sprintf(ip320carrier,"%s",carrierDefault);
  int ip320BoardLocation;
  int readout=0;
  char hkreadout[9];

  if (argc==2) {
    readout=atoi(argv[1]);
  }
  else {
    printf("specify board 1-3\n");
    exit(1);
  }
  /* Configuration */
  /* deactivates all chans */
  memset(desc, 0, sizeof(ip320_desc)*NUM_ACRO_CHANS); 
  kvpReset();
  sprintf(hkreadout,"hkreadout%d",readout);
  status = configLoad("HkReadout.config",hkreadout);
  status += configLoad ("HkReadout.config","hkd") ;
  eString = configErrorString(status);
  if (status == CONFIG_E_OK) {
    char *temp;
    temp = kvpGetString("ip320carrier");
    if (temp != NULL) sprintf(ip320carrier,"%s",temp);
    ip320BoardLocation = kvpGetInt("ip320BoardLocation",-1);
    prettyFormat = kvpGetInt("prettyformat",0);
    int ip320Ranges[3];
    int tempNum=3;
    kvpGetIntArray("ip320Ranges",ip320Ranges,&tempNum);
//    useRange=ip320Ranges[readout-1];
    numRows= kvpGetInt("numrows",0);
    if (numRows > MAX_ROW) numRows=MAX_ROW;
    if (numRows)
      {
         char configstring[50];
         int i, nentries;
    	 memset(rowaddr, 0, sizeof(unsigned int)*MAX_ROW*MAX_COLUMN);
         for (i=0;i<numRows;i++)
         {
           nentries=5;
           sprintf(configstring,"row%dchannels", i+1);
           kvpGetIntArray(configstring,rowaddr[i],&nentries);
         }
	 for (i=0;i<numRows;i++)
           printf("%d %d %d %d %d\n", rowaddr[i][0], rowaddr[i][1],
                  rowaddr[i][2], rowaddr[i][3], rowaddr[i][4]);
      }
    temp = kvpGetString("headername");
    if (temp != NULL) header=temp;
    for (i=0;i<NUM_ACRO_CHANS;i++)
      {
	char configstring[50];
	sprintf(configstring, "chan%dname", i+1);
	temp = kvpGetString(configstring);
	if (temp != NULL) {
	  desc[i].description = temp;
	  desc[i].active = 1;
	  sprintf(configstring, "chan%dconvert", i+1);
	  desc[i].conversion = kvpGetFloat(configstring, 1.0);
          sprintf(configstring, "chan%doffset", i+1);
          desc[i].offset = kvpGetFloat(configstring, 0.0); 
	}
      }
  }
  else {
    printf("Unable to read config file.\n");
    printf("%s\n", eString);
    exit(1);
  }
  acromagSetup(ip320BoardLocation, ip320carrier);
  ip320_Setup();
  ip320_GetCal();
  ip320_Read();

  dumpArrayValues(); 
  prettyprint();
  return 0;
}

void acromagSetup(unsigned int ip320BoardLocation, char *ip320carrier)
{
  long addr = 0;

    /* Check for Carrier Library */
  if(InitCarrierLib() != S_OK) {
    printf("\nCarrier library failure");
    exit(1);
  }
  
  /* Connect to Carrier */ 
  if(CarrierOpenDev(0, &carrierHandle, ip320carrier) != S_OK) {
    printf("\nUnable to Open instance of carrier.\n");
    exit(2);
  }
  cblk_320.nHandle = carrierHandle;
  
  switch (ip320BoardLocation) {
      case 0: cblk_320.slotLetter='A'; break;
      case 1: cblk_320.slotLetter='B'; break;
      case 2: cblk_320.slotLetter='C'; break;
      case 3: cblk_320.slotLetter='D'; break;
      default: 
	  printf("No idea what board %d is.\n",ip320BoardLocation);
	  exit(3);
  }
  printf("Inititalizing IP320 in slot %c\n",cblk_320.slotLetter); 



  if(CarrierInitialize(carrierHandle) == S_OK) 
    { /* Initialize Carrier */
      SetInterruptLevel(carrierHandle, INTERRUPT_LEVEL);
      /* Set carrier interrupt level */
    } 
  else 
    {
      printf("\nUnable initialize the carrier: %lX", addr);
      exit(3);
    }

  cblk_320.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_320.slotLetter,
		     (long *) &cblk_320.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP320\n.");
      exit(5);
    }
  cblk_320.bInitialized = TRUE;
}

int ip320_Setup()
{
  int i;
  rsts320(&cblk_320);
  if((byte)cblk_320.id_prom[5]!=0x32) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",cblk_320.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)cblk_320.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)cblk_320.id_prom[5]);
      printf("\nRevision:                    %X",(byte)cblk_320.id_prom[6]);
      printf("\nReserved:                    %X",(byte)cblk_320.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)cblk_320.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)cblk_320.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)cblk_320.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)cblk_320.id_prom[11]);
      exit(0);
  }
  else {
      printf("Board ID correct %X\n",(byte)cblk_320.id_prom[5]);
  }
  for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
      ip320_s_array[i].gain = GAIN_X1;            /*  gain=2 */
      ip320_s_array[i].chn = i;             /*  channel */
      ip320_az_data[i] = 0;                 /* clear auto zero buffer */
      ip320_cal_data[i] = 0;                /* clear calibration buffer */
      ip320_raw_data[i] = 0;                /* clear raw input buffer */
      ip320_cor_data[i] = 0;                /* clear corrected data buffer */
    }
//  cblk_320.range = RANGE_10TO10;   /*RJN hack for Kurt test*/
//  if(useRange==5)
  cblk_320.range = RANGE_5TO5;   /*RJN hack for Kurt test*/
//  else
//      cblk_320.range = RANGE_10TO10;   /*RJN hack for Kurt test*/
//  cblk_320.range = RANGE_10TO10;   /*RJN hack for Kurt test*/
  /* full range */
  cblk_320.trigger = STRIG;            /* 0 = software triggering */
  cblk_320.mode = SEI;                 /* differential input */
  cblk_320.gain = GAIN_X1;             /* gain for analog input */
  cblk_320.average = 1;                /* number of samples to average */
  cblk_320.channel = 0;                /* default channel */
  cblk_320.data_mask = BIT12;          /* A/D converter data mask */
  cblk_320.bit_constant = CON12;       /* constant for data correction */
  cblk_320.s_raw_buf = &ip320_raw_data[0];   /* raw buffer start */
  cblk_320.s_az_buf = &ip320_az_data[0];     /* auto zero buffer start */
  cblk_320.s_cal_buf = &ip320_cal_data[0];   /* calibration buffer start */
  cblk_320.s_cor_buf = &ip320_cor_data[0];   /* corrected buffer start */
  cblk_320.sa_start = &ip320_s_array[0];     /* address of start of scan array */
  cblk_320.sa_end = &ip320_s_array[NUM_ACRO_CHANS-1];  /* address of end of scan array*/
  return 0;
}

int ip320_GetCal()
{
  byte temp_mode;
  temp_mode=cblk_320.mode;
  cblk_320.mode= AZV; /* auto zero */
  ainmc320(&cblk_320);
  cblk_320.mode= CAL; /* calibration */
  ainmc320(&cblk_320);
  cblk_320.mode=temp_mode;
  /* really should only do this every so often, */
  /* and send down the cal constants */
  return 0;
}


int ip320_Read()
{
  ainmc320(&cblk_320);
  mccd320(&cblk_320);
  return 0;
}

void prettyprint()
{
  int i, linecount=0;
  if (header)
    printf("%s:\n", header);
  else
    printf("No description available!\n");
  
  if (!numRows || !prettyFormat) {
  for (i=0;i<NUM_ACRO_CHANS;i++)
    {
      if (prettyFormat)
        {
      if (desc[i].active)
	{
	    /* RJN HACK */
	    printf("%5.5s: %+7.3f ", desc[i].description, (ip320_cor_data[i]*10.0/4095.-5.0*desc[i].conversion+desc[i].offset));
//	  printf("%5.5s: %+7.2f ", desc[i].description, (ip320_cor_data[i]*20.0/4095.-10.0));
//	  printf("%5.5s: %+7.2f ", desc[i].description,ip320_cor_data[i]);
	  linecount++;
	  if (!(linecount%5)) printf("\n");
	}
        }
      else
        {
          printf("CH%2.2d: %5.3f ", i+1, (ip320_cor_data[i]*10.0/4095.-5.0));
          if (!((i+1)%5)) printf("\n");
        }
    }
  printf("\n");
  }
  else {
  for (i=0;i<numRows;i++) {
      int j;
      for (j=0;j<MAX_COLUMN;j++) {
          if (rowaddr[i][j]) {
             int chan = rowaddr[i][j]-1;  
	    /* RJN HACK */
	     if(useRange==10)
		 printf("%5.5s: %+7.2f ", desc[chan].description, (ip320_cor_data[chan]*20.0/4095.-10.0)*desc[chan].conversion+desc[chan].offset);
	     else 
		 printf("%5.5s: %+7.2f ", desc[chan].description, (ip320_cor_data[chan]*10.0/4095.-5.0)*desc[chan].conversion+desc[chan].offset);
//             printf("%5.5s: %+7.2f ", desc[chan].description, (ip320_cor_data[chan]*10.0/4095.-5.0)*desc[chan].conversion+desc[chan].offset);
          } else printf("               ");
      }
      printf("\n");
  }
}

}

void dumpArrayValues() {
    int i;
    printf("Data from %c\n\n",cblk_320.slotLetter);

    printf("Channel: ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_s_array[i].chn);
    }
    printf("\n\n");
    printf("Gain:    ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_s_array[i].gain);
    }
    printf("\n\n");
    printf("AVZ:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_az_data[i]);
    }
    printf("\n\n");
    printf("CAL:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_cal_data[i]);
    }
    printf("\n\n");
    printf("RAW:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_raw_data[i]);
    }
    printf("\n\n");
    printf("COR:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d\t%+7.3f\n",i,ip320_cor_data[i]*10.0/4095.-5.0);
    }
    printf("\n\n");
}
