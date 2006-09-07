/*! \file utilLib.c
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
#include <sys/statvfs.h>
#include <string.h>
#include <sys/dir.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <zlib.h>
#include <fcntl.h>
#define NO_ZLIB


extern  int versionsort(const void *a, const void *b);

int diskBitMasks[DISK_TYPES]={0x1,0x2,0x4,0x8,0x10};
char *diskNames[DISK_TYPES]={BLADE_DATA_MOUNT,PUCK_DATA_MOUNT,USBINT_DATA_MOUNT,USBEXT_DATA_MOUNT,SAFE_DATA_MOUNT};
int bufferDisk[DISK_TYPES]={1,0,1,1,0};

int closeHkFilesAndTidy(AnitaHkWriterStruct_t *awsPtr) {
    int diskInd;
    pid_t childPid;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(diskBitMasks[diskInd]&awsPtr->writeBitMask)) continue;
//	char *gzipArg[] = {"gzip",awsPtr->currentFileName[diskInd],(char*)0};
//	char *zipAndMoveArg[]={"/bin/bash","/home/anita/flightSoft/bin/zipAndMoveFile.sh",awsPtr->currentFileName[diskInd],(char*)0};
	if(awsPtr->currentFilePtr[diskInd]) {
	    fclose(awsPtr->currentFilePtr[diskInd]);
	    childPid=fork();
	    if(childPid==0) {
		//Child
//	    awsPtr->currentFileName;
		if(!bufferDisk[diskInd]) {
		    zipFileInPlace(awsPtr->currentFileName[diskInd]);
//		    execv("/bin/gzip",gzipArg);
		}
		else {
		    zipBufferedFileAndMove(awsPtr->currentFileName[diskInd]);
//		    execv("/bin/bash",zipAndMoveArg);
		}
		exit(0);
	    }
	    else if(childPid<0) {
		//Something wrong
		syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
		fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
//		return -1;
	    }
	}
    }
    return 0;
}


int cleverHkWrite(unsigned char *buffer, int numBytes, unsigned long unixTime, AnitaHkWriterStruct_t *awsPtr)
{
    int diskInd;
    int retVal=0;
    static int errorCounter=0;
    char *tempDir;
    pid_t childPid;
    char fullBasename[FILENAME_MAX];
    char bufferName[FILENAME_MAX];
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(diskBitMasks[diskInd]&awsPtr->writeBitMask)) continue;
	if(!awsPtr->currentFilePtr[diskInd]) {
	    //First time
	    sprintf(fullBasename,"%s/%s",diskNames[diskInd],awsPtr->relBaseName);
	    tempDir=getCurrentHkDir(fullBasename,unixTime);
	    strncpy(awsPtr->currentDirName[diskInd],tempDir,FILENAME_MAX-1);

	    if(bufferDisk[diskInd]) {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentDirName[diskInd]);
		makeDirectories(bufferName);

	    }
	    awsPtr->dirCount[diskInd]=0;
	    free(tempDir);
	    tempDir=getCurrentHkDir(awsPtr->currentDirName[diskInd],unixTime);
	    strncpy(awsPtr->currentSubDirName[diskInd],tempDir,FILENAME_MAX-1);
	    if(bufferDisk[diskInd]) {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentSubDirName[diskInd]);
		makeDirectories(bufferName);
	    }
	    awsPtr->fileCount[diskInd]=0;
	    free(tempDir);
	
	    tempDir=getCurrentHkFilename(awsPtr->currentSubDirName[diskInd],
					 awsPtr->filePrefix,unixTime);
	    strncpy(awsPtr->currentFileName[diskInd],tempDir,FILENAME_MAX-1);
	    free(tempDir);
	    if(!bufferDisk[diskInd]) {
		awsPtr->currentFilePtr[diskInd]=fopen(awsPtr->currentFileName[diskInd],"wb");	    
	    }
	    else {		
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentFileName[diskInd]);
		awsPtr->currentFilePtr[diskInd]=fopen(bufferName,"ab");
	    }
	    awsPtr->writeCount[diskInd]=0;						      
	}
    
	//Check if we need a new file or directory
	if(awsPtr->writeCount[diskInd]>=HK_PER_FILE) {
	    //Need a new file
//	    char *gzipArg[] = {"gzip",awsPtr->currentFileName[diskInd],(char*)0};
//	    char *zipAndMoveArg[]={"/bin/bash","/home/anita/flightSoft/bin/zipAndMoveFile.sh",awsPtr->currentFileName[diskInd],(char*)0};
	    fclose(awsPtr->currentFilePtr[diskInd]);
	    childPid=fork();
	    if(childPid==0) {
		//Child
//	    awsPtr->currentFileName;
		if(!bufferDisk[diskInd]) {
		    zipFileInPlace(awsPtr->currentFileName[diskInd]);
//		    execv("/bin/gzip",gzipArg);
		}
		else {
		    zipBufferedFileAndMove(awsPtr->currentFileName[diskInd]);
//		    execv("/bin/bash",zipAndMoveArg);
		}
		exit(0);
	    }
	    else if(childPid<0) {
		//Something wrong
		syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
		fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
	    }
	    
	    awsPtr->fileCount[diskInd]++;
	    if(awsPtr->fileCount[diskInd]>=HK_FILES_PER_DIR) {
		awsPtr->dirCount[diskInd]++;
		if(awsPtr->dirCount[diskInd]>=HK_FILES_PER_DIR) {
		    sprintf(fullBasename,"%s/%s",diskNames[diskInd],awsPtr->relBaseName);
		    tempDir=getCurrentHkDir(fullBasename,unixTime);
		    strncpy(awsPtr->currentDirName[diskInd],tempDir,FILENAME_MAX-1);
		    makeDirectories(awsPtr->currentDirName[diskInd]);
		    if(bufferDisk[diskInd]) {
			sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentDirName[diskInd]);
			makeDirectories(bufferName);
			
		    }
		    awsPtr->dirCount[diskInd]=0;
		    free(tempDir);
		}
		
		tempDir=getCurrentHkDir(awsPtr->currentDirName[diskInd],unixTime);
		strncpy(awsPtr->currentSubDirName[diskInd],tempDir,FILENAME_MAX-1);
		makeDirectories(awsPtr->currentSubDirName[diskInd]);
		if(bufferDisk[diskInd]) {
		    sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentSubDirName[diskInd]);
		    makeDirectories(bufferName);
		}
		awsPtr->fileCount[diskInd]=0;
		free(tempDir);
	    }
	    
	    tempDir=getCurrentHkFilename(awsPtr->currentSubDirName[diskInd],
					 awsPtr->filePrefix,unixTime);
	    strncpy(awsPtr->currentFileName[diskInd],tempDir,FILENAME_MAX-1);
	    free(tempDir);
	    if(!bufferDisk[diskInd]) {
		awsPtr->currentFilePtr[diskInd]=fopen(awsPtr->currentFileName[diskInd],"wb");  
	    }
	    else {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentFileName[diskInd]);
		awsPtr->currentFilePtr[diskInd]=fopen(bufferName,"wb");
	    }
	    
	    awsPtr->writeCount[diskInd]=0;
	}
	
	awsPtr->writeCount[diskInd]++;
	if(errorCounter<100) {
	    retVal=fwrite(buffer,numBytes,1,awsPtr->currentFilePtr[diskInd]);
	    if(retVal<0) {
		errorCounter++;
		printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		       errorCounter,awsPtr->writeCount[diskInd],
		       strerror(errno),retVal);
	    }
	    retVal=fflush(awsPtr->currentFilePtr[diskInd]);  
//	printf("Here %d -- fflush RetVal %d \n",awsPtr->writeCount[diskInd],
//	       retVal);
	}
    }	        	       
    return retVal;

    
}


int closeEventFilesAndTidy(AnitaEventWriterStruct_t *awsPtr) {

    int diskInd;
    pid_t childPid;    
    int cloneMasks[DISK_TYPES]={awsPtr->bladeCloneMask,
				awsPtr->puckCloneMask,
				awsPtr->usbintCloneMask,0,0};
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(awsPtr->currentHeaderFilePtr[diskInd])) continue;
	if(awsPtr->currentHeaderFilePtr[diskInd]) {
	    fclose(awsPtr->currentHeaderFilePtr[diskInd]);
	    awsPtr->currentHeaderFilePtr[diskInd]=0;
	    childPid=fork();
	    if(childPid==0) {
		//Child
		if(!bufferDisk[diskInd]) {
		    
		    zipFileInPlaceAndClone(awsPtr->currentHeaderFileName[diskInd],cloneMasks[diskInd],diskInd);
//		    execv("/bin/gzip",gzipHeadArg);
		}
		else {
		    zipBufferedFileAndCloneAndMove(awsPtr->currentHeaderFileName[diskInd],cloneMasks[diskInd],diskInd);
		}
		exit(0);
	    }
	    else if(childPid<0) {
		//Something wrong
		syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentHeaderFileName[diskInd]);
		fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentHeaderFileName[diskInd]);
	    }
	}
	
	if(awsPtr->currentEventFilePtr[diskInd]) {
	    fclose(awsPtr->currentEventFilePtr[diskInd]);
	    awsPtr->currentEventFilePtr[diskInd]=0;
	    childPid=fork();
	    if(childPid==0) {
		//Child
		if(!bufferDisk[diskInd]) {
		    zipFileInPlaceAndClone(awsPtr->currentEventFileName[diskInd],cloneMasks[diskInd],diskInd);
		}
		else {
		    zipBufferedFileAndCloneAndMove(awsPtr->currentEventFileName[diskInd],cloneMasks[diskInd],diskInd);
		}
		exit(0);
	    }
	    else if(childPid<0) {
		//Something wrong
		syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentHeaderFileName[diskInd]);
		fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentHeaderFileName[diskInd]);
	    }
	}
	
    }
    return 0;
}


int cleverEventWrite(unsigned char *outputBuffer, int numBytes,AnitaEventHeader_t *hdPtr, AnitaEventWriterStruct_t *awsPtr)
{
    int diskInd,cloneInd;
    int retVal=0;
    static int errorCounter=0;
    int dirNum;
//    pid_t childPid;

    char tempDir[FILENAME_MAX];
    char fullBasename[FILENAME_MAX];
    char bufferName[FILENAME_MAX];

    int cloneMasks[DISK_TYPES]={awsPtr->bladeCloneMask,
				awsPtr->puckCloneMask,
				awsPtr->usbintCloneMask,0,0};
    
    if(awsPtr->gotData && (hdPtr->eventNumber>=awsPtr->fileEpoch)) {
	closeEventFilesAndTidy(awsPtr);
	awsPtr->gotData=0;
    }

//    printf("cleverEncEventWrite %lu -- %#x %d %lu\n",hdPtr->eventNumber,awsPtr->writeBitMask,awsPtr->gotData,awsPtr->fileEpoch);
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(diskBitMasks[diskInd]&awsPtr->writeBitMask)) continue;

	if(!awsPtr->currentEventFilePtr[diskInd]) {
	    //Maybe First time
	    sprintf(fullBasename,"%s/%s",diskNames[diskInd],awsPtr->relBaseName);	    	    	    

	    //Make base dir
	    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
	    sprintf(awsPtr->currentDirName[diskInd],"%s/ev%d",fullBasename,dirNum);
	    makeDirectories(awsPtr->currentDirName[diskInd]);

	    if(bufferDisk[diskInd]) {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentDirName[diskInd]);
		makeDirectories(bufferName);

	    }

	    //Make sub dir
	    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
	    sprintf(awsPtr->currentSubDirName[diskInd],"%s/ev%d",awsPtr->currentDirName[diskInd],dirNum);
	    makeDirectories(awsPtr->currentSubDirName[diskInd]);	    

	    if(bufferDisk[diskInd]) {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentSubDirName[diskInd]);
		makeDirectories(bufferName);
	    }

	    //Make files
	    dirNum=(EVENTS_PER_FILE)*(hdPtr->eventNumber/EVENTS_PER_FILE);

	    sprintf(awsPtr->currentHeaderFileName[diskInd],"%s/hd_%d.dat",
		    awsPtr->currentSubDirName[diskInd],dirNum);
	    if(!bufferDisk[diskInd]) {
		awsPtr->currentHeaderFilePtr[diskInd]=fopen(awsPtr->currentHeaderFileName[diskInd],"ab");
	    }
	    else {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentHeaderFileName[diskInd]);
		awsPtr->currentHeaderFilePtr[diskInd]=fopen(bufferName,"ab");
	    }
		    
	    if(awsPtr->currentHeaderFilePtr[diskInd]<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentHeaderFileName[diskInd]);
		}
	    }
	    sprintf(awsPtr->currentEventFileName[diskInd],"%s/%s_%d.dat",
		    awsPtr->currentSubDirName[diskInd],awsPtr->filePrefix,
		    dirNum);
	    if(!bufferDisk[diskInd]) {
		awsPtr->currentEventFilePtr[diskInd]=
		    fopen(awsPtr->currentEventFileName[diskInd],"ab");
	    }
	    else {
		sprintf(bufferName,"/tmp/buffer/%s",awsPtr->currentEventFileName[diskInd]);
		awsPtr->currentEventFilePtr[diskInd]=fopen(bufferName,"ab");
	    }

	    if(awsPtr->currentEventFilePtr[diskInd]<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentEventFileName[diskInd]);
		}
	    }
	    awsPtr->fileEpoch=dirNum+EVENTS_PER_FILE;
//	    printf("%s %s %d\n",awsPtr->currentHeaderFileName[diskInd],
//		   awsPtr->currentEventFileName[diskInd],awsPtr->fileEpoch);


	    if(cloneMasks[diskInd]>0) {
		for(cloneInd=0;cloneInd<DISK_TYPES;cloneInd++) {
		    //Need to make dirs for clones
		    sprintf(fullBasename,"%s/%s",diskNames[cloneInd],awsPtr->relBaseName);	    	    	    

		    //Make base dir
		    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
		    sprintf(tempDir,"%s/ev%d",fullBasename,dirNum);
		    makeDirectories(tempDir);

		    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
		    sprintf(tempDir,"%s/ev%d",tempDir,dirNum);
		    makeDirectories(tempDir);	    
		}
	    }
}
	if(awsPtr->currentEventFilePtr[diskInd] && awsPtr->currentHeaderFilePtr[diskInd]) {
	    
	    if(errorCounter<100 && awsPtr->currentEventFilePtr[diskInd] && awsPtr->currentHeaderFilePtr[diskInd]) {
		retVal=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,awsPtr->currentHeaderFilePtr[diskInd]);
		if(retVal<0) {
		    errorCounter++;
		    printf("Error (%d of 100) writing to file -- %s (%d)\n",
			   errorCounter,
			   strerror(errno),retVal);
		}
		else 
		    fflush(awsPtr->currentHeaderFilePtr[diskInd]);  

		retVal=fwrite(outputBuffer,numBytes,1,awsPtr->currentEventFilePtr[diskInd]);
		if(retVal<0) {
		    errorCounter++;
		    printf("Error (%d of 100) writing to file -- %s (%d)\n",
			   errorCounter,
			   strerror(errno),retVal);
		}
		else 
		    fflush(awsPtr->currentEventFilePtr[diskInd]);  
		
		awsPtr->gotData=1;
	    }
	    else continue;       	
	}
    }
    return 0;
}



int cleverIndexWriter(IndexEntry_t *indPtr, AnitaHkWriterStruct_t *awsPtr)
{
    int diskInd;
    int retVal=0;
    static int errorCounter=0;
    int dirNum;
    pid_t childPid;
    
    char fullBasename[FILENAME_MAX];
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(!(diskBitMasks[diskInd]&awsPtr->writeBitMask)) continue;

	if(!awsPtr->currentFilePtr[diskInd]) {
	    //First time
	    sprintf(fullBasename,"%s/%s",diskNames[diskInd],awsPtr->relBaseName);	    	    
	    //Make base dir
	    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(indPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
	    sprintf(awsPtr->currentDirName[diskInd],"%s/ev%d",fullBasename,dirNum);
	    makeDirectories(awsPtr->currentDirName[diskInd]);
	    awsPtr->fileCount[diskInd]=0;

	    //Make sub dir
	    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(indPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
	    sprintf(awsPtr->currentSubDirName[diskInd],"%s/ev%d",awsPtr->currentDirName[diskInd],dirNum);
	    makeDirectories(awsPtr->currentSubDirName[diskInd]);	    
	    awsPtr->dirCount[diskInd]=0;

	    //Make files
	    dirNum=(EVENTS_PER_FILE)*(indPtr->eventNumber/EVENTS_PER_FILE);
	    sprintf(awsPtr->currentFileName[diskInd],"%s/index_%d.dat",
		    awsPtr->currentSubDirName[diskInd],dirNum);
	    awsPtr->currentFilePtr[diskInd]=fopen(awsPtr->currentFileName[diskInd],"ab");
	    awsPtr->writeCount[diskInd]=0;
	    
	    if(awsPtr->currentFilePtr[diskInd]<0) {	    
		if(errorCounter<100) {
		    errorCounter++;
		    printf("Error (%d of 100) trying to open file %s\n",
			   errorCounter,awsPtr->currentFileName[diskInd]);
		}
	    }
	}
	if(awsPtr->currentFilePtr[diskInd]) {
	    if(indPtr->eventNumber%EVENTS_PER_FILE==0 ||
	       awsPtr->writeCount[diskInd]>=EVENTS_PER_FILE) {
		fclose(awsPtr->currentFilePtr[diskInd]);
		
		char *gzipArg[] = {"nice","/bin/gzip",awsPtr->currentFileName[diskInd],(char*)0};
		childPid=fork();
		if(childPid==0) {
		    //Child
		    execv("/usr/bin/nice",gzipArg);
		    exit(0);
		}
		else if(childPid<0) {
		    //Something wrong
		    syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
		    fprintf(stderr,"Couldn't zip file %s\n",awsPtr->currentFileName[diskInd]);
		}

		awsPtr->fileCount[diskInd]++;
		if(awsPtr->fileCount[diskInd]>=EVENT_FILES_PER_DIR ||
		   indPtr->eventNumber%(EVENT_FILES_PER_DIR*EVENTS_PER_FILE)==0) {
		    awsPtr->dirCount[diskInd]++;
		    if(awsPtr->dirCount[diskInd]>=EVENT_FILES_PER_DIR || 
		       
		       indPtr->eventNumber%(EVENT_FILES_PER_DIR*EVENTS_PER_FILE*EVENT_FILES_PER_DIR)==0)  {
			//Make base dir
			dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(indPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
			sprintf(awsPtr->currentDirName[diskInd],"%s/ev%d",fullBasename,dirNum);
			makeDirectories(awsPtr->currentDirName[diskInd]);
			awsPtr->dirCount[diskInd]=0;
		    }
		    
		    //Make sub dir
		    dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(indPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
		    sprintf(awsPtr->currentSubDirName[diskInd],"%s/ev%d",awsPtr->currentDirName[diskInd],dirNum);
		    makeDirectories(awsPtr->currentSubDirName[diskInd]);
		    awsPtr->fileCount[diskInd]=0;
		}
		
	    //Make files
		dirNum=(EVENTS_PER_FILE)*(indPtr->eventNumber/EVENTS_PER_FILE);
		sprintf(awsPtr->currentFileName[diskInd],"%s/index_%d.dat",
			awsPtr->currentSubDirName[diskInd],dirNum);
//	    printf("Trying to open %s\n",awsPtr->currentFileName);
		awsPtr->currentFilePtr[diskInd]=fopen(awsPtr->currentFileName[diskInd],"ab");
		if(awsPtr->currentFilePtr[diskInd]<0) {	    
		    if(errorCounter<100) {
			errorCounter++;
			printf("Error (%d of 100) trying to open file %s\n",
			       errorCounter,awsPtr->currentFileName[diskInd]);
		    }
		}
		awsPtr->writeCount[diskInd]=0;
	    }

	    
	    awsPtr->writeCount[diskInd]++;
	    if(errorCounter<100 && awsPtr->currentFilePtr[diskInd]) {
		retVal=fwrite(indPtr,sizeof(IndexEntry_t),1,awsPtr->currentFilePtr[diskInd]);
		if(retVal<0) {
		    errorCounter++;
		    printf("Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
			   errorCounter,awsPtr->writeCount[diskInd],
			   strerror(errno),retVal);
		}
		else 
		    fflush(awsPtr->currentFilePtr[diskInd]);  

	    }
	    else continue;       	
	}
    }
    return 0;
}



char *getCurrentHkDir(char *baseHkDir,unsigned long unixTime)
{
    char *newDir;   
    int ext=0;
    newDir=malloc(FILENAME_MAX);
    sprintf(newDir,"%s/sub_%lu",baseHkDir,unixTime);
    while(is_dir(newDir)) {
	ext++;
	sprintf(newDir,"%s/sub_%lu_%d",baseHkDir,unixTime,ext);
    }
    makeDirectories(newDir);
    return newDir;    
}

char *getCurrentHkFilename(char *currentDir, char *prefix, 
			   unsigned long unixTime) 
{
    char *newFile;
    int ext=0;
    FILE *fpTest;
    newFile=malloc(FILENAME_MAX);
    sprintf(newFile,"%s/%s_%lu.dat",currentDir,prefix,unixTime);
    fpTest=fopen(newFile,"r");   
    while(fpTest) {
	fclose(fpTest);
	ext++;
	sprintf(newFile,"%s/%s_%lu.dat_%d",currentDir,prefix,unixTime,ext);
	fpTest=fopen(newFile,"r");
    }
    return newFile;
}
    

int normalSingleWrite(unsigned char *buffer, char *filename, int numBytes)
{
    static int errorCounter=0;
   int numObjs;    
   FILE *outfile = fopen (filename, "wb");
   if(outfile == NULL) {
       if(errorCounter<100) {
	   syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	   fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),filename);
	   errorCounter++;
       }
       return -1;
   }   
   numObjs=fwrite(buffer,numBytes,1,outfile);
   fclose(outfile);
   return 0;
   
}

int zippedSingleWrite(unsigned char *buffer, char *filename, int numBytes) 
{
    static int errorCounter=0;
   int numObjs;    
   gzFile outfile = gzopen (filename, "wb");  
   if(outfile == NULL) { 
       if(errorCounter<100) {
	   syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	   fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),filename);
	   errorCounter++;
       }
       return -1;
   } 
   numObjs=gzwrite(outfile,buffer,numBytes);
   gzclose(outfile);  
   return 0;   
}


void makeDirectories(char *theTmpDir) 
{
    char copyDir[FILENAME_MAX];
    char newDir[FILENAME_MAX];
    char *subDir;
    int retVal;
    
    strncpy(copyDir,theTmpDir,FILENAME_MAX);

    strcpy(newDir,"");

    subDir = strtok(copyDir,"/");
    while(subDir != NULL) {
	sprintf(newDir,"%s/%s",newDir,subDir);
	retVal=is_dir(newDir);
	if(!retVal) {	
	    retVal=mkdir(newDir,0777);
// 	    printf("%s\t%s\n",theTmpDir,newDir);
/* 	    printf("Ret Val %d\n",retVal); */
	}
	subDir = strtok(NULL,"/");
    }
       

}

