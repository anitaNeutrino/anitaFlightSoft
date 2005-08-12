/* Housekeeping control routines for ANITA-lite Acromag setup*/
/* Vendor routines modified by JJB to fix global namespace collision */
#include "../carrier/apc8620.h"
#include "../ip220/ip220.h"
#include "../ip320/ip320.h"
#include "../ip470/ip470.h"
#include <string.h>
#include <sys/timeb.h>
#include <math.h>
#include <signal.h>
#define INTERRUPT_LEVEL 10

/* global variables for acromag control */
int carrierHandle;
struct conf_blk_220 cblk_220;
struct conf_blk_320 cblk_320;
struct conf_blk_470 cblk_470;
/* tables for ip220 data and calibration constants */
char ip220_off_data[A_SIZE];
char ip220_gain_data[A_SIZE]; 
word ip220_ideal_data[A_SIZE];   
word ip220_cor_data[A_SIZE];     
/* tables for ip320 data and calibration constants */
/* note sa_size is sort of arbitrary; 
   single channel reads only use first one */
struct scan_array ip320_s_array[SA_SIZE]; 
word ip320_az_data[SA_SIZE];        
word ip320_cal_data[SA_SIZE];       
word ip320_raw_data[SA_SIZE];       
long ip320_cor_data[SA_SIZE];       
/* headers for wrappers */
int acromag_setup();
int ip220_setup();
int ip320_setup();
int ip470_setup();
/* functions for ip220 */
word convert_volt_to_hex(float volts);
int ip220_write(int chan,word data);
/* functions for ip320 */
int ip320_read();

/* add headers for top-level functions here */

/* global variables for control parameters */

int dac[8]; /* dac default settings, referenced as dac0..dac7 in conf file */
int servo[4]; /* 1 if dac is under loop control, 0 if not. servo0..servo3 */
int lint[48]; /* lint outputs,0 or 1 lint00-lint47 in file 
		 the ones hardcoded for read get replaced 
		 with the actual values*/
float looppar[32]; /* control loop parameters, looppar00-looppar31 */
int srv_enabled ;  /* global to show servo is enabled.  added  - SM */
int sample; /* interval to sleep between samples, in 0.01s intervals . 
	       This is multiplied by 10000 and passed to usleep*/
int smooth; /* time constant of one-pole fir filter, in samples */
int hk1int; /* time between hk readouts of loop-related variables, in seconds */
int hk2int; /* time between hk readouts of other data, in seconds */

/* current dac values go here. servo determines if we use defaults or loop. */

int cdac[8];

float average[40]; /* averaged values of adcs end up here */

/* this variable is used when catching signals */

int prog_state; /* 0 initially or on SIGINT, 1 for looping, 2 for terminate */

/* signal handlers */

void sigint_handler(int sig) { prog_state = 0; }
void sigterm_handler(int sig) { prog_state = 2; }

/* logging routines */
FILE *logfile;

int log_open();

int log_write();

int short_log_write();

int log_close();

