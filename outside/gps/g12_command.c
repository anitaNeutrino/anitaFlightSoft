/*--------------------------------------------
   This is an ADU5 GPS readout code.
   [data bits=8, stop bits=1, parity=none]

   Bei Cai, Michael DuVernois,  09/08/04
--------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include "serialLib/serialLib.h"

#define LEAP_SECONDS 13

#define COMMAND_SIZE 250
#define DATA_SIZE 150
#define COMMAND "$PASHS,ELM,0\n"  /* set elevation mask angle to 0 degree */

void printGPSCommand(char *tempBuffer, int length);
void processGPSCommand(char *tempBuffer, int length);
int breakdownTimeString(char *subString, int *hour, int *minute, int *second); 


#define MYBAUDRATE B9600

int main() { 
    char devName[]="/dev/ttyUSB1";

    int fd, i, retVal,currentIndex ; 
    struct termios options;
    char buff[COMMAND_SIZE] = "";
    char data[DATA_SIZE]={' '};
    char tempBuffer[DATA_SIZE]={' '};
    
/* Silly hack for reboot problem*/
// toggleCRTCTS(devName);
/* open a serial port */
fd = open(devName, O_RDWR | O_NOCTTY);
if(fd<0)
   {
     perror("open");
   printf(" unable to open port \n");
   exit(1);
   }
else
  printf(" Port Opened\n");

/* port settings */
/*  printf("%d %d %d %d %s %d %d\n",options.c_iflag,options.c_oflag,options.c_cflag,options.c_lflag,options.c_cc,options.c_ispeed,options.c_ospeed); */

 retVal=tcgetattr(fd, &options);             /* get current port settings  */
 if(retVal<0) {
     perror("tcgetattr");
     printf("Something fucked\n");     
 }
/*  printf("%d %d %d %d %s %d %d\n",options.c_iflag,options.c_oflag,options.c_cflag,options.c_lflag,options.c_cc,options.c_ispeed,options.c_ospeed); */
/*  printf("Here1\n"); */

cfsetispeed(&options, MYBAUDRATE); //Set input speed 
cfsetospeed(&options, MYBAUDRATE); //Set output speed 
options.c_cflag &= ~PARENB; //Clear the parity bit 
options.c_cflag &= ~CSTOPB; //Clear the stop bit 
options.c_cflag &= ~CSIZE; //Clear the character size 
options.c_cflag |= CHARACTER_SIZE; //Set charater size to 8 bits 
options.c_cflag &= ~CRTSCTS; //Clear the flow control bits 
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //Raw input mode 
options.c_cflag |= (CLOCAL | CREAD); 
//options.c_oflag |= 0; 
tcsetattr(fd, TCSANOW, &options);

 
 
/* send the commands to G12 */
strcat(buff, "$PASHS,INI\r\n");
 strcat(buff, "$PASHS,NME,ALL,B,OFF\r\n");
 strcat(buff, "$PASHS,NME,ALL,A,OFF\r\n");
 strcat(buff, "$PASHQ,PRT\r\n"); 
 strcat(buff, "$PASHQ,RIO\r\n");
 strcat(buff, "$PASHQ,BIT\r\n");
 strcat(buff, "$PASHQ,TST\r\n");
 strcat(buff, "$PASHQ,MEM\r\n");
 strcat(buff, "$PASHQ,PRT\r\n"); 
 //strcat(buff, "$PASHS,RCI,000.05\r\n"); 
// strcat(buff, "$PASHQ,RAW\n"); 
/* strcat(buff, "$PASHQ,STA\n"); */
//strcat(buff, "$PASHS,PPS,0.2,3,R\n");
/* strcat(buff, "$PASHQ,STA\n"); */
/* strcat(buff, "$PASHQ,POS\n"); */
/* strcat(buff, "$PASHQ,PPS\n"); */
// strcat(buff, "$PASHS,SPD,B,4\n");

// strcat(buff, "$PASHS,NME,RMC,B,ON\n");
 //strcat(buff, "$PASHS,LTZ,0,0\n");
 //strcat(buff, "$PASHS,UTS,ON\n");
// strcat(buff, "$PASHS,PPS,1,3,R\n");
 //strcat(buff, "$PASHQ,SAT\n");
 //strcat(buff, "$PASHQ,POS\n");
 //strcat(buff, "$PASHQ,CLK\n");


// strcat(buff, "$PASHS,NME,ZDA,B,ON,5\n");
// strcat(buff, "$PASHQ,PPS\n");//,1,2,R\n");

printf("buff -- %s\n",buff);
write(fd, buff, strlen(buff));
//printf("wrote buff -- %s\n",buff);


/* for(i=0; i < COMMAND_SIZE; i++) printf("%c", buff[i]); */
	
printf("\n");
//sleep(2);
 currentIndex=0;
 while(1) { 
/* read & print output */
     retVal=read(fd, data, DATA_SIZE);
/*     printf("DATA SIZE %d\t%d\n",DATA_SIZE,retVal); */
     for(i=0; i < retVal; i++) {
// 	 printf("%c", data[i]); 
	 if(data[i]=='$') {
/* 	 printf("%d\n",currentIndex); */
	     if(currentIndex) {
		 printGPSCommand(tempBuffer,currentIndex);
//		 processGPSCommand(tempBuffer,currentIndex);
/* 		 printf("%d\t%s\n",time(NULL),tempBuffer); */
/* 		 for(i=0; i < DATA_SIZE; i++) tempBuffer[i]=0; */
	     }
	     currentIndex=0;
	 }
	 tempBuffer[currentIndex++]=data[i];
/* 	 printf("%d\n",currentIndex); */
	 data[i]=0;
     }
/*      printf("\n"); */
/*      printf("\nRet: %d\n",retVal); */
     usleep(1);
 }

close(fd);

}


