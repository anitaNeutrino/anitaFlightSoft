/* SIPd - Program that talks to sip.
 *
 * Ryan Nichol, July '05
 * Started off as a modified version of Marty's driver program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>
#include <zlib.h>

/* Flight soft includes */
#include "sipcomLib/sipcom.h"
#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"
  
#define SLBUF_SIZE 240
// MAX_WRITE_RATE - maximum rate (bytes/sec) to write to SIP
#define MAX_WRITE_RATE	680L

#define SEND_REAL_SLOW_DATA 1

int getTdrssNumber();
void commandHandler(unsigned char *cmd);
void comm1Handler();
void comm2Handler();
void highrateHandler(int *ignore);
int readConfig();
void readAndSendEventRamdisk(char *headerLinkFilename);
void readAndSendEventRamdiskWavePackets(char *headerLinkFilename);
int checkLinkDirAndTdrss(int maxCopy, char *telemDir, char *linkDir, int fileSize);
void sendWakeUpBuffer();
void sendSomeHk(int maxBytes);

// Config Thingies
char sipdPidFile[FILENAME_MAX];
char lastTdrssNumberFile[FILENAME_MAX];
int cmdLengths[256];


//Packet Dirs
char eventTelemDirs[NUM_PRIORITIES][FILENAME_MAX];

//Packet Link Dirs
char eventTelemLinkDirs[NUM_PRIORITIES][FILENAME_MAX];


//Output variables
int verbosity;
int printToScreen;

// Bandwidth variables
int eventBandwidth=80;
int priorityBandwidths[NUM_PRIORITIES];
int priorityOrder[NUM_PRIORITIES*100];
int numOrders=1000;
int orderIndex=0;
int currentPri=0;
unsigned long eventDataSent=0;
unsigned long hkDataSent=0;

//Data buffer
unsigned char theBuffer[MAX_EVENT_SIZE*5];
unsigned char chanBuffer[MAX_EVENT_SIZE*5];


//High rate thread
pthread_t Hr_thread;
int throttleRate=MAX_WRITE_RATE;
int sendWavePackets=0;

//Low Rate Struct
SlowRateType1_t comm1Data;
SlowRateType1_t comm2Data;