int makeLink(const char *theFile, const char *theLinkDir)
{
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theLinkDir,justFile);
//    printf("Linking %s to %s\n",theFile,newFile);
    return symlink(theFile,newFile);

}


int moveFile(const char *theFile, const char *theDir)
{

    int retVal;
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    sprintf(newFile,"%s/%s",theDir,justFile);
    retVal=rename(theFile,newFile);
//    retVal+=unlink(theFile);
    return retVal;
    
}


char *readFile(const char *theFile, unsigned long *numBytes)
{
    static int errorCounter=0;
    char *buffer;
    //Open the input file
    FILE *fin = fopen(theFile,"r");
    if(!fin) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error reading file %s -- Error %s\n",
		   theFile,strerror(errno));
	    fprintf(stderr,"Error reading file %s-- Error %s\n",
		    theFile,strerror(errno));
	    errorCounter++;
	}
	return NULL;
    }
		   
    fseek(fin,0,SEEK_END);
    (*numBytes)=ftell(fin);
    fseek(fin,SEEK_SET,0);
    buffer=malloc((*numBytes));
    fread(buffer,1,(*numBytes),fin);
    fclose(fin);
    return buffer;
}

int copyFile(const char *theFile, const char *theDir)
{
    static int errorCounter=0;
    //Only works on relative small files (will write a better version)
    char *justFile=basename((char *)theFile);
    char newFile[FILENAME_MAX];
    unsigned long numBytes=0;
    char *buffer=readFile(theFile,&numBytes);

    //Open the output file
    sprintf(newFile,"%s/%s",theDir,justFile);
    FILE *fout = fopen(newFile,"w");
    if(!fout) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Couldn't open %s for copy of %s -- Error %s\n",
		   newFile,theFile,strerror(errno));
	    fprintf(stderr,"Couldn't open %s for copy of %s -- Error %s\n",
		    newFile,theFile,strerror(errno));
	    errorCounter++;
	}
	return -1;
    }
    fwrite(buffer,1,numBytes,fout);
    fclose(fout);
    free(buffer);
    return 0;
}