void printGPSCommand(char *tempBuffer, int length)
{
    char gpsString[DATA_SIZE];
    int count=0;
    char *subString;
    int hour,minute,second,subSecond;
    int day=-1,month=-1,year=-1;
    int tzHour,tzMin;
    time_t rawtime,gpsRawTime,tempTime;
    struct tm timeinfo;    
    char checksum[3];
    char otherString[180];
    char unixString[180];
   
    strncpy(gpsString,tempBuffer,length);
    gpsString[length]='\0';

    time ( &rawtime );
    printf("%ld\t%s\n",rawtime,gpsString);

    /* Should do checksum */
    subString = strtok (gpsString,",");
    if(!strcmp(subString,"$GPZDA") && length==39) {
	printf("Got %s\n",subString);
	while (subString != NULL)
	{
	    switch(count) {
		case 1: 
		    printf("subString: %s\n",subString);
		    breakdownTimeString(subString,&hour,&minute,&second);
/* 		    printf("Length %d\n",strlen(subString)); */
		    break;
		case 2:
		    subSecond=atoi(subString);
		    break;
		case 3:
		    day=atoi(subString);
		    break;
		case 4:
		    month=atoi(subString);
		    break;
		case 5:
		    year=atoi(subString);
		    break;
		case 6:
		    tzHour=atoi(subString);
		    break;
		case 7:
		    tzMin=atoi(subString);
		    break;
		case 8:
		    checksum[0]=subString[0];
		    checksum[1]=subString[1];
		    checksum[2]='\0';		    
		    break;
		default: 
		    break;
	    }
	    count++;
/* 	    printf ("%d\t%s\n",strlen(subString),subString); */
	    subString = strtok (NULL, " ,.*");
	}
	    

/* 	printf("%02d:%02d:%02d.%02d %02d %02d %02d\n%s\n",hour,minute,second,subSecond,day,month,year,ctime(&rawtime)); */
	timeinfo.tm_hour=hour;
	timeinfo.tm_mday=day;
	timeinfo.tm_min=minute;
	timeinfo.tm_mon=month-1;
	timeinfo.tm_sec=second;
	timeinfo.tm_year=year-1900;
	gpsRawTime=mktime(&timeinfo);
	tempTime=(time_t)((long)gpsRawTime);
	printf("%ld\t%ld\t%ld\n",rawtime,gpsRawTime,tempTime);
	strncpy(unixString,ctime(&rawtime),179);
	strncpy(otherString,ctime(&tempTime),179);
	printf("%s%s\n",unixString,otherString);
    }

}