int main(int argc, char *argv[])
{
    //Temporary variables
    int retVal,numCmds=256,count,pri;
    char *tempString;

    /* Config file thingies */
    int status=0;
    KvpErrorCode kvpStatus=0;
    char* eString ;
    
    char *progName=basename(argv[0]);
  
    /* Set signal handlers */
    signal(SIGUSR1, sigUsr1Handler);
    signal(SIGUSR2, sigUsr2Handler);

    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;


    //Zero low rate structs
    memset(&comm1Data,0,sizeof(SlowRateType1_t));
    memset(&comm2Data,0,sizeof(SlowRateType1_t));

   /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
    status += configLoad ("Cmdd.config","lengths") ;
    eString = configErrorString (status) ;

    if (status == CONFIG_E_OK) {
	kvpStatus=kvpGetIntArray ("cmdLengths",cmdLengths,&numCmds);
	
	//Get PID File location
	tempString=kvpGetString("sipdPidFile");
	if(tempString) {
	    strncpy(sipdPidFile,tempString,FILENAME_MAX);
	    writePidFile(sipdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get sipdPidFile");
	    fprintf(stderr,"Couldn't get sipdPidFile\n");
	}

	//Get TDRSS number file
	tempString=kvpGetString("lastTdrssNumberFile");
	if(tempString) {
	    strncpy(lastTdrssNumberFile,tempString,FILENAME_MAX);
	}
	else {
	    syslog(LOG_ERR,"Couldn't get lastLosNumberFile");
	    fprintf(stderr,"Couldn't get lastLosNumberFile\n");
	}

    }
    else {
	syslog(LOG_ERR,"Error reading config file: %s\n",eString);
	fprintf(stderr,"Error reading config file: %s\n",eString);
    }

    
    //Fill event dir names
    for(pri=0;pri<NUM_PRIORITIES;pri++) {
	sprintf(eventTelemDirs[pri],"%s/%s%d",BASE_EVENT_TELEM_DIR,
		EVENT_PRI_PREFIX,pri);
	sprintf(eventTelemLinkDirs[pri],"%s/link",eventTelemDirs[pri]);
	makeDirectories(eventTelemLinkDirs[pri]);
    }


    retVal=readConfig();
            
    retVal = sipcom_set_slowrate_callback(COMM1, comm1Handler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM1 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM1 Handler -- %s\n",s);
    }

    retVal = sipcom_set_slowrate_callback(COMM2, comm2Handler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM2 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM2 Handler -- %s\n",s);
    }

    sipcom_set_cmd_callback(commandHandler);
    if (retVal) {
	char *s = sipcom_strerror();
	syslog(LOG_ERR,"Couldn't set COMM2 Handler -- %s\n",s);
	fprintf(stderr,"Couldn't set COMM2 Handler -- %s\n",s);
    }
    for(count=0;count<numCmds;count++) {
	if(cmdLengths[count]) {
	    printf("%d\t%d\n",count,cmdLengths[count]);
	    sipcom_set_cmd_length(count,cmdLengths[count]);
	}
    }
    printf("Max Write Rate %ld\n",MAX_WRITE_RATE);
    retVal = sipcom_init(MAX_WRITE_RATE);
    if (retVal) {
	char *s = sipcom_strerror();
	fprintf(stderr, "%s\n", s);
	exit(1);
    }
    
     // Start the high rate writer process. 
    pthread_create(&Hr_thread, NULL, (void *)highrateHandler, NULL);
    pthread_detach(Hr_thread);


    sipcom_wait();
    pthread_cancel(Hr_thread);
    fprintf(stderr, "Bye bye\n");
    return 0;
}


void highrateHandler(int *ignore)
{
    char currentHeader[FILENAME_MAX];
    int currentPri=0;
    int numLinks=0;
    struct dirent **linkList;    

    {
        // We make this thread cancellable by any thread, at any time. This
        // should be okay since we don't have any state to undo or locks to
        // release.
        int oldtype;
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    }

    sendWakeUpBuffer();

    while(1) {
	currentPri=priorityOrder[orderIndex];	
	numLinks=getListofLinks(eventTelemLinkDirs[currentPri],&linkList);

	if(numLinks) {
	    //Got an event	    
	    sprintf(currentHeader,"%s/%s",eventTelemLinkDirs[currentPri],
		    linkList[numLinks-1]->d_name);
	    if(!sendWavePackets)
		readAndSendEventRamdisk(currentHeader); //Also deletes
	    else
		readAndSendEventRamdiskWavePackets(currentHeader); //Also deletes
		

	    while(numLinks) {
		free(linkList[numLinks-1]);
		numLinks--;
	    }
	    free(linkList);
		 
	}
	usleep(1);
	orderIndex++;
	if(orderIndex>=numOrders) orderIndex=0;

	//Need to think about housekeeping and add something here
	sendSomeHk(10000);
    }	

}


void commandHandler(unsigned char *cmd)
{
    fprintf(stderr, "commandHandler: cmd[0] = %02x (%d)\n", cmd[0], cmd[0]);
    CommandStruct_t theCmd;
    int byteNum=0;
    int retVal=0;
    theCmd.numCmdBytes=cmdLengths[cmd[0]];
    for(byteNum=0;byteNum<cmdLengths[cmd[0]];byteNum++) 
	theCmd.cmd[byteNum]=cmd[byteNum];
    
    retVal=writeCommandAndLink(&theCmd);

	       

/*     if (cmd[0] == 129) { */
/* 	fprintf(stderr, "DISABLE_DATA_COLL\n"); */
/*     } else if (cmd[0] == 131) { */
/* 	// Use this command to quit. */
/* 	fprintf(stderr, "QUIT CMD\n"); */
/* 	sipcom_end(); */
/*     } else if (cmd[0] == 138) { */
/* 	fprintf(stderr, "HV_PWR_ON\n"); */
/*     } else if (cmd[0] == 221) { */
/* 	// Use the MARK command to change the throttle rate. Oops, need to */
/* 	// tell the highrate writer process to change the rate. */
/* 	unsigned short mark = (cmd[2] << 8) | cmd[1]; */
/* 	fprintf(stderr, "MARK %u\n", mark); */
/* 	sipcom_highrate_set_throttle(mark); */
/*     } else { */
/* 	fprintf(stderr, "Unknown command = 0x%02x (%d)\n", cmd[0], cmd[0]); */
/*     } */
}

void comm1Handler()
{

    static unsigned char start = 0;
    unsigned char buf[SLBUF_SIZE + 6];
    int i;
    int ret;
    static unsigned char count = 0;

    buf[0] = 0xbe;
    buf[1] = 0xef;
    buf[2] = count;
    for (i=3; i<SLBUF_SIZE+3; i++) {
	buf[i] = start + i;
    }
    buf[SLBUF_SIZE+3] = 0xca;
    buf[SLBUF_SIZE+4] = 0xfe;
    buf[SLBUF_SIZE+5] = count;
    ++count;

    ++start;
    fprintf(stderr, "comm1Handler %02x %02x\n", count, start);

#ifdef SEND_REAL_SLOW_DATA
//    fprintf(stderr,"Last Temp %d\n",comm1Data.sbsTemp[0]);
    comm1Data.unixTime=time(NULL);
    fillGenericHeader(&comm1Data,PACKET_SLOW1,sizeof(SlowRateType1_t));
    printf("Comm1 %lu\n",comm1Data.unixTime);
    ret = sipcom_slowrate_write(COMM1, (unsigned char*)&comm1Data, sizeof(SlowRateType1_t));
#else
    ret = sipcom_slowrate_write(COMM1, buf, SLBUF_SIZE+6);
#endif
    if (ret) {
	fprintf(stderr, "comm1Handler: %s\n", sipcom_strerror());
    }
}

void comm2Handler()
{
    static unsigned char start = 255;
    unsigned char buf[SLBUF_SIZE];
    int i;
    int ret;

    for (i=0; i<SLBUF_SIZE; i++) {
	buf[i] = start - i;
    }
    --start;
    fprintf(stderr, "comm2Handler %02x\n", start);
    
#ifdef SEND_REAL_SLOW_DATA
    comm2Data.unixTime=time(NULL);
    fillGenericHeader(&comm2Data,PACKET_SLOW1,sizeof(SlowRateType1_t));
    ret = sipcom_slowrate_write(COMM2,(unsigned char*) &comm2Data, sizeof(SlowRateType1_t));
#else
    ret = sipcom_slowrate_write(COMM2, buf, SLBUF_SIZE);
#endif

    if (ret) {
	fprintf(stderr, "comm2Handler: %s\n", sipcom_strerror());
    }
}


int readConfig()
// Load SIPd config stuff
{
    // Config file thingies
    KvpErrorCode kvpStatus=0;
    int status=0,tempNum,count,maxVal,maxIndex;
    char* eString ;
    
    //Load output settings
    kvpReset();
    status = configLoad ("SIPd.config","output") ;
    if(status == CONFIG_E_OK) {
	printToScreen=kvpGetInt("printToScreen",-1);
	verbosity=kvpGetInt("verbosity",-1);
	if(printToScreen<0) {
	    printf("Couldn't fetch printToScreen, defaulting to zero\n");
	    printToScreen=0;
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    //Load SIPd specfic settings
    kvpReset();
    status = configLoad ("SIPd.config","sipd");
    if(status == CONFIG_E_OK) {
	throttleRate=kvpGetInt("throttleRate",680);
	sendWavePackets=kvpGetInt("sendWavePackets",0);
	syslog(LOG_INFO,"sendWavePackets %d\n",sendWavePackets);
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    //Load bandwidth settings
    kvpReset();
    status = configLoad ("SIPd.config","bandwidth") ;
    if(status == CONFIG_E_OK) {
//	maxEventsBetweenLists=kvpGetInt("maxEventsBetweenLists",100);
	eventBandwidth=kvpGetInt("eventBandwidth",80);
	tempNum=10;
	kvpStatus = kvpGetIntArray("priorityBandwidths",priorityBandwidths,&tempNum);
	if(kvpStatus!=KVP_E_OK) {
	    syslog(LOG_WARNING,"kvpGetIntArray(priorityBandwidths): %s",
		   kvpErrorString(kvpStatus));
	    if(printToScreen)
		fprintf(stderr,"kvpGetIntArray(priorityBandwidths): %s\n",
			kvpErrorString(kvpStatus));
	}
	else {
	    for(orderIndex=0;orderIndex<NUM_PRIORITIES*100;orderIndex++) {
		//Check for hundreds first
		for(count=0;count<tempNum;count++) {
		    if(priorityBandwidths[count]>=100) 
			priorityOrder[orderIndex++]=count;
		}
		
		//Next look for highest number
		maxVal=0;
		maxIndex=-1;
		for(count=0;count<tempNum;count++) {
		    if(priorityBandwidths[count]<100) {
			if(priorityBandwidths[count]>maxVal) {
			    maxVal=priorityBandwidths[count];
			    maxIndex=count;
			}
		    }
		}
		if(maxIndex>-1) {
		    priorityOrder[orderIndex]=maxIndex;
		    priorityBandwidths[maxIndex]--;
		}
		else break;
	    }			
	    numOrders=orderIndex;
	    if(printToScreen) {
		printf("Priority Order\n");
		for(orderIndex=0;orderIndex<numOrders;orderIndex++) {
		    printf("%d ",priorityOrder[orderIndex]);
		}
		printf("\n");
	    }
	}
    }
    else {
	eString=configErrorString (status) ;
	syslog(LOG_ERR,"Error reading SIPd.config: %s\n",eString);
	fprintf(stderr,"Error reading SIPd.config: %s\n",eString);
    }

    return status;
}



void readAndSendEvent(char *headerFilename) {
    AnitaEventHeader_t *theHeader;
    EncodedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,useGzip=0,retVal=0;
    char waveFilename[FILENAME_MAX];
    FILE *eventFile;
    gzFile gzippedEventFile=0;

    if(printToScreen && verbosity) printf("Trying file %s\n",headerFilename);

    //Fitst load header
    theHeader=(AnitaEventHeader_t*) theBuffer;
    fillHeader(theHeader,headerFilename);
    theHeader->gHdr.packetNumber=getTdrssNumber();
    retVal = sipcom_highrate_write(theBuffer,sizeof(AnitaEventHeader_t));
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());
	fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());	
    }
    eventDataSent+=sizeof(AnitaEventHeader_t);


    //Now get event file
    sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri],
	    theHeader->eventNumber);
    eventFile = fopen(waveFilename,"r");
    if(!eventFile) {
	sprintf(waveFilename,"%s/ev_%ld.dat.gz",eventTelemDirs[currentPri],
		theHeader->eventNumber);
	gzippedEventFile = gzopen(waveFilename,"rb");
	if(!gzippedEventFile) {
	    syslog(LOG_ERR,"Couldn't open %s -- %s",waveFilename,strerror(errno));
	    fprintf(stderr,"Couldn't open %s -- %s\n",waveFilename,strerror(errno));	
	    removeFile(headerFilename);
	    removeFile(waveFilename);
	    return;
	}
	useGzip=1;
    }

    if(useGzip) {
	numBytes = gzread(gzippedEventFile,theBuffer,MAX_EVENT_SIZE);
	gzclose(gzippedEventFile);	
    }
    else {
	numBytes = fread(theBuffer,1,MAX_EVENT_SIZE,eventFile);
	fclose(eventFile);
    }
    
    // Remember what the file contains is actually:
    //   9 EncodedSurfPacketHeader_t's
    count=0;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &theBuffer[count];
	surfHdPtr->gHdr.packetNumber=getTdrssNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	    retVal = sipcom_highrate_write(theBuffer, numBytes);
	    if(retVal<0) {
		//Problem sending data
		syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());
		fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());	
	    }
	    eventDataSent+=numBytes;
	}
	else break;			
    }
    if(printToScreen && verbosity>1) printf("Removing files %s\t%s\n",headerFilename,waveFilename);
	        
    removeFile(headerFilename);
    removeFile(waveFilename);

}