int copyFileToFile(const char *theFile, const char *newFile)
{
    static int errorCounter=0;
    //Only works on relative small files (will write a better version)
    unsigned long numBytes=0;
    char *buffer=readFile(theFile,&numBytes);

    //Open the output file
    FILE *fout = fopen(newFile,"w");
    if(!fout) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Couldn't open %s for copy of %s -- Error %s\n",
		   newFile,theFile,strerror(errno));
	    fprintf(stderr,"Couldn't open %s for copy of %s -- Error %s\n",
		    newFile,theFile,strerror(errno));
	    errorCounter++;
	}
	return -1;
    }
    fwrite(buffer,1,numBytes,fout);
    fclose(fout);
    free(buffer);
    return 0;
}


int removeFile(const char *theFile)
{
    static int errorCounter=0;
    int retVal=unlink(theFile);
    if(retVal!=0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) removing %s:\t%s",errorCounter,theFile,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) removing %s:\t%s\n",errorCounter,theFile,strerror(errno));
	    errorCounter++;
	}
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
    static int errorCounter=0;
    int n = scandir(theEventLinkDir, namelist, filterOnDats, versionsort);
    if (n < 0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"scandir %s: %s",theEventLinkDir,strerror(errno));
	    fprintf(stderr,"scandir %s: %s\n",theEventLinkDir,strerror(errno));
	    errorCounter++;
	}
	    
    }	
 /*    for(count=0;count<n;count++)  */
