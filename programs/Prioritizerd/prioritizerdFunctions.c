#include "prioritizerdFunctions.h"
#include "anitaGpu.h"

void panicWriteAllLinks(int wd, int panicPri, int panicQueueLength, int priorityPPS1, int priorityPPS2){

  char linkFilename[FILENAME_MAX];
  char hdFilename[FILENAME_MAX];
  char bodyFilename[FILENAME_MAX];
  char telemHdFilename[FILENAME_MAX];
  char archiveHdFilename[FILENAME_MAX];
  char archiveBodyFilename[FILENAME_MAX];
  int numEventLinksAtPanic = getNumLinks(wd);
  if(numEventLinksAtPanic==0) return;
  if(numEventLinksAtPanic>panicQueueLength) {
    syslog(LOG_WARNING,"Prioritizerd queue has reached panicQueueLength=%d!\n", panicQueueLength);
    syslog(LOG_WARNING, "Trying to recover by writing %d priority 7 events!\n", numEventLinksAtPanic);
  }
  int doingEvent=0;
  int count=0;
  char* tempString;
  int lastEventNumber=0;

  for(count=0; count<numEventLinksAtPanic; count++){
    int numEventLinks=getNumLinks(wd);
    tempString=getFirstLink(wd);
    if(numEventLinks==0) break;
    if(tempString==NULL) continue; /* Just to be safe */
    sscanf(tempString,"hd_%d.dat",&doingEvent);
    if(lastEventNumber>0 && doingEvent!=lastEventNumber+1) {
      syslog(LOG_INFO,"Non-sequential event numbers %d and %d\n",
	     lastEventNumber,doingEvent);
    }
    lastEventNumber=doingEvent;

    sprintf(linkFilename,"%s/%s",EVENTD_EVENT_LINK_DIR,
	    tempString);
    sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
	    doingEvent);

    sprintf(bodyFilename,"%s/psev_%d.dat", ACQD_EVENT_DIR, doingEvent);

    //    fprintf(stderr,"Doing %s -- %s\n",linkFilename,hdFilename);

    AnitaEventHeader_t panicHeader;
    int retVal=fillHeader(&panicHeader,hdFilename);
    if(retVal!=0){
      fprintf(stderr, "fillHeader returned %d\n", retVal);
    }

    panicHeader.priority = panicPri;

    int pri=panicHeader.priority&0xf;
    if((panicHeader.turfio.trigType&0x2) && (priorityPPS1>=0 && priorityPPS1<=9))
      pri=priorityPPS1;
    if((panicHeader.turfio.trigType&0x4) && (priorityPPS2>=0 && priorityPPS2<=9))
      pri=priorityPPS2;
    if(pri<0 || pri>9) pri=9;
    panicHeader.priority=(16*panicHeader.priority)+pri;

    //Now Fill Generic Header and calculate checksum
    fillGenericHeader(&panicHeader,panicHeader.gHdr.code,sizeof(AnitaEventHeader_t));

    //Rename body and write header for Archived
    sprintf(archiveBodyFilename,"%s/psev_%u.dat",PRIORITIZERD_EVENT_DIR,
	    panicHeader.eventNumber);
    if(rename(bodyFilename,archiveBodyFilename)==-1)
      {
	syslog(LOG_ERR,"Error moving file %s -- %s",archiveBodyFilename,
	       strerror(errno));
      }

    sprintf(archiveHdFilename,"%s/hd_%u.dat",PRIORITIZERD_EVENT_DIR,
	    panicHeader.eventNumber);
    writeStruct(&panicHeader,archiveHdFilename,sizeof(AnitaEventHeader_t));

    makeLink(archiveHdFilename,PRIORITIZERD_EVENT_LINK_DIR);

    //Write Header and make Link for telemetry
    sprintf(telemHdFilename,"%s/hd_%d.dat",HEADER_TELEM_DIR,
	    panicHeader.eventNumber);
    retVal=writeStruct(&panicHeader,telemHdFilename,sizeof(AnitaEventHeader_t));
    if(retVal!=0){
      fprintf(stderr, "writeStruct returned %d\n", retVal);
    }
    makeLink(telemHdFilename,HEADER_TELEM_LINK_DIR);

    /* Delete input */
    sprintf(linkFilename,"%s/hd_%d.dat",EVENTD_EVENT_LINK_DIR,
	    panicHeader.eventNumber);
    sprintf(hdFilename,"%s/hd_%d.dat",EVENTD_EVENT_DIR,
	    panicHeader.eventNumber);
    removeFile(linkFilename);
    removeFile(hdFilename);
  }
}


