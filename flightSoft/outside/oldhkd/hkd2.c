/*****************************************************
         Housekeeping control routines  

  Vendor routines modified by JJB to fix global namespace collision 

created: JJB ~ 24 JUNE 2003
updated: C. Hebert 15 JULY 2003
to include tip tilt monitor, GPS reset and keep logfiles 

**********************************************************/

#include <hkd2.h>
#define INTERRUPT_LEVEL 10
int run_no=0;



/* This section of our program is for Function Prototypes */
int getopt(int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;


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
int t_level;	   /* default value for tilt filter level*/    
 

float looppar[32]; /* control loop parameters, looppar00-looppar31 */
int sample; /* interval to sleep between samples, in 0.01s intervals . 
	       This is multiplied by 10000 and passed to usleep*/
int timeconst; /* time constant of one-pole fir filter, in samples */
int hk1int; /* time between hk readouts of loop-related variables, in seconds */
int hk2int; /* time between hk readouts of other data, in seconds */
FILE *logfile;
/*  char *logfilename="/home/hebertcl/anita/hkd/hkd/tmp2.log"; */
/*  sprintf(logfilename,"%s/daq_run%d_%ld.log", L_DIR, run_no, ltime) ; */


char logfilename[32];
/*  sprintf(logfilename,"%s/hkd.log", L_DIR); */


/* current dac values go here. servo determines if we use defaults or loop. */

int cdac[8];

float average[40]; /* averaged values of adcs end up here */

/* this variable is used when catching signals */

int prog_state; /* 0 initially or on SIGINT, 1 for looping, 2 for terminate */

int ich;


/* signal handlers */

void sigint_handler(int sig) { prog_state = 0; }
void sigterm_handler(int sig) { prog_state = 2; }



////////////////////////////////////////////////////////////////////////////////


 
int acromag_setup()
{

  long addr;
  
  /* Check for Carrier Library */
  if(InitCarrierLib() != S_OK) {
    fprintf(logfile,"\nacromag_setup:Carrier library failure");
    exit(1);
  }
  
  /* Connect to Carrier */ 
  if(CarrierOpen(0, &carrierHandle) != S_OK) {
    fprintf(logfile,"\nacromag_setup:Unable to Open instance of carrier.\n");
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
      fprintf(logfile,"\nacromag_setup:Unable initialize the carrier", addr);
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
      fprintf(logfile,"\nacromag_setup:Ipack address failure for IP220\n.");
      exit(4);
    }
  if(GetIpackAddress(carrierHandle,cblk_320.slotLetter,
		     (long *) &cblk_320.brd_ptr) != S_OK)
    {
      fprintf(logfile,"\nacromag_setup:Ipack address failure for IP320\n.");
      exit(5);
    }
  if(GetIpackAddress(carrierHandle,cblk_470.slotLetter,
		     (long *) &cblk_470.brd_ptr) != S_OK)
    {
      fprintf(logfile,"\nacromag_setup:Ipack address failure for IP470\n.");
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
      wro220(&cblk_220,chan,ip220_ideal_data[chan] << 4);
    }
}

ip320_setup()
{
  int i;
  for( i = 0; i < SA_SIZE; i++ ) 
    {
        ip320_s_array[i].gain = GAIN_X2;            /*  gain=2 */
        ip320_s_array[i].chn = i;             /*  channel */
        ip320_az_data[i] = 0;                 /* clear auto zero buffer */
        ip320_cal_data[i] = 0;                /* clear calibration buffer */
        ip320_raw_data[i] = 0;                /* clear raw input buffer */
        ip320_cor_data[i] = 0;                /* clear corrected data buffer */
    }
  cblk_320.range = RANGE_10TO10;       /* full range */
  cblk_320.trigger = STRIG;            /* 0 = software triggering */
  cblk_320.mode = SEI;                 /* single ended input */
  cblk_320.gain = GAIN_X2;             /* gain for analog input */
  cblk_320.average = 1;                /* number of samples to average */
  cblk_320.channel = 0;                /* default channel */
  cblk_320.data_mask = BIT12;          /* A/D converter data mask */
  cblk_320.bit_constant = CON12;       /* constant for data correction */
  cblk_320.s_raw_buf = &ip320_raw_data[0];   /* raw buffer start */
  cblk_320.s_az_buf = &ip320_az_data[0];     /* auto zero buffer start */
  cblk_320.s_cal_buf = &ip320_cal_data[0];   /* calibration buffer start */
  cblk_320.s_cor_buf = &ip320_cor_data[0];   /* corrected buffer start */
  cblk_320.sa_start = &ip320_s_array[0];     /* address of start of scan array */
  cblk_320.sa_end = &ip320_s_array[SA_SIZE];  /* address of end of scan array */
}

int ip320_read()
{
  ainmc320(&cblk_320);
}





int set_default()
{
  /* default settings of all parameters go here */
  int i;
  /* dac default settings, referenced as dac0..dac7 in conf file */
  for (i=0; i<8; i++){
    dac[i]=0xf00; /* almost a volt out of DINT */
    cdac[i]=dac[i];
  }
  for (i=0; i<4; i++){ 
    servo[i]=0;
  } 
  for (i=0; i<48; i++){ 
  lint[i]=0;
  } 
  for (i=0; i<8; i++){
  looppar[i]=0;
  } 
  sample=100; /* interval to sleep between samples, in 0.01s intervals . 
	       This is multiplied by 10000 and passed to usleep*/
  timeconst=5; /* time constant of one-pole fir filter, in samples */
  hk1int=15;
  hk2int=60;
}

int average_setup(){
  int i;
  for (i=0; i<SA_SIZE; i++){
    average[i]=(float) (ip320_raw_data[i]>>4); /* data are left justified */
  }
}

int average_update(){
  int i;
  float p;
  p = 1./(float) timeconst;
  for (i=0; i<SA_SIZE; i++){
   average[i] = p*(ip320_raw_data[i]>>4) +(1.-p)*average[i]; 
/*    printf("this is cal 0x%x ",ip320_cal_data[i]);  */
/*       printf("0x%x ",ip320_raw_data[i]);  */
  }
/*    printf("\n") */;
}

int dac_write(){
  int i;
  for (i=0; i<8; i++){
    ip220_write(i,(word) cdac[i]);
  }
}


int servo_compute(){
  int i;
  /* just a dummy for now. 
     Copy first 4 averaged adcs to first 4 dacs as proof of principle*/  
  for (i = 0; i < 4; i++){
    if (servo[i] == 1){
      cdac[i] = average[i];
    }
    else{
      cdac[i] = dac[i];
    }
  }
}



int tilt_read(char c, int t_level){
  int fd; /* File descriptor for the port */
  struct termios options, old_options;
  int i;
  int n,s;
  int len;
  int count;
  int loop;
  int err;
  int inp;
  int num_byte;
  int header;
  int chksum;
  int sum;
  int chksum_mask=255;
  float pitch, roll;
  double pitch2,roll2,pitchint,pitch_dec,roll_dec,temp;
  int temp2;
  unsigned char temp3[23];
  signed char buff[20];
  signed char data[20];
  char out[21];
 /*   c = 'N'; */

  n = 1;

/* input R,G.N for c return pitch and roll*/
/*     printf("R - reset\n"); */
/*     printf("G - get response packet\n"); */
/*     printf("N - to set loop time\n"); */
  /*     printf("Q - to quit)  */


/* opens com3 on the cp604 on the backside */
     fd = open("/dev/ttyS2", O_RDWR | O_NOCTTY | O_NDELAY  ); 


if (fd == -1)
 {

/*   Could not open the port. */

   perror("open_port: Unable to open /dev/ttyS2   "); /*perror sends to /var/log/messages */
   exit(0);
 }
 else 
 {
   err = fcntl(fd, F_SETFL, FNDELAY);          /* READ returns immediately with whatever is in buffer */
   /* err = fcntl(fd, F_SETFL, 0); */            /*   READ waits for characters */
   if (err < 0)
     perror("fcntl err:"); 
 }
 /* get the current options for the port  */
 tcgetattr(fd, &old_options);
 options = old_options;
/*   set baud rate to 9600 */
 cfsetispeed(&options, B9600);
 cfsetospeed(&options, B9600);
/*  enable receiver and set local mode */
 options.c_cflag |= (CLOCAL | CREAD);
/*   set parity checking to 8N1 */
 options.c_cflag &= ~PARENB;
 options.c_cflag &= ~CSTOPB;
 options.c_cflag &= ~CSIZE;
 options.c_cflag |= CS8;
 /*   disable hardware flow control */
 options.c_cflag &= ~CRTSCTS;                /*  aka CNEW_RTSCTS */
/*   disable software flow control */
 options.c_iflag &= ~(IXON | IXOFF | IXANY);
/* do not strip parity, or check parity */
 options.c_iflag &= ~(INPCK | ISTRIP);
/*   no mapping */
 options.c_iflag &= ~(INLCR | ICRNL | IUCLC);
/*    select RAW input */
 options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
/*   disable output post processing */
 options.c_oflag &= ~OPOST;
/* set the new options (flush the buffers and do it now) */
 tcsetattr(fd, TCSAFLUSH, &options);


 c = toupper(c);   


 /* this is where the filter level is set */

 out[0]='N';   /*   setup up for filter level setting former case N */
 if ((t_level<10) && (t_level >=0)){
     out[1]=t_level;
 }
 else {fprintf(logfile, "\ntilt_read:No such level using default value of 5");
       t_level=5;
 }     
     n = write (fd,out,2);
     fprintf(logfile,"\ntilt_read:number of bytes written from 'N' = %i",n);
     fprintf(logfile, "\ntilt_read:N wrote [%c%x]\n",out[0],out[1]);
     if (n<2)
     fprintf(logfile,"\ntilt_read:input data write failed");
     fflush(logfile);


     switch(c)
     {
     case 'R':
       n = write (fd,"R",1);
        fprintf(logfile,"\n tilt_read:number of bytes written from 'R' = %i",n); 
       if (n<1)
  	 fprintf(logfile,"tilt_read:input data write failed"); 
       usleep(10000);                         /*   wait 10 millisec */
         fprintf(logfile,"\ntilt_read:return from sleep"); 
       bzero(buff,20);
       for (i=0;i<5; i++)                     /*  hang aroung waiting for bytes */
	 {
	   ioctl(fd, FIONREAD, &num_byte);    /*  see how many bytes are available */
	   if (num_byte >= 1)
	     n = read (fd,buff,1);            /*  read the byte if it is there */
	   else
	     usleep(100000);                    /*  wait if byte not available */
	 }
/*           fprintf(logfile,"\ntilt_read:return from read\n");  */
       if (n<1)
	 {
  	   fprintf(logfile,"\ntilt_read:data read failed"); 
  	   perror(" read error: tilt_read case R or reset"); 
	 }
/*   uncomment if you want to debug tip tilt readout problems */
/*           fprintf(logfile,"\ntilt_read:number of bytes read = %d\n",n);  */
/*           fprintf(logfile, "\ntilt_read:%x %x %x %x %x %x\n",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);  */
       break;
     case 'G':
       n = write (fd,"G",1);
       

/*         fprintf(logfile,"tilt_read:number of bytes written from 'G' = %i",n); */
       if (n<0)
	 fprintf(logfile,"\ntilt_read:input data write failed case G or get response packet");
       usleep(1000000);                          /* wait 10 msec */
       num_byte = 0;
       bzero(data,20);
       for (i=0;i<5; i++)                     /*  hang around waiting for bytes */
	 {
	   ioctl(fd, FIONREAD, &num_byte);    /*  see how many bytes are available */
	   if (num_byte >= 6)
	     n = read (fd,data,6);            /*  read the byte if it is there */
	   else
	     usleep(1000000);                   /*  wait if byte not available */
	 }
/*  remember to comment this out after debug */
       printf("read %i bytes\n",n);	    
       printf("data %x %x %x %x %x %x\n",data[0],data[1],data[2],data[3],data[4],data[5]);
       header = (int) data[0]& 255;

/*  print out if you want to look at header packet for tip tilt */
/*         fprintf(logfile,"\nheader = %i",header); */

       if (header != 255)
	 {fprintf(logfile,"\ntilt_read:header incorrect");}
       sum = (int)data[1]+(int)data[2]+(int)data[3]+(int)data[4];
       sum = sum & chksum_mask;
/*         fprintf(logfile,"\ntilt_read:calc checksum = %x",sum); */
       chksum=(int)data[5]&chksum_mask;
/*         fprintf(logfile,"\ntilt_read:checksum = %x",chksum); */
       if (sum != chksum)
	 {fprintf(logfile,"\ntilt_read:checksum failure");}
       if (n==6)
	 {

	   pitch =(float) data[2] + 256*(float)data[1];

	   pitch = pitch*180/65535;
	   roll = (float) data[4] + 256*(float)data[3];
	   roll = roll*180/65535;
	   /*  printf("header = %x \n",data[0]); */

/*  	   printf(" dec  %i %i %i %i %i\n",data[1],data[2],data[3],data[4],data[5]); */
/*  	   printf(" hex  %x %x %x %x %x\n",data[1],data[2],data[3],data[4],data[5]); */
	   printf("pitch = %5.2f  roll = %5.2f\n",pitch,roll);
/*  	resolution is 1/4 of a degree    +/- 45 degrees */

/*this is where I round off to the nearest quarter degree*/

  	pitch2=4*pitch; 
  	pitch_dec=modf(pitch2,&pitchint); 

	if (fabs(pitch_dec) >= 0.5) 
	  {
	    printf(" abs(pitch_dec).ge..5 %f\n",temp); 
	    pitch2=floor(pitch2);
          }
	if (fabs(pitch_dec) < 0.5){
          printf("abs(pitch_dec).lt..5 %f\n",pitch_dec);       
 	  pitch2=ceil(pitch2);
	}

  	roll2=4*roll; 
  	roll_dec=modf(roll2,&pitchint); 

	if (fabs(roll_dec) >= 0.5) 
	  {
	    printf("abs(roll_dec).ge..5 %f\n",roll_dec); 
	    roll2=floor(roll2);
/*  	    printf("pitch2 %f \n", (pitch2)/4); */
          }
	if (fabs(roll_dec) < 0.5){
	 /*   printf("abs(roll_dec).lt..5 %f\n",temp);  */       
 	  roll2=(int)ceil(roll2);
	/*    printf("pitch2 %f\n", (pitch2)/4); */
	}
     


           printf("pitch2 %f\n",(pitch2)/4); 
           printf("roll2 %f\n",(roll2)/4); 

       
       break;
     case 'Q':
       break; 
     default:
       fprintf(logfile,"\ntilt_read:not a choice for tip tilt");
     }
   }
          
/*  	  temp=-200; */
/*  	  temp2 = (int)(temp); */
/*            temp3[0] = (unsigned char)(temp2);  */
/*  	  printf("%i",temp2); */
/*  	  printf("%c",temp3[0]); */
/*            printf("\n you missed it"); */

 tcsetattr(fd, TCSAFLUSH, &old_options);       /*  restore the old TTY options */
 close (fd);
 return 0;

   }



int gps() 
{ 
int error=0;
int fd,n,i; 
struct termios options;
//char buff[COMMAND_SIZE]={' '};
char buff[COMMAND_SIZE]=COMMAND;
//char buff0[20]=COMMAND1;
char data[DATA_SIZE]={' '};
 
int pt,sat,time,fsec,latd1,latf1,latd2,latf2,alt,falt,track,
  ftrack,speed,fspeed,vv,fvv,pdop,pdop_f,
  hdop,hdop_f,vdop,vdop_f,tdop,tdop_f;
char dir1,dir2,firm[8];
     

fd = open("/dev/ttyS3", O_RDWR | O_NOCTTY|O_NONBLOCK); //Open serial port
 
if(fd<0)
   {
   perror("GPS:unable to open port");
   exit(1);
   }
else
  fprintf(logfile,"\n GPS: Port Opened");
 
tcgetattr(fd, &options); //Get current port settings 
cfsetispeed(&options, BAUDRATE); //Set input speed 
cfsetospeed(&options, BAUDRATE); //Set output speed 
options.c_cflag &= ~PARENB; //Clear the parity bit 
options.c_cflag &= ~CSTOPB; //Clear the stop bit 
options.c_cflag &= ~CSIZE; //Clear the character size 
options.c_cflag |= CHARACTER_SIZE; //Set charater size to 8 bits 
options.c_cflag &= ~CRTSCTS; //Clear the flow control bits 
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //Raw input mode 
options.c_cflag |= (CLOCAL | CREAD); 
//options.c_oflag |= 0; 
tcsetattr(fd, TCSANOW, &options);



write(fd, buff , 100); //Send the commands to G12
 for(i=0;i<100;i++)
   usleep(500);
 	
 if (read(fd, data, DATA_SIZE) == 0 ||
     sscanf(data, "$PASHR,POS,%d,%d,%d.%d.%d.%d,%c,%d.%d,%c,%d.%d,,%d.$d,
                   %d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%s",
	    &pt, &sat, &time, &fsec, &latd1, &latf1, &dir1, &latd2, &latf2, &dir2,
	    &alt, &falt, &track, &ftrack, &speed, &fspeed, &vv, &fvv, &pdop, &pdop_f,
	    &hdop, &hdop_f, &vdop, &vdop_f, &tdop, &tdop_f, &firm) != 27) 
   fprintf(logfile,"\nGPS: read of PASHR,POS error") ;
 else {
   fprintf(logfile,"\nGPS: Here is the position and time !!!!! %d, %d, %d, %d, %d, %x\n", 
	   time,latd1,latd2,alt,speed)  ;

}

close(fd); 
}


int read_config(){
  FILE *fp;
  char *filename = "/home/anita/etc/hkd.conf";
  char option[256];
  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(logfile, "\n read_config: Can't openfile file,%s",filename);
    fflush(logfile);
    exit(1);
  } 
  else {
    while ((fscanf(fp, "%s", option)) != EOF) {
      /* dac */
      if (strcmp(option, "dac0") == 0)
	fscanf(fp, "%i", &(dac[0]));
      if (strcmp(option, "dac1") == 0)
	fscanf(fp, "%i", &(dac[1]));
      if (strcmp(option, "dac2") == 0)
	fscanf(fp, "%i", &(dac[2]));
      if (strcmp(option, "dac3") == 0)
	fscanf(fp, "%i", &(dac[3]));
      if (strcmp(option, "dac4") == 0)
	fscanf(fp, "%i", &(dac[4]));
      if (strcmp(option, "dac5") == 0)
	fscanf(fp, "%i", &(dac[5]));
      if (strcmp(option, "dac6") == 0)
	fscanf(fp, "%i", &(dac[6]));
      if (strcmp(option, "dac7") == 0)
	fscanf(fp, "%i", &(dac[7]));
      /*  tip tilt config read */
      if (strcmp(option, "tiltlevel") == 0)
	fscanf(fp, "%i", &(t_level));      
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
      if (strcmp(option, "looppar00") == 0)
	fscanf(fp, "%f", &(looppar[0]));
      if (strcmp(option, "looppar01") == 0)
	fscanf(fp, "%f", &(looppar[1]));
      if (strcmp(option, "looppar02") == 0)
	fscanf(fp, "%f", &(looppar[2]));
      if (strcmp(option, "looppar03") == 0)
	fscanf(fp, "%f", &(looppar[3]));
      if (strcmp(option, "looppar04") == 0)
	fscanf(fp, "%f", &(looppar[4]));
      if (strcmp(option, "looppar05") == 0)
	fscanf(fp, "%f", &(looppar[5]));
      if (strcmp(option, "looppar06") == 0)
	fscanf(fp, "%f", &(looppar[6]));
      if (strcmp(option, "looppar07") == 0)
	fscanf(fp, "%f", &(looppar[7]));
      if (strcmp(option, "looppar08") == 0)
	fscanf(fp, "%f", &(looppar[8]));
      if (strcmp(option, "looppar09") == 0)
	fscanf(fp, "%f", &(looppar[9]));
      if (strcmp(option, "looppar10") == 0)
	fscanf(fp, "%f", &(looppar[10]));
      if (strcmp(option, "looppar11") == 0)
	fscanf(fp, "%f", &(looppar[11]));
      if (strcmp(option, "looppar12") == 0)
	fscanf(fp, "%f", &(looppar[12]));
      if (strcmp(option, "looppar13") == 0)
	fscanf(fp, "%f", &(looppar[13]));
      if (strcmp(option, "looppar14") == 0)
	fscanf(fp, "%f", &(looppar[14]));
      if (strcmp(option, "looppar15") == 0)
	fscanf(fp, "%f", &(looppar[15]));
      if (strcmp(option, "looppar16") == 0)
	fscanf(fp, "%f", &(looppar[16]));
      if (strcmp(option, "looppar17") == 0)
	fscanf(fp, "%f", &(looppar[17]));
      if (strcmp(option, "looppar18") == 0)
	fscanf(fp, "%f", &(looppar[18]));
      if (strcmp(option, "looppar19") == 0)
	fscanf(fp, "%f", &(looppar[19]));
      if (strcmp(option, "looppar20") == 0)
	fscanf(fp, "%f", &(looppar[20]));
      if (strcmp(option, "looppar21") == 0)
	fscanf(fp, "%f", &(looppar[21]));
      if (strcmp(option, "looppar22") == 0)
	fscanf(fp, "%f", &(looppar[22]));
      if (strcmp(option, "looppar23") == 0)
	fscanf(fp, "%f", &(looppar[23]));
      if (strcmp(option, "looppar24") == 0)
	fscanf(fp, "%f", &(looppar[24]));
      if (strcmp(option, "looppar25") == 0)
	fscanf(fp, "%f", &(looppar[25]));
      if (strcmp(option, "looppar26") == 0)
	fscanf(fp, "%f", &(looppar[26]));
      if (strcmp(option, "looppar27") == 0)
	fscanf(fp, "%f", &(looppar[27]));
      if (strcmp(option, "looppar28") == 0)
	fscanf(fp, "%f", &(looppar[28]));
      if (strcmp(option, "looppar29") == 0)
	fscanf(fp, "%f", &(looppar[29]));
      if (strcmp(option, "looppar30") == 0)
	fscanf(fp, "%f", &(looppar[30]));
      if (strcmp(option, "looppar31") == 0)
	fscanf(fp, "%f", &(looppar[31]));
      /* sample */
      if (strcmp(option, "sample") == 0)
	fscanf(fp, "%i", &(sample));
      /* sample */
      if (strcmp(option, "sample") == 0)
	fscanf(fp, "%i", &(sample));
           /* timeconst */
      if (strcmp(option, "timeconst") == 0)
	fscanf(fp, "%i", &(timeconst));
      /* hk1int; hk2int */
      if (strcmp(option, "hk1int") == 0)
	fscanf(fp, "%i", &(hk1int));
      if (strcmp(option, "hk2int") == 0)
	fscanf(fp, "%i", &(hk1int));
    }
    fclose(fp);
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
	  tb_ptr.time-1000000000, tb_ptr.millitm);
  time=tb_ptr.time-1000000000;
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
/*   packet[18]= */
/*   packet[19]= */

  /* send down the averages for the adcs here */
  /* shift things over to use all the bits, since these are 12 bit adcs */     
  /* 0 */
  adcdata=(int)floor(average[0]*128+0.5);
  packet[18]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[19]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[1]*128+0.5);
  packet[20]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[21]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[2]*128+0.5);
  packet[22]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[23]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[3]*128+0.5);
  packet[24]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[25]=(unsigned char)(adcdata & 0xff);
  adcdata=(int)floor(average[4]*128+0.5);
  packet[26]=(unsigned char) ((adcdata & 0xff00) >> 8);
  packet[27]=(unsigned char)(adcdata & 0xff);
  packet[28]=0; /* already true, but... */
  for (i=7;i>=0; i--){
    packet[28]= packet[28] << 1 + (unsigned char) lint[i];
  }
  packet[29]=0 ;


