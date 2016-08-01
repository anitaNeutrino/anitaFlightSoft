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
//#include <sys/vfs.h>
#include <unistd.h>
#include <zlib.h>
#include <fcntl.h>
#define NO_ZLIB

ProgramStateCode currentState;


//

int diskBitMasks[DISK_TYPES]={HELIUM1_DISK_MASK,HELIUM2_DISK_MASK,USB_DISK_MASK,PMC_DISK_MASK,NTU_DISK_MASK};
char *diskNames[DISK_TYPES]={HELIUM1_DATA_MOUNT,HELIUM2_DATA_MOUNT,USB_DATA_MOUNT,SAFE_DATA_MOUNT,NTU_DATA_MOUNT};
int bufferDisk[DISK_TYPES]={0,0,0,0,0};
int eventBufferDisk[DISK_TYPES]={1,1,1,0,1};

int closeHkFilesAndTidy(AnitaHkWriterStruct_t *awsPtr) {
  //    sync();
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
	nice(10);
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


int simpleMultiDiskWrite(void *buffer, int numBytes, unsigned int unixTime, char *partialDir, char *filePrefix, int writeBitMask)
{
  int diskInd;
  int retVal=0;
  char filename[FILENAME_MAX];  
  char dirName[FILENAME_MAX];  
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(!(diskBitMasks[diskInd]&writeBitMask)) continue;
    sprintf(dirName,"%s/%s",diskNames[diskInd],partialDir);
    makeDirectories(dirName);
    sprintf(filename,"%s/%s_%d.dat",dirName,filePrefix,unixTime);
    retVal=writeStruct(buffer,filename,numBytes);
  }
  return retVal;
}

int cleverHkWrite(unsigned char *buffer, int numBytes, unsigned int unixTime, AnitaHkWriterStruct_t *awsPtr)
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
      printf("Here: %s -- %#x\n",fullBasename,awsPtr->writeBitMask);
      tempDir=getCurrentHkDir(fullBasename,unixTime);
      strncpy(awsPtr->currentDirName[diskInd],tempDir,FILENAME_MAX-1);

      if(bufferDisk[diskInd]) {
	sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentDirName[diskInd]);
	makeDirectories(bufferName);

      }
      awsPtr->dirCount[diskInd]=0;
      free(tempDir);
      tempDir=getCurrentHkDir(awsPtr->currentDirName[diskInd],unixTime);
      strncpy(awsPtr->currentSubDirName[diskInd],tempDir,FILENAME_MAX-1);
      if(bufferDisk[diskInd]) {
	sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentSubDirName[diskInd]);
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
	sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentFileName[diskInd]);
	awsPtr->currentFilePtr[diskInd]=fopen(bufferName,"ab");
      }
      awsPtr->writeCount[diskInd]=0;
      if(awsPtr->currentFilePtr[diskInd]<0) {	    
	errorCounter++;			
	if(errorCounter<100 || errorCounter%1000==0) {
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s -- %s\n",
		 errorCounter,awsPtr->currentFileName[diskInd],strerror(errno));
	}
      }
						      
    }
    
    //Check if we need a new file or directory
    if(awsPtr->writeCount[diskInd]>=HK_PER_FILE) {
      //Need a new file
      //	    char *gzipArg[] = {"gzip",awsPtr->currentFileName[diskInd],(char*)0};
      //	    char *zipAndMoveArg[]={"/bin/bash","/home/anita/flightSoft/bin/zipAndMoveFile.sh",awsPtr->currentFileName[diskInd],(char*)0};
      if(awsPtr->currentFilePtr[diskInd]>=0) {
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
	    sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentDirName[diskInd]);
	    makeDirectories(bufferName);
       	
	  }
	  awsPtr->dirCount[diskInd]=0;
	  free(tempDir);
	}
	    
	tempDir=getCurrentHkDir(awsPtr->currentDirName[diskInd],unixTime);
	strncpy(awsPtr->currentSubDirName[diskInd],tempDir,FILENAME_MAX-1);
	makeDirectories(awsPtr->currentSubDirName[diskInd]);
	if(bufferDisk[diskInd]) {
	  sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentSubDirName[diskInd]);
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
	sprintf(bufferName,"%s/%s",DISK_BUFFER_DIR,awsPtr->currentFileName[diskInd]);
	awsPtr->currentFilePtr[diskInd]=fopen(bufferName,"wb");
      }
	  
      if(awsPtr->currentFilePtr[diskInd]<0) {	    
	errorCounter++;			
	if(errorCounter<100 || errorCounter%1000==0) {
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s -- %s\n",
		 errorCounter,awsPtr->currentFileName[diskInd],strerror(errno));
	}
      }
      awsPtr->writeCount[diskInd]=0;
    }
	
    awsPtr->writeCount[diskInd]++;
    if(awsPtr->currentFilePtr[diskInd]>0) {
      retVal=fwrite(buffer,numBytes,1,awsPtr->currentFilePtr[diskInd]);
      //      printf("fwrite %d %d\n",numBytes,awsPtr->currentFilePtr[diskInd]);
      if(retVal<0 ) {
	errorCounter++;
	if(errorCounter<100  || errorCounter%1000==0) {
	  syslog(LOG_ERR,"Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		 errorCounter,awsPtr->writeCount[diskInd],
		 strerror(errno),retVal);
	}
      }
      else {
	retVal=fflush(awsPtr->currentFilePtr[diskInd]);  
	if(retVal<0) {
	    errorCounter++;
	  if(errorCounter<100  || errorCounter%1000==0) {
	    syslog(LOG_ERR,"Error (%d of 100) flushing to file (write %d) -- %s (%d)\n",
		   errorCounter,awsPtr->writeCount[diskInd],
		   strerror(errno),retVal);
	    
	  }	    
	}
      }
    }
  }	        	       
  return retVal;


}