void readInEvent(PedSubbedEventBody_t *psev, AnitaEventHeader_t* head, const char* dir, int eventNumber){

  char fileName[FILENAME_MAX];
  sprintf(fileName, "%s/psev_%d.dat.gz", dir, eventNumber);
  fillPedSubbedBody(psev, fileName);

  sprintf(fileName, "%s/hd_%d.dat.gz", dir, eventNumber);
  fillHeader(head, fileName);

  printf("Just read in event %d %d\n", head->eventNumber, psev->eventNumber);
}








/* // where the first event is n=0 */
/* void readInNthUnzippedEvent(int n, const char* psevFileName, PedSubbedEventBody_t *theBody, const char* headFileName, AnitaEventHeader_t* theHeader){ */

/*   /\* gzFile infile = gzopen (psevFileName, "rb"); *\/ */
/*   FILE* infile = fopen (psevFileName, "rb"); */

/*   int numElements; */
/*   /\* int i=0; *\/ */
/*   fseek(infile, n*sizeof(PedSubbedEventBody_t), SEEK_SET); */
/*   /\* for(i=0;i<100;i++) { *\/ */
/*     /\* numBytes=gzread(infile,&theBody[i],sizeof(PedSubbedEventBody_t)); *\/ */
/*     numElements=fread(&theBody[i],sizeof(PedSubbedEventBody_t), 1, infile); */
/*     if(numElements!=1) { */
/*       printf("%s\n", psevFileName); */
/*       fprintf(stderr, "Balls\n"); */
/*       break; */
/*     } */

/*     //RJN hack for now */
/*     /\* if(runNumber>=10000 && runNumber<=10057) { *\/ */
/*     /\*   if(counter>1 && i==1) { *\/ */
/*     /\* 	if(theBody.eventNumber!=lastEventNumber+1) { *\/ */
/*     /\* 	  gzrewind(infile); *\/ */
/*     /\* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); *\/ */
/*     /\* 	  gzread(infile,&theBody,616); *\/ */
/*     /\* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); *\/ */
/*     /\* 	} *\/ */
/*     /\*   } *\/ */
/*     /\* } *\/ */
/*     /\* int lastEventNumber=theBody.eventNumber; *\/ */
/*     /\* printf("i=%d\n", i); *\/ */
/*   /\* } *\/ */
/*   /\* gzclose(infile); *\/ */
/*   fclose(infile); */

/*   /\* infile = gzopen(headFileName, "rb"); *\/ */
/*   infile = fopen(headFileName, "rb"); */

/*   /\* int j=0; *\/ */
/*   /\* for(j=0;j<100;j++) { *\/ */
/*   fseek(infile, n*sizeof(AnitaEventHeader_t), SEEK_SET); */
/*     /\* int numBytesExpected=sizeof(AnitaEventHeader_t); *\/ */
/*     /\* if(version==VER_EVENT_HEADER) { *\/ */
/*       /\* numBytes=gzread(infile,&theHeader[j],sizeof(AnitaEventHeader_t)); *\/ */
/*     numElements=fread(&theHeader[j],sizeof(AnitaEventHeader_t), 1, infile); */
/*     /\* } *\/ */
/*     /\* if(numBytes==-1) { *\/ */
/*     /\*   int errorNum=0; *\/ */
/*     /\*   /\\* printf("%s\t%d\n", gzerror(infile,&errorNum), errorNum); *\\/ *\/ */
/*     /\* } *\/ */
/*     if(numElements!=1) { */
/*       fprintf(stderr, "Balls\n"); */
/*     } */
/*     /\* printf("j=%d\n", j); *\/ */
/*     /\* processHeader(version); *\/ */
/*   /\* } *\/ */
/*   /\* gzclose(infile); *\/ */
/*   fclose(infile); */

/* } */