int main()
{
  int i, hk1ctr,hk2ctr,hk1max,hk2max ,shortlogmax,logmax,shortlogctr,logctr; 
  char *pidf_name = "/home/anita/var/pid/hkd.pid" ;
  FILE *pid_f ;
  pid_t pid_hkd ;
  /* insert process id code here, file=/home/anita/var/hkd.pid */
  if (!(pid_f=fopen(pidf_name, "w"))) {
    printf(" failed to open a file for PID, %s\n", pidf_name);
    exit(1) ;
  }
  fprintf(pid_f,"%d\n", pid_hkd=getpid()) ;
  fclose(pid_f) ;
  /* prepare hardware */
  signal(SIGINT, sigint_handler);
  signal(SIGTERM,sigterm_handler);
  log_open();
  acromag_setup();
  ip220_setup();
  ip320_setup(); 
  ip470_setup(); 
  set_default(); 
  ip320_getcal();
  ip320_read(); /* get current values of adcs */
  average_setup(); /* and copy them to signal averages */
  prog_state = 0 ;
  while (prog_state == 0){
    read_config();
    lint_write(); /* only once per config file load */
    dac_write();
    prog_state=1;
    /* convert houskeeping intervals to number of samples */
    hk1max=(int)((float)hk1int*100./(float)sample); 
    hk2max=(int)((float)hk2int*100./(float)sample);
   /*    shortlogmax=(int)((float)smooth*0.5*100./(float)sample); 1/2 smooth */
    logmax=(int)((float)30.*100./(float)sample); 
    shortlogmax = logmax/5 ;   /* short log 5 times more frequent the log. */
    if (shortlogmax < 1) shortlogmax = 1 ;
    /* force housekeeping on first entry */
    hk1ctr=hk1max;
    hk2ctr=hk2max;
    shortlogctr=shortlogmax;
    logctr=logmax;
    while (prog_state == 1)
      {
	/* lint_read(); */
	ip320_getcal();
	ip320_read();
	average_update();
       	/*  for (i=0;i<SA_SIZE;i++){ */
	/*          printf("%d %d %ld %f\n",i,ip320_raw_data[i]>>4,ip320_cor_data[i],average[i]); */
	/*  	  } */
	/*  	printf("\n");  */
	servo_compute();
	dac_write();
	if (hk1ctr == hk1max){
	  hk1_write();
	  hk1ctr=0;	  
	}
	if (hk2ctr == hk2max){
	  hk2_write();
	  hk2ctr=0;
	}
	if (shortlogctr == shortlogmax){
	  short_log_write();
	  fflush(logfile);
	  shortlogctr=0;	  
	}
	if (logctr == logmax){
	  log_write();
	  fflush(logfile);
	  logctr=0;
	}
	hk1ctr++;
	hk2ctr++;
	shortlogctr++;
	logctr++;
	/* log_write(); */
	usleep(10000*sample);
      }
  }
  log_close();
}
 