/* 	printf("%s\n",(*namelist)[count]->d_name); */
    return n;	    
}


unsigned long getDiskSpace(char *dirName) {
    struct statvfs diskStat;
    static int errorCounter=0;
    int retVal=statvfs(dirName,&diskStat); 
    if(retVal<0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Unable to get disk space %s: %s",dirName,strerror(errno));  
	    fprintf(stderr,"Unable to get disk space %s: %s\n",dirName,strerror(errno));            
	}
//	if(printToScreen) fprintf(stderr,"Unable to get disk space %s: %s\n",dirName,strerror(errno));       
	return 0;
    }    
    unsigned long megabytesAvailable=(diskStat.f_bavail/1024)*(diskStat.f_bsize/1024);
    return megabytesAvailable;
//    printf("%lu\n",megabytesAvailable);
//    if(printToScreen) {
	printf("Dir: %s\n",dirName);
	printf("Ret Val: %d\n",retVal);
	printf("MegaBytes Available: %lu\n",megabytesAvailable);
	printf("Available Blocks: %ld\n",diskStat.f_bavail);
	printf("Free Blocks: %ld\n",diskStat.f_bfree);
	printf("Total Blocks: %ld\n",diskStat.f_blocks);
	printf("Block Size: %lu\n",diskStat.f_bsize);
//	printf("Free File Nodes: %ld\n",diskStat.f_ffree);
//	printf("Total File Nodes: %ld\n",diskStat.f_files);
//	printf("Type Of Info: %d\n",diskStat.f_type);
//    printf("File System Id: %d\n",diskStat.f_fsid);
//    }
    return megabytesAvailable;
}

unsigned short countFilesInDir(char *dirName) {
    static int errorCounter=0;
    unsigned short fileCount=0;
    struct dirent *subPtr;
    DIR *TheDir = opendir(dirName);
    while (TheDir) {
	errno = 0;
	if ((subPtr = readdir(TheDir)) != NULL) {
	    if(subPtr->d_name[0]!='.') {
		fileCount++;
//		printf("File Number %d\t%s\n",fileCount,subPtr->d_name);
	    }
	} 
	else {
	    if (errno == 0) {
		closedir(TheDir);
		break;
	    }
	    else {
		closedir(TheDir);
		if(errorCounter<100) {
		    syslog(LOG_ERR,"Error reading directory %s:\t%s",
			   dirName,strerror(errno));
		    fprintf(stderr,"Error reading directory %s:\t%s\n",
			    dirName,strerror(errno));
		}
		    
		return -1;
	    }
	}
    }    
//    if(printToScreen && verbosity>1) 
//	printf("%s contains %d thingies\n",dirName,fileCount);
    return fileCount;	
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
    int numBytes=genericReadOfFile((unsigned char*)theStruct,filename,sizeof(CalibStruct_t));
    if(numBytes==sizeof(CalibStruct_t)) return 0;
    return numBytes;

}

int fillUsefulPedStruct(PedestalStruct_t *pedPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)pedPtr,filename,sizeof(PedestalStruct_t));
    if(numBytes==sizeof(PedestalStruct_t)) return 0;
    return numBytes;
}


int fillLabChipPedstruct(FullLabChipPedStruct_t *pedPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)pedPtr,filename,sizeof(FullLabChipPedStruct_t));
    if(numBytes==sizeof(FullLabChipPedStruct_t)) return 0;
    return numBytes;
}


int fillCommand(CommandStruct_t *cmdPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)cmdPtr,filename,sizeof(CommandStruct_t));
    if(numBytes==sizeof(CommandStruct_t)) return 0;
    return numBytes;
}


int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)theEventHdPtr,filename,
				   sizeof(AnitaEventHeader_t));
    if(numBytes==sizeof(AnitaEventHeader_t)) return 0;
    return numBytes;  
}


int fillHeaderWithThisEvent(AnitaEventHeader_t *hdPtr, char *filename,
			    unsigned long eventNumber)
{
    static int errorCounter=0;
    int numBytes;  
    int readNum=0;
//    int firstEvent=0;
//    int error=0;  
    
    gzFile infile = gzopen (filename, "rb");    
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),filename);
	    errorCounter++;
	}
	return -1;
    }   

    //Read first event
    numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
    if(numBytes!=sizeof(AnitaEventHeader_t)) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytes,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytes,filename);
	    errorCounter++;
	}	
	return -2;
    }

    if(abs(((int)eventNumber-(int)hdPtr->eventNumber))>1000) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
	    fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
	    errorCounter++;
	}	
	return -3;
    }