int closeEventFilesAndTidy(AnitaEventWriterStruct_t *awsPtr) {
  //    sync();
    printf("closeEventFilesAndTidy: %s %d\n",awsPtr->bufferEventFileName,awsPtr->fileEpoch);
  int diskInd;
  pid_t childPid;    
  int cloneMasks[DISK_TYPES]={awsPtr->helium1CloneMask,
			      awsPtr->helium2CloneMask,
			      awsPtr->usbCloneMask,0};
  
  int eventBufferDiskMask=0;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(eventBufferDisk[diskInd])
      eventBufferDiskMask|=diskBitMasks[diskInd];
  }
  eventBufferDiskMask&=awsPtr->writeBitMask;

  //Deal with the buffered file first
  if(awsPtr->bufferHeaderFilePtr) {
    fclose(awsPtr->bufferHeaderFilePtr);
    
    if(awsPtr->bufferEventFilePtr) {
      fclose(awsPtr->bufferEventFilePtr);
      childPid=fork();
      if(childPid==0) {
      //Child
	zipBufferedEventFileAndCopy(eventBufferDiskMask,awsPtr);
	exit(0);
      }
      else if(childPid<0) {
      //Something wrong
      	syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->bufferHeaderFileName);
      	fprintf(stderr,"Couldn't zip file %s\n",awsPtr->bufferHeaderFileName);
      }    
      
    }
    
    childPid=fork();
    if(childPid==0) {
      //Child
      zipBufferedHeaderFileAndCopy(eventBufferDiskMask,awsPtr);
      exit(0);
    }
    else if(childPid<0) {
    //Something wrong
      syslog(LOG_ERR,"Couldn't zip file %s\n",awsPtr->bufferHeaderFileName);
      fprintf(stderr,"Couldn't zip file %s\n",awsPtr->bufferHeaderFileName);
    }
  }
  awsPtr->bufferEventFilePtr=0;
  awsPtr->bufferHeaderFilePtr=0;
   

  //  printf("Finished buffer thingy\n");
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(eventBufferDisk[diskInd]) {
      awsPtr->currentHeaderFilePtr[diskInd]=0;
      awsPtr->currentEventFilePtr[diskInd]=0;
      continue;
    }
  }
  return 0;

  {
    //    if(!(awsPtr->currentHeaderFilePtr[diskInd])) continue;
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
  //  printf("cleverEventWrite %u -- %#x %d %u\n",hdPtr->eventNumber,awsPtr->writeBitMask,awsPtr->gotData,awsPtr->fileEpoch);
  int diskInd,cloneInd;
  int retVal=0;
  static int errorCounter=0;
  int dirNum;
  //    pid_t childPid;

  char tempDir[FILENAME_MAX];
  char fullBasename[FILENAME_MAX];


  int cloneMasks[DISK_TYPES]={awsPtr->helium1CloneMask,
			      awsPtr->helium2CloneMask,
			      awsPtr->usbCloneMask,0,0};
    
  if(awsPtr->gotData && (hdPtr->eventNumber>=awsPtr->fileEpoch)) {
    closeEventFilesAndTidy(awsPtr);
    awsPtr->gotData=0;
  }



  int eventBufferDiskMask=0;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(eventBufferDisk[diskInd])
      eventBufferDiskMask|=diskBitMasks[diskInd];
  }
  eventBufferDiskMask&=awsPtr->writeBitMask;

  //RJN need to revamp this, the new model will be:
  //Write file to /tmp/buffer/something/file
  //Zip the file in place
  //Copy it to the N disks that need a full copy
  {
    //So now we write the buffered copy
    //Now need to think about this
    if(!awsPtr->bufferHeaderFilePtr) {
      //Maybe First time
      sprintf(fullBasename,"%s",awsPtr->relBaseName);	    	    	    
      
      //Make base dir
      dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
      sprintf(awsPtr->bufferDirName,"%s/%s/ev%d",DISK_BUFFER_DIR,fullBasename,dirNum);
      makeDirectories(awsPtr->bufferDirName);
      
      //Make sub dir
      dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
      sprintf(awsPtr->bufferSubDirName,"%s/ev%d",awsPtr->bufferDirName,dirNum);
      makeDirectories(awsPtr->bufferSubDirName);


      //Make files
      dirNum=(EVENTS_PER_FILE)*(hdPtr->eventNumber/EVENTS_PER_FILE);

      sprintf(awsPtr->bufferHeaderFileName,"%s/hd_%d.dat",
	      awsPtr->bufferSubDirName,dirNum);
      awsPtr->bufferHeaderFilePtr=fopen(awsPtr->bufferHeaderFileName,"ab");
      
      if(awsPtr->bufferHeaderFilePtr<0) {	    
	if(errorCounter<100) {
	  errorCounter++;
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
		 errorCounter,awsPtr->bufferHeaderFileName);
	}
      }
      
      
      awsPtr->fileEpoch=dirNum+EVENTS_PER_FILE;
    } //Now we have succesfully (hopefully) opened the buffer header file ptr

    if(!awsPtr->justHeader && !awsPtr->bufferEventFilePtr) {
      dirNum=(EVENTS_PER_FILE)*(hdPtr->eventNumber/EVENTS_PER_FILE);
		
      sprintf(awsPtr->bufferEventFileName,"%s/%s_%d.dat",
	      awsPtr->bufferSubDirName,awsPtr->filePrefix,dirNum);
      awsPtr->bufferEventFilePtr=
	fopen(awsPtr->bufferEventFileName,"ab");
      
      
      if(awsPtr->bufferEventFilePtr<0) {	    
	if(errorCounter<100) {
	  errorCounter++;
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
	       errorCounter,awsPtr->bufferEventFileName);
	}	    
      }
    } //Now we have opened the buffer event file ptr
	    
  	    
    if(errorCounter<100  && awsPtr->bufferHeaderFilePtr) {
      retVal=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,awsPtr->bufferHeaderFilePtr);
      //      printf("Writing event %d (%d bytes) to %s %d\n",hdPtr->eventNumber,sizeof(AnitaEventHeader_t),awsPtr->bufferHeaderFileName,awsPtr->bufferHeaderFilePtr);
      if(retVal<0) {
	errorCounter++;
	syslog(LOG_ERR,"Error (%d of 100) writing to file -- %s (%d)\n",
	       errorCounter,
	       strerror(errno),retVal);
      }		
      awsPtr->gotData=1;
    }
    
    if(errorCounter<100 && !awsPtr->justHeader && awsPtr->bufferEventFilePtr) {
      retVal=fwrite(outputBuffer,numBytes,1,awsPtr->bufferEventFilePtr);
      //      printf("Writing %d bytes to %s %d\n",numBytes,awsPtr->bufferEventFileName,awsPtr->bufferEventFilePtr);
      if(retVal<0) {
	errorCounter++;
	syslog(LOG_ERR,"Error (%d of 100) writing to file -- %s (%d)\n",
	       errorCounter,
	       strerror(errno),retVal);
      }
    }
  } //Now we have written the buffer header and event file	

  


  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(!(diskBitMasks[diskInd]&awsPtr->writeBitMask)) continue;
    if(!awsPtr->currentHeaderFilePtr[diskInd]) {
      //Maybe First time
      sprintf(fullBasename,"%s/%s",diskNames[diskInd],awsPtr->relBaseName);	    	    	    

      //Make base dir
      dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR*EVENT_FILES_PER_DIR));
      sprintf(awsPtr->currentDirName[diskInd],"%s/ev%d",fullBasename,dirNum);
      makeDirectories(awsPtr->currentDirName[diskInd]);


      //Make sub dir
      dirNum=(EVENTS_PER_FILE*EVENT_FILES_PER_DIR)*(hdPtr->eventNumber/(EVENTS_PER_FILE*EVENT_FILES_PER_DIR));
      sprintf(awsPtr->currentSubDirName[diskInd],"%s/ev%d",awsPtr->currentDirName[diskInd],dirNum);
      makeDirectories(awsPtr->currentSubDirName[diskInd]);	    


      //Make files
      dirNum=(EVENTS_PER_FILE)*(hdPtr->eventNumber/EVENTS_PER_FILE);

      sprintf(awsPtr->currentHeaderFileName[diskInd],"%s/hd_%d.dat",
	      awsPtr->currentSubDirName[diskInd],dirNum);
      if(!eventBufferDisk[diskInd]) {
	awsPtr->currentHeaderFilePtr[diskInd]=fopen(awsPtr->currentHeaderFileName[diskInd],"ab");
      }
      else {
	awsPtr->currentHeaderFilePtr[diskInd]=awsPtr->bufferHeaderFilePtr;
      }
		    
      if(awsPtr->currentHeaderFilePtr[diskInd]<0) {	    
	if(errorCounter<100) {
	  errorCounter++;
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
		 errorCounter,awsPtr->currentHeaderFileName[diskInd]);
	}
      }

	    
      awsPtr->fileEpoch=dirNum+EVENTS_PER_FILE;
      //	    printf("%s %s %d\n",awsPtr->currentHeaderFileName[diskInd],
      //		   awsPtr->currentEventFileName[diskInd],awsPtr->fileEpoch);

    }
    
    if(!awsPtr->justHeader && !awsPtr->currentEventFilePtr[diskInd]) {
      dirNum=(EVENTS_PER_FILE)*(hdPtr->eventNumber/EVENTS_PER_FILE);
		
      sprintf(awsPtr->currentEventFileName[diskInd],"%s/%s_%d.dat",
	      awsPtr->currentSubDirName[diskInd],awsPtr->filePrefix,
	      dirNum);
      if(!eventBufferDisk[diskInd]) {
	awsPtr->currentEventFilePtr[diskInd]=
	  fopen(awsPtr->currentEventFileName[diskInd],"ab");
      }
      else {
	awsPtr->currentEventFilePtr[diskInd]=awsPtr->bufferEventFilePtr;
      }

      if(awsPtr->currentEventFilePtr[diskInd]<0) {	    
	if(errorCounter<100) {
	  errorCounter++;
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
		 errorCounter,awsPtr->currentEventFileName[diskInd]);
	}
	    
      }	    
    }

    //    if(!eventBufferDisk[diskInd]) { 
    //      //Actually write something
    //      if(errorCounter<100  && awsPtr->currentHeaderFilePtr[diskInd]) {
    //	retVal=fwrite(hdPtr,sizeof(AnitaEventHeader_t),1,awsPtr->currentHeaderFilePtr[diskInd]);
    //	printf("Writing %d bytes to %s\n",sizeof(AnitaEventHeader_t),awsPtr->currentHeaderFileName[diskInd]);
    //	
    //	if(retVal<0) {
    //	  errorCounter++;
    //	  syslog(LOG_ERR,"Error (%d of 100) writing to file -- %s (%d)\n",
    //		 errorCounter,
    //		 strerror(errno),retVal);
    //	}
	//	else 
	//	  fflush(awsPtr->currentHeaderFilePtr[diskInd]);    	
    //	awsPtr->gotData=1;
    //      }
    //      if(errorCounter<100 && !awsPtr->justHeader && awsPtr->currentEventFilePtr[diskInd]) {
    //	retVal=fwrite(outputBuffer,numBytes,1,awsPtr->currentEventFilePtr[diskInd]);
    //	printf("Writing %d bytes to %s\n",numBytes,awsPtr->currentEventFileName[diskInd]);
    //	if(retVal<0) {
    //	  errorCounter++;
    //	  syslog(LOG_ERR,"Error (%d of 100) writing to file -- %s (%d)\n",
    //		 errorCounter,
    //		 strerror(errno),retVal);
    //	}
	//	else 
	//	  fflush(awsPtr->currentEventFilePtr[diskInd]);
    //  }
    //      else continue;       	      
    //}
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
	  syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
		 errorCounter,awsPtr->currentFileName[diskInd]);
	}
      }
    }
    if(awsPtr->currentFilePtr[diskInd]) {
      if(indPtr->eventNumber%EVENTS_PER_FILE==0 ||
	 awsPtr->writeCount[diskInd]>=EVENTS_PER_FILE) {
	fclose(awsPtr->currentFilePtr[diskInd]);
		
	//		char *gzipArg[] = {"nice","/bin/gzip",awsPtr->currentFileName[diskInd],(char*)0};
	childPid=fork();
	if(childPid==0) {
	  //Child
		    
	  zipFileInPlace(awsPtr->currentFileName[diskInd]);
	  //		    execv("/usr/bin/nice",gzipArg);
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
	    syslog(LOG_ERR,"Error (%d of 100) trying to open file %s\n",
		   errorCounter,awsPtr->currentFileName[diskInd]);
	  }
	}
	awsPtr->writeCount[diskInd]=0;
      }

	    
      awsPtr->writeCount[diskInd]++;
      if(errorCounter<100 && awsPtr->currentFilePtr[diskInd]) {
	retVal=fwrite(indPtr,sizeof(IndexEntry_t),1,awsPtr->currentFilePtr[diskInd]);
	printf("Writing index %d -- %s %d",sizeof(IndexEntry_t),awsPtr->currentFileName[diskInd],awsPtr->currentFilePtr[diskInd]);
	if(retVal<0) {
	  errorCounter++;
	  syslog(LOG_ERR,"Error (%d of 100) writing to file (write %d) -- %s (%d)\n",
		 errorCounter,awsPtr->writeCount[diskInd],
		 strerror(errno),retVal);
	}
	//	else 
	// fflush(awsPtr->currentFilePtr[diskInd]);  

      }
      else continue;       	
    }
  }
  return 0;
}



