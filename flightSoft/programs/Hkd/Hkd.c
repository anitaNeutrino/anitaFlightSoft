/*! \file Hkd.c
    \brief The Hkd program that creates Hk objects 
    
    Reads in all the various different housekeeping quantities (temperatures, 
    voltages, etc.)

    Novemember 2004  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <time.h>

/* Flight soft includes */
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip320/ip320.h"

#define NUM_ACRO_CHANS 40

/* SBS Temperature Reading Functions */
void printSBSTemps (void);
int readSBSTemps (int *localTemp, int *remoteTemp);

/* Acromag Functions */
void acromagSetup();
int ip320_1Setup();
int ip320_2Setup();
int ip320_1Read();
int ip320_2Read();
int ip320_1GetCal();
int ip320_2GetCal();
void dumpArrayValues_1();
void dumpArrayValues_2();


/* global variables for acromag control */
int carrierHandle;
struct conf_blk_320 cblk_320_1;
struct conf_blk_320 cblk_320_2;
/* tables for ip320 data and calibration constants */
/* note sa_size is sort of arbitrary; 
   single channel reads only use first one */
struct scan_array ip320_1_s_array[NUM_ACRO_CHANS]; 
word ip320_1_az_data[NUM_ACRO_CHANS];        
word ip320_1_cal_data[NUM_ACRO_CHANS];       
word ip320_1_raw_data[NUM_ACRO_CHANS];       
long ip320_1_cor_data[NUM_ACRO_CHANS];   
struct scan_array ip320_2_s_array[NUM_ACRO_CHANS]; 
word ip320_2_az_data[NUM_ACRO_CHANS];        
word ip320_2_cal_data[NUM_ACRO_CHANS];       
word ip320_2_raw_data[NUM_ACRO_CHANS];       
long ip320_2_cor_data[NUM_ACRO_CHANS];   

/* Acromag constants */
#define INTERRUPT_LEVEL 10



int main (int argc, char *argv[])
{
    int retVal;
/*     int localSBSTemp,remoteSBSTemp; */
    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Log stuff */
    char *progName=basename(argv[0]);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
    if (status == CONFIG_E_OK) {
    }
    
    retVal=0;
    printSBSTemps();
    acromagSetup();
    
   /*  readSBSTemps(&localSBSTemp,&remoteSBSTemp); */
/*     printf("Local: %d (C)\tRemote: %d (C)\n",localSBSTemp,remoteSBSTemp); */
    printSBSTemps();
    ip320_1Setup();
    ip320_1GetCal();
    ip320_2Setup();
    ip320_2GetCal();
    ip320_1Read();
    ip320_2Read();
    dumpArrayValues_1();
    dumpArrayValues_2();
    return 0;
}

int readSBSTemps (int *localTemp, int *remoteTemp)
{
    API_RESULT main_Api_Result;
    int gotError=0;

    *localTemp = 0;
    *remoteTemp = 0;
    main_Api_Result = 
	fn_ApiCr7ReadLocalTemperature(localTemp);
    
    if (main_Api_Result != API_SUCCESS)
    {
	gotError+=1;
	syslog(LOG_WARNING,"Couldn't read SBS Local Temp: %d\t%s",
	       main_Api_Result,fn_ApiGetErrorMsg(main_Api_Result));
    }

    main_Api_Result = 
	fn_ApiCr7ReadRemoteTemperature(remoteTemp);
    
    if (main_Api_Result != API_SUCCESS)
    {
	gotError+=2;
	syslog(LOG_WARNING,"Couldn't read SBS Remote Temp: %d\t%s",
	       main_Api_Result,fn_ApiGetErrorMsg(main_Api_Result));
    }

    return gotError;
}


