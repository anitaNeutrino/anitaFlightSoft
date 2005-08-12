
#include "hkd3.h"


#define INTERRUPT_LEVEL 10
int run_no=0;
int pos_int = 6; /* gps/tilt logging cycles in seconds */
int hk3_int = 10; /* number of logging cycles per hk3 */
int pt,sat,anitatime,fsec,latd1,latf1,latd2,latf2,alt,falt,track, 
  ftrack,speed,fspeed,vv,fvv,pdop,pdop_f, hdop, hdop_f,vdop,vdop_f,tdop,tdop_f;
float pitch, roll;
double pitch2,roll2,pitchint,pitch_dec,roll_dec;
/* This section of our program is for Function Prototypes */

FILE *logfile;

char logfilename[32];

int prog_state; /* 0 initially or on SIGINT, 1 for looping, 2 for terminate */

int ich;

int t_level;

/* signal handlers */

void sigint_handler(int sig) { prog_state = 0; }
void sigterm_handler(int sig) { prog_state = 2; }



 

int set_default()
{
  /* default settings of all parameters go here */
  int i;
  t_level=5;
  /*  hk3_int=6;*/
  /* pos_int=10;*/
  hk3_int=1;
  pos_int=1;
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
  /* now in global scope
  float pitch, roll;
  double pitch2,roll2,pitchint,pitch_dec,roll_dec;
  */
  double temp;
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
   err = fcntl(fd, F_SETFL, O_NDELAY);          /* READ returns immediately with whatever is in buffer */
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
/*       fprintf(logfile,"\ntilt_read:number of bytes written from 'N' = %i",n); */
/*       fprintf(logfile, "\ntilt_read:N wrote [%c%x]\n",out[0],out[1]); */
     if (n<2)
     fprintf(logfile,"\ntilt_read:input data write failed");
     fflush(logfile);


     switch(c)
     {
     case 'R':
       n = write (fd,"R",1);
   /*       fprintf(logfile,"\n tilt_read:number of bytes written from 'R' = %i",n);  */
       if (n<1)
  	 fprintf(logfile,"tilt_read:input data write failed"); 
       usleep(10000);                         /*   wait 10 millisec */
       /*       fprintf(logfile,"\ntilt_read:return from sleep"); */
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
/*         printf("read %i bytes\n",n);	     */
/*         printf("data %x %x %x %x %x %x\n",data[0],data[1],data[2],data[3],data[4],data[5]); */
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
/*  	   	   printf("pitch = %5.2f  roll = %5.2f\n",pitch,roll);  */
/*  	resolution is 1/4 of a degree    +/- 45 degrees */

/*this is where I round off to the nearest quarter degree*/

  	pitch2=4*pitch; 
  	pitch_dec=modf(pitch2,&pitchint); 

	if (fabs(pitch_dec) >= 0.5) 
	  {
	    /*  	    printf(" abs(pitch_dec).ge..5 %f\n",temp); */ 
	    pitch2=floor(pitch2);
          }
	if (fabs(pitch_dec) < 0.5){
	  /*          printf("abs(pitch_dec).lt..5 %f\n",pitch_dec);    */    
 	  pitch2=ceil(pitch2);
	}

  	roll2=4*roll; 
  	roll_dec=modf(roll2,&pitchint); 

	if (fabs(roll_dec) >= 0.5) 
	  {
	    /*	printf("abs(roll_dec).ge..5 %f\n",roll_dec); */ 
	    roll2=floor(roll2);
	    /*  printf("pitch2 %f \n", (pitch2)/4); */
          }
	if (fabs(roll_dec) < 0.5){
	 /*   printf("abs(roll_dec).lt..5 %f\n",temp);  */       
 	  roll2=(int)ceil(roll2);
	/*    printf("pitch2 %f\n", (pitch2)/4); */
	}
     


	/*           printf("pitch2 %f\n",(pitch2)/4);*/ 
        /*   printf("roll2 %f\n",(roll2)/4); */

       
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

#define DATA_SIZE 128
int gps() 
{ 
  int error=0;
  int fd,n,i; 
  struct termios options;
  /* char buff[COMMAND_SIZE]={' '}; */
  char buff[COMMAND_SIZE]=COMMAND;
  /* char buff0[20]=COMMAND1; */
  char data[DATA_SIZE]={' '};
  /* These are now in global scope JJB
     int pt,sat,anitatime,fsec,latd1,latf1,latd2,
     latf2,alt,falt,track,
     ftrack,speed,fspeed,vv,fvv,pdop,pdop_f,
     hdop,hdop_f,vdop,vdop_f,tdop,tdop_f;
  */
  
  char dir1,dir2,firm[8];
  

  fd = open("/dev/ttyS3", O_RDWR | O_NOCTTY|O_NONBLOCK); /* Open serial port */
  
  if(fd<0){
    perror("GPS:unable to open port");
    exit(1);
  }
  else{
    fprintf(logfile,"\n GPS: Port Opened");
  }
  flock(fd,LOCK_EX); /* set a Posix lock on the serial port */
  tcgetattr(fd, &options); /* Get current port settings */
  cfsetispeed(&options, BAUDRATE); /* Set input speed */
  cfsetospeed(&options, BAUDRATE); /* Set output speed */
  options.c_cflag &= ~PARENB; /* Clear the parity bit */
  options.c_cflag &= ~CSTOPB; /* Clear the stop bit */
  options.c_cflag &= ~CSIZE; /* Clear the character size */
  options.c_cflag |= CHARACTER_SIZE; /* Set charater size to 8 bits */
  options.c_cflag &= ~CRTSCTS; /* Clear the flow control bits */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Raw input mode */
  options.c_cflag |= (CLOCAL | CREAD); 
  /* options.c_oflag |= 0; */
  tcsetattr(fd, TCSANOW, &options);
  
  

  write(fd, buff , 100); /* Send the commands to G12 */
  for(i=0;i<100;i++)
    usleep(500);
  
  if (read(fd, data, DATA_SIZE) == 0 ||
      sscanf(data, "$PASHR,POS,%d,%d,%d.%d.%d.%d,%c,%d.%d,%c,%d.%d,,%d.$d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%d.%d,%s",
	     &pt, &sat, &anitatime, &fsec, &latd1, &latf1, &dir1, &latd2, &latf2, &dir2,
	     &alt, &falt, &track, &ftrack, &speed, &fspeed, &vv, &fvv, &pdop, &pdop_f,
	     &hdop, &hdop_f, &vdop, &vdop_f, &tdop, &tdop_f, &firm) != 27) 
      fprintf(logfile,"\nGPS: read of PASHR,POS error") ;
  else {
    fprintf(logfile,"\nGPS: Here is the position and time !!!!! %d, %d, %d, %d, %d, %x\n", 
	    anitatime,latd1,latd2,alt,speed)  ;
    
  }
  flock(fd,LOCK_UN); /* clear the lock on the serial port */
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
      /*  tip tilt config read */
      if (strcmp(option,"tiltlevel") == 0)
	fscanf(fp, "%i", &(t_level));      
      /* servo */
      if (strcmp(option,"hk3_int") == 0)
	fscanf(fp, "%i", &(hk3_int));
      if (strcmp(option,"pos_int") == 0)
	fscanf(fp, "%i", &(pos_int));
    }
    fclose(fp);
  }
}