char *getCurrentHkDir(char *baseHkDir,unsigned int unixTime)
{
  char *newDir;   
  //    int ext=0;
  newDir=malloc(FILENAME_MAX);
  sprintf(newDir,"%s/sub_%u",baseHkDir,unixTime);
  //    while(is_dir(newDir)) {
  //	ext++;
  //	sprintf(newDir,"%s/sub_%u_%d",baseHkDir,unixTime,ext);
  //    }
  makeDirectories(newDir);
  return newDir;    
}

char *getCurrentHkFilename(char *currentDir, char *prefix, 
			   unsigned int unixTime) 
{
  char *newFile;
  int ext=0;
  FILE *fpTest;
  newFile=malloc(FILENAME_MAX);
  sprintf(newFile,"%s/%s_%u.dat",currentDir,prefix,unixTime);
  fpTest=fopen(newFile,"r");   
  while(fpTest) {
    fclose(fpTest);
    ext++;
    sprintf(newFile,"%s/%s_%u.dat_%d",currentDir,prefix,unixTime,ext);
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
//  printf("normalSingleWrite: %d bytes to %s\n",numBytes,filename);
  numObjs=fwrite(buffer,numBytes,1,outfile);
  if(numObjs!=1) {
    if(errorCounter<100) {
      syslog (LOG_ERR,"fwrite: %s ---  %s\n",strerror(errno),filename);
      fprintf(stderr,"fwrite: %s -- %s\n",strerror(errno),filename);
      errorCounter++;
    }
  }
    
  fclose(outfile);
  return 0;
    
}

int zippedSingleWrite(unsigned char *buffer, char *filename, int numBytes) 
{
  static int errorCounter=0;
  int numWritten;    
  gzFile outfile = gzopen (filename, "wb");  
  if(outfile == NULL) { 
    if(errorCounter<100) {
      syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),filename);
      fprintf(stderr,"fopen: %s -- %s\n",strerror(errno),filename);
      errorCounter++;
    }
    return -1;
  } 
  numWritten=gzwrite(outfile,buffer,numBytes);
  //  printf("zippedSingleWrite: %d bytes to %s\n",numBytes,filename);
  if(numWritten!=numBytes) {
    if(errorCounter<100) {
      syslog (LOG_ERR,"gzwrite: wrote %d of %d ---  %s\n",numWritten,numBytes,filename);
      fprintf(stderr,"gzwrite: wrote %d of %d-- %s\n",numWritten,numBytes,filename);
      errorCounter++;
    }
  }
    
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
  static int errorCounter=0;
  char *justFile=basename((char *)theFile);
  char newFile[FILENAME_MAX];
  sprintf(newFile,"%s/%s",theLinkDir,justFile);
  //    printf("Linking %s to %s\n",theFile,newFile);
  int retVal=symlink(theFile,newFile);
  if(retVal!=0) {
    if(errorCounter<100) {
      syslog(LOG_ERR,"Error (%d of 100) linking %s to %s:\t%s",errorCounter,theFile,newFile,strerror(errno));
      fprintf(stderr,"Error (%d of 100) linking %s to %s:\t%s\n",errorCounter,theFile,newFile,strerror(errno));
      errorCounter++;
    }
  }
  return retVal;
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