int acromag_setup()
{

  long addr;
  
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
  cblk_220.nHandle = carrierHandle;
  cblk_320.nHandle = carrierHandle;
  cblk_470.nHandle = carrierHandle;
  
  cblk_220.slotLetter='D';
  cblk_320.slotLetter='C';
  cblk_470.slotLetter='B';


  if(CarrierInitialize(carrierHandle) == S_OK) 
    { /* Initialize Carrier */
      SetInterruptLevel(carrierHandle, INTERRUPT_LEVEL);
      /* Set carrier interrupt level */
    } 
  else 
    {
      printf("\nUnable initialize the carrier", addr);
      exit(3);
    }

  cblk_220.bCarrier=TRUE;
  cblk_320.bCarrier=TRUE;
  cblk_470.bCarrier=TRUE;

  /* GetCarrierAddress(carrierHandle, &addr);
     SetCarrierAddress(carrierHandle, addr); */

  if(GetIpackAddress(carrierHandle,cblk_220.slotLetter,
		     (long *) &cblk_220.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP220\n.");
      exit(4);
    }
  if(GetIpackAddress(carrierHandle,cblk_320.slotLetter,
		     (long *) &cblk_320.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP320\n.");
      exit(5);
    }
  if(GetIpackAddress(carrierHandle,cblk_470.slotLetter,
		     (long *) &cblk_470.brd_ptr) != S_OK)
    {
      printf("\nIpack address failure for IP470\n.");
      exit(6);
    }
  cblk_220.bInitialized = TRUE;
  cblk_320.bInitialized = TRUE;
  cblk_470.bInitialized = TRUE;
}

ip220_setup()
{
  int i;
  for (i=0;i<A_SIZE;i++)
    {
      ip220_ideal_data[i]=0;
      ip220_off_data[i]=0;
      ip220_gain_data[i]=0;
      ip220_cor_data[i]=0;
    }
  cblk_220.mode=TM;                       /* transparent mode */
  cblk_220.bit_constant=CON12;            /* controls data correction */
  cblk_220.ideal_buf = &ip220_ideal_data[0]; /* ideal value */
  cblk_220.off_buf = &ip220_off_data[0];     /* offset buffer start */
  cblk_220.gain_buf = &ip220_gain_data[0];   /* gain buffer start */
  cblk_220.cor_buf = &ip220_cor_data[0];     /* corrected */
  mode_select(&cblk_220);      /* makes the mode take in hardware */
  rccid220(&cblk_220);         /* get calibration constants */
}

word convert_volt_to_hex(float volts) {
  float hex;

  hex = 4095.0 * ((volts + 10.0)/20.0); 
  return (word) hex;
}

ip220_write(int chan, word data)
{
  if (chan<A_SIZE) 
    {
      ip220_ideal_data[chan]=data;
      cd220(&cblk_220,chan);
      /*      printf("dac %i %i %i\n",chan,ip220_ideal_data[chan],ip220_cor_data[chan]); */
      if(ip220_ideal_data[chan] == 0 || ip220_ideal_data[chan] == 0xFFF)
	ip220_cor_data[chan] = ip220_ideal_data[chan];
      wro220(&cblk_220,chan,(ip220_cor_data[chan] << 4));
    }
}

ip320_setup()
{
  int i;
  for( i = 0; i < SA_SIZE; i++ ) 
    {
      ip320_s_array[i].gain = GAIN_X1;            /*  gain=2 */
      ip320_s_array[i].chn = i;             /*  channel */
      ip320_az_data[i] = 0;                 /* clear auto zero buffer */
      ip320_cal_data[i] = 0;                /* clear calibration buffer */
      ip320_raw_data[i] = 0;                /* clear raw input buffer */
      ip320_cor_data[i] = 0;                /* clear corrected data buffer */
    }
  cblk_320.range = RANGE_5TO5;       /* full range */
  cblk_320.trigger = STRIG;            /* 0 = software triggering */
  cblk_320.mode = SEI;                 /* single ended input */
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
  cblk_320.sa_end = &ip320_s_array[SA_SIZE-1];  /* address of end of scan array*/
}

int ip320_read()
{
  ainmc320(&cblk_320);
  mccd320(&cblk_320);
}

int ip320_getcal()
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
}

int ip470_setup()
{
  int i;
  /* channel assignments:
     0: Veto_dis, default high
     1: holdoff, default low
     2: 1ppsdis, default low
     5: nd disable, low (unused)
     6: gpsdis, unknown (unused?) low
     these won't work; I/0 settable
     3: 1ppsmon, input
     4: vetomon, input
     7: varpps, input
     write mask is 0x0
  */
  cblk_470.param=0;
  cblk_470.e_mode=0;
  cblk_470.mask_reg=0;
  cblk_470.deb_control=0;
  cblk_470.deb_clock=1;
  cblk_470.enable=0;
  cblk_470.vector=0;
  for(i = 0; i != 2; i++)
    {
      cblk_470.ev_control[i] = 0;/* event control registers */
      cblk_470.deb_duration[i] = 0;/* debounce duration registers */
    }
  for(i = 0; i < 6; i++)
    cblk_470.event_status[i] = 0; /* initialize event status flags */
}

int ip470_write(int chan,int val)
{
  wpnt470(&cblk_470,0,chan,val);
}

int lint_write()
{
  int i;
  for (i=0; i<8; i++)
    ip470_write(i,lint[i]);
}

int set_default()
{
  /* default settings of all parameters go here */
  int i;
  /* dac default settings, referenced as dac0..dac7 in conf file */
  for (i=0; i<8; i++){
    dac[i]=0x800; /* (zero) */
    cdac[i]=dac[i];
  }
  srv_enabled = 0 ;    /* servo should be off. */
  for (i=0; i<4; i++){
    servo[i]=0;
  }
  for (i=0; i<48; i++){ 
    lint[i]=0;
  }
  lint[0]=1; lint[1]=0; lint[2]=0; lint[5]=0; lint[6]=0;
  for (i=0; i<8; i++){
    looppar[i]=0;
  } 
  sample=100; /* interval to sleep between samples, in 0.01s intervals . 
	       This is multiplied by 10000 and passed to usleep*/
  smooth=5; /* time constant of one-pole fir filter, in samples */
  hk1int=15;
  hk2int=60;
}

int average_setup(){
  int i;
  for (i=0; i<40; i++){
    average[i]=(float) (ip320_cor_data[i]); /* data are left justified */
  }
}

int average_update(){
  int i;
  float p;
  p = 1./(float) smooth;
  for (i=0; i<40; i++){
    average[i] = p*(ip320_cor_data[i]) +(1.-p)*average[i];
    /* printf("0x%x ",ip320_cor_data[i]); */
  }
  /* printf("\n"); */
}

int ip220_getcal(){
}

int dac_write(){
  int i;
  for (i=0; i<8; i++){
    ip220_write(i,(word) cdac[i]);
  }
}

int my_max(int a, int b)
{
  if (a > b){
    return a;
  }
  else{
    return b;
  }
}

int my_min(int a, int b)
{
  if (a < b){
    return a;
  }
  else{
    return b;
  }
}
    

int servo_compute(){
  /*define names for servo states*/
#define NONEWS -1
#define FINETIME 0
#define HIGHTIME 1
#define LOWTIME 2
#define PANIC 3
#define T_OUT 200   /* nominal timeout used to change increment*2.  SM */
  static int servo_state=0;
  static int servo_status=0;
  static int panic_status=0;
  FILE *fp;
  char *filename = "/home/anita/var/waittime";
  int enable,desired,deadband,increment,interval,panic,panicincrement;
  int panicinterval,minthreshold,maxthreshold;
  int waittime, i;
  /* ALL THRESHOLDS IN DAC CHANNELS (2048=0V, ~2.5 mV/channel) */
  /* Gary put 1/10 convertor in DAC out, so it's 0.25 mV. - SM */
  /* a little syntactic sugar for the reader*/
  srv_enabled=enable=looppar[0]; 
  /* this turns us on; we can set up parameters below first*/
  desired=looppar[1];
  /* midpoint of the desired rate band */
  deadband=looppar[2];
  /* amount above and below desired for which no action is taken */
  increment=looppar[3];
  /* amount we change threshold by per attempt*/
  interval=looppar[4];
  /* how many times we see out of range befora acting */
  panic=looppar[5];
  /* low interval (high rate) at which immediate action occurs */
  panicincrement=looppar[6];
  /* threshold change (channels) for a panic */
  panicinterval=looppar[7];
  /* how long we let a panic continue after a change before acting again */
  minthreshold=looppar[8];
  /* lowest threshold we allow */
  maxthreshold=looppar[9];
  /* highest threshold we allow */
  if (enable !=0){
    /* read rate from Shige's file; if it isn't there do nothing (FINETIME)*/
    if ((fp = fopen(filename, "r")) != NULL) {    
      if (fscanf(fp,"%d",&waittime) == 0) servo_state=NONEWS;
      /* read the waittime from the file; 
	 if we we can't parse it, force NONEWS */
      /* if we got data, determine the state */ 
      if (waittime<panic){
	servo_state=PANIC;
      }  
      else if ((waittime<(desired+deadband))&(waittime>(desired-deadband))){
	/*we are inside the deadband*/
	servo_state=FINETIME;
      }
      else if (waittime>=(desired+deadband)){
	servo_state=HIGHTIME;
      }
      else if (waittime<=(desired-deadband)){
	servo_state=LOWTIME;
      }
      else servo_state=NONEWS; 
      /* this should never happen since cases are exhaustive */
      fclose(fp);
      fprintf(logfile, " DBG: trg interval read = %d, state = %d / %d\n", 
	     waittime, servo_status, panic_status) ;
      unlink(filename); 
      /* delete the file so we don't count the same info from Shige twice */
    } 
    else{
      servo_state=NONEWS; /* if we can't find the file, force NONEWS */
    }
    switch (servo_state){
    case FINETIME:
      /* clear any old status since things are OK */
      servo_status=0;
      panic_status=0;
      break;
    case HIGHTIME:
      /* clear possible old lowtime indication */
      servo_status=my_max(0,servo_status); 
      panic_status=0; /* and clear possible old panic */
      servo_status++;
      if (servo_status > interval){
	/* lower thresholds for all dacs with servo[i] nonzero*/    
	increment *= 1+waittime/T_OUT ;  /* all time-out case only */
	for (i=0; i<4; i++){
	  if (servo[i] != 0) cdac[i]=my_min(cdac[i]+increment,minthreshold);
	}
	fprintf(logfile," HT: servo/daq %1d/%x, %1d/%x, %1d/%x, %1d/%x\n",
		servo[0], cdac[0], servo[1], cdac[1], 
		servo[2], cdac[2], servo[3], cdac[3]) ;
	/* we took action; restart counter */
	servo_status=0;
      }
      break;
    case LOWTIME:
      /* clear possiblie hightime indication */
      servo_status=my_min(0,servo_status); 
      panic_status=0; /* and clear old panic */
      servo_status--;
      if (servo_status < (-interval)){
	/* raise thresholds for all dacs with servo[i] nonzero*/
	for (i=0; i<4; i++){
	  if (servo[i] != 0) cdac[i]=my_max(cdac[i]-increment,maxthreshold);
	}
	fprintf(logfile," LT: servo/daq %1d/%x, %1d/%x, %1d/%x, %1d/%x\n",
		servo[0], cdac[0], servo[1], cdac[1], 
		servo[2], cdac[2], servo[3], cdac[3]) ;
	/* we took action; restart counter */
	servo_status=0;
      }
      break;
    case PANIC:
      /* here we act immediately, then start counting */
      servo_status=0;
      if (panic_status==0){
	/* raise thresholds for all dacs with servo[i] nonzero*/    
	for (i=0; i<4; i++){
	  if (servo[i] != 0) 
	    cdac[i]=my_max(cdac[i]-panicincrement,maxthreshold);
	}
	fprintf(logfile," PT: servo/daq %1d/%x, %1d/%x, %1d/%x, %1d/%x\n",
		servo[0], cdac[0], servo[1], cdac[1], 
		servo[2], cdac[2], servo[3], cdac[3]) ;
      }
      /* if we already are in panic, wait for results...*/
      panic_status++;
      if (panic_status>panicinterval) panic_status=0; /* act on next panic */
      break;
    }
  }
}

int read_config(){
  FILE *fp;
  char *filename = "/home/anita/etc/hkd.conf";
  char option[256];
  int i ;

  if ((fp = fopen(filename, "r")) == NULL) {
    /* needs to be replaced with a log entry */
    printf("hkd: Can't find configuration file. Using defaults.\n");
  } 
  else {
    while ((fscanf(fp, "%s", option)) != EOF) {
      /* dac */
      if (strcmp(option, "dacv00") == 0)
	fscanf(fp, "%i", &(dac[0]));
      if (strcmp(option, "dacv01") == 0)
	fscanf(fp, "%i", &(dac[1]));
      if (strcmp(option, "dacv02") == 0)
	fscanf(fp, "%i", &(dac[2]));
      if (strcmp(option, "dacv03") == 0)
	fscanf(fp, "%i", &(dac[3]));
      if (strcmp(option, "dacv04") == 0)
	fscanf(fp, "%i", &(dac[4]));
      if (strcmp(option, "dacv05") == 0)
	fscanf(fp, "%i", &(dac[5]));
      if (strcmp(option, "dacv06") == 0)
	fscanf(fp, "%i", &(dac[6]));
      if (strcmp(option, "dacv07") == 0)
	fscanf(fp, "%i", &(dac[7]));
      /* servo */
      if (strcmp(option, "servo0") == 0)
	fscanf(fp, "%i", &(servo[0]));
      if (strcmp(option, "servo1") == 0)
	fscanf(fp, "%i", &(servo[1]));
      if (strcmp(option, "servo2") == 0)
	fscanf(fp, "%i", &(servo[2]));
      if (strcmp(option, "servo3") == 0)
	fscanf(fp, "%i", &(servo[3]));
      /* lint:only define first 10 channels */
      if (strcmp(option, "lint00") == 0)
	fscanf(fp, "%i", &(lint[0]));
      if (strcmp(option, "lint01") == 0)
	fscanf(fp, "%i", &(lint[1]));
      if (strcmp(option, "lint02") == 0)
	fscanf(fp, "%i", &(lint[2]));
      if (strcmp(option, "lint03") == 0)
	fscanf(fp, "%i", &(lint[3]));
      if (strcmp(option, "lint04") == 0)
	fscanf(fp, "%i", &(lint[4]));
      if (strcmp(option, "lint05") == 0)
	fscanf(fp, "%i", &(lint[5]));
      if (strcmp(option, "lint06") == 0)
	fscanf(fp, "%i", &(lint[6]));
      if (strcmp(option, "lint07") == 0)
	fscanf(fp, "%i", &(lint[7]));
      if (strcmp(option, "lint08") == 0)
	fscanf(fp, "%i", &(lint[8]));
      if (strcmp(option, "lint09") == 0)
	fscanf(fp, "%i", &(lint[9]));
      /* looppar */
      if (strcmp(option, "loop00") == 0)
	fscanf(fp, "%f", &(looppar[0]));
      if (strcmp(option, "loop01") == 0)
	fscanf(fp, "%f", &(looppar[1]));
      if (strcmp(option, "loop02") == 0)
	fscanf(fp, "%f", &(looppar[2]));
      if (strcmp(option, "loop03") == 0)
	fscanf(fp, "%f", &(looppar[3]));
      if (strcmp(option, "loop04") == 0)
	fscanf(fp, "%f", &(looppar[4]));
      if (strcmp(option, "loop05") == 0)
	fscanf(fp, "%f", &(looppar[5]));
      if (strcmp(option, "loop06") == 0)
	fscanf(fp, "%f", &(looppar[6]));
      if (strcmp(option, "loop07") == 0)
	fscanf(fp, "%f", &(looppar[7]));
      if (strcmp(option, "loop08") == 0)
	fscanf(fp, "%f", &(looppar[8]));
      if (strcmp(option, "loop09") == 0)
	fscanf(fp, "%f", &(looppar[9]));
      if (strcmp(option, "loop10") == 0)
	fscanf(fp, "%f", &(looppar[10]));
      if (strcmp(option, "loop11") == 0)
	fscanf(fp, "%f", &(looppar[11]));
      if (strcmp(option, "loop12") == 0)
	fscanf(fp, "%f", &(looppar[12]));
      if (strcmp(option, "loop13") == 0)
	fscanf(fp, "%f", &(looppar[13]));
      if (strcmp(option, "loop14") == 0)
	fscanf(fp, "%f", &(looppar[14]));
      if (strcmp(option, "loop15") == 0)
	fscanf(fp, "%f", &(looppar[15]));
      if (strcmp(option, "loop16") == 0)
	fscanf(fp, "%f", &(looppar[16]));
      if (strcmp(option, "loop17") == 0)
	fscanf(fp, "%f", &(looppar[17]));
      if (strcmp(option, "loop18") == 0)
	fscanf(fp, "%f", &(looppar[18]));
      if (strcmp(option, "loop19") == 0)
	fscanf(fp, "%f", &(looppar[19]));
      if (strcmp(option, "loop20") == 0)
	fscanf(fp, "%f", &(looppar[20]));
      if (strcmp(option, "loop21") == 0)
	fscanf(fp, "%f", &(looppar[21]));
      if (strcmp(option, "loop22") == 0)
	fscanf(fp, "%f", &(looppar[22]));
      if (strcmp(option, "loop23") == 0)
	fscanf(fp, "%f", &(looppar[23]));
      if (strcmp(option, "loop24") == 0)
	fscanf(fp, "%f", &(looppar[24]));
      if (strcmp(option, "loop25") == 0)
	fscanf(fp, "%f", &(looppar[25]));
      if (strcmp(option, "loop26") == 0)
	fscanf(fp, "%f", &(looppar[26]));
      if (strcmp(option, "loop27") == 0)
	fscanf(fp, "%f", &(looppar[27]));
      if (strcmp(option, "loop28") == 0)
	fscanf(fp, "%f", &(looppar[28]));
      if (strcmp(option, "loop29") == 0)
	fscanf(fp, "%f", &(looppar[29]));
      if (strcmp(option, "loop30") == 0)
	fscanf(fp, "%f", &(looppar[30]));
      if (strcmp(option, "loop31") == 0)
	fscanf(fp, "%f", &(looppar[31]));
      /* sample */
      if (strcmp(option, "sample") == 0)
	fscanf(fp, "%i", &(sample));
      /* sample */
      if (strcmp(option, "sample") == 0)
	fscanf(fp, "%i", &(sample));
      /* smooth */
      if (strcmp(option, "smooth") == 0)
	fscanf(fp, "%i", &(smooth));
      /* hk1int; hk2int */
      if (strcmp(option, "hk1int") == 0)
	fscanf(fp, "%i", &(hk1int));
      if (strcmp(option, "hk2int") == 0)
	fscanf(fp, "%i", &(hk2int));
    }
    fclose(fp);

    /* put dac value into cdac array.   -- SM 17-Nov-03 */
    for (i=0; i<8; i++) 
      if (i > 3 || !srv_enabled || looppar[0] == 0) cdac[i]=dac[i];
  }
}

int hk1_write(){
  /* this packet type contains the critical threshold-related parameters */
  int i, adcdata;
  FILE* packetfile;
  unsigned char packet[100];
  char filename[64];
  char * tempfilename="/home/anita/tmp/hk1.tmp";
  struct timeb tb_ptr;   /* to use millisecond accuracy time */
  int length;
  int time;
  unsigned short millitime;
  memset(filename, 0 , 64);
  ftime(&tb_ptr);
  sprintf(filename, "/home/anita/packet/0x1/hk1_%ld%03d", 
	  tb_ptr.time, tb_ptr.millitm);
  time=tb_ptr.time;
  millitime= tb_ptr.millitm;
  /* packet length must be even inclisive of length byte,
     which is number of byte that _follow_ starting from 1, 
     i.e. index of last used bye or the byte after to make odd*/
  length = 29;
  packet[0]=length;
  packet[1]=0x10;  /* packet type 1, subtype 0 */
  memcpy(&packet[2],&time,4); /* 2-5 */
  memcpy(&packet[6],&millitime,2); /* 6-7 */
  /* get first five dacs and first five ADCs */
  /* these should be the parameters related to the trigger*/
  packet[8]=(unsigned char) ((cdac[0] & 0xff00) >> 8);
  packet[9]=(unsigned char) ((cdac[0] & 0xff));
  packet[10]=(unsigned char) ((cdac[1] & 0xff00) >> 8);
  packet[11]=(unsigned char) ((cdac[1] & 0xff));
  packet[12]=(unsigned char) ((cdac[2] & 0xff00) >> 8);
  packet[13]=(unsigned char) ((cdac[2] & 0xff));
  packet[14]=(unsigned char) ((cdac[3] & 0xff00) >> 8);
  packet[15]=(unsigned char) ((cdac[3] & 0xff));
  packet[16]=(unsigned char) ((cdac[4] & 0xff00) >> 8);
  packet[17]=(unsigned char) ((cdac[4] & 0xff));
  /* send down the averages for the adcs here */
  /* shift things over to use all the bits, since these are 12 bit adcs */     
  /* 0 */
  adcdata=(int)floor(average[0]*8+0.5);
  packet[18]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[19]=(unsigned char)(adcdata & 0xff);
  /*  printf("%f %i %i %i\n",average[0],adcdata,(int)packet[18],(int)packet[19]); */
  adcdata=(int)floor(average[1]*8+0.5);
  packet[20]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[21]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[2]*8+0.5);
  packet[22]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[23]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[3]*8+0.5);
  packet[24]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[25]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[4]*8+0.5);
  packet[26]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[27]=(unsigned char)(adcdata & 0xff);
  packet[28]=0; /* already true, but... */
  for (i=7;i>=0; i--){
    packet[28]= packet[28]+(lint[i] << i);
  }
  packet[29]=0 ;
  /* write to a temporary file first so packet appears atomically*/
  packetfile = fopen(tempfilename, "w+"); 
  fwrite(packet, sizeof(unsigned char),length+1 , packetfile);
  fclose(packetfile);
  link(tempfilename, filename);
  unlink(tempfilename);
}

