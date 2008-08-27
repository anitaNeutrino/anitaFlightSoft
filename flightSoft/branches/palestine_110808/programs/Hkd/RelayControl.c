/*! \file RelayControl.c
    \brief The RelayControl program that controls the relays 
    
    Code for the IP470 module that controls the relays. 

    April 2005  rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <libgen.h> //For Mac OS X

/* Flight soft includes */
#include "includes/anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "includes/anitaStructures.h"

/* Vendor includes */
#include "cr7.h"
#include "carrier/apc8620.h"
#include "ip470/ip470.h"



/* Acromag Functions */
void acromagSetup(int ip470BoardLocation, char *ip470carrier);
void ip470Setup();
int ip470Write(int port, int chan,int val);
int usage(char *progName);

void gpsOn();
void gpsOff();
void ndOn();
void ndOff();
void rfcmOn();
void rgcmOff();

/* global variables for acromag control */
int carrierHandle;
struct cblk470 cblk_470;

/* Acromag constants */
#define INTERRUPT_LEVEL 10

/* Acromag carrier default */
const char carrierDefault[] = "/dev/apc8620";

int main (int argc, char *argv[])
{
    char ip470carrier[FILENAME_MAX];
    sprintf(ip470carrier,"%s",carrierDefault);
    int retVal,port,channel,value,ip470BoardLocation=0;
    int gpsOnPort,gpsOffPort,ndOnPort,ndOffPort,rfcmOnPort,rfcmOffPort;

    /* Config file thingies */
    int status=0;
    char* eString ;

    /* Log stuff */
    char *progName=basename(argv[0]);


    if(argc==4) {
	port = atoi(argv[1]);
	channel = atoi(argv[2]);
	value = atoi(argv[3]);
	
    }
    else return usage(progName);


    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;

   
    /* Load Config */
    kvpReset () ;
    status = configLoad ("Hkd.config","relaycontrol") ;
    eString = configErrorString (status) ;

    /* Get Port Numbers */
    if (status == CONFIG_E_OK) {
        char *temp;
	temp = kvpGetString("ip470carrier");
	if (temp != NULL) sprintf(ip470carrier,"%s",temp);
	ip470BoardLocation=kvpGetInt("ip470BoardLocation",-1);
	gpsOnPort=kvpGetInt("gpsOnLogic",-1);
	gpsOffPort=kvpGetInt("gpsOffLogic",-1);
	ndOnPort=kvpGetInt("ndOnLogic",-1);
	ndOffPort=kvpGetInt("ndOffLogic",-1);
	rfcmOnPort=kvpGetInt("rfcmOnLogic",-1);
	rfcmOffPort=kvpGetInt("rfcmOffLogic",-1);
    }
    else {
	printf("Error trying to read %s:\t%s\n","Hkd.config",eString);
    }
    
    printf("ip470carrier: %s\n", ip470carrier);
    retVal=0;

    acromagSetup(ip470BoardLocation, ip470carrier);
    ip470Setup();
	
    retVal=ip470Write(port,channel,value);
    if(retVal<0) {
	printf("Error setting: port %d, chan %d, value %d\n%d\n",port,channel,value,retVal);
	return 0;
    }
    printf("Set: port %d, chan %d, value %d\n%d\n",port,channel,value,retVal);
    return 0;
}

 
void acromagSetup(int ip470BoardLocation, char *ip470carrier)
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

  cblk_470.nHandle = carrierHandle;
  
  switch (ip470BoardLocation) {
      case 0: cblk_470.slotLetter='A'; break;
      case 1: cblk_470.slotLetter='B'; break;
      case 2: cblk_470.slotLetter='C'; break;
      case 3: cblk_470.slotLetter='D'; break;
      default: 
	  printf("No idea what board %d is.\n",ip470BoardLocation);
	  exit(3);
  }
  printf("Inititalizing IP470 in slot %c\n",cblk_470.slotLetter); 


  if(CarrierInitialize(carrierHandle) == S_OK) 
    { /* Initialize Carrier */
      SetInterruptLevel(carrierHandle, INTERRUPT_LEVEL);
      /* Set carrier interrupt level */
    } 
  else 
    {
      printf("\nUnable initialize the carrier %lX", addr);
      exit(3);
    }


  cblk_470.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_470.slotLetter,
		     (long *) &cblk_470.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP470\n.");
      exit(6);
    }
  cblk_470.bInitialized = TRUE;
}


void ip470Setup()
{

  int i;
  /* channel assignments:
 ??
  */

  rmid470(&cblk_470);
  if((byte)cblk_470.id_prom[5]!=0x8) {
      printf("Board ID Wrong\n\n");
      printf("Board ID Information\n");
      printf("\nIdentification:              ");
      for(i = 0; i < 4; i++)          /* identification */
	  printf("%c",cblk_470.id_prom[i]);
      printf("\nManufacturer's ID:           %X",(byte)cblk_470.id_prom[4]);
      printf("\nIP Model Number:             %X",(byte)cblk_470.id_prom[5]);
      printf("\nRevision:                    %X",(byte)cblk_470.id_prom[6]);
      printf("\nReserved:                    %X",(byte)cblk_470.id_prom[7]);
      printf("\nDriver I.D. (low):           %X",(byte)cblk_470.id_prom[8]);
      printf("\nDriver I.D. (high):          %X",(byte)cblk_470.id_prom[9]);
      printf("\nTotal I.D. Bytes:            %X",(byte)cblk_470.id_prom[10]);
      printf("\nCRC:                         %X\n",(byte)cblk_470.id_prom[11]);
      exit(0);
  }
  else {
      printf("Board ID correct %d\n",(byte)cblk_470.id_prom[5]);
  }

  cblk_470.param=0;
  cblk_470.e_mode=0;
  cblk_470.mask_reg=0;
  cblk_470.deb_control=0;
  cblk_470.deb_clock=1;
  cblk_470.enable=0;
  cblk_470.vector=0;
  for(i = 0; i != 2; i++) /* Curious why != as oppposed to < */
    {
      cblk_470.ev_control[i] = 0;/* event control registers */
      cblk_470.deb_duration[i] = 0;/* debounce duration registers */
    }
  for(i = 0; i < 6; i++)
    cblk_470.event_status[i] = 0; /* initialize event status flags */
}

int ip470Write(int port,int chan,int val)
/* Each port is a group of eight channels. 
   Port 0 == 0-7
   Port 1 == 8-15
   Port 2 == 16-23
   Port 3 == 24-31
   Port 4 == 32-39
   Port 5 == 40-47
*/
{
  return wpnt470(&cblk_470,port,chan,val);
}

int usage(char *progName) {
    printf("usage:\t%s <port[0-5]> <channel[0-7]> <value[0-1]>\n",progName);
    return 0;
}