char *readFile(const char *theFile, unsigned int *numBytes)
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
  unsigned int numBytes=0;
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
  //  printf("copyFile %s %s\n",theFile,theDir);
  fclose(fout);
  free(buffer);
  return 0;
}


/* int copyFileToFile(const char *fromHere, const char *toHere) { */
/*     static int errorCounter=0; */
/*     char buffer[512]; */
/*     int bytes,inhandle,outhandle; */
/*     inhandle=open(fromHere,O_RDONLY); */
/*     if(inhandle<0) {	 */
/* 	if(errorCounter<100) { */
/* 	    syslog(LOG_ERR,"Error reading file %s -- Error %s\n", */
/* 		   fromHere,strerror(errno)); */
/* 	    fprintf(stderr,"Error reading file %s-- Error %s\n", */
/* 		    fromHere,strerror(errno)); */
/* 	    errorCounter++; */
/* 	} */
/* 	return -1; */
/*     }     */
/*     outhandle=open(toHere,O_CREAT|O_WRONLY|S_IWRITE); */
/*     if(outhandle<0) {	 */
/* 	if(errorCounter<100) { */
/* 	    syslog(LOG_ERR,"Error opening file %s -- Error %s\n", */
/* 		   toHere,strerror(errno)); */
/* 	    fprintf(stderr,"Error opening file %s-- Error %s\n", */
/* 		    toHere,strerror(errno)); */
/* 	    errorCounter++; */
/* 	} */
/* 	return -1; */
/*     }     */
    
/*     while(1) { */
/*         bytes=read(inhandle,buffer,512); */
/*         if(bytes>0) */
/* 	    write(outhandle,buffer,bytes); */
/*         else */
/* 	    break; */
/*     } */
/*     close(inhandle); */
/*     close(outhandle); */
/*     return 0; */
/* } */

