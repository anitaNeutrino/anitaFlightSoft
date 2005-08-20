/*! \file socketLib.c
    \brief Utility library, full of (hopefully) useful stuff
    
    Some functions that do some useful things
    October 2004  rjn@mps.ohio-state.edu
*/

#include "utilLib/utilLib.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/dir.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>


extern  int versionsort(const void *a, const void *b);


void makeDirectories(char *theTmpDir) 
{
    char theCommand[FILENAME_MAX+20];
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(theCommand,"mkdir ");
    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(theCommand,"%s/%s",theCommand,subDir);
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=is_dir(newDir);
	if(!retVal) {	
	    retVal=system(theCommand);
/* 	    printf("%s\n",theCommand); */
/* 	    printf("Ret Val %d\n",retVal); */
	}
	subDir = strtok(NULL,"/");
    }
       

}

int makeLink(const char *theFile, const char *theLinkDir)
{
/*     char theCommand[2*FILENAME_MAX]; */
/*     int retVal; */
/*     sprintf(theCommand,"ln -s %s %s",theFile,theLinkDir); */
/*     retVal=system(theCommand); */
/*     if(retVal!=0) { */
/* 	syslog(LOG_ERR,"%s returned %d",theCommand,retVal); */
/*     } */
/*     return retVal;     */
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theLinkDir,justFile);
//    printf("Linking %s to %s\n",theFile,newFile);
    return symlink(theFile,newFile);

}


int moveFile(const char *theFile, const char *theDir)
{
    char theCommand[2*FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"mv %s %s",theFile,theDir);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}


int copyFile(const char *theFile, const char *theDir)
{
    char theCommand[2*FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"cp %s %s",theFile,theDir);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}


int removeFile(const char *theFile)
{
    char theCommand[FILENAME_MAX];
    int retVal;
    sprintf(theCommand,"rm %s",theFile);
    retVal=system(theCommand);
    if(retVal!=0) {
	syslog(LOG_ERR,"%s returned %d",theCommand,retVal);
    }
    return retVal;    
}

int is_dir(const char *path)
{
    struct stat s;
 
    if (stat(path, &s))
	return 0;
 
    return S_ISDIR(s.st_mode) ? 1 : 0;
}


int getListofLinks(const char *theEventLinkDir, struct dirent ***namelist)
{
/*     int count; */
    int n = scandir(theEventLinkDir, namelist, filterOnDats, versionsort);
    if (n < 0) {
	syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}



int filterHeaders(const struct dirent *dir)
{
    if(strncmp(dir->d_name,"hd",2)==0)
	return 1;
    return 0;
}


int filterOnDats(const struct dirent *dir)
{

    if(strstr(dir->d_name,".dat")!=NULL)
	return 1;
    return 0;
}


int fillCalibStruct(CalibStruct_t *theStruct, char *filename)
{
    /* Takes a pointer to the next struct in the array */
    
    FILE * pFile;

    pFile = fopen (filename, "r");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 0;
    }
    fscanf(pFile,"%d %c",&(theStruct->unixTime),&(theStruct->status));
    fclose (pFile); 
    return 0;
}


int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename)
{
    /* Returns 0 if successful */
    int numObjs;
    FILE * pFile;
        
    pFile = fopen (filename, "rb");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 1;
    }
    numObjs=fread(theEventHdPtr,sizeof(AnitaEventHeader_t),1,pFile);
/*     printf("Read %d objects from %s\n",numObjs,filename); */
    fclose (pFile); 
    if(numObjs==1) return 0; /*Success*/
    return 1;
}


int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename)
{
    /* Returns 0 if successful */
    int numObjs;
    FILE * pFile;
        
    pFile = fopen (filename, "rb");
    if(pFile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 1;
    }
    numObjs=fread(theEventBodyPtr,sizeof(AnitaEventBody_t),1,pFile);
/*     printf("Read %d objects from %s\n",numObjs,filename); */
    fclose (pFile); 
    if(numObjs==1) return 0; /*Success*/
    return 1;
}


int fillGpsStruct(GpsSubTime_t *tttPtr, char *filename)
{
    /* Takes a pointer to the next struct in the array */
    /* Returns number of lines read*/
    
    int numObjs;
    FILE *pFILE = fopen (filename, "rb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fread(tttPtr,sizeof(GpsSubTime_t),1,pFILE);
    fclose(pFILE);
    if(numObjs==1) return 0;
    return 1;
}


int writeHeader(AnitaEventHeader_t *hdPtr, char *filename)
/* Writes the header pointed to by hdPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(bodyPtr,sizeof(AnitaEventBody_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeWaveformPacket(WaveformPacket_t *wavePtr, char *filename)
/* Writes the waveform pointed to by wavePtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(wavePtr,sizeof(WaveformPacket_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeGPSPat(GpsPatStruct_t *patPtr, char *filename)
/* Writes the pat pointed to by patPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(patPtr,sizeof(GpsPatStruct_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeGPSSat(GpsSatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(satPtr,sizeof(GpsSatStruct_t),1,pFILE);
    fclose(pFILE);
    return 0;
}


int writeGPSTTT(GpsSubTime_t *tttPtr, char *filename)
/* Writes the ttt pointed to by tttPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(tttPtr,sizeof(GpsSubTime_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

int writeHk(HkDataStruct_t *hkPtr, char *filename)
/* Writes the hk pointed to by hkPtr to filename */
{
    int numObjs;
    FILE *pFILE = fopen (filename, "wb");
    if(pFILE == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }
    numObjs=fwrite(hkPtr,sizeof(HkDataStruct_t),1,pFILE);
    fclose(pFILE);
    return 0;
}

 
void sigIntHandler(int sig)
{
    currentState=PROG_STATE_INIT;
} 

void sigTermHandler(int sig)
{
    currentState=PROG_STATE_TERMINATE;
} 

void writePidFile(char *fileName)
{
    FILE *fpPid ;
    char theDir[FILENAME_MAX];
    char tempFileName[FILENAME_MAX];
    strncpy(tempFileName,fileName,FILENAME_MAX-1);
    strncpy(theDir,dirname(tempFileName),FILENAME_MAX-1);
    makeDirectories(theDir);
//    pid_t thePid;
    if (!(fpPid=fopen(fileName, "w"))) {
	fprintf(stderr," failed to open a file for PID, %s\n", fileName);
	syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
	exit(1) ;
    }
    fprintf(fpPid,"%d\n", getpid()) ;
    fclose(fpPid) ;
}

void addDay(struct tm *timeinfo) {
    //Must do leap year check
    int year=timeinfo->tm_year;
    int isLeap=((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0));
    int daysInMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(isLeap) daysInMonth[1]=29; //Leap year
    
    timeinfo->tm_mday++;
    if(timeinfo->tm_mday>daysInMonth[timeinfo->tm_mon]) {
	timeinfo->tm_mday=1;
	timeinfo->tm_mon++;
	if(timeinfo->tm_mon>=12) {
	    timeinfo->tm_mon=0;
	    timeinfo->tm_year++;
	}
    }
}