void readAndSendEventRamdisk(char *headerLinkFilename) {
    AnitaEventHeader_t *theHeader;
    EncodedSurfPacketHeader_t *surfHdPtr;
    int numBytes,count=0,surf=0,retVal;
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char waveFilename[FILENAME_MAX];
    char headerFilename[FILENAME_MAX];
    char crapBuffer[FILENAME_MAX];
    unsigned long thisEventNumber;

    sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
	    thisEventNumber);
    sprintf(currentTouchname,"%s.sipd",headerFilename);
    sprintf(currentLOSTouchname,"%s.losd",headerFilename);


    if(checkFileExists(currentLOSTouchname)) 
	return;
    touchFile(currentTouchname);

//     Next load header 
    theHeader=(AnitaEventHeader_t*) &theBuffer[0]; 
    retVal=fillHeader(theHeader,headerLinkFilename); 
        
    if(retVal<0) {
	removeFile(headerLinkFilename);
	sscanf(headerLinkFilename,"%s/hd_%lu.dat",crapBuffer,
	       &thisEventNumber);
	sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(headerFilename);
	sprintf(waveFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(waveFilename);
	
	//Bollocks
	return;
    }


    theHeader->gHdr.packetNumber=getTdrssNumber();
    retVal = sipcom_highrate_write(theBuffer,sizeof(AnitaEventHeader_t));
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());
	fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());	
    }
    eventDataSent+=sizeof(AnitaEventHeader_t);
    
    
    thisEventNumber=theHeader->eventNumber;
    

    //Now get event file
    sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);
    sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);


    retVal=genericReadOfFile((char*)theBuffer,waveFilename,MAX_EVENT_SIZE);
    if(retVal<0) {
	fprintf(stderr,"Problem reading %s\n",waveFilename);
	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);	
	//Bollocks
	return;
    }


    // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's
    count=0;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &theBuffer[count];
	surfHdPtr->gHdr.packetNumber=getTdrssNumber();
	numBytes = surfHdPtr->gHdr.numBytes;
	if(numBytes) {
	    retVal = sipcom_highrate_write(&theBuffer[count],numBytes);
	    if(retVal<0) {
		//Problem sending data
		syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());
		fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());	
	    }
	    count+=numBytes;
	    eventDataSent+=numBytes;
	}
	else break;
    }
    
    if(printToScreen && verbosity>1) 
	printf("Removing files %s\t%s\n%s\n",headerFilename,waveFilename,
	       headerLinkFilename);
	        
    if(!checkFileExists(currentLOSTouchname)) {
	removeFile(headerFilename);
	removeFile(headerLinkFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
    }
    else {
	sleep(1);
	removeFile(headerFilename);
	removeFile(headerLinkFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
	removeFile(currentLOSTouchname);
    }

    comm1Data.lastEventNumber=thisEventNumber;
    comm2Data.lastEventNumber=thisEventNumber;

}


void readAndSendEventRamdiskWavePackets(char *headerLinkFilename) {
    AnitaEventHeader_t *theHeader;
    EncodedSurfPacketHeader_t *surfHdPtr;
    EncodedSurfChannelHeader_t *chanHdPtr;    
    EncodedWaveformPacket_t *wavPtr;
    int numBytes,count=0,surf=0,retVal,chan;
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char waveFilename[FILENAME_MAX];
    char headerFilename[FILENAME_MAX];
    char crapBuffer[FILENAME_MAX];
    unsigned long thisEventNumber;
    unsigned char *tempBuffer;


    sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
	    thisEventNumber);
    sprintf(currentTouchname,"%s.sipd",headerFilename);
    sprintf(currentLOSTouchname,"%s.losd",headerFilename);


    if(checkFileExists(currentLOSTouchname)) 
	return;
    touchFile(currentTouchname);

//     Next load header 
    theHeader=(AnitaEventHeader_t*) &theBuffer[0]; 
    retVal=fillHeader(theHeader,headerLinkFilename); 
        
    if(retVal<0) {
	removeFile(headerLinkFilename);
	sscanf(headerLinkFilename,"%s/hd_%lu.dat",crapBuffer,
	       &thisEventNumber);
	sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(headerFilename);
	sprintf(waveFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
		thisEventNumber);
	removeFile(waveFilename);
	
	//Bollocks
	return;
    }


    theHeader->gHdr.packetNumber=getTdrssNumber();
    retVal = sipcom_highrate_write(theBuffer,sizeof(AnitaEventHeader_t));
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());
	fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",headerFilename,sipcom_strerror());	
    }
    eventDataSent+=sizeof(AnitaEventHeader_t);
    
    
    thisEventNumber=theHeader->eventNumber;
    

    //Now get event file
    sprintf(headerFilename,"%s/hd_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);
    sprintf(waveFilename,"%s/ev_%ld.dat",eventTelemDirs[currentPri], 
 	    thisEventNumber);


    retVal=genericReadOfFile((char*)theBuffer,waveFilename,MAX_EVENT_SIZE);
    if(retVal<0) {
	fprintf(stderr,"Problem reading %s\n",waveFilename);
	removeFile(headerLinkFilename);
	removeFile(headerFilename);
	removeFile(waveFilename);	
	//Bollocks
	return;
    }


    // Remember what the file contains is actually 9 EncodedSurfPacketHeader_t's

    count=0;
    for(surf=0;surf<ACTIVE_SURFS;surf++) {
	surfHdPtr = (EncodedSurfPacketHeader_t*) &theBuffer[count];
        count+=sizeof(EncodedSurfPacketHeader_t);
        for(chan=0;chan<CHANNELS_PER_SURF;chan++) {
          //      cout << count << "\t" << numBytes << endl;
	    wavPtr = (EncodedWaveformPacket_t*) &chanBuffer[0];
            chanHdPtr = (EncodedSurfChannelHeader_t*) &theBuffer[count];
            count+=sizeof(EncodedSurfChannelHeader_t);
	    
	    wavPtr->eventNumber=surfHdPtr->eventNumber;
	    if(wavPtr->eventNumber!=thisEventNumber) {
		printf("Something horribly wrong %lu is not %lu\n",wavPtr->eventNumber,thisEventNumber);
		syslog(LOG_ERR,"Something horribly wrong %lu is not %lu\n",wavPtr->eventNumber,thisEventNumber);

		removeFile(headerLinkFilename);
		removeFile(headerFilename);
		removeFile(waveFilename);	
		//Bollocks
		return;
	    }
	    wavPtr->chanHead=(*chanHdPtr);
	    memcpy(&chanBuffer[sizeof(EncodedWaveformPacket_t)],&theBuffer[count],chanHdPtr->numBytes);
	    tempBuffer = (unsigned char*) wavPtr;
//	    syslog(LOG_INFO,"1st 3 %d %d %d and %d %d %d\n",
//		   tempBuffer[sizeof(EncodedWaveformPacket_t)+0],
//		   tempBuffer[sizeof(EncodedWaveformPacket_t)+1],
//		   tempBuffer[sizeof(EncodedWaveformPacket_t)+2],
//		   theBuffer[count+0],
//		   theBuffer[count+1],
//		   theBuffer[count+2]);
		   
            count+=chanHdPtr->numBytes;	
	    numBytes=chanHdPtr->numBytes+sizeof(EncodedWaveformPacket_t);
	    fillGenericHeader(wavPtr,PACKET_ENC_WV,numBytes);
//	    syslog(LOG_INFO,"wavPtr info %d %d %d\n",wavPtr->gHdr.code,wavPtr->gHdr.numBytes,wavPtr->gHdr.checksum);
	    wavPtr->gHdr.packetNumber=getTdrssNumber();
	     
	    if(numBytes) {
		retVal = sipcom_highrate_write(wavPtr,
					       numBytes);
		if(retVal<0) {
		    //Problem sending data
		    syslog(LOG_ERR,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());
		    fprintf(stderr,"Problem sending file %s over TDRSS high rate -- %s\n",waveFilename,sipcom_strerror());	
		}
//		count+=numBytes;
		eventDataSent+=numBytes;
	    }
	    else break;
	    
        }
    }
    //    exit(0);

    
    if(printToScreen && verbosity>1) 
	printf("Removing files %s\t%s\n%s\n",headerFilename,waveFilename,
	       headerLinkFilename);
	        
    if(!checkFileExists(currentLOSTouchname)) {
	removeFile(headerFilename);
	removeFile(headerLinkFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
    }
    else {
	sleep(1);
	removeFile(headerFilename);
	removeFile(headerLinkFilename);
	removeFile(waveFilename);
	removeFile(currentTouchname);
	removeFile(currentLOSTouchname);
    }

    comm1Data.lastEventNumber=thisEventNumber;
    comm2Data.lastEventNumber=thisEventNumber;

}



int getTdrssNumber() {
    int retVal=0;
    static int firstTime=1;
    static int tdrssNumber=0;
    /* This is just to get the lastTdrssNumber in case of program restart. */
    FILE *pFile;
    if(firstTime) {
	pFile = fopen (lastTdrssNumberFile, "r");
	if(pFile == NULL) {
	    syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		    lastTdrssNumberFile);
	}
	else {	    	    
	    retVal=fscanf(pFile,"%d",&tdrssNumber);
	    if(retVal<0) {
		syslog (LOG_ERR,"fscanff: %s ---  %s\n",strerror(errno),
			lastTdrssNumberFile);
	    }
	    fclose (pFile);
	}
	if(printToScreen) printf("The last tdrss number is %d\n",tdrssNumber);
	firstTime=0;
    }
    tdrssNumber++;

    pFile = fopen (lastTdrssNumberFile, "w");
    if(pFile == NULL) {
	syslog (LOG_ERR,"fopen: %s ---  %s\n",strerror(errno),
		lastTdrssNumberFile);
    }
    else {
	retVal=fprintf(pFile,"%d\n",tdrssNumber);
	if(retVal<0) {
	    syslog (LOG_ERR,"fprintf: %s ---  %s\n",strerror(errno),
		    lastTdrssNumberFile);
	    }
	fclose (pFile);
    }

    return tdrssNumber;

}