int copyFileToFile(const char *theFile, const char *newFile) 
{ 
  static int errorCounter=0; 
  //Only works on relative small files (will write a better version) 
  unsigned int numBytes=0; 
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
  //  printf("copyFile %s %s\n",theFile,newFile);
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


unsigned int getDiskSpace(char *dirName) {
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
  unsigned int megabytesAvailable=(diskStat.f_bavail/1024)*(diskStat.f_bsize/1024);
  return megabytesAvailable;
  //    printf("%u\n",megabytesAvailable);
  //    if(printToScreen) {
  printf("Dir: %s\n",dirName);
  printf("Ret Val: %d\n",retVal);
  printf("MegaBytes Available: %u\n",megabytesAvailable);
  printf("Available Blocks: %ld\n",(long)diskStat.f_bavail);
  printf("Free Blocks: %ld\n",(long)diskStat.f_bfree);
  printf("Total Blocks: %ld\n",(long)diskStat.f_blocks);
  printf("Block Size: %lu\n",(long)diskStat.f_bsize);
  //	printf("Free File Nodes: %d\n",diskStat.f_ffree);
  //	printf("Total File Nodes: %d\n",diskStat.f_files);
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

int fillLogWatchRequest(LogWatchRequest_t *logPtr, char *filename)
{
  int numBytes=genericReadOfFile((unsigned char*)logPtr,filename,sizeof(LogWatchRequest_t));
  if(numBytes==sizeof(LogWatchRequest_t)) return 0;
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
			    unsigned int eventNumber)
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
      syslog(LOG_ERR,"Events %u and %u can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
      fprintf(stderr,"Events %u and %u can not both be in file %s\n",eventNumber,hdPtr->eventNumber,filename);
      errorCounter++;
    }	
    return -3;
  }

  //    printf("Looking for %u (first read %u)\n",eventNumber,hdPtr->eventNumber);
	
  readNum=(eventNumber-hdPtr->eventNumber);
  if(readNum) {	    
    if(readNum>1) {
      gzseek(infile,sizeof(AnitaEventHeader_t)*(readNum-1),SEEK_CUR);
      //	    printf("Sought %d bytes am now at %d\n",sizeof(AnitaEventHeader_t)*(readNum-1),(int)gztell(infile));
    }

    numBytes=gzread(infile,hdPtr,sizeof(AnitaEventHeader_t));
    //	printf("Sought %d bytes got event %u\n",sizeof(AnitaEventHeader_t)*(readNum-1),hdPtr->eventNumber);

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
			     unsigned int eventNumber) 
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

  //    printf("First read %u (want %u)\n\t%s\n",encEvWrap->eventNumber,eventNumber,filename);

  if(abs(((int)eventNumber-(int)encEvWrap->eventNumber))>1000) {
    if(errorCounter<100) {
      syslog(LOG_ERR,"Events %u and %u can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
      fprintf(stderr,"Events %u and %u can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
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
	syslog(LOG_ERR,"Events %u and %u can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
	fprintf(stderr,"Events %u and %u can not both be in file %s\n",eventNumber,encEvWrap->eventNumber,filename);
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


int writeStruct(void *thePtr, char *filename, int numBytes)
{
#ifdef NO_ZLIB
  return normalSingleWrite((unsigned char*)thePtr,filename,numBytes);
#else
  return zippedSingleWrite((unsigned char*)thePtr,filename,numBytes);  
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


int checkPidFile(char *fileName)
{
  int retVal=0;
  FILE *fpPid ;
  char procDir[FILENAME_MAX];
  
  //    pid_t thePid;
  int theNaughtyPid;
  if (!(fpPid=fopen(fileName, "r"))) {
    return 0;
  }
  else {
    //There shouldn't be a pidFile here
    retVal=fscanf(fpPid,"%d", &theNaughtyPid) ;
    if(retVal>0) {
      retVal=fclose(fpPid) ;
      if(retVal<0) {
	syslog(LOG_ERR,"Error closing %s  -- %s",fileName,strerror(errno));
      }
      
      //Now check to see the process is running
      retVal=snprintf(procDir,sizeof(procDir),"/proc/%d",theNaughtyPid);
      if(retVal<=0) {
	syslog(LOG_ERR,"Error using snprintf -- %s",strerror(errno));
      }
    
      retVal=access(procDir,F_OK);
      if(retVal==0) {
	//The process is running
	return theNaughtyPid;
      }
      else {
	syslog(LOG_ERR,"%s exists but %d is not running, will delete the file and start process",fileName,theNaughtyPid);
	retVal=unlink(fileName);
	if(retVal<=0) {
	  syslog(LOG_ERR,"Error trying to unlink %s -- %s",fileName,strerror(errno));
	}
	return 0;
      }
    }
  }
  return -1;
	  
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


unsigned int simpleIntCrc(unsigned int *p, unsigned int n)
{
  unsigned int sum = 0;
  unsigned int i;
  for (i=0L; i<n; i++) {
    //
    sum += *p++;
  }
  //    printf("%u %u\n",*p,sum);
  return ((0xffffffff - sum) + 1);
}




void fillGenericHeader(void *thePtr, PacketCode_t code, unsigned short numBytes) {
  unsigned int intBytes=(numBytes-sizeof(GenericHeader_t))/4;
  GenericHeader_t *gHdr= (GenericHeader_t*)thePtr;
  unsigned int *dataPtr=(unsigned int*) (thePtr+sizeof(GenericHeader_t));
  //    printf("%u -- %u\n",thePtr,dataPtr);
  //    printf("%u -- %u -- %u\n",thePtr,dataPtr,((unsigned int)(dataPtr)-(unsigned int)(thePtr)));
  gHdr->code=code;
  gHdr->numBytes=numBytes;
  gHdr->feByte=0xfe;
  switch(code&BASE_PACKET_MASK) {
  case PACKET_BD: gHdr->verId=VER_EVENT_BODY; break;
  case PACKET_HD: gHdr->verId=VER_EVENT_HEADER; break;
  case PACKET_HD_SLAC: gHdr->verId=SLAC_VER_EVENT_HEADER; break;
  case PACKET_WV: gHdr->verId=VER_WAVE_PACKET; break;
  case PACKET_SURF: gHdr->verId=VER_SURF_PACKET; break;
  case PACKET_SURF_HK: gHdr->verId=VER_SURF_HK; break;
  case PACKET_TURF_RATE: gHdr->verId=VER_TURF_RATE; break;
  case PACKET_AVG_SURF_HK: gHdr->verId=VER_AVG_SURF_HK; break;
  case PACKET_SUM_TURF_RATE: gHdr->verId=VER_SUM_TURF_RATE; break;
  case PACKET_TURF_REGISTER: gHdr->verId=VER_TURF_REG; break;
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
  case PACKET_GPS_GGA: gHdr->verId=VER_GPS_GGA; break;
  case PACKET_HKD: gHdr->verId=VER_HK_FULL; break;  
  case PACKET_HKD_SS: gHdr->verId=VER_HK_SS; break;
  case PACKET_CMD_ECHO: gHdr->verId=VER_CMD_ECHO; break;
  case PACKET_MONITOR: gHdr->verId=VER_MONITOR; break;
  case PACKET_SLOW1: gHdr->verId=VER_SLOW_1; break;
  case PACKET_SLOW2: gHdr->verId=VER_SLOW_2; break;
  case PACKET_SLOW_FULL: gHdr->verId=VER_SLOW_FULL; break;
  case PACKET_ZIPPED_FILE: gHdr->verId=VER_ZIPPED_FILE; break;
  case PACKET_ZIPPED_PACKET: gHdr->verId=VER_ZIPPED_PACKET; break;
  case PACKET_RUN_START: gHdr->verId=VER_RUN_START; break;
  case PACKET_OTHER_MONITOR: gHdr->verId=VER_OTHER_MON; break;
  case PACKET_GPSD_START: gHdr->verId=VER_GPSD_START; break;
  case PACKET_LOGWATCHD_START: gHdr->verId=VER_LOGWATCHD_START; break;
  case PACKET_ACQD_START: gHdr->verId=VER_ACQD_START; break;
  case PACKET_GPU_AVE_POW_SPEC: gHdr->verId=VER_GPU_POW_SPEC; break;
  case PACKET_RTLSDR_POW_SPEC: gHdr->verId=VER_RTLSDR_POW_SPEC; break;
  case PACKET_TUFF_STATUS: gHdr->verId=VER_TUFF_STATUS; break;
  case PACKET_TUFF_RAW_CMD: gHdr->verId=VER_TUFF_RAW_CMD; break;
  default: 
    gHdr->verId=0; break;
  }
  gHdr->checksum=simpleIntCrc(dataPtr,intBytes);
  //    printf("Int bytes %u\t checksum %u\n",intBytes,gHdr->checksum);
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
  unsigned int intBytes=(gHdr->numBytes-sizeof(GenericHeader_t))/4;
  unsigned int *dataPtr=(unsigned int*) (thePtr+sizeof(GenericHeader_t)); 
  unsigned int checksum=0;
  if(intBytes<4000)
    checksum=simpleIntCrc(dataPtr,intBytes);
  PacketCode_t code=gHdr->code;
  if(checksum!=gHdr->checksum) {
    printf("Checksum Mismatch (possibly %s (%d)) (%u bytes) %u -- %u \n",
	   packetCodeAsString(code),code,intBytes,checksum,gHdr->checksum);	
    retVal+=PKT_E_CHECKSUM;
  }
  switch(code&BASE_PACKET_MASK) {
  case PACKET_BD: packetSize=sizeof(AnitaEventBody_t); break;	    
  case PACKET_HD: packetSize=sizeof(AnitaEventHeader_t); break;	    
  case PACKET_WV: packetSize=sizeof(RawWaveformPacket_t); break;
  case PACKET_PEDSUB_WV: packetSize=sizeof(PedSubbedWaveformPacket_t); break;
  case PACKET_PEDSUB_SURF: packetSize=sizeof(PedSubbedSurfPacket_t); break;
  case PACKET_SURF: packetSize=sizeof(RawSurfPacket_t); break;
  case PACKET_SURF_HK: packetSize=sizeof(FullSurfHkStruct_t); break;
  case PACKET_TURF_RATE: packetSize=sizeof(TurfRateStruct_t); break;
  case PACKET_TURF_REGISTER: packetSize=sizeof(TurfRegisterContents_t); break;
  case PACKET_AVG_SURF_HK: packetSize=sizeof(AveragedSurfHkStruct_t); break;
  case PACKET_SUM_TURF_RATE: packetSize=sizeof(SummedTurfRateStruct_t); break;
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
  case PACKET_GPS_GGA: packetSize=sizeof(GpsGgaStruct_t); break;
  case PACKET_HKD: packetSize=sizeof(HkDataStruct_t); break;    
  case PACKET_HKD_SS: packetSize=sizeof(SSHkDataStruct_t); break;    
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
  case PACKET_SLOW_FULL: packetSize=sizeof(SlowRateFull_t); 
    break;
  case PACKET_RUN_START: packetSize=sizeof(RunStart_t); 
    break;
  case PACKET_OTHER_MONITOR: packetSize=sizeof(OtherMonitorStruct_t); 
    break;
  case PACKET_ZIPPED_PACKET: break;
  case PACKET_ZIPPED_FILE: break;
  case PACKET_GPSD_START: packetSize=sizeof(GpsdStartStruct_t); break;
  case PACKET_LOGWATCHD_START: packetSize=sizeof(LogWatchdStart_t); break;
  case PACKET_ACQD_START: packetSize=sizeof(AcqdStartStruct_t); break;
  case PACKET_GPU_AVE_POW_SPEC: packetSize=sizeof(GpuPhiSectorPowerSpectrumStruct_t); break;
  case PACKET_TUFF_STATUS: packetSize=sizeof(TuffNotchStatus_t); break;
  case PACKET_TUFF_RAW_CMD: packetSize=sizeof(TuffRawCmd_t); break;
  case PACKET_RTLSDR_POW_SPEC: packetSize=sizeof(RtlSdrPowerSpectraStruct_t); break;

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
  switch(code&BASE_PACKET_MASK) {
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
  case PACKET_HKD_SS: string="SSHkDataStruct_t"; break;
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
  case PACKET_RUN_START: string="RunStart_t"; break;
  case PACKET_OTHER_MONITOR: string="OtherMonitorStruct_t"; break;
  case PACKET_GPSD_START: string="GpsdStartStruct_t"; break;
  case PACKET_LOGWATCHD_START: string="LogWatchdStart_t"; break;
  case PACKET_ACQD_START: string="AcqdStartStruct_t"; break;
  case PACKET_GPU_AVE_POW_SPEC: string="GpuPhiSectorPowerSpectrumStruct_t"; break;
  case PACKET_GPS_GGA: string="GpsGgaStruct_t"; break;
  case PACKET_AVG_SURF_HK: string="AveragedSurfHkStruct_t"; break;
  case PACKET_SUM_TURF_RATE: string="SummedTurfRateStruct_t"; break;
  case PACKET_RTLSDR_POW_SPEC: string ="RtlSdrPowerSpectraStruct_t"; break; 
  case PACKET_TUFF_STATUS: string ="TuffNotchStatus_t"; break; 
  case PACKET_TUFF_RAW_CMD: string ="TuffRawCmd_t"; break; 


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
  syslog(LOG_INFO,"Sending command %d %d %d %d, %d bytes\n",
	 theCmd->cmd[0],theCmd->cmd[1],theCmd->cmd[2],theCmd->cmd[3],
	 theCmd->numCmdBytes);

  writeStruct(theCmd,filename,sizeof(CommandStruct_t));
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

int checkFileExists(char *filename) 
// Returns 1 if file exists, 0 otherwise

{
  int retVal=access(filename,F_OK);
  if(retVal==0)
    return 1;
  return 0;
}

int zipBuffer(char *input, char *output, unsigned long inputBytes, unsigned long *outputBytes)
{
  static int errorCounter=0;
  int retVal=compress((unsigned char*)output,(unsigned long*)outputBytes,(unsigned char*)input,(unsigned long)inputBytes);
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
  int retVal=uncompress((unsigned char*)output,(unsigned long*)outputBytes,(unsigned char*)input,(unsigned long)inputBytes);
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
  char tempFilename[FILENAME_MAX];
  unsigned int numBytesIn=0;
  char *buffer=readFile(filename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    free(buffer);
    return -1;
  }    
  sprintf(outputFilename,"%s.gz",filename);
  sprintf(tempFilename,"%s.gz.temp",filename);
  retVal=zippedSingleWrite((unsigned char*)buffer,tempFilename,numBytesIn);
  free(buffer);
  unlink(filename);
  link(tempFilename,outputFilename);
  unlink(tempFilename);
  return retVal;
}

int cloneFile(char *filename,unsigned int cloneMask, int baseInd)
{
  //Doesn't clone yet
  if(cloneMask==0) return 0; 
  int retVal=0;
  char cloneFilename[FILENAME_MAX];
  int diskInd;
  char *relPath=strstr(filename,"current");
  if(!relPath) return -1;

  unsigned int numBytesIn=0;
  char *buffer=readFile(filename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    fprintf(stderr,"Problem reading file %s\n",filename);
    free(buffer);
    return -1;
  }      
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
    sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
    normalSingleWrite((unsigned char*)buffer,cloneFilename,numBytesIn);
  }
  free(buffer);
  return retVal;

}

int zipFileInPlaceAndClone(char *filename,unsigned int cloneMask,int baseInd) {
  //Doesn't clone yet
  if(cloneMask==0) return zipFileInPlace(filename);
  int retVal;
  char outputFilename[FILENAME_MAX];
  char cloneFilename[FILENAME_MAX];
  int diskInd;
  unsigned int numBytesIn=0;
  char *buffer=readFile(filename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    free(buffer);
    return -1;
  }    
  sprintf(outputFilename,"%s.gz",filename);
  retVal=zippedSingleWrite((unsigned char*)buffer,outputFilename,numBytesIn);
  unlink(filename);
  char *relPath=strstr(outputFilename,"current");
  if(!relPath) return -1;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
    sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
    copyFileToFile(outputFilename,cloneFilename);	
  }

  free(buffer);
  return retVal;
}


int zipBufferedFileAndMove(char *filename) {
  //    printf("zipBufferedFileAndMove %s\n",filename);
  int retVal;
  char bufferFilename[FILENAME_MAX];
  char bufferZippedFilename[FILENAME_MAX];
  char outputFilename[FILENAME_MAX];
  unsigned int numBytesIn=0;
  sprintf(bufferFilename,"%s/%s",DISK_BUFFER_DIR,filename);
  char *buffer=readFile(bufferFilename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    fprintf(stderr,"Problem reading file %s\n",bufferFilename);
    free(buffer);
    return -1;
  }    
  sprintf(bufferZippedFilename,"%s.gz",bufferFilename);
  sprintf(outputFilename,"%s.gz",filename);
  retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
  unlink(bufferFilename);
  //    printf("move %s %s\n",bufferZippedFilename,outputFilename);
  copyFileToFile(bufferZippedFilename,outputFilename);
  //    rename(bufferZippedFilename,outputFilename);
  free(buffer);
  unlink(bufferZippedFilename);
  return retVal;
}


int zipBufferedHeaderFileAndCopy(unsigned int eventBufferDiskMask, AnitaEventWriterStruct_t *awsPtr) {
  //  printf("zipBufferedHeaderFileAndCopy %s\n",awsPtr->bufferHeaderFileName);
  //    printf("zipBufferedFileAndMove %s\n",filename);
  int retVal;
  int diskInd=0;
  char bufferZippedFilename[FILENAME_MAX];
  char outputFilename[FILENAME_MAX];
  char tempFilename[FILENAME_MAX];
  unsigned int numBytesIn=0;
  char *buffer=readFile(awsPtr->bufferHeaderFileName,&numBytesIn);
  if(!buffer || !numBytesIn) {
    fprintf(stderr,"Problem reading file %s\n",awsPtr->bufferHeaderFileName);
    free(buffer);
    return -1;
  }    
  sprintf(bufferZippedFilename,"%s.gz",awsPtr->bufferHeaderFileName);
  retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
  unlink(awsPtr->bufferHeaderFileName);
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(eventBufferDiskMask&diskBitMasks[diskInd]) {
      sprintf(outputFilename,"%s.gz",awsPtr->currentHeaderFileName[diskInd]);
      sprintf(tempFilename,"%s.gz.new",awsPtr->currentHeaderFileName[diskInd]);
      copyFileToFile(bufferZippedFilename,tempFilename);
      link(tempFilename,outputFilename);
      unlink(tempFilename);
    }
  }
  free(buffer);
  unlink(bufferZippedFilename);
  return retVal;
}



int zipBufferedEventFileAndCopy(unsigned int eventBufferDiskMask, AnitaEventWriterStruct_t *awsPtr) {
  //  printf("zipBufferedEventFileAndCopy %s\n",awsPtr->bufferEventFileName);
  int retVal;
  int diskInd=0;
  char bufferZippedFilename[FILENAME_MAX];
  char outputFilename[FILENAME_MAX];
  char tempFilename[FILENAME_MAX];
  unsigned int numBytesIn=0;
  char *buffer=readFile(awsPtr->bufferEventFileName,&numBytesIn);
  if(!buffer || !numBytesIn) {
    fprintf(stderr,"Problem reading file %s\n",awsPtr->bufferEventFileName);
    free(buffer);
    return -1;
  }    
  sprintf(bufferZippedFilename,"%s.gz",awsPtr->bufferEventFileName);
  retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
  unlink(awsPtr->bufferEventFileName);
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(eventBufferDiskMask&diskBitMasks[diskInd]) {
      sprintf(outputFilename,"%s.gz",awsPtr->currentEventFileName[diskInd]);
      sprintf(tempFilename,"%s.gz.new",awsPtr->currentEventFileName[diskInd]);
      copyFileToFile(bufferZippedFilename,tempFilename);
      link(tempFilename,outputFilename);
      unlink(tempFilename);
    }
  }
  free(buffer);
  unlink(bufferZippedFilename);
  return retVal;
}



int moveBufferedFile(char *filename) {
  int retVal=0;
  char bufferFilename[FILENAME_MAX];
  unsigned int numBytesIn=0;
  sprintf(bufferFilename,"%s/%s",DISK_BUFFER_DIR,filename);
  char *buffer=readFile(bufferFilename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    fprintf(stderr,"Problem reading file %s\n",bufferFilename);
    free(buffer);
    return -1;
  }    
  normalSingleWrite((unsigned char*)buffer,filename,numBytesIn);
  //    rename(bufferZippedFilename,outputFilename);
  free(buffer);
  unlink(bufferFilename);
  return retVal;
}


int cloneAndMoveBufferedFile(char *filename,unsigned int cloneMask, int baseInd)
{
  //    printf("zipBufferedFileAndCloneAndMove %s, %u %d\n",filename,cloneMask,baseInd);
  if(cloneMask==0) return moveBufferedFile(filename);
  //    printf("Here\n");
  //Doesn't actually clone yet
  int retVal=0;
  char bufferFilename[FILENAME_MAX];
  char cloneFilename[FILENAME_MAX];
  int diskInd;
  unsigned int numBytesIn=0;
  sprintf(bufferFilename,"%s/%s",DISK_BUFFER_DIR,filename);
  char *buffer=readFile(bufferFilename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    free(buffer);
    return -1;
  }    
  char *relPath=strstr(filename,"current");
  if(!relPath) return -1;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
    sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
    normalSingleWrite((unsigned char*)buffer,cloneFilename,numBytesIn);	
    //    printf("copy %s %s\n",bufferFilename,cloneFilename);
  }
  //  printf("move2 %s %s\n",bufferFilename,filename);
  //    rename(bufferZippedFilename,outputFilename);
  normalSingleWrite((unsigned char*)buffer,filename,numBytesIn);
  free(buffer);
  unlink(bufferFilename);
  return retVal;
}

int zipBufferedFileAndCloneAndMove(char *filename,unsigned int cloneMask,int baseInd) 
{
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
  unsigned int numBytesIn=0;
  sprintf(bufferFilename,"%s/%s",DISK_BUFFER_DIR,filename);
  char *buffer=readFile(bufferFilename,&numBytesIn);
  if(!buffer || !numBytesIn) {
    free(buffer);
    return -1;
  }    
  sprintf(bufferZippedFilename,"%s.gz",bufferFilename);
  sprintf(outputFilename,"%s.gz",filename);
  retVal=zippedSingleWrite((unsigned char*)buffer,bufferZippedFilename,numBytesIn);
  unlink(bufferFilename);
  char *relPath=strstr(outputFilename,"current");
  if(!relPath) return -1;
  for(diskInd=0;diskInd<DISK_TYPES;diskInd++) {
    if(diskInd==baseInd || !(diskBitMasks[diskInd]&cloneMask)) continue;
    sprintf(cloneFilename,"%s/%s",diskNames[diskInd],relPath);
    copyFileToFile(bufferZippedFilename,cloneFilename);	
    //    printf("copy %s %s\n",bufferZippedFilename,cloneFilename);
  }
  //  printf("move2 %s %s\n",bufferZippedFilename,outputFilename);
  //    rename(bufferZippedFilename,outputFilename);
  copyFileToFile(bufferZippedFilename,outputFilename);
  free(buffer);
  unlink(bufferZippedFilename);
  return retVal;
}

int makeZippedPacket(char *input, unsigned int numBytes, char *output, unsigned int numBytesOut) {
  ZippedPacket_t *zipPacket = (ZippedPacket_t*)output;
  zipPacket->numUncompressedBytes=numBytes;
  unsigned long realBytesOut=numBytesOut;
  int retVal=zipBuffer(input,&output[sizeof(ZippedPacket_t)],numBytes,&realBytesOut);
  numBytesOut=realBytesOut;
  if(retVal!=0) 
    return retVal;
  fillGenericHeader(output,PACKET_ZIPPED_PACKET,sizeof(ZippedPacket_t)+numBytesOut);
  return sizeof(ZippedPacket_t)+numBytesOut;   
}

int unzipZippedPacket(ZippedPacket_t *zipPacket, char *output, unsigned int numBytesOut) {
  char *input = (char*) zipPacket;
  unsigned long returnBytes=numBytesOut;
  int retVal=unzipBuffer(&input[sizeof(ZippedPacket_t)],output,zipPacket->gHdr.numBytes-sizeof(ZippedPacket_t),&returnBytes);
  if(retVal!=0) 
    return retVal;
  if(zipPacket->numUncompressedBytes!=returnBytes)
    return -1;
  return 0;
}
 


int getRunNumber() {
  int retVal=0;
  int runNumber=0;
 
  FILE *pFile;    
  pFile = fopen (LAST_RUN_NUMBER_FILE, "r");
  if(!pFile) {
    syslog (LOG_ERR,"Couldn't open %s",LAST_RUN_NUMBER_FILE);
    fprintf(stderr,"Couldn't open %s\n",LAST_RUN_NUMBER_FILE);
    return -1;
  }

  retVal=fscanf(pFile,"%d",&runNumber);
  if(retVal<0) {
    syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
	    LAST_RUN_NUMBER_FILE);
  }
  fclose (pFile);
  return runNumber;
    
}


int getIdMask(ProgramId_t prog) {
  switch(prog) {
  case ID_ACQD: return ACQD_ID_MASK;
  case ID_ARCHIVED: return ARCHIVED_ID_MASK;
  case ID_CALIBD: return CALIBD_ID_MASK;
  case ID_CMDD: return CMDD_ID_MASK;
  case ID_EVENTD: return EVENTD_ID_MASK;
  case ID_GPSD: return GPSD_ID_MASK;
  case ID_HKD: return HKD_ID_MASK;
  case ID_LOSD: return LOSD_ID_MASK;
  case ID_PRIORITIZERD: return PRIORITIZERD_ID_MASK;
  case ID_SIPD: return SIPD_ID_MASK;
  case ID_MONITORD: return MONITORD_ID_MASK;
  case ID_PLAYBACKD: return PLAYBACKD_ID_MASK;
  case ID_LOGWATCHD: return LOGWATCHD_ID_MASK;
  case ID_NTUD: return NTUD_ID_MASK;
  case ID_OPENPORTD: return OPENPORTD_ID_MASK;
  case ID_RTLD: return RTLD_ID_MASK;
  case ID_TUFFD: return TUFFD_ID_MASK;
  default: break;
  }
  return 0;
}


char *getProgName(ProgramId_t prog) {
  char *string;
  switch(prog) {
  case ID_ACQD: string="Acqd"; break;
  case ID_ARCHIVED: string="Archived"; break;
  case ID_CALIBD: string="Calibd"; break;
  case ID_CMDD: string="Cmdd"; break;
  case ID_EVENTD: string="Eventd"; break;
  case ID_GPSD: string="GPSd"; break;
  case ID_HKD: string="Hkd"; break;
  case ID_LOSD: string="LOSd"; break;
  case ID_PRIORITIZERD: string="Prioritizerd"; break;
  case ID_SIPD: string="SIPd"; break;
  case ID_MONITORD: string="Monitord"; break;
  case ID_PLAYBACKD: string="Playbackd"; break;
  case ID_LOGWATCHD: string="LogWatchd"; break;
  case ID_NTUD: string="NTUd"; break;
  case ID_OPENPORTD: string="Openportd"; break;
  case ID_RTLD: string="RTLd"; break;
  case ID_TUFFD: string="Tuffd"; break;
  default: string=NULL; break;
  }
  return string;
}

char *getPidFile(ProgramId_t prog) {
  switch(prog) {
  case ID_ACQD: return ACQD_PID_FILE;
  case ID_ARCHIVED: return ARCHIVED_PID_FILE;
  case ID_CALIBD: return CALIBD_PID_FILE;
  case ID_CMDD: return CMDD_PID_FILE;
  case ID_EVENTD: return EVENTD_PID_FILE;
  case ID_GPSD: return GPSD_PID_FILE;
  case ID_HKD: return HKD_PID_FILE;
  case ID_LOSD: return LOSD_PID_FILE;
  case ID_PRIORITIZERD: return PRIORITIZERD_PID_FILE;
  case ID_SIPD: return SIPD_PID_FILE;
  case ID_MONITORD: return MONITORD_PID_FILE;
  case ID_PLAYBACKD: return PLAYBACKD_PID_FILE;
  case ID_LOGWATCHD: return LOGWATCHD_PID_FILE;
  case ID_NTUD: return NTUD_PID_FILE;
  case ID_OPENPORTD: return OPENPORTD_PID_FILE;
  case ID_RTLD: return RTLD_PID_FILE;
  case ID_TUFFD: return TUFFD_PID_FILE;
  default: break;
  }
  return NULL;
}


int telemeterFile(char *filename, int numLines) {
  static int counter=0;
  LogWatchRequest_t theRequest;
  char outName[FILENAME_MAX];
  time_t rawtime;
  time(&rawtime);
  theRequest.numLines=numLines;
  strncpy(theRequest.filename,filename,179);
  sprintf(outName,"%s/request_%d.dat",LOGWATCH_DIR,counter);
  counter++;
  writeStruct(&theRequest,outName,sizeof(LogWatchRequest_t));
  makeLink(outName,LOGWATCH_LINK_DIR);
  return rawtime;
}

int sendSignal(ProgramId_t progId, int theSignal) 
{
  int retVal=0;
  FILE *fpPid ;
  char fileName[FILENAME_MAX];
  pid_t thePid;
  switch(progId) {
  case ID_ACQD:
    sprintf(fileName,"%s",ACQD_PID_FILE);
    break;
  case ID_ARCHIVED:
    sprintf(fileName,"%s",ARCHIVED_PID_FILE);
    break;
  case ID_EVENTD:
    sprintf(fileName,"%s",EVENTD_PID_FILE);
    break;
  case ID_CALIBD:
    sprintf(fileName,"%s",CALIBD_PID_FILE);
    break;
  case ID_GPSD:
    sprintf(fileName,"%s",GPSD_PID_FILE);
    break;
  case ID_HKD:
    sprintf(fileName,"%s",HKD_PID_FILE);
    break;
  case ID_LOSD:
    sprintf(fileName,"%s",LOSD_PID_FILE);
    break;
  case ID_PRIORITIZERD:
    sprintf(fileName,"%s",PRIORITIZERD_PID_FILE);
    break;
  case ID_MONITORD:
    sprintf(fileName,"%s",MONITORD_PID_FILE);
    break;
  case ID_SIPD:
    sprintf(fileName,"%s",SIPD_PID_FILE);
    break;    
  case ID_NTUD:
    sprintf(fileName,"%s",NTUD_PID_FILE);
    break;    
  case ID_OPENPORTD:
    sprintf(fileName,"%s",OPENPORTD_PID_FILE);
    break;    
  case ID_PLAYBACKD:
    sprintf(fileName,"%s",PLAYBACKD_PID_FILE);
    break;    
  case ID_LOGWATCHD:
    sprintf(fileName,"%s",LOGWATCHD_PID_FILE);
    break;    
  case ID_TUFFD: 
    sprintf(fileName,"%s",TUFFD_PID_FILE);
    break; 
  case ID_RTLD: 
    sprintf(fileName,"%s",RTLD_PID_FILE);
    break; 

  default:
    fprintf(stderr,"Unknown program id: %d\n",progId);
    syslog(LOG_ERR,"Unknown program id: %d\n",progId);
  }
  if (!(fpPid=fopen(fileName, "r"))) {
    fprintf(stderr," failed to open a file for PID, %s\n", fileName);
    syslog(LOG_ERR," failed to open a file for PID, %s\n", fileName);
    return -1;
  }
  fscanf(fpPid,"%d", &thePid) ;
  fclose(fpPid) ;
        
  syslog(LOG_INFO,"Sending %d to %s (pid = %d)\n",theSignal,getProgName(progId),thePid);
  retVal=kill(thePid,theSignal);
  if(retVal!=0) {
    syslog(LOG_ERR,"Error sending %d to pid %d:\t%s",theSignal,thePid,strerror(errno));
    fprintf(stderr,"Error sending %d to pid %d:\t%s\n",theSignal,thePid,strerror(errno));
  }
  return retVal;
} 