int hk2_write(){
  /* other ADCs not in HK1 */
  int i, adcdata;
  FILE* packetfile;
  unsigned char packet[100];
  char filename[64];
  char * tempfilename="/home/anita/tmp/hk2.tmp";
  struct timeb tb_ptr;   /* to use millisecond accuracy time */
  int length;
  int time;
  unsigned short millitime;
  memset(filename, 0 , 64);
  ftime(&tb_ptr);
  sprintf(filename, "/home/anita/packet/0x1/hk2_%ld%03d", 
	  tb_ptr.time, tb_ptr.millitm);
  time=tb_ptr.time;
  millitime= tb_ptr.millitm;
  /* packet length must be even inclisive of length byte,
     which is number of byte that _follow_ starting from 1, 
     i.e. index of last used bye or the byte after to make odd*/
  length = 77;
  packet[0]=length;
  packet[1]=0x11;  /* packet type 1, subtype 1 */
  memcpy(&packet[2],&time,4); /* 2-5 */
  memcpy(&packet[6],&millitime,2); /* 6-7 */
  adcdata=(int)floor(average[5]*8+0.5);
  packet[8]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[9]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[6]*8+0.5);
  packet[10]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[11]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[7]*8+0.5);
  packet[12]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[13]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[8]*8+0.5);
  packet[14]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[15]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[9]*8+0.5);
  packet[16]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[17]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[10]*8+0.5);
  packet[18]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[19]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[11]*8+0.5);
  packet[20]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[21]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[12]*8+0.5);
  packet[22]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[23]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[13]*8+0.5);
  packet[24]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[25]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[14]*8+0.5);
  packet[26]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[27]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[15]*8+0.5);
  packet[28]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[29]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[16]*8+0.5);
  packet[30]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[31]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[17]*8+0.5);
  packet[32]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[33]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[18]*8+0.5);
  packet[34]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[35]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[19]*8+0.5);
  packet[36]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[37]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[20]*8+0.5);
  packet[38]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[39]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[21]*8+0.5);
  packet[40]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[41]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[22]*8+0.5);
  packet[42]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[43]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[23]*8+0.5);
  packet[44]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[45]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[24]*8+0.5);
  packet[46]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[47]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[25]*8+0.5);
  packet[48]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[49]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[26]*8+0.5);
  packet[50]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[51]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[27]*8+0.5);
  packet[52]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[53]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[28]*8+0.5);
  packet[54]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[55]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[29]*8+0.5);
  packet[56]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[57]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[30]*8+0.5);
  packet[58]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[59]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[31]*8+0.5);
  packet[60]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[61]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[32]*8+0.5);
  packet[62]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[63]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[33]*8+0.5);
  packet[64]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[65]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[34]*8+0.5);
  packet[66]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[67]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[35]*8+0.5);
  packet[68]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[69]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[36]*8+0.5);
  packet[70]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[71]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[37]*8+0.5);
  packet[72]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[73]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[38]*8+0.5);
  packet[74]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[75]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[39]*8+0.5);
  packet[76]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[77]=(unsigned char)(adcdata & 0xff);
  /* write to a temporary file first so packet appears atomically*/
  packetfile = fopen(tempfilename, "w+"); 
  fwrite(packet, sizeof(unsigned char),length+1 , packetfile);
  fclose(packetfile);
  link(tempfilename, filename);
  unlink(tempfilename);
}