void printSBSTemps (void)
{
    int localTemp,remoteTemp,retVal,unixTime;
    retVal=readSBSTemps(&localTemp,&remoteTemp);
    unixTime=time(NULL);
    if(!retVal) 
	printf("%d %d %d\n",unixTime,localTemp,remoteTemp);
    /* Don't know what to do if it doesn't work */
}

 
void acromagSetup()
/* Seemingly does exactly what it says on the tin */
{

  long addr=0;
  
  /* Check for Carrier Library */
  if(InitCarrierLib() != S_OK) {
    printf("\nCarrier library failure");
    exit(1);
  }
  
  /* Connect to Carrier */ 
  if(CarrierOpen(0, &carrierHandle) != S_OK) {
    printf("\nUnable to Open instance of carrier.\n");
    exit(2);
  }
  cblk_320_1.nHandle = carrierHandle;
  cblk_320_2.nHandle = carrierHandle;
  
  cblk_320_1.slotLetter='B';
  cblk_320_2.slotLetter='C';


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


  cblk_320_1.bCarrier=TRUE;
  cblk_320_2.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_320_1.slotLetter,
		     (long *) &cblk_320_1.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP320\n.");
      exit(5);
    }
  if(GetIpackAddress(carrierHandle,cblk_320_2.slotLetter,
		     (long *) &cblk_320_2.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP320 (2)\n.");
      exit(6);
    }
  cblk_320_1.bInitialized = TRUE;
  cblk_320_2.bInitialized = TRUE;
}


int ip320_1Setup()
{
  int i;
  rsts320(&cblk_320_1);
  if((byte)cblk_320_1.id_prom[5]!=0x32) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",cblk_320_1.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)cblk_320_1.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)cblk_320_1.id_prom[5]);
      printf("\nRevision:                    %X",(byte)cblk_320_1.id_prom[6]);
      printf("\nReserved:                    %X",(byte)cblk_320_1.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)cblk_320_1.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)cblk_320_1.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)cblk_320_1.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)cblk_320_1.id_prom[11]);
      exit(0);
  }
  else {
      printf("Board ID correct %X\n",(byte)cblk_320_1.id_prom[5]);
  }
  for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
      ip320_1_s_array[i].gain = GAIN_X1;            /*  gain=2 */
      ip320_1_s_array[i].chn = i;             /*  channel */
      ip320_1_az_data[i] = 0;                 /* clear auto zero buffer */
      ip320_1_cal_data[i] = 0;                /* clear calibration buffer */
      ip320_1_raw_data[i] = 0;                /* clear raw input buffer */
      ip320_1_cor_data[i] = 0;                /* clear corrected data buffer */
    }
  cblk_320_1.range = RANGE_5TO5;       /* full range */
  cblk_320_1.trigger = STRIG;            /* 0 = software triggering */
  cblk_320_1.mode = SEI;                 /* differential input */
  cblk_320_1.gain = GAIN_X1;             /* gain for analog input */
  cblk_320_1.average = 1;                /* number of samples to average */
  cblk_320_1.channel = 0;                /* default channel */
  cblk_320_1.data_mask = BIT12;          /* A/D converter data mask */
  cblk_320_1.bit_constant = CON12;       /* constant for data correction */
  cblk_320_1.s_raw_buf = &ip320_1_raw_data[0];   /* raw buffer start */
  cblk_320_1.s_az_buf = &ip320_1_az_data[0];     /* auto zero buffer start */
  cblk_320_1.s_cal_buf = &ip320_1_cal_data[0];   /* calibration buffer start */
  cblk_320_1.s_cor_buf = &ip320_1_cor_data[0];   /* corrected buffer start */
  cblk_320_1.sa_start = &ip320_1_s_array[0];     /* address of start of scan array */
  cblk_320_1.sa_end = &ip320_1_s_array[NUM_ACRO_CHANS-1];  /* address of end of scan array*/
  return 0;
}