//    printf("Looking for %lu (first read %lu)\n",eventNumber,hdPtr->eventNumber);
	
    readNum=(eventNumber-hdPtr->eventNumber);
    if(readNum) {	    
	if(readNum>1) {
	    gzseek(infile,sizeof(AnitaEventHeader_t)*(readNum-1),SEEK_CUR);
//	    printf("Sought %d bytes am now at %d\n",sizeof(AnitaEventHeader_t)*(readNum-1),(int)gztell(infile));
	}

	numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
//	printf("Sought %d bytes got event %lu\n",sizeof(AnitaEventHeader_t)*(readNum-1),hdPtr->eventNumber);

	if(numBytes!=sizeof(AnitaEventHeader_t)) {
	    gzclose(infile);
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytes,filename);
		fprintf(stderr,"Only read %d bytes from %s\n",numBytes,filename);
		errorCounter++;
	    }	
	    return -4;
	}
	
	if(eventNumber!=hdPtr->eventNumber) {
	    //File not ordered numerically
	    gzseek(infile,sizeof(AnitaEventHeader_t),SEEK_SET);
	    do {
		numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
		if(numBytes!=sizeof(AnitaEventHeader_t)) {
		    gzclose(infile);
		    if(errorCounter<100) {
			syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytes,filename);
			fprintf(stderr,"Only read %d bytes from %s\n",numBytes,filename);
			errorCounter++;
		    }	
		    return -4;
		}
	    } while(eventNumber!=hdPtr->eventNumber);
		
	}
	    

    }
    gzclose(infile);
    return 0;
}


int readSingleEncodedEvent(unsigned char *buffer, char *filename) 
{
    EncodedEventWrapper_t *encEvWrap = (EncodedEventWrapper_t*) &buffer[0];
    static int errorCounter=0;
    int count=0;
    int numBytesRead;  
    int numBytesToRead;
    
    gzFile infile = gzopen (filename, "rb");    
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),filename);
	    errorCounter++;
	}
	return -1;
    }  


    //Read first header
    numBytesRead=gzread(infile,encEvWrap,sizeof(EncodedEventWrapper_t));
    if(numBytesRead!=sizeof(EncodedEventWrapper_t)) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
	    errorCounter++;
	}	
	return -2;
    } 
    //Now ready to read event
    count=sizeof(EncodedEventWrapper_t);
    numBytesToRead=encEvWrap->numBytes;	
    numBytesRead=gzread(infile,&buffer[count],
			numBytesToRead);
    if(numBytesRead!=numBytesToRead) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
	    errorCounter++;
	}	
	return -6;
    }
    count+=numBytesRead;
    gzclose(infile);
    return count;

}

int readEncodedEventFromFile(unsigned char *buffer, char *filename,
			     unsigned long eventNumber) 
{
    EncodedEventWrapper_t *encEvWrap = (EncodedEventWrapper_t*) &buffer[0];

    static int errorCounter=0;
    int count=0;
    int numBytesRead;  
    int numBytesToSeek;
    int numBytesToRead;
    
    gzFile infile = gzopen (filename, "rb");    
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
	    fprintf(stderr,"fopen: %s ---  %s\n",strerror(errno),filename);
	    errorCounter++;
	}
	return -1;
    }   

    //Read event wrapper
    numBytesRead=gzread(infile,encEvWrap,sizeof(EncodedEventWrapper_t));
    if(numBytesRead!=sizeof(EncodedEventWrapper_t)) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
	    errorCounter++;
	}	
	return -2;
    }

//    printf("First read %lu (want %lu)\n\t%s\n",encEvWrap->eventNumber,eventNumber,filename);

    if(abs(((int)eventNumber-(int)encEvWrap->eventNumber))>1000) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
	    fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
	    errorCounter++;
	}	
	gzclose(infile);
	return -3;
    }

    while(encEvWrap->eventNumber!=eventNumber) {
	numBytesToSeek=encEvWrap->numBytes;	
	gzseek(infile,numBytesToSeek,SEEK_CUR);
	numBytesRead=gzread(infile,encEvWrap,sizeof(EncodedEventWrapper_t));
	if(numBytesRead!=sizeof(EncodedEventWrapper_t)) {
	    gzclose(infile);
	    if(errorCounter<100) {
		syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
		fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
		errorCounter++;
	    }	
	    return -4;
	}
		
	if(abs(((int)eventNumber-(int)encEvWrap->eventNumber))>1000) {

	    if(errorCounter<100) {
		syslog(LOG_ERR,"Events %lu and %lu can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
		fprintf(stderr,"Events %lu and %lu can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
		errorCounter++;
	    }	
	    gzclose(infile);
	    return -5;
	}
    }


    //Now ready to read event
    count=sizeof(EncodedEventWrapper_t);
    numBytesToRead=encEvWrap->numBytes;
    numBytesRead=gzread(infile,&buffer[count],
			numBytesToRead);
    if(numBytesRead!=numBytesToRead) {
	gzclose(infile);
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Only read %d bytes from %s\n",numBytesRead,filename);
	    fprintf(stderr,"Only read %d bytes from %s\n",numBytesRead,filename);
	    errorCounter++;
	}	
	return -6;
    }
    count+=numBytesRead;
    gzclose(infile);
    return count;
}



int fillBody(AnitaEventBody_t *theEventBodyPtr, char *filename)
{ 
    int numBytes=genericReadOfFile((unsigned char*)theEventBodyPtr,filename,
				   sizeof(AnitaEventBody_t));
    if(numBytes==sizeof(AnitaEventBody_t)) return 0;
    return numBytes;  
}


int fillPedSubbedBody(PedSubbedEventBody_t *theEventBodyPtr, char *filename)
{ 
    int numBytes=genericReadOfFile((unsigned char*)theEventBodyPtr,filename,
				   sizeof(PedSubbedEventBody_t));
    if(numBytes==sizeof(PedSubbedEventBody_t)) return 0;
    return numBytes;  
}



int fillGpsStruct(GpsSubTime_t *tttPtr, char *filename)
{
    int numBytes=genericReadOfFile((unsigned char*)tttPtr,filename,
				   sizeof(GpsSubTime_t));
    if(numBytes==sizeof(GpsSubTime_t)) return 0;
    return numBytes;  
}

int genericReadOfFile(unsigned char * buffer, char *filename, int maxBytes) 
/*!
  Reads all the bytes in a file and returns the number of bytes read
*/
{
    static int errorCounter=0;
    int numBytes;
/* #ifdef NO_ZLIB */
/*     int fd; */
/*     fd = open(filename,O_RDONLY); */
/*     if(fd == 0) { */
/* 	if(errorCounter<100) { */
/* 	    syslog(LOG_ERR,"Error (%d of 100) opening file %s -- %s\n", */
/* 		   errorCounter,filename,strerror(errno)); */
/* 	    fprintf(stderr,"Error (%d of 100) opening file %s -- %s\n", */
/* 		   errorCounter,filename,strerror(errno)); */
/* 	    errorCounter++; */
/* 	} */
/* 	return -1; */
/*     } */
/*     numBytes=read(fd,buffer,maxBytes); */
/*     if(numBytes<=0) { */
/* 	if(errorCounter<100) { */
/* 	    syslog(LOG_ERR,"Error (%d of 100) reading file %s -- %s\n", */
/* 		   errorCounter,filename,strerror(errno)); */
/* 	    fprintf(stderr,"Error (%d of 100) reading file %s -- %s\n", */
/* 		   errorCounter,filename,strerror(errno)); */
/* 	    errorCounter++; */
/* 	} */
/* 	close (fd); */
/* 	return -2; */
/*     } */
/*     close(fd); */
/*     return numBytes; */
/* #else */
    gzFile infile = gzopen(filename,"rb");
    if(infile == NULL) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) opening file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	return -1;
    }       
    numBytes=gzread(infile,buffer,maxBytes);
    if(numBytes<=0) {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    fprintf(stderr,"Error (%d of 100) reading file %s -- %s\n",
		   errorCounter,filename,strerror(errno));
	    errorCounter++;
	}
	gzclose(infile);
	return -2;
    }
    gzclose(infile);
    return numBytes;
/* #endif */
}


int writeHeader(AnitaEventHeader_t *hdPtr, char *filename)
/* Writes the header pointed to by hdPtr to filename */
{
    return normalSingleWrite((unsigned char*)hdPtr,filename,sizeof(AnitaEventHeader_t));
}

int writeBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
#else
    return normalSingleWrite((unsigned char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
    return zippedSingleWrite((unsigned char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
#endif
}

int writePedSubbedBody(PedSubbedEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)bodyPtr,filename,sizeof(PedSubbedEventBody_t));
#else
    return normalSingleWrite((unsigned char*)bodyPtr,filename,sizeof(PedSubbedEventBody_t));
    return zippedSingleWrite((unsigned char*)bodyPtr,filename,sizeof(PedSubbedEventBody_t));
#endif
}



int writeZippedBody(AnitaEventBody_t *bodyPtr, char *filename)
/* Writes the body pointed to by bodyPtr to filename */
{
    return zippedSingleWrite((unsigned char*)bodyPtr,filename,sizeof(AnitaEventBody_t));
}

int writeWaveformPacket(RawWaveformPacket_t *wavePtr, char *filename)
/* Writes the waveform pointed to by wavePtr to filename */
{
    
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)wavePtr,filename,sizeof(RawWaveformPacket_t));
#else
    return zippedSingleWrite((unsigned char*)wavePtr,filename,sizeof(RawWaveformPacket_t));
#endif
}


int writeSurfPacket(RawSurfPacket_t *surfPtr, char *filename)
/* Writes the surf packet pointed to by surfPtr to filename */
{
  
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)surfPtr,filename,sizeof(RawSurfPacket_t));
#else
    return zippedSingleWrite((unsigned char*)surfPtr,filename,sizeof(RawSurfPacket_t));
#endif
}


int writeSurfHk(FullSurfHkStruct_t *surfPtr, char *filename)
/* Writes the surf hk packet pointed to by surfPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)surfPtr,filename,sizeof(FullSurfHkStruct_t));
#else
    return zippedSingleWrite((unsigned char*)surfPtr,filename,sizeof(FullSurfHkStruct_t));
#endif

}

int writeGpsPat(GpsAdu5PatStruct_t *patPtr, char *filename)
/* Writes the pat pointed to by patPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)patPtr,filename,sizeof(GpsAdu5PatStruct_t));
#else
    return zippedSingleWrite((unsigned char*)patPtr,filename,sizeof(GpsAdu5PatStruct_t));
#endif

}


int writeGpsVtg(GpsAdu5VtgStruct_t *vtgPtr, char *filename)
/* Writes the vtg pointed to by vtgPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)vtgPtr,filename,sizeof(GpsAdu5VtgStruct_t));
#else
    return zippedSingleWrite((unsigned char*)vtgPtr,filename,sizeof(GpsAdu5VtgStruct_t));
#endif

}

int writeGpsPos(GpsG12PosStruct_t *posPtr, char *filename)
/* Writes the pos pointed to by posPtr to filename */
{


#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)posPtr,filename,sizeof(GpsG12PosStruct_t));
#else
    return zippedSingleWrite((unsigned char*)posPtr,filename,sizeof(GpsG12PosStruct_t));
#endif

}

int writeGpsAdu5Sat(GpsAdu5SatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{   

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)satPtr,filename,sizeof(GpsAdu5SatStruct_t));
#else
    return zippedSingleWrite((unsigned char*)satPtr,filename,sizeof(GpsAdu5SatStruct_t));
#endif

}


int writeGpsG12Sat(GpsG12SatStruct_t *satPtr, char *filename)
/* Writes the sat pointed to by satPtr to filename */
{   

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)satPtr,filename,sizeof(GpsG12SatStruct_t));
#else
    return zippedSingleWrite((unsigned char*)satPtr,filename,sizeof(GpsG12SatStruct_t));
#endif

}


int writeGpsTtt(GpsSubTime_t *tttPtr, char *filename)
/* Writes the ttt pointed to by tttPtr to filename */
{     
    return normalSingleWrite((unsigned char*)tttPtr,filename,sizeof(GpsSubTime_t));
}

int writeHk(HkDataStruct_t *hkPtr, char *filename)
/* Writes the hk pointed to by hkPtr to filename */
{     
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)hkPtr,filename,sizeof(HkDataStruct_t));
#else
    return zippedSingleWrite((unsigned char*)hkPtr,filename,sizeof(HkDataStruct_t));
#endif

}


int writeCmdEcho(CommandEcho_t *echoPtr, char *filename)
/* Writes the echo pointed to by echoPtr to filename */
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)echoPtr,filename,sizeof(CommandEcho_t));
#else
    return zippedSingleWrite((unsigned char*)echoPtr,filename,sizeof(CommandEcho_t));
#endif
}


int writeCmd(CommandStruct_t *cmdPtr, char *filename)
/* Writes the cmd pointed to by cmdPtr to filename */
{   
    return normalSingleWrite((unsigned char*)cmdPtr,filename,sizeof(CommandStruct_t));
}

int writeCalibStatus(CalibStruct_t *calibPtr, char *filename)
/* Writes the cmd pointed to by cmdPtr to filename */
{   
    return normalSingleWrite((unsigned char*)calibPtr,filename,sizeof(CalibStruct_t));
}

int writePedCalcStruct(PedCalcStruct_t *pedPtr, char *filename) 
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)pedPtr,filename,sizeof(PedCalcStruct_t));
#else
    return zippedSingleWrite((unsigned char*)pedPtr,filename,sizeof(PedCalcStruct_t));
#endif

}


int writeUsefulPedStruct(PedestalStruct_t *pedPtr, char *filename)
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)pedPtr,filename,sizeof(PedestalStruct_t));
#else
    return zippedSingleWrite((unsigned char*)pedPtr,filename,sizeof(PedestalStruct_t));
#endif

}


int writeLabChipPedStruct(FullLabChipPedStruct_t *pedPtr, char *filename) 
{
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)pedPtr,filename,sizeof(FullLabChipPedStruct_t));
#else
    return zippedSingleWrite((unsigned char*)pedPtr,filename,sizeof(FullLabChipPedStruct_t));
#endif

}


int writeMonitor(MonitorStruct_t *monitorPtr, char *filename)
/* Writes the monitor object pointed to by monitorPtr to filename */
{   
#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)monitorPtr,filename,sizeof(MonitorStruct_t));
#else
    return zippedSingleWrite((unsigned char*)monitorPtr,filename,sizeof(MonitorStruct_t));
#endif

}