void readIn100UnzippedEvents(const char* psevFileName, PedSubbedEventBody_t *theBody, const char* headFileName, AnitaEventHeader_t* theHeader){

  /* gzFile infile = gzopen (psevFileName, "rb"); */
  FILE* infile = fopen (psevFileName, "rb");

  int numElements;
  int i=0;
  for(i=0;i<100;i++) {
    /* numBytes=gzread(infile,&theBody[i],sizeof(PedSubbedEventBody_t)); */
    numElements=fread(&theBody[i],sizeof(PedSubbedEventBody_t), 1, infile);
    if(numElements!=1) {
      printf("Balls\t%d\t%s\n", numElements, psevFileName);
      /* fprintf(stderr, "Balls\n"); */
      break;
    }

    //RJN hack for now
    /* if(runNumber>=10000 && runNumber<=10057) { */
    /*   if(counter>1 && i==1) { */
    /* 	if(theBody.eventNumber!=lastEventNumber+1) { */
    /* 	  gzrewind(infile); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	  gzread(infile,&theBody,616); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	} */
    /*   } */
    /* } */
    /* int lastEventNumber=theBody.eventNumber; */
    /* printf("i=%d\n", i); */
  }
  /* gzclose(infile); */
  fclose(infile);

  /* infile = gzopen(headFileName, "rb"); */
  infile = fopen(headFileName, "rb");

  int j=0;
  for(j=0;j<100;j++) {
    int numBytesExpected=sizeof(AnitaEventHeader_t);
    /* if(version==VER_EVENT_HEADER) { */
      /* numBytes=gzread(infile,&theHeader[j],sizeof(AnitaEventHeader_t)); */
    numElements=fread(&theHeader[j],sizeof(AnitaEventHeader_t), 1, infile);
    /* } */
    /* if(numBytes==-1) { */
    /*   int errorNum=0; */
    /*   /\* printf("%s\t%d\n", gzerror(infile,&errorNum), errorNum); *\/ */
    /* } */
    if(numElements!=1) {
      printf("Balls\t%d\t%s\t%d\t%d\n", numElements, headFileName, ferror(infile), feof(infile));
      /* fprintf(stderr, "Balls\n"); */
    }
    /* printf("j=%d\n", j); */
    /* processHeader(version); */
  }
  /* gzclose(infile); */
  fclose(infile);

}


void assignCpuPriorities(int eventInd, double* finalVolts[], AnitaEventHeader_t* header){

  header[eventInd].priority = 6;

  int pol=0;
  for(pol=0; pol < NUM_POLARIZATIONS; pol++){
    int ant=0;
    for(ant=0; ant < NUM_ANTENNAS; ant++){
      int samp=0;
      const int baseInd = (pol*NUM_ANTENNAS + ant)*NUM_SAMPLES;
      for(samp=0; samp < NUM_SAMPLES; samp++){
	finalVolts[baseInd + samp];
      }
    }
  }


}



void readIn100Events(const char* psevFileName, PedSubbedEventBody_t *theBody, const char* headFileName, AnitaEventHeader_t* theHeader){

  gzFile infile = gzopen (psevFileName, "rb");

  int numBytes;
  int i=0;
  for(i=0;i<100;i++) {
    numBytes=gzread(infile,&theBody[i],sizeof(PedSubbedEventBody_t));
    if(numBytes!=sizeof(PedSubbedEventBody_t)) {
      if(numBytes>0) {
	fprintf(stderr, "Read problem: %d\t%d of %lu\n", i, numBytes, sizeof(PedSubbedEventBody_t));
	printf("%s\n", psevFileName);
      }
      fprintf(stderr, "Balls\n");
      break;
    }

    //RJN hack for now
    /* if(runNumber>=10000 && runNumber<=10057) { */
    /*   if(counter>1 && i==1) { */
    /* 	if(theBody.eventNumber!=lastEventNumber+1) { */
    /* 	  gzrewind(infile); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	  gzread(infile,&theBody,616); */
    /* 	  numBytes=gzread(infile,&theBody,sizeof(PedSubbedEventBody_t)); */
    /* 	} */
    /*   } */
    /* } */
    /* int lastEventNumber=theBody.eventNumber; */
    /* printf("i=%d\n", i); */
  }
  gzclose(infile);

  infile = gzopen(headFileName, "rb");

  int j=0;
  for(j=0;j<100;j++) {
    int numBytesExpected=sizeof(AnitaEventHeader_t);
    /* if(version==VER_EVENT_HEADER) { */
      numBytes=gzread(infile,&theHeader[j],sizeof(AnitaEventHeader_t));
    /* } */
    if(numBytes==-1) {
      int errorNum=0;
      printf("%s\t%d\n", gzerror(infile,&errorNum), errorNum);
    }
    if(numBytes!=numBytesExpected) {
      if(numBytes!=0) {
	break;
	fprintf(stderr, "Balls\n");
      }
      else break;
      fprintf(stderr, "Balls\n");
    }
    /* printf("j=%d\n", j); */
    /* processHeader(version); */
  }
  gzclose(infile);

}


void handleBadSigs(int sig)
{
  syslog(LOG_WARNING,"Received sig %d -- will exit immeadiately\n",sig);
  unlink(PRIORITIZERD_PID_FILE);
  tidyUpGpuThings(); // be more graceful
  syslog(LOG_INFO,"Prioritizerd terminating");
  exit(0);
}


int sortOutPidFile(char *progName)
{

  int retVal=checkPidFile(PRIORITIZERD_PID_FILE);
  if(retVal) {
    fprintf(stderr,"%s already running (%d)\nRemove pidFile to over ride (%s)\n",progName,retVal,PRIORITIZERD_PID_FILE);
    syslog(LOG_ERR,"%s already running (%d)\n",progName,retVal);
    return -1;
  }
  writePidFile(PRIORITIZERD_PID_FILE);
  return 0;
}