void sendWakeUpBuffer() 
{
    GenericHeader_t *gHdr = (GenericHeader_t*) &theBuffer[0];
    int count,retVal=0;
    for(count=sizeof(GenericHeader_t);count<WAKEUP_TDRSS_BUFFER_SIZE-sizeof(GenericHeader_t);count++) {
	theBuffer[count]=0xfe;
    }
    fillGenericHeader(gHdr,PACKET_WAKEUP_HIGHRATE,WAKEUP_TDRSS_BUFFER_SIZE);
    gHdr->packetNumber=getTdrssNumber();
    retVal = sipcom_highrate_write(theBuffer, count);
    if(retVal<0) {
	//Problem sending data
	syslog(LOG_ERR,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());
	fprintf(stderr,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());	
    }
    
}

void sendSomeHk(int maxBytes) 
{
    int hkCount=0;

    if((maxBytes-hkCount)>sizeof(CommandEcho_t)) {
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,
		     CMD_ECHO_TELEM_DIR,CMD_ECHO_TELEM_LINK_DIR,
		     sizeof(CommandEcho_t)); 
    }
    if((maxBytes-hkCount)>sizeof(MonitorStruct_t)) {
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,
		     MONITOR_TELEM_DIR,MONITOR_TELEM_LINK_DIR,
		     sizeof(MonitorStruct_t)); 
    }
    if((maxBytes-hkCount)>sizeof(GpsAdu5SatStruct_t)) {
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,GPS_TELEM_DIR,
		     GPS_TELEM_LINK_DIR,sizeof(GpsAdu5SatStruct_t)); 
    }
    if((maxBytes-hkCount)>sizeof(HkDataStruct_t)) {
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,HK_TELEM_DIR,
		     HK_TELEM_LINK_DIR,sizeof(HkDataStruct_t)); 
    }
    if((maxBytes-hkCount)>sizeof(FullSurfHkStruct_t)) {
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,SURFHK_TELEM_DIR,
		     SURFHK_TELEM_LINK_DIR,sizeof(FullSurfHkStruct_t)); 
    }
    if((maxBytes-hkCount)>sizeof(TurfRateStruct_t)) {	
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,TURFHK_TELEM_DIR,
		     TURFHK_TELEM_LINK_DIR,sizeof(TurfRateStruct_t)); 
    }
    if((maxBytes-hkCount)>sizeof(AnitaEventHeader_t)) {	
	hkCount+=checkLinkDirAndTdrss(maxBytes-hkCount,HEADER_TELEM_DIR,
		     HEADER_TELEM_LINK_DIR,sizeof(AnitaEventHeader_t));
    }        

    hkDataSent+=hkCount;
    
}