int writeTurfRate(TurfRateStruct_t *turfPtr, char *filename)
/* Writes the TurfRateStruct_t object pointed to by turfPtr to filename */
{

#ifdef NO_ZLIB
    return normalSingleWrite((unsigned char*)turfPtr,filename,sizeof(TurfRateStruct_t));
#else
    return zippedSingleWrite((unsigned char*)turfPtr,filename,sizeof(TurfRateStruct_t));
#endif

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


unsigned long simpleLongCrc(unsigned long *p, unsigned long n)
{
    unsigned long sum = 0;
    unsigned long i;
    for (i=0L; i<n; i++) {
//
	sum += *p++;
    }
//    printf("%lu %lu\n",*p,sum);
    return ((0xffffffff - sum) + 1);
}




void fillGenericHeader(void *thePtr, PacketCode_t code, unsigned short numBytes) {
    unsigned long longBytes=(numBytes-sizeof(GenericHeader_t))/4;
    GenericHeader_t *gHdr= (GenericHeader_t*)thePtr;
    unsigned long *dataPtr=(unsigned long*) (thePtr+sizeof(GenericHeader_t));
    gHdr->code=code;
    gHdr->numBytes=numBytes;
    gHdr->feByte=0xfe;
    switch(code) {
	case PACKET_BD: gHdr->verId=VER_EVENT_BODY; break;
	case PACKET_HD: gHdr->verId=VER_EVENT_HEADER; break;
	case PACKET_HD_SLAC: gHdr->verId=SLAC_VER_EVENT_HEADER; break;
	case PACKET_WV: gHdr->verId=VER_WAVE_PACKET; break;
	case PACKET_SURF: gHdr->verId=VER_SURF_PACKET; break;
	case PACKET_SURF_HK: gHdr->verId=VER_SURF_HK; break;
	case PACKET_TURF_RATE: gHdr->verId=VER_TURF_RATE; break;
	case PACKET_LAB_PED: gHdr->verId=VER_LAB_PED; break;
	case PACKET_FULL_PED: gHdr->verId=VER_FULL_PED; break;
	case PACKET_ENC_WV_PEDSUB: gHdr->verId=VER_ENC_WAVE_PACKET; break;
	case PACKET_ENC_SURF: gHdr->verId=VER_ENC_SURF_PACKET; break;
	case PACKET_PED_SUBBED_EVENT: gHdr->verId=VER_PEDSUBBED_EVENT_BODY; break;
	case PACKET_GPS_ADU5_PAT: gHdr->verId=VER_ADU5_PAT; break;
	case PACKET_GPS_ADU5_SAT: gHdr->verId=VER_ADU5_SAT; break;
	case PACKET_GPS_ADU5_VTG: gHdr->verId=VER_ADU5_VTG; break;
	case PACKET_GPS_G12_POS: gHdr->verId=VER_G12_POS; break;
	case PACKET_GPS_G12_SAT: gHdr->verId=VER_G12_SAT; break;
	case PACKET_HKD: gHdr->verId=VER_HK_FULL; break;
	case PACKET_CMD_ECHO: gHdr->verId=VER_CMD_ECHO; break;
	case PACKET_MONITOR: gHdr->verId=VER_MONITOR; break;
	case PACKET_SLOW1: gHdr->verId=VER_SLOW_1; break;
	case PACKET_SLOW2: gHdr->verId=VER_SLOW_2; break;
	case PACKET_ZIPPED_FILE: gHdr->verId=VER_ZIPPED_FILE; break;
	case PACKET_ZIPPED_PACKET: gHdr->verId=VER_ZIPPED_PACKET; break;
	default: 
	    gHdr->verId=0; break;
    }
    gHdr->checksum=simpleLongCrc(dataPtr,longBytes);
//    printf("Long bytes %lu\t checksum %lu\n",longBytes,gHdr->checksum);
}

#define PKT_E_CHECKSUM 0x1
#define PKT_E_CODE 0x2
#define PKT_E_FEBYTE 0x4
#define PKT_E_SIZE 0x8

int checkPacket(void *thePtr)
//0 means no errors
//1 means checksum mismatch
//2 means unknown packet code
//4 means missing feByte missing
//8 means packet size mismatch
{
    int retVal=0,packetSize=0;
    GenericHeader_t *gHdr= (GenericHeader_t*)thePtr;
    unsigned long longBytes=(gHdr->numBytes-sizeof(GenericHeader_t))/4;
    unsigned long *dataPtr=(unsigned long*) (thePtr+sizeof(GenericHeader_t)); 
    unsigned long checksum=simpleLongCrc(dataPtr,longBytes);
    PacketCode_t code=gHdr->code;
    if(checksum!=gHdr->checksum) {
	printf("Checksum Mismatch (possibly %s (%d)) (%lu bytes) %lu -- %lu \n",
	       packetCodeAsString(code),code,longBytes,checksum,gHdr->checksum);	
	retVal+=PKT_E_CHECKSUM;
    }
    switch(code) {
	case PACKET_BD: packetSize=sizeof(AnitaEventBody_t); break;	    
	case PACKET_HD: packetSize=sizeof(AnitaEventHeader_t); break;	    
	case PACKET_WV: packetSize=sizeof(RawWaveformPacket_t); break;
	case PACKET_PEDSUB_WV: packetSize=sizeof(PedSubbedWaveformPacket_t); break;
	case PACKET_PEDSUB_SURF: packetSize=sizeof(PedSubbedSurfPacket_t); break;
	case PACKET_SURF: packetSize=sizeof(RawSurfPacket_t); break;
	case PACKET_SURF_HK: packetSize=sizeof(FullSurfHkStruct_t); break;
	case PACKET_TURF_RATE: packetSize=sizeof(TurfRateStruct_t); break;
	case PACKET_ENC_WV_PEDSUB: break;
	case PACKET_ENC_SURF: break;
	case PACKET_ENC_SURF_PEDSUB: break;
	case PACKET_PED_SUBBED_EVENT: packetSize=sizeof(PedSubbedEventBody_t); break;
	case PACKET_LAB_PED: packetSize=sizeof(FullLabChipPedStruct_t); break;
	case PACKET_FULL_PED: packetSize=sizeof(FullPedStruct_t); break;
	case PACKET_GPS_ADU5_PAT: packetSize=sizeof(GpsAdu5PatStruct_t); break;
	case PACKET_GPS_ADU5_SAT: packetSize=sizeof(GpsAdu5SatStruct_t); break;
	case PACKET_GPS_ADU5_VTG: packetSize=sizeof(GpsAdu5VtgStruct_t); break;
	case PACKET_GPS_G12_POS: packetSize=sizeof(GpsG12PosStruct_t); break;
	case PACKET_GPS_G12_SAT: packetSize=sizeof(GpsG12SatStruct_t); break;
	case PACKET_HKD: packetSize=sizeof(HkDataStruct_t); break;
	case PACKET_CMD_ECHO: packetSize=sizeof(CommandEcho_t); break;
	case PACKET_MONITOR: packetSize=sizeof(MonitorStruct_t); break;
	case PACKET_WAKEUP_LOS: packetSize=WAKEUP_LOS_BUFFER_SIZE; break;
	case PACKET_WAKEUP_HIGHRATE: packetSize=WAKEUP_TDRSS_BUFFER_SIZE; 
	    break;
	case PACKET_WAKEUP_COMM1: packetSize=WAKEUP_LOW_RATE_BUFFER_SIZE; 
	    break;
	case PACKET_WAKEUP_COMM2: packetSize=WAKEUP_LOW_RATE_BUFFER_SIZE; 
	    break;
	case PACKET_SLOW1: packetSize=sizeof(SlowRateType1_t); 
	    break;
	case PACKET_SLOW2: packetSize=sizeof(SlowRateType1_t); 
	    break;
	case PACKET_ZIPPED_PACKET: break;
	case PACKET_ZIPPED_FILE: break;
	default: 
	    retVal+=PKT_E_CODE; break;
    }
    if(packetSize && (packetSize!=gHdr->numBytes))
	retVal+=PKT_E_SIZE;
    if(gHdr->feByte!=0xfe) retVal+=PKT_E_FEBYTE;
    return retVal;

}

char *packetCodeAsString(PacketCode_t code) {
    char* string ;
    switch(code) {
	case PACKET_BD: string="AnitaEventBody_t"; break;	    
	case PACKET_HD: string="AnitaEventHeader_t"; break;	    
	case PACKET_WV: string="RawWaveformPacket_t"; break;
	case PACKET_PEDSUB_WV: string="PedSubbedWaveformPacket_t"; break;
	case PACKET_SURF: string="RawSurfPacket_t"; break;
	case PACKET_PEDSUB_SURF: string="PedSubbedSurfPacket_t"; break;
	case PACKET_SURF_HK: string="FullSurfHkStruct_t"; break;
	case PACKET_TURF_RATE: string="TurfRateStruct_t"; break;
	case PACKET_LAB_PED: string="FullLabChipPedStruct_t"; break;
	case PACKET_FULL_PED: string="FullPedStruct_t"; break;
	case PACKET_ENC_WV_PEDSUB: string="EncodedPedSubbedChannelPacketHeader_t"; break;
	case PACKET_ENC_SURF: string="EncodedSurfPacketHeader_t"; break;
	case PACKET_ENC_SURF_PEDSUB: string="EncodedPedSubbedSurfPacketHeader_t"; break;
	case PACKET_GPS_ADU5_PAT: string="GpsAdu5PatStruct_t"; break;
	case PACKET_GPS_ADU5_SAT: string="GpsAdu5SatStruct_t"; break;
	case PACKET_GPS_ADU5_VTG: string="GpsAdu5VtgStruct_t"; break;
	case PACKET_GPS_G12_POS: string="GpsG12PosStruct_t"; break;
	case PACKET_GPS_G12_SAT: string="GpsG12SatStruct_t"; break;
	case PACKET_HKD: string="HkDataStruct_t"; break;
	case PACKET_CMD_ECHO: string="CommandEcho_t"; break;
	case PACKET_MONITOR: string="MonitorStruct_t"; break;
	case PACKET_WAKEUP_LOS: string="LosWakeUpPacket"; break;
	case PACKET_WAKEUP_HIGHRATE: string="TdrssWakeUpPacket"; break;
	case PACKET_WAKEUP_COMM1: string="Comm1WakeUpPacket"; break;
	case PACKET_WAKEUP_COMM2: string="Comm2WakeUpPacket"; break;
	case PACKET_SLOW1: string="SlowRateType1_t"; break;
	case PACKET_SLOW2: string="SlowRateType1_t"; break;
	case PACKET_ZIPPED_PACKET: string="ZippedPacket_t"; break;
	case PACKET_ZIPPED_FILE: string="ZippedFile_t"; break;

	default: 
	    string="Unknown Packet Code"; break;
    }
    return string;
}


int writeCommandAndLink(CommandStruct_t *theCmd) {
    time_t unixTime;
    time(&unixTime);
    int retVal=0;
    char filename[FILENAME_MAX];

    sprintf(filename,"%s/cmd_%ld.dat",CMDD_COMMAND_DIR,unixTime);
    {
	FILE *pFile;
	int fileTag=1;
	pFile = fopen(filename,"rb");
	while(pFile!=NULL) {
	    fclose(pFile);
	    sprintf(filename,"%s/cmd_%ld_%d.dat",CMDD_COMMAND_DIR,unixTime,fileTag);
	    pFile=fopen(filename,"rb");
	}
    }
    writeCmd(theCmd,filename);
    makeLink(filename,CMDD_COMMAND_LINK_DIR);    
    return retVal;
}

int touchFile(char *filename) {
    FILE *fp = fopen(filename,"w");
    if(!fp) {
	//Add error checking here
	return -1;
    }
    fclose(fp);
    return 0;
}

int checkFileExists(char *filename) {
    struct stat fileStat ;
    if(stat (filename, &fileStat) != 0) {
	//File doesn't exist
	return 0;
    }
    return 1;
}

int zipBuffer(char *input, char *output, unsigned long inputBytes, unsigned long *outputBytes)
{
    static int errorCounter=0;
    int retVal=compress((unsigned char*)output,outputBytes,(unsigned char*)input,inputBytes);
    if(retVal==Z_OK)
	return 0;
    else {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"zlib compress returned %d  (%d of 100)\n",retVal,errorCounter);
	    fprintf(stderr,"zlib compress returned %d  (%d of 100)\n",retVal,errorCounter);
	    errorCounter++;
	}
	return -1;
    }	
}