int breakdownTimeString(char *subString, int *hour, int *minute, int *second) {
    char hourStr[3],minuteStr[3],secondStr[3];
    
    hourStr[0]=subString[0];
    hourStr[1]=subString[1];
    hourStr[2]='\0';
    minuteStr[0]=subString[2];
    minuteStr[1]=subString[3];
    minuteStr[2]='\0';
    secondStr[0]=subString[4];
    secondStr[1]=subString[5];
    secondStr[2]='\0';
    
    *hour=atoi(hourStr);
    *minute=atoi(minuteStr);
    *second=atoi(secondStr);    
    return 0;
}



void processGPSCommand(char *tempBuffer, int length)
{
    char gpsString[DATA_SIZE];
    char dateCommand[180];
    int count=0,retVal;
    char *subString;
    int hour,minute,second,subSecond;
    int day=-1,month=-1,year=-1;
    int tzHour,tzMin;
    time_t rawtime,gpsRawTime;
    struct tm timeinfo;    
    char checksum[3];
    char otherString[180];
    char unixString[180];
    char writeString[180];
   
    strncpy(gpsString,tempBuffer,length);
    gpsString[length]='\0';

    printf("%s",gpsString);

    /* Should do checksum */
    subString = strtok (gpsString,",");
    if(!strcmp(subString,"$GPZDA") && length==39) {
	while (subString != NULL)
	{
	    switch(count) {
		case 1: 
/* 		    printf("subString: %s\n",subString); */
		    breakdownTimeString(subString,&hour,&minute,&second);
/* 		    printf("Length %d\n",strlen(subString)); */
		    break;
		case 2:
		    subSecond=atoi(subString);
		    break;
		case 3:
		    day=atoi(subString);
		    break;
		case 4:
		    month=atoi(subString);
		    break;
		case 5:
		    year=atoi(subString);
		    break;
		case 6:
		    tzHour=atoi(subString);
		    break;
		case 7:
		    tzMin=atoi(subString);
		    break;
		case 8:
		    checksum[0]=subString[0];
		    checksum[1]=subString[1];
		    checksum[2]='\0';		    
		    break;
		default: 
		    break;
	    }
	    count++;
/* 	    printf ("%d\t%s\n",strlen(subString),subString); */
	    subString = strtok (NULL, " ,.*");
	}
	    

/* 	printf("%02d:%02d:%02d.%02d %02d %02d %02d\n%s\n",hour,minute,second,subSecond,day,month,year,ctime(&rawtime)); */
	timeinfo.tm_hour=hour;
	timeinfo.tm_mday=day;
	timeinfo.tm_min=minute;
	timeinfo.tm_mon=month-1;
	timeinfo.tm_sec=second;
	timeinfo.tm_year=year-1900;
	gpsRawTime=mktime(&timeinfo);
/* 	printf("%d\t%d\n",rawtime,gpsRawTime); */
	time ( &rawtime );
	strncpy(otherString,ctime(&gpsRawTime),179);	    
	strncpy(unixString,ctime(&rawtime),179);
	printf("%s%s\n",unixString,otherString);
	if(abs(gpsRawTime-rawtime)>0) {
	    gpsRawTime++; /*Silly*/	
	    strncpy(writeString,ctime(&gpsRawTime),179);
	    sprintf(dateCommand,"sudo date -s \"%s\"",writeString);
	    retVal=system(dateCommand);
	    if(retVal==-1) perror("system");
	}
	    
    }

}

