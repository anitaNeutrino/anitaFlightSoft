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
#include <zlib.h>


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
/*     char theCommand[2*FILENAME_MAX]; */
/*     int retVal; */
/*     sprintf(theCommand,"mv %s %s",theFile,theDir); */
/*     retVal=system(theCommand); */
/*     if(retVal!=0) { */
/* 	syslog(LOG_ERR,"%s returned %d",theCommand,retVal); */
/*     } */
/*     return retVal;     */
    int retVal;
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theDir,justFile);
    retVal=link(theFile,newFile);
    retVal+=unlink(theFile);
    return retVal;
    
}


int copyFile(const char *theFile, const char *theDir)
{
/*     char theCommand[2*FILENAME_MAX]; */
/*     int retVal; */
/*     sprintf(theCommand,"cp %s %s",theFile,theDir); */
/*     retVal=system(theCommand); */
/*     if(retVal!=0) { */
/* 	syslog(LOG_ERR,"%s returned %d",theCommand,retVal); */
/*     } */
/*     return retVal;   */
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theDir,justFile);
    return link(theFile,newFile);
}


int removeFile(const char *theFile)
{
/*     int retVal; */
/*     char theCommand[FILENAME_MAX]; */
/*     sprintf(theCommand,"rm %s",theFile); */
/*     retVal=system(theCommand); */
/*     if(retVal!=0) { */
/* 	syslog(LOG_ERR,"%s returned %d",theCommand,retVal); */
/*     } */
/*     return retVal; */
    int retVal=unlink(theFile);
    if(retVal!=0) {
	syslog(LOG_ERR,"Error removing %s:\t%s",theFile,strerror(errno));
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
    
    FILE * infile;

    infile = fopen (filename, "r");
    if(infile == NULL) {
	syslog (LOG_ERR,"Couldn't open file: %s\n",filename);
	return 0;
    }
    fscanf(infile,"%ld %c",&(theStruct->unixTime),&(theStruct->status));
    fclose (infile); 
    return 0;
}


int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename)
{
    int numObjs;    
#ifdef NO_ZLIB
    FILE *infile = fopen (filename, "rb");
#else
    gzFile infile = gzopen (filename, "rb");    
#endif
    if(infile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fread(theEventHdPtr,sizeof(AnitaEventHeader_t),1,infile);
    fclose(infile);
    if(numObjs==1) return 0; /*Success*/
#else
    numObjs=gzread(infile,theEventHdPtr,sizeof(AnitaEventHeader_t));
    gzclose(infile);
    if(numObjs==sizeof(AnitaEventHeader_t)) return 0;
#endif
    return 1;
}


int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename)
{    
    int numObjs;    
#ifdef NO_ZLIB
    FILE *infile = fopen (filename, "rb");
#else
    gzFile infile = gzopen (filename, "rb");    
#endif
    if(infile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fread(theEventBodyPtr,sizeof(AnitaEventBody_t),1,infile);
    fclose(infile);
    if(numObjs==1) return 0; /*Success*/
#else
    numObjs=gzread(infile,theEventBodyPtr,sizeof(AnitaEventBody_t));
    gzclose(infile);
    if(numObjs==sizeof(AnitaEventBody_t)) return 0;
#endif
    return 1;
}


int fillGpsStruct(GpsSubTime_t *tttPtr, char *filename)
{
    /* Takes a pointer to the next struct in the array */
    /* Returns number of lines read*/
       
    int numObjs;    
#ifdef NO_ZLIB
    FILE *infile = fopen (filename, "rb");
#else
    gzFile infile = gzopen (filename, "rb");    
#endif
    if(infile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fread(tttPtr,sizeof(GpsSubTime_t),1,infile);
    fclose(infile);
    if(numObjs==1) return 0; /*Success*/
#else
    numObjs=gzread(infile,tttPtr,sizeof(GpsSubTime_t));
    gzclose(infile);
    if(numObjs==sizeof(GpsSubTime_t)) return 0;
#endif
    return 1;
}


int writeHeader(AnitaEventHeader_t *hdPtr, char *filename)
/* Writes the header pointed to by hdPtr to filename */
{
    int numObjs;    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,hdPtr,sizeof(AnitaEventHeader_t));
    gzclose(outfile);
#endif
    return 0;
}

int writeBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
    int numObjs;    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(bodyPtr,sizeof(AnitaEventBody_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,bodyPtr,sizeof(AnitaEventBody_t));
    gzclose(outfile);
#endif
    return 0;
}

int writeWaveformPacket(WaveformPacket_t *wavePtr, char *filename)
/* Writes the waveform pointed to by wavePtr to filename */
{
     int numObjs;    
  
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(wavePtr,sizeof(WaveformPacket_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,wavePtr,sizeof(WaveformPacket_t));
    gzclose(outfile);  
#endif
    return 0;
}


int writeSurfPacket(SurfPacket_t *surfPtr, char *filename)
/* Writes the surf packet pointed to by surfPtr to filename */
{
    int numObjs;    
      
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(surfPtr,sizeof(SurfPacket_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,surfPtr,sizeof(SurfPacket_t));
    gzclose(outfile);  
#endif
    return 0;
}

int writeGPSPat(GpsPatStruct_t *patPtr, char *filename)
/* Writes the pat pointed to by patPtr to filename */
{
      int numObjs;    
    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(patPtr,sizeof(GpsPatStruct_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,patPtr,sizeof(GpsPatStruct_t));
    gzclose(outfile);  
#endif
    return 0;
}

int writeGPSSat(GpsSatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{   
    int numObjs;        
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(satPtr,sizeof(GpsSatStruct_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,satPtr,sizeof(GpsSatStruct_t));
    gzclose(outfile);  
#endif
    return 0;
}


int writeGPSTTT(GpsSubTime_t *tttPtr, char *filename)
/* Writes the ttt pointed to by tttPtr to filename */
{     
    int numObjs;    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(tttPtr,sizeof(GpsSubTime_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,tttPtr,sizeof(GpsSubTime_t));
    gzclose(outfile);  
#endif
    return 0;
}

int writeHk(HkDataStruct_t *hkPtr, char *filename)
/* Writes the hk pointed to by hkPtr to filename */
{     
    int numObjs;    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(hkPtr,sizeof(HkDataStruct_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,hkPtr,sizeof(HkDataStruct_t));
    gzclose(outfile);  
#endif
    return 0;
}


int writeCmdEcho(CommandEcho_t *echoPtr, char *filename)
/* Writes the echo pointed to by echoPtr to filename */
{   
    int numObjs;    
#ifdef NO_ZLIB
    FILE *outfile = fopen (filename, "wb");
#else
    gzFile outfile = gzopen (filename, "wb");    
#endif
    if(outfile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	return -1;
    }   
#ifdef NO_ZLIB
    numObjs=fwrite(echoPtr,sizeof(CommandEcho_t),1,outfile);
    fclose(outfile);
#else
    numObjs=gzwrite(outfile,echoPtr,sizeof(CommandEcho_t));
    gzclose(outfile);  
#endif
    return 0;
}

 
void sigUsr1Handler(int sig)
{
    currentState=PROG_STATE_INIT;
} 

void sigUsr2Handler(int sig)
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