int unzipBuffer(char *input, char *output, unsigned long inputBytes, unsigned long *outputBytes)
{
    static int errorCounter=0;
    int retVal=uncompress((unsigned char*)output,outputBytes,(unsigned char*)input,inputBytes);
    if(retVal==Z_OK)
	return 0;
    else {
	if(errorCounter<100) {
	    syslog(LOG_ERR,"zlib compress returned %d  (%d of 100)\n",retVal,errorCounter);
	    fprintf(stderr,"zlib compress returned %d  (%d of 100)\n",retVal,errorCounter);
	    errorCounter++;
	}
	return -1;
    }	
}


int zipFileInPlace(char *filename) {
    int retVal;
    char outputFilename[FILENAME_MAX];
    unsigned long numBytesIn=0;
    char *buffer=readFile(filename,&numBytesIn);
    if(!buffer || !numBytesIn) {
	free(buffer);
	return -1;
    }    
    sprintf(outputFilename,"%s.gz",filename);
    retVal=zippedSingleWrite((unsigned char*)buffer,outputFilename,numBytesIn);
    free(buffer);
    unlink(filename);
    return retVal;
}


int zipFileInPlaceAndClone(char *filename,unsigned int cloneMask,int baseInd) {
    //Doesn't clone yet
    if(cloneMask==0) return zipFileInPlace(filename);
    int retVal;
    char outputFilename[FILENAME_MAX];
    char cloneFilename[FILENAME_MAX];
    int diskInd;
    unsigned long numBytesIn=0;
    char *buffer=readFile(filename,&numBytesIn);
    if(!buffer || !numBytesIn) {
	free(buffer);
	return -1;
    }    
    sprintf(outputFilename,"%s.gz",filename);
    retVal=zippedSingleWrite((unsigned char*)buffer,outputFilename,numBytesIn);
    char *relPath=strstr(outputFilename,"anita");
    if(!relPath) return -1;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
	sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
	copyFileToFile(outputFilename,cloneFilename);	
    }

    free(buffer);
    unlink(filename);
    return retVal;
}


int zipBufferedFileAndMove(char *filename) {
//    printf("zipBufferedFileAndMove %s\n",filename);
    int retVal;
    char bufferFilename[FILENAME_MAX];
    char bufferZippedFilename[FILENAME_MAX];
    char outputFilename[FILENAME_MAX];
    unsigned long numBytesIn=0;
    sprintf(bufferFilename,"/tmp/buffer/%s",filename);
    char *buffer=readFile(bufferFilename,&numBytesIn);
    if(!buffer || !numBytesIn) {
	fprintf(stderr,"Problem reading file %s\n",bufferFilename);
	free(buffer);
	return -1;
    }    
    sprintf(bufferZippedFilename,"%s.gz",bufferFilename);
    sprintf(outputFilename,"%s.gz",filename);
    retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
    printf("move %s %s\n",bufferZippedFilename,outputFilename);
    rename(bufferZippedFilename,outputFilename);
    free(buffer);
//    unlink(bufferZippedFilename);
    unlink(bufferFilename);
    return retVal;
}

int zipBufferedFileAndCloneAndMove(char *filename,unsigned int cloneMask,int baseInd) {
//    printf("zipBufferedFileAndCloneAndMove %s, %u %d\n",filename,cloneMask,baseInd);
    if(cloneMask==0) return zipBufferedFileAndMove(filename);
//    printf("Here\n");
    //Doesn't actually clone yet
    int retVal;
    char bufferFilename[FILENAME_MAX];
    char bufferZippedFilename[FILENAME_MAX];
    char outputFilename[FILENAME_MAX];
    char cloneFilename[FILENAME_MAX];
    int diskInd;
    unsigned long numBytesIn=0;
    sprintf(bufferFilename,"/tmp/buffer/%s",filename);
    char *buffer=readFile(bufferFilename,&numBytesIn);
    if(!buffer || !numBytesIn) {
	free(buffer);
	return -1;
    }    
    sprintf(bufferZippedFilename,"%s.gz",bufferFilename);
    sprintf(outputFilename,"%s.gz",filename);
    retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
    char *relPath=strstr(outputFilename,"anita");
    if(!relPath) return -1;
    for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
	if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
	sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
	copyFileToFile(bufferZippedFilename,cloneFilename);	
    }
//    printf("move2 %s %s\n",bufferZippedFilename,outputFilename);
    rename(bufferZippedFilename,outputFilename);
    free(buffer);
    unlink(bufferFilename);
    return retVal;
}

int makeZippedPacket(char *input, unsigned long numBytes, char *output, unsigned long numBytesOut) {
    ZippedPacket_t *zipPacket = (ZippedPacket_t*)output;
    zipPacket->numUncompressedBytes=numBytes;
    int retVal=zipBuffer(input,&output[sizeof(ZippedPacket_t)],numBytes,&numBytesOut);
    if(retVal!=0) 
	return retVal;
    fillGenericHeader(output,PACKET_ZIPPED_PACKET,sizeof(ZippedPacket_t)+numBytesOut);
    return sizeof(ZippedPacket_t)+numBytesOut;   
}