/*  send down tip tilt information here 1 byte for each */


  /* write to a temporary file first so packet appears atomically*/
  packetfile = fopen(tempfilename, "w+"); 
  fwrite(packet, sizeof(unsigned char),length+1 , packetfile);
  fclose(packetfile);


  printf("\n Values of DAQ at end of hk1_write \n");
  fprintf(stdout, "%1$i,%2$i,%3$i,%4$i,%5$i\n", dac[0],dac[1],dac[2],dac[3],dac[4]); 
  printf("\n Values of cdaq \n");  
  fprintf(stdout, "adc:%1$i,%2$i,%3$i,%4$i,%5$i\n", average[0],average[1],average[2],average[3],average[4]); 

  fprintf(stdout, "%1$i,%2$i,%3$i,%4$i,%5$i\n", packet[18],packet[19],packet[20],packet[21],packet[22]); 

  fprintf(stdout,"%1$f, %2$f, %3$f, %4$f, %5$f\n", average[0],average[1],average[2],average[3],average[4]);
  tilt_read('g',t_level);
  

  link(tempfilename, filename);
  unlink(tempfilename);
}

int hk2_write(){
}





int main(int argc, char **argv)
{
  int i, hk1ctr,hk2ctr,hk1max,hk2max ;
/*    long ltime ;             time stamp used for a part of file name */
/*    time_t t_buf ; */

  /* insert process id code here, file=/home/anita/var/hkd.pid */
  /* prepare hardware */
  signal(SIGINT, sigint_handler);
  signal(SIGTERM,sigterm_handler);
  acromag_setup();
  ip220_setup();
  ip320_setup(); 
  /* ip470_setup(); */
  set_default(); 
  ip320_read(); /* get current values of adcs */
  average_setup(); /* and copy them to signal averages */
  prog_state = 0 ;
  
  while (prog_state == 0){
    ++run_no;
    read_config();
    logfile = stderr ;   /* logfile = stderr, until otherwise set. */
/*      ltime = time(&t_buf) - 1000000000 ;        get time stamp. */

    sprintf(logfilename,"%shkd.log", L_DIR);
/*      sprintf(logfilename,"%s/daq_run%d_%ld.log", L_DIR, run_no, 101); */
/*        sprintf(logfilename,"tmp2.log"); */
/*      logfilename="/home/hebertcl/anita/hkd/hkd/tmp2.log"; */

    if (!(logfile=fopen(logfilename,"w+"))){
         fprintf(stderr," failed to open logfile %s\n", logfilename) ;
	 exit(1) ;  /*this will exit the program */
	}
    
	fprintf(logfile, " log is written to %s\n", logfilename) ;
          tilt_read('R',5);  
/*    	  tilt_read('N',t_level);  */
          gps(); 
/*  put a wait in here */
/*      set_tilt();    set the tip tilt */
    /* getopt switch*/
/*  this is where you can enter information from the screen by running ./hkd -a */

 while ((ich = getopt (argc, argv, "ab:c")) != EOF) {
      switch (ich) { 
        case 'a': /* Flags/Code when -a is specified */ 
   printf("Enter RCP1 Thresh\n");
   scanf("%i",&dac[0]);
   printf("Enter LCP1 Thresh\n");
   scanf("%i",&dac[1]);
   printf("Enter RCP2 Thresh\n");
   scanf("%i",&dac[2]);
   printf("Enter LCP2 Thresh\n");
   scanf("%i",&dac[3]);
   printf("Enter veto Thresh\n");
   scanf("%i",&dac[4]);
   printf("\nValues of dac after read\n");
   fprintf(stderr,"%1$i,%2$i,%3$i,%4$i,%5$i\n", dac[0],dac[1],dac[2],dac[3],dac[4]);
                       break; 
      case 'b': /* Flags/Code when -b is specified */ 
                  /* The argument passed in with b is specified */ 
  		/* by optarg */ 
          break; 
      case 'c': /* Flags/Code when -c is specified */ 
	break; 
      default: /* Code when there are no parameters */ 
	break; 
      } 
 } 
 
 if (optind < argc) { 
   printf ("non-option ARGV-elements: "); 
   while (optind < argc) 
     printf ("%s ", argv[optind++]); 
   printf ("\n"); 
 } 
/*  end of getopt section  */

 
 /* lint_write(); */
 dac_write();
 prog_state=1;
    /* convert houskeeping intervals to number of samples */
    hk1max=(int)((float)hk1int*100./(float)sample); 
    hk2max=(int)((float)hk2int*100./(float)sample); 
    /* force housekeeping on first entry */
    hk1ctr=hk1max;
    hk2ctr=hk2max;
    while (prog_state == 1)
      {
	/* lint_read(); */
	ip320_read();
	average_update();
	servo_compute();
	dac_write();
	usleep(10000*sample);
	if (hk1ctr == hk1max){
	  hk1_write();
	  hk1ctr=0;
	}
	if (hk2ctr == hk1max){
	  hk2_write();
	  hk2ctr=0;
	}
	hk1ctr++;
	hk2ctr++;
      }
  }
  fclose(logfile);
}