int log_open(){
  char logfilename[64];
  struct timeb tb_ptr;   /* to use millisecond accuracy time */
  int length;
  int time;
  unsigned short millitime;
  memset(logfilename, 0 , 64);
  ftime(&tb_ptr);
  sprintf(logfilename, "/home/anita/log/hkd/hkd_%ld%03d", 
	  tb_ptr.time, tb_ptr.millitm);
  logfile = fopen(logfilename,"w+");
}

int short_log_write(){  
  int i,j;
  struct timeb tb_ptr;   /* to use millisecond accuracy time */
  ftime(&tb_ptr);
  fprintf(logfile,"Time:%ld %i\n",tb_ptr.time, tb_ptr.millitm);
  i=0;
  for (j=0;j<5;j++){
    fprintf(logfile,"%2i:%5.3f ",5 * i+j,average[5 * i+j]);
  }
  fprintf(logfile,"\n");
}

int log_write(){
  int i,j;
  struct timeb tb_ptr;   /* to use millisecond accuracy time */
  ftime(&tb_ptr);
  fprintf(logfile,"Time:%ld %i\n",tb_ptr.time, tb_ptr.millitm);
  for (i=1;i<8;i++){
    for (j=0;j<5;j++){
      fprintf(logfile,"%2i:%5.3f ",5 * i+j,average[5 * i+j]);
    }
    fprintf(logfile,"\n");
  }
  fprintf(logfile,"\n");
}

int log_close(){
  fclose(logfile);
}