int checkLinkDirAndTdrss(int maxCopy, char *telemDir, char *linkDir, int fileSize) 
/* Looks in the specified directroy and TDRSS's up to maxCopy bytes of data */
/* fileSize is the maximum size of a packet in the directory */
{
    char currentFilename[FILENAME_MAX];
    char currentTouchname[FILENAME_MAX];
    char currentLOSTouchname[FILENAME_MAX];
    char currentLinkname[FILENAME_MAX];
    int retVal,numLinks,count,numBytes,totalBytes=0;//,checkVal=0;
    GenericHeader_t *gHdr;
    struct dirent **linkList;
    
    GpsAdu5PatStruct_t *patPtr;
    HkDataStruct_t *hkPtr;


    numLinks=getListofLinks(linkDir,&linkList); 
    if(numLinks<=0) {
	return 0;
    }
        
    for(count=numLinks-1;count>=0;count--) {
	sprintf(currentFilename,"%s/%s",telemDir,
		linkList[count]->d_name);
	sprintf(currentTouchname,"%s.sipd",currentFilename);
	sprintf(currentLOSTouchname,"%s.losd",currentFilename);
	sprintf(currentLinkname,"%s/%s",
		linkDir,linkList[count]->d_name);

	if(checkFileExists(currentLOSTouchname)) 
	    continue;
	touchFile(currentTouchname);

	retVal=genericReadOfFile((char*)theBuffer,
				 currentFilename,
				 MAX_EVENT_SIZE);
	if(retVal<=0) {
	    syslog(LOG_ERR,"Error opening file, will delete: %s",
		   currentFilename);
	    fprintf(stderr,"Error reading file %s -- %d\n",currentFilename,retVal);
	    removeFile(currentFilename);

	    removeFile(currentLinkname);
	    removeFile(currentTouchname);
	    continue;
	}
	numBytes=retVal;

	if(printToScreen && verbosity>1) {
	    printf("Read File: %s -- (%d bytes)\n",currentFilename,numBytes);
	}
	

//	printf("Read %d bytes from file\n",numBytes);
//	Maybe I'll add a packet check here
	gHdr = (GenericHeader_t*)theBuffer;
//	checkVal=checkPacket(gHdr);
//	if(checkVal!=0 ) {
//	    printf("Bad packet %s == %d\n",currentFilename,checkVal);
//	}
	gHdr->packetNumber=getTdrssNumber();
	retVal = sipcom_highrate_write(theBuffer, numBytes);
	if(retVal<0) {
	    //Problem sending data
	    syslog(LOG_ERR,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());
	    fprintf(stderr,"Problem sending Wake up Packet over TDRSS high rate -- %s\n",sipcom_strerror());	
	}
	
	totalBytes+=numBytes;

	if(!checkFileExists(currentLOSTouchname)) {
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	    removeFile(currentTouchname);
	}
	else {
	    sleep(1);
	    removeFile(currentLinkname);
	    removeFile(currentFilename);
	    removeFile(currentTouchname);
	    removeFile(currentLOSTouchname);
	}

	//Now fill low rate structs
	switch (gHdr->code) {
	    case PACKET_GPS_ADU5_PAT:
		patPtr=(GpsAdu5PatStruct_t*)gHdr;
		comm1Data.altitude=patPtr->altitude;
		comm2Data.altitude=patPtr->altitude;
		comm1Data.longitude=patPtr->longitude;
		comm2Data.longitude=patPtr->longitude;
		comm1Data.latitude=patPtr->latitude;
		comm2Data.latitude=patPtr->latitude;
		break;
	    case PACKET_HKD:
		hkPtr=(HkDataStruct_t*)gHdr;
		comm1Data.sbsTemp[0]=hkPtr->sbs.temp[0];
		comm2Data.sbsTemp[0]=hkPtr->sbs.temp[0];
		comm1Data.sbsTemp[1]=hkPtr->sbs.temp[1];
		comm2Data.sbsTemp[1]=hkPtr->sbs.temp[1];
	    default:
		break;
	}

	if((totalBytes+fileSize)>maxCopy) break;
	break;
    }
    
    for(count=0;count<numLinks;count++)
	free(linkList[count]);
    free(linkList);
    
    return totalBytes;
}