int call_GPS() {

  int fd, i, error=0; 
  int date, hour, min, sec, f_sec, cs ;
  struct termios options;
  char data[DATA_SIZE] = {' '};

  static int first=1 ;

  lockf(fd,F_LOCK,0); /* set a Posix lock on the serial port */

  if ((fd = open("/dev/ttyS3", O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
    fprintf(logfile, " call_GPS: unable to open port \n");
    return 1;
  }

  if (first) {
      tcgetattr(fd, &options); /* Get current port settings */
      cfsetispeed(&options, BAUDRATE); /* Set input speed */
      cfsetospeed(&options, BAUDRATE); /* Set output speed */
      options.c_cflag &= ~PARENB; /* Clear the parity bit */
      options.c_cflag &= ~CSTOPB; /* Clear the stop bit */
      options.c_cflag &= ~CSIZE; /* Clear the character size */
      options.c_cflag |= CHARACTER_SIZE; /* Set charater size to 8 bits */
      options.c_cflag &= ~CRTSCTS; /* Clear the flow control bits */
      options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Raw input mode */
      options.c_cflag |= (CLOCAL | CREAD); 
      /* options.c_oflag |= 0; */
      tcsetattr(fd, TCSANOW, &options);
      first = 0 ;
  }

  /* wait for 1 ms.  usleep seems to be awaikened by each charactor 
     input to a port, so usleep(1000) does not work but this does. */
  for (i=0 ; i<50 ; i++) usleep(20) ;

/*  read(fd, data, DATA_SIZE);  Read the output of G12 into data */ 

/*    for(i=0;i<DATA_SIZE;i++) */
/*      fprintf(logfile, "%c", data[i]);  */
  if ((i=read(fd, data, DATA_SIZE)) <= 0 || i > DATA_SIZE ||
      sscanf(data, "$PASHR,TTT,%d,%d:%d:%d.%d*%x", 
	     &date, &hour, &min, &sec, &f_sec, &cs) != 6)  
    printf("No GPS data.\n");
      /* header.GPSday = error = -1 ;*/    /* day = -1 means no GPS data. */
  else {
    fprintf(logfile, " call_GPS: %d, %d, %d, %d, %d, %x\n", 
	    date, hour, min, sec, f_sec, cs)  ;
    printf(" call_GPS: %d, %d, %d, %d, %d, %x\n", 
	    date, hour, min, sec, f_sec, cs)  ;
   /*   header.GPSday = date ; */
/*      header.GPShour = hour ; */
/*      header.GPSmin = min ; */
/*      header.GPSsec = sec ; */
/*      header.GPSfsec = f_sec ; */
  }

  close(fd); 
  lockf(fd,F_ULOCK,0); /* reset a Posix lock on the serial port */

  return error ;
}


int hk3_write(){
  /* this packet type contains the critical threshold-related parameters */
  int i, adcdata;
  FILE* packetfile;
  unsigned char packet[100];
  char filename[64];
  char * tempfilename="/home/anita/tmp/hk3.tmp";
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
  packet[1]=0x12;  /* packet type 1, subtype 2 */
  memcpy(&packet[2],&time,4); /* 2-5 */
  memcpy(&packet[6],&millitime,2); /* 6-7 */


  packet[8]= 1 ;
  
  packet[29]=0 ; /* pad to odd */
/* These are now in global scope JJB
  int pt,sat,time,fsec,latd1,latf1,latd2,
  latf2,alt,falt,track,
  ftrack,speed,fspeed,vv,fvv,pdop,pdop_f,
  hdop,hdop_f,vdop,vdop_f,tdop,tdop_f;
*/
/*float pitch, roll;*/
/*double pitch2,roll2,pitchint,pitch_dec,roll_dec*/

/*send down tip tilt information here 1 byte for each */


  /* write to a temporary file first so packet appears atomically*/
  packetfile = fopen(tempfilename, "w+"); 
  fwrite(packet, sizeof(unsigned char),length+1 , packetfile);
  fclose(packetfile);
  fprintf(stderr,"pt=%i,sat=%i,anitatime=%i,fsec=%i,latd1=%i,latf1=%i,latd2=%i,latf2=%i,\n\
alt=%i,falt=%i,track=%i,ftrack=%i,speed=%i,fspeed=%i,\n\
vv=%i,fvv=%i,pdop=%i,pdop_f=%i,hdop=%i,hdop_f=%i,\n\
vdop=%i,vdop_f=%i,tdop=%i,tdop_f=%i\n"
	 ,pt,sat,time,fsec,latd1,latf1,latd2,latf2,alt,falt,track,ftrack,
	 speed,fspeed,vv,fvv,pdop,pdop_f,hdop,hdop_f,vdop,vdop_f,tdop,tdop_f);
  
  fprintf(stderr,"pitch=%f, roll=%f,pitch2=%lf,roll2=%lf,\npitchint=%lf,pitch_dec=%lf,roll_dec=%lf\n",
	  pitch, roll,pitch2,roll2,pitchint,pitch_dec,roll_dec);
  

  link(tempfilename, filename);
  unlink(tempfilename);
}

int main(int argc, char **argv)
{
  int i, do_hk3;
  long ltime;
  time_t t_buf ; 
  char *pidf_name = "/home/anita/var/pid/hkd3.pid" ;
  FILE *pid_f ;
  pid_t pid_hkd ;
  /* process id code here, file=/home/anita/var/hkd3.pid */
  if (!(pid_f=fopen(pidf_name, "w"))) {
    printf(" failed to open a file for PID, %s\n", pidf_name);
	  exit(1) ;
  }
  fprintf(pid_f,"%d\n", pid_hkd=getpid()) ;
  fclose(pid_f) ;
/*    long ltime ;             time stamp used for a part of file name */      

  /* insert process id code here, file=/home/anita/var/hkd.pid */
  /* prepare hardware */
  signal(SIGINT, sigint_handler);
  signal(SIGTERM,sigterm_handler);
  /*  acromag_setup(); */
  /* ip320_setup(); */
  /* ip470_setup(); */
  set_default(); 
  /* ip320_read(); *//* get current values of adcs */
  /*average_setup();*/ /* and copy them to signal averages */
  prog_state = 0 ;
  
  while (prog_state == 0){
    ++run_no;
    read_config();
    logfile = stderr ;   /* logfile = stderr, until otherwise set. */
    ltime = time(&t_buf) - 1000000000 ;  /*      get time stamp. */

    sprintf(logfilename,"%shkd%i.log", L_DIR,ltime);
/*      sprintf(logfilename,"%s/daq_run%d_%ld.log", L_DIR, run_no, 101); */
/*        sprintf(logfilename,"tmp2.log"); */
/*      logfilename="/home/hebertcl/anita/hkd/hkd/tmp2.log"; */

    if (!(logfile=fopen(logfilename,"w+"))){
         fprintf(stderr," failed to open logfile %s\n", logfilename) ;
	 exit(1) ;  /*this will exit the program */
	}
    fprintf(logfile, " log is written to %s\n", logfilename) ;
/*    	  tilt_read('N',t_level);  */

/*  put a wait in here */
/*      set_tilt();    set the tip tilt */
    prog_state=1;
    do_hk3=hk3_int;
    /* convert houskeeping intervals to number of samples */
    while (prog_state == 1)
      {
	/* read gps and tip-tilt */
        tilt_read('g',t_level);
	/*gps();*/
/* 	call_GPS(); */
	/* write an hk3 event */
	if (do_hk3==hk3_int){
	  hk3_write();
	  do_hk3=0;
	}
	do_hk3++;
	/* sleep a while and do it again */
	sleep(pos_int);
      }
  }
  fclose(logfile);
}
