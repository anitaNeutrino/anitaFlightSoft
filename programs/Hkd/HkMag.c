#include <stdio.h>
#include <stdlib.h>
#include "serialLib/serialLib.h"
#include <errno.h>
#include <string.h>
#include "utilLib/utilLib.h"
#include <signal.h>

#define MAGNETOMETER_DEV_NAME "/dev/ttyMag" 

int openMagnetometer();
int setupMagnetometer();
int checkMagnetometer();
int closeMagnetometer();


int fdMag = 0; 
int printToScreen = 1;
FILE * outfile = 0; 

int openMagnetometer()
{

    int retVal;
    retVal=openMagnetometerDevice(MAGNETOMETER_DEV_NAME);
    fdMag=0;
    if(retVal<=0) {
        syslog(LOG_ERR,"Couldn't open: %s\r\n",MAGNETOMETER_DEV_NAME);
        if(printToScreen) printf("Couldn't open: %s\r\n",MAGNETOMETER_DEV_NAME);
    }
    else fdMag=retVal;

    printf("Opened %s %d\r\n",MAGNETOMETER_DEV_NAME,fdMag);
    return 0;
}


int setupMagnetometer() 
{
//    printf("Here\r\n");
    int retVal=0;
    char setupCommand[128];
    char tempData[256];

    sprintf(setupCommand,"*\r\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));

    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen) fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }

    sprintf(setupCommand,"M?\r\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen) fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }
    else {
//        syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//        if(printToScreen)
            printf("Sent %d bytes to Magnetometer serial port:\t%s\r\n",retVal,MAGNETOMETER_DEV_NAME);
    }

    usleep(10000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
        retVal=read(fdMag,tempData,256);
        printf("%s\r\n",tempData);
    }

    usleep(10000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
        retVal=read(fdMag,tempData,256);
        printf("%s\r\n",tempData);
    }
    sprintf(setupCommand,"M=T\r\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen)
            fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }
    else {
//        syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//        if(printToScreen)
            printf("Sent %d bytes to Magnetometer serial port:\t%s\r\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(10000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
        retVal=read(fdMag,tempData,256);
        printf("%s\r\n",tempData);
    }
    sprintf(setupCommand,"M=C\r\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen)
            fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }
    else {
//        syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//        if(printToScreen)
            printf("Sent %d bytes to Magnetometer serial port:\t%s\r\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(10000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
        retVal=read(fdMag,tempData,256);
        printf("%s\r\n",tempData);
    }

    sprintf(setupCommand,"M=E\r\n");
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen)
            fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }
    else {
//        syslog(LOG_INFO,"Sent %d bytes to Magnetometer serial port",retVal);
//        if(printToScreen)
//            printf("Sent %d bytes to Magnetometer serial port:\t%s\r\n",retVal,MAGNETOMETER_DEV_NAME);
    }
    usleep(10000);
    retVal=isThereDataNow(fdMag);
    if(retVal) {
        retVal=read(fdMag,tempData,256);
        printf("%s\r\n",tempData);
    }


    sprintf(setupCommand,"S\r\n"); //Stop autosend data
    retVal=write(fdMag, setupCommand, strlen(setupCommand));
    if(retVal<0) {
        syslog(LOG_ERR,"Unable to write to Magnetometer Serial port\r\n, write: %s", strerror(errno));
        if(printToScreen) fprintf(stderr,"Unable to write to Magnetometer Serial port\r\n");
        return -1;
    }

    usleep(10000); 

    return 0;
}

typedef struct
{

  double x; 
  double y; 
  double z; 
}  mag_t; 

mag_t magData; 


int checkMagnetometer()
{
    char tempData[256];
    int retVal=0,i;
    float x,y,z;
    int checksum,otherChecksum;
    int countAttempts=0;
    int countSpaces =0; 
    char c; 
    char cmd[] = "D\r\n"; 


    while (isThereDataNow(fdMag))
    {
      read(fdMag, tempData, sizeof(tempData)); 
    }

    retVal = write(fdMag,cmd, sizeof(cmd));   

    if (retVal < sizeof(cmd))
    {
      syslog(LOG_ERR, "couldn't write to Magnetometer\n"); 
      fprintf(stderr, "couldn't write to Magnetometer\n"); 
    }


    while(!isThereDataNow(fdMag) && countAttempts < 5)
    {
      usleep(10000); 
      countAttempts++; 
    }

    if (countAttempts >= 5)
    {
      syslog(LOG_ERR, "didn't hear back from Magnetometer in time\n"); 
      fprintf(stderr, "didn't hear back from Magnetometer in time\n"); 
    }


    memset(tempData,0, sizeof(tempData)); 
    i = 0; 
    do 
    {
      read(fdMag,&c, 1); 
      tempData[i++] =c; 
    } while (c!='>'); 


    if (printToScreen)
      printf("Retval %d -- %s\r\n",i,tempData);

    retVal = sscanf(tempData+sizeof(cmd),"%f %f %f %x",&x,&y,&z,&checksum);

    if (retVal < 4) 
    {
      fprintf(stderr,"Malformed string: %s\n", tempData); 
      syslog(LOG_ERR,"Malformed string: %s\n", tempData); 
      return 1; 
    }

    if(printToScreen) printf("Mag:\t%f %f %f\t%d\r\n",x,y,z,checksum);

    magData.x=x;
    magData.y=y;
    magData.z=z;
        
    otherChecksum=0;
    for(i=0;i<strlen(tempData);i++)
    {
        if(tempData[i]=='1') otherChecksum+=1;
        if(tempData[i]=='2') otherChecksum+=2;
        if(tempData[i]=='3') otherChecksum+=3;
        if(tempData[i]=='4') otherChecksum+=4;
        if(tempData[i]=='5') otherChecksum+=5;
        if(tempData[i]=='6') otherChecksum+=6;
        if(tempData[i]=='7') otherChecksum+=7;
        if(tempData[i]=='8') otherChecksum+=8;
        if(tempData[i]=='9') otherChecksum+=9;
        if(otherChecksum && tempData[i]==' ') countSpaces++;
        if(countSpaces==3) break;
    }

    if(printToScreen) printf("Checksums:\t%d %d\r\n",checksum,otherChecksum);

    if (outfile) 
    {
      fprintf(outfile, "%g,%g,%g,%d,%d\n",x,y,z,checksum,otherChecksum);
    }


    if(checksum!=otherChecksum) 
    {
      syslog(LOG_WARNING,"Bad Magnetometer Checksum %s",tempData);
      memset(&magData,0, sizeof(magData)); 
      return -1;
    }

    return retVal;
}

int closeMagnetometer()
{
  return close(fdMag);   
}


static int alive; 
void handle_ctrlc(int sig) 
{
  alive = 0; 

}

int main (int nargs, char ** args) 
{


  alive =1; 
  openMagnetometer(); 
  setupMagnetometer(); 
  signal(SIGINT, handle_ctrlc); 
   
  if (nargs > 1) 
  {
    outfile = fopen(args[1],"w"); 
  }


  while(alive)
  {
    sleep(1); 
    checkMagnetometer(); 
  }

  if (outfile) fclose(outfile); 
  closeMagnetometer(); 
}