int ip320_2Setup()
{
  int i;  
  rsts320(&cblk_320_2);
  if((byte)cblk_320_2.id_prom[5]!=0x32) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",cblk_320_2.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)cblk_320_2.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)cblk_320_2.id_prom[5]);
      printf("\nRevision:                    %X",(byte)cblk_320_2.id_prom[6]);
      printf("\nReserved:                    %X",(byte)cblk_320_2.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)cblk_320_2.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)cblk_320_2.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)cblk_320_2.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)cblk_320_2.id_prom[11]);
      exit(0);
  }
  else {
      printf("Board ID correct %X\n",(byte)cblk_320_2.id_prom[5]);
  }
  for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
      ip320_2_s_array[i].gain = GAIN_X1;            /*  gain=2 */
      ip320_2_s_array[i].chn = i;             /*  channel */
      ip320_2_az_data[i] = 0;                 /* clear auto zero buffer */
      ip320_2_cal_data[i] = 0;                /* clear calibration buffer */
      ip320_2_raw_data[i] = 0;                /* clear raw input buffer */
      ip320_2_cor_data[i] = 0;                /* clear corrected data buffer */
    }
  cblk_320_2.range = RANGE_5TO5;       /* full range */
  cblk_320_2.trigger = STRIG;            /* 0 = software triggering */
  cblk_320_2.mode = SEI;                 /* differential input */
  cblk_320_2.gain = GAIN_X1;             /* gain for analog input */
  cblk_320_2.average = 1;                /* number of samples to average */
  cblk_320_2.channel = 0;                /* default channel */
  cblk_320_2.data_mask = BIT12;          /* A/D converter data mask */
  cblk_320_2.bit_constant = CON12;       /* constant for data correction */
  cblk_320_2.s_raw_buf = &ip320_2_raw_data[0];   /* raw buffer start */
  cblk_320_2.s_az_buf = &ip320_2_az_data[0];     /* auto zero buffer start */
  cblk_320_2.s_cal_buf = &ip320_2_cal_data[0];   /* calibration buffer start */
  cblk_320_2.s_cor_buf = &ip320_2_cor_data[0];   /* corrected buffer start */
  cblk_320_2.sa_start = &ip320_2_s_array[0];     /* address of start of scan array */
  cblk_320_2.sa_end = &ip320_2_s_array[NUM_ACRO_CHANS-1];  /* address of end of scan array*/
  return 0;
}


int ip320_1Read()
{
  ainmc320(&cblk_320_1);
  mccd320(&cblk_320_1);
  return 0;
}

int ip320_2Read()
{
  ainmc320(&cblk_320_2);
  mccd320(&cblk_320_2);
  return 0;
}

int ip320_1GetCal()
{
  byte temp_mode;
  temp_mode=cblk_320_1.mode;
  cblk_320_1.mode= AZV; /* auto zero */
  ainmc320(&cblk_320_1);
  cblk_320_1.mode= CAL; /* calibration */
  ainmc320(&cblk_320_1);
  cblk_320_1.mode=temp_mode;
  /* really should only do this every so often, */
  /* and send down the cal constants */
  return 0;
}


int ip320_2GetCal()
{
  byte temp_mode;
  temp_mode=cblk_320_2.mode;
  cblk_320_2.mode= AZV; /* auto zero */
  ainmc320(&cblk_320_2);
  cblk_320_2.mode= CAL; /* calibration */
  ainmc320(&cblk_320_2);
  cblk_320_2.mode=temp_mode;
  /* really should only do this every so often, */
  /* and send down the cal constants */
  return 0;
}

void dumpArrayValues_1() {
    int i;
    printf("Data from %c\n\n",cblk_320_1.slotLetter);

    printf("Channel: ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_1_s_array[i].chn);
    }
    printf("\n\n");
    printf("Gain:    ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_1_s_array[i].gain);
    }
    printf("\n\n");
    printf("AVZ:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_1_az_data[i]);
    }
    printf("\n\n");
    printf("CAL:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_1_cal_data[i]);
    }
    printf("\n\n");
    printf("RAW:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_1_raw_data[i]);
    }
    printf("\n\n");
    printf("COR:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%lX ",ip320_1_cor_data[i]);
    }
    printf("\n\n");
}


void dumpArrayValues_2() {
    int i;
    printf("Data from %c\n\n",cblk_320_2.slotLetter);

    printf("Channel: ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_2_s_array[i].chn);
    }
    printf("\n\n");
    printf("Gain:    ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%d ",ip320_2_s_array[i].gain);
    }
    printf("\n\n");
    printf("AVZ:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_2_az_data[i]);
    }
    printf("\n\n");
    printf("CAL:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_2_cal_data[i]);
    }
    printf("\n\n");
    printf("RAW:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%X ",ip320_2_raw_data[i]);
    }
    printf("\n\n");
    printf("COR:     ");
    for( i = 0; i < NUM_ACRO_CHANS; i++ ) 
    {
	printf("%lX ",ip320_2_cor_data[i]);
    }
    printf("\n\n");
}
