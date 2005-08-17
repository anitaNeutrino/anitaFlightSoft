/*! \file Prioritizerd.c
    \brief The Prioritizerd program that creates Event objects 
    
    Reads the event objects written by Eventd, assigns a priority to each event based on the likelihood that it is an interesting event. Events with the highest priority will be transmitted to the ground first.
    March 2005 rjn@mps.ohio-state.edu
*/
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>


#include "anitaFlight.h"
#include "configLib/configLib.h"
#include "kvpLib/keyValuePair.h"
#include "utilLib/utilLib.h"
#include "anitaStructures.h"


/* Structure defininitions */

typedef struct {
    float re; 
    float im; 
} complex;  /* for FFT */

typedef struct {
    float data[MAX_NUMBER_SAMPLES];
} channel_volts;

typedef struct {
    short numChannels;
    short numSamples;
    channel_volts channel[NUM_DIGITZED_CHANNELS];
} event_volts;

/* Function Definitions */

void writePackets(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr); 

/* int setPriority(AnitaEventHeader_t *headerPtr, AnitaEventBody_t *bodyPtr); */
/* void convertToVoltageAndGetStats(AnitaEventBody_t *bodyPtr, event_volts *voltsPtr, double means[], double rmss[], int peakBins[], int trigPeakBins[], int trigBin, int trigHalfWindow); */
/* void nrFFTForward(complex data[], int nn); */
/* void nrFFT(float data[], int nn, int isign); */
/* void getPowerSpec(event_volts *voltsPtr, float *pwr[], float *freq, float fs1); */
/* int checkPayloadFrequencies( float *power[], float freq[], int numChannels, int numFreqs); */
/* int writeOutputFiles(AnitaEventHeader_t *hdPtr, AnitaEventBody_t *bodyPtr, char eventDir[], char backupDir[], char linkBaseDir[], float probWriteOut9); */


/* Directories and gubbins */
char eventdEventDir[FILENAME_MAX];
char eventdEventLinkDir[FILENAME_MAX];
char prioritizerdSipdDir[FILENAME_MAX];
char prioritizerdSipdLinkDir[FILENAME_MAX];
char prioritizerdArchiveDir[FILENAME_MAX];
char prioritizerdArchiveLinkDir[FILENAME_MAX];
char prioritizerdPidFile[FILENAME_MAX];

int main (int argc, char *argv[])
{
    int retVal,count;
    char linkFilename[FILENAME_MAX];
    char hdFilename[FILENAME_MAX];
    char bodyFilename[FILENAME_MAX];
    char sipdHdFilename[FILENAME_MAX];
    char archivedHdFilename[FILENAME_MAX];

    char *tempString;
//    int priority;
//    float probWriteOut9=0.03; /* Will become a config file thingy */

    /* Config file thingies */
    int status=0;
    int doingEvent=0;
    //    char* eString ;

    /* Directory reading things */
    struct dirent **eventLinkList;
    int numEventLinks;
    
    /* Log stuff */
    char *progName=basename(argv[0]);

   /*Event object*/
    AnitaEventHeader_t theEventHeader;
    AnitaEventBody_t theEventBody;
    	    
    /* Setup log */
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog (progName, LOG_PID, ANITA_LOG_FACILITY) ;
   
    /* Load Config */
    kvpReset () ;
    status = configLoad (GLOBAL_CONF_FILE,"global") ;
/*     eString = configErrorString (status) ; */

    if (status == CONFIG_E_OK) {
	tempString=kvpGetString("prioritizerdPidFile");
	if(tempString) {
	    strncpy(prioritizerdPidFile,tempString,FILENAME_MAX-1);
	    writePidFile(prioritizerdPidFile);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdPidFile");
	    fprintf(stderr,"Error getting prioritizerdPidFile\n");
	}
	tempString=kvpGetString("eventdEventDir");
	if(tempString) {
	    strncpy(eventdEventDir,tempString,FILENAME_MAX-1);
	    makeDirectories(eventdEventDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting eventdEventDir");
	    fprintf(stderr,"Error getting eventdEventDir\n");
	}
	tempString=kvpGetString("eventdEventLinkDir");
	if(tempString) {
	    strncpy(eventdEventLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(eventdEventLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting eventdEventLinkDir");
	    fprintf(stderr,"Error getting eventdEventLinkDir\n");
	}
	tempString=kvpGetString("prioritizerdSipdDir");
	if(tempString) {
	    strncpy(prioritizerdSipdDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdDir");
	    fprintf(stderr,"Error getting prioritizerdSipdDir\n");
	}
	tempString=kvpGetString("prioritizerdSipdLinkDir");
	if(tempString) {
	    strncpy(prioritizerdSipdLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdSipdLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdSipdLinkDir");
	    fprintf(stderr,"Error getting prioritizerdSipdLinkDir\n");
	}
	tempString=kvpGetString("prioritizerdArchiveDir");
	if(tempString) {
	    strncpy(prioritizerdArchiveDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdArchiveDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdArchiveDir");
	    fprintf(stderr,"Error getting prioritizerdArchiveDir\n");
	}
	tempString=kvpGetString("prioritizerdArchiveLinkDir");
	if(tempString) {
	    strncpy(prioritizerdArchiveLinkDir,tempString,FILENAME_MAX-1);
	    makeDirectories(prioritizerdArchiveLinkDir);
	}
	else {
	    syslog(LOG_ERR,"Error getting prioritizerdArchiveLinkDir");
	    fprintf(stderr,"Error getting prioritizerdArchiveLinkDir\n");
	}
    }
    


    
    retVal=0;
    /* Main event getting loop. */
    while(1) {
	numEventLinks=getListofLinks(eventdEventLinkDir,&eventLinkList);

	/* What to do with our events? */	
	for(count=0;count<numEventLinks;count++) {
/* 	    printf("%s\n",eventLinkList[count]->d_name); */
	    sscanf(eventLinkList[count]->d_name,"hd_%d.dat",&doingEvent);
	    sprintf(linkFilename,"%s/%s",eventdEventLinkDir,
		    eventLinkList[count]->d_name);
	    sprintf(hdFilename,"%s/hd_%d.dat",eventdEventDir,
		    doingEvent);
	    sprintf(bodyFilename,"%s/ev_%d.dat",eventdEventDir,
		    doingEvent);
	    sprintf(sipdHdFilename,"%s/hd_%d.dat",prioritizerdSipdDir,
		    doingEvent);
	    sprintf(archivedHdFilename,"%s/hd_%d.dat",prioritizerdArchiveDir,
		    doingEvent);
	    

	    retVal=fillBody(&theEventBody,bodyFilename);
	    retVal=fillHeader(&theEventHeader,hdFilename);
	    
	    /* Write output for SIPd*/	    
	    writePackets(&theEventBody,&theEventHeader);

	    //Copy and link header
	    copyFile(hdFilename,prioritizerdSipdDir);
	    makeLink(sipdHdFilename,prioritizerdSipdLinkDir);

	    /*Write output for Archived*/
	    copyFile(hdFilename,prioritizerdArchiveDir);
	    copyFile(bodyFilename,prioritizerdArchiveDir);
	    makeLink(archivedHdFilename,prioritizerdArchiveLinkDir);


	    /* Delete input */
	    removeFile(linkFilename);
	    removeFile(bodyFilename);
	    removeFile(hdFilename);
	}
	
	/* Free up the space used by dir queries */
        for(count=0;count<numEventLinks;count++)
            free(eventLinkList[count]);
        free(eventLinkList);
	usleep(10000);
    }	
}

void writePackets(AnitaEventBody_t *bodyPtr, AnitaEventHeader_t *hdPtr) 
{
    int chan;
    char packetName[FILENAME_MAX];
    WaveformPacket_t wavePacket;
    wavePacket.gHdr.code=PACKET_WV;
    for(chan=0;chan<hdPtr->numChannels;chan++) {
	sprintf(packetName,"%s/wvpk_%d_%d.dat",prioritizerdSipdDir,hdPtr->eventNumber,chan);
	wavePacket.eventNumber=hdPtr->eventNumber;
	wavePacket.packetNumber=chan;
	memcpy(&(wavePacket.waveform),&(bodyPtr->channel[chan]),sizeof(SurfChannelFull_t));
    }
    writeWaveformPacket(&wavePacket,packetName);
}


/* int writeOutputFiles(AnitaEventHeader_t *hdPtr, AnitaEventBody_t *bodyPtr, char eventDir[], char backupDir[], char linkBaseDir[], float probWriteOut9) */
/* /\* Writes out upto two versions of the header and body files, and makes links for the telemetry programs. *\/ */
/* { */
/*     char hdName[FILENAME_MAX]; */
/*     char bodyName[FILENAME_MAX]; */
/*     char linkDir[FILENAME_MAX]; */
/*     int writeOutEvent=0;    */
    
/*     /\* Generate a random number to decide if we'll write out */
/*        the priority 9 event *\/ */
/*     if(hdPtr->priority<9) writeOutEvent=1; */
/*     else writeOutEvent =(drand48() <= probWriteOut9) ? 1 : 0; */
    
/*     /\* Write Header *\/ */
/*     sprintf(hdName,"%s/hd_%d.dat",eventDir,hdPtr->eventNumber); */
/*     writeHeader(hdPtr,hdName); */
/*     if(writeOutEvent) { */
/* 	/\* Write Body *\/ */
/* 	sprintf(bodyName,"%s/ev_%d.dat",eventDir,hdPtr->eventNumber); */
/* 	writeBody(bodyPtr,bodyName); */
/*     } */
    
/*     /\* Make Links *\/ */
/*     sprintf(linkDir,"%s/%d/",linkBaseDir,hdPtr->priority); */
/*     makeLink(hdName,linkDir); */
/*     if(writeOutEvent) makeLink(bodyName,linkDir); */

/*     /\* Write Backups *\/ */
/*     sprintf(hdName,"%s/hd_%d.dat",backupDir,hdPtr->eventNumber); */
/*     writeHeader(hdPtr,hdName); */
/*     if(writeOutEvent) { */
/* 	sprintf(bodyName,"%s/ev_%d.dat",backupDir,hdPtr->eventNumber); */
/* 	writeBody(bodyPtr,bodyName); */
/*     } */
/*     return 0; */
/* } */

/* int setPriority(AnitaEventHeader_t *headerPtr, AnitaEventBody_t *bodyPtr) */
/* /\* Argh not quite sure what to do here *\/ */
/* { */
/*     event_volts voltEvent; */
/*     double theMeans[NUM_DIGITZED_CHANNELS]; */
/*     double theRMSs[NUM_DIGITZED_CHANNELS]; */
/*     int peakBins[NUM_DIGITZED_CHANNELS]; */
/*     int trigPeakBins[NUM_DIGITZED_CHANNELS]; */
/*     float *power[NUM_DIGITZED_CHANNELS]; */
/*     float *freqArray; */
/*     float sampFreq=1./3e-9;; */
/*     int np2,numPoints,chan,trigBin=100,trigHalfWindow,numPeaksMatch; */
/*     int expectedPeaks; */

/*     voltEvent.numSamples=headerPtr->numSamples; */
/*     voltEvent.numChannels=headerPtr->numChannels; */
/* //    trigBin=TRIGGER_OFFSET+headerPtr->trigDelay; /\* Got to work out sign convention *\/ */
/*     trigHalfWindow=30; /\* Made up number, should maybe be configurable *\/ */
/*     convertToVoltageAndGetStats(bodyPtr,&voltEvent,theMeans,theRMSs,peakBins,trigPeakBins,trigBin,trigHalfWindow); */

/*     /\* What can we do without having to calculate FFTS? *\/ */
/*     if(headerPtr->calibStatus!=0)  */
/* 	return headerPtr->priority=PRI_CALIB; */
/*     //  if(headerPtr->Dtype<0)  */
/* //	return headerPtr->priority=PRI_TIMEOUT; */
    
/*     /\* Get ready to get power spectra *\/ */
/*     np2=(int)(log10(headerPtr->numSamples)/log10(2));  */
/*     numPoints = 1 << np2; /\* An integer power of two (probably was anyhow) *\/ */
/*     for(chan=0;chan<headerPtr->numChannels;chan++) { */
/* 	power[chan] = (float*) calloc((numPoints/2)+1,sizeof(float)); */
/*     } */
/*     freqArray = (float*) calloc((numPoints/2)+1,sizeof(float)); */
/* //    sampFreq=1./(((float)headerPtr->sampInt)*10.e-12);  /\* 2 Gsamp/sec unless otherwise set  *\/ */
/*     getPowerSpec(&voltEvent,power,freqArray,sampFreq); */
/*     headerPtr->priority=checkPayloadFrequencies(power,freqArray,headerPtr->numChannels,numPoints/2); */
/*     if(headerPtr->priority==9) return headerPtr->priority; /\* Reject *\/ */

/*     numPeaksMatch=0; */
/*     for(chan=0;chan<headerPtr->numChannels;chan++) { */
/* 	if(peakBins[chan]==trigPeakBins[chan]) numPeaksMatch++; */
/*     } */
/*     expectedPeaks= (int) ( ((float) (2*trigHalfWindow*headerPtr->numChannels)) / ((float) (headerPtr->numSamples))); */
/* /\*     printf("Num Peaks in Trig Window %d, expected %d\n",numPeaksMatch,expectedPeaks);  *\/ */

/*     if(numPeaksMatch>expectedPeaks+20)  */
/* 	return headerPtr->priority=PRI_1; */
/*     if(numPeaksMatch>expectedPeaks+15)  */
/* 	return headerPtr->priority=PRI_2; */
/*     if(numPeaksMatch>expectedPeaks+10)  */
/* 	return headerPtr->priority=PRI_3; */
/*     if(numPeaksMatch>expectedPeaks+5)  */
/* 	return headerPtr->priority=PRI_4; */
    
/*     return headerPtr->priority; */
/* } */


/* void convertToVoltageAndGetStats(AnitaEventBody_t *bodyPtr, event_volts *voltsPtr, double means[], double rmss[], int peakBins[], int trigPeakBins[], int trigBin, int trigHalfWindow) */
/* /\* This will need to be updated to whatever the correct format is *\/ */
/* { */
/*     int chan,samp; */
/*     int raw, temp; */
/*     float volts,mean,meanSqd,rms,peak,trigPeak; */
/*     int offset=2048, trigLow,trigHigh; */
/*     float scale=1.0; */
    
/*     trigLow= (trigBin>trigHalfWindow) ? trigBin-trigHalfWindow : 0; */
/*     trigHigh = (trigBin>=voltsPtr->numSamples-trigHalfWindow) ? voltsPtr->numSamples : trigBin+trigHalfWindow; */
/*     for(chan=0;chan<voltsPtr->numChannels;chan++) { */
/* 	mean=0; */
/* 	meanSqd=0; */
/* //	offset=bodyPtr->channel[chan].header.ch_offs; */
/* //	scale=((float)bodyPtr->channel[chan].header.ch_fs)/ */
/* //	    (float)(ADC_MAX*1000/2); */
/* 	peak=0; */
/* 	trigPeak=0; */
/* /\* 	printf("%d %f\n",offset,scale);  *\/ */
/* 	for(samp=0;samp<voltsPtr->numSamples;samp++) { */
/* 	    raw=bodyPtr->channel[chan].data[samp]; */
/* 	    volts=((float)(raw-offset))*scale; */
/* /\* 	    printf("%d %f\n",raw,volts); *\/ */
/* 	    voltsPtr->channel[chan].data[samp]=volts; */
/* 	    mean+=volts; */
/* 	    meanSqd+=(volts*volts); */
/* 	    if(fabs(volts)>peak) { */
/* 		peak=fabs(volts); */
/* 		peakBins[chan]=samp; */
/* 	    } */
/* 	    if(samp>=trigLow && samp<=trigHigh) { */
/* 		if(fabs(volts)>trigPeak) { */
/* 		    trigPeak=fabs(volts); */
/* 		    trigPeakBins[chan]=samp; */
/* 		} */
/* 	    } */
		
/* 	} */
/* /\* 	printf("%f %f\n",mean,meanSqd); *\/ */
/* 	mean/=(float)voltsPtr->numSamples; */
/* 	meanSqd/=(float)voltsPtr->numSamples; */
/* 	rms=sqrt(fabs(meanSqd-(mean*mean))); /\* fabs should be unnecessary *\/ */
/* /\* 	printf("%f %f %f\n",scale,mean/scale,rms/scale); *\/ */
/* 	means[chan]=mean; */
/* 	rmss[chan]=rms; */

/* 	temp=200*mean/(scale*(float)ADC_MAX); */
/* 	if(abs(temp)>127) { */
/* 	    /\* Shouldn't happen *\/ */
/* 	    if(temp>0) temp=127; */
/* 	    else temp=-127; */
/* 	} */
/* //	bodyPtr->channel[chan].header.ch_mean=temp; */
/* 	temp=200*rms/(scale*(float)ADC_MAX); */
/* 	if(temp>255) { */
/* 	    /\* Shouldn't happen *\/ */
/* 	    temp=255; */
/* 	} */
/* //	bodyPtr->channel[chan].header.ch_sdev=temp;		 */
/*     } */
/* } */


/* void getPowerSpec( event_volts *voltPtr, float *pwr[], float *freq, float fs1) */

/* { */
/* /\* requires external arrays for data and power spectra,  */
/*    and frequency index array *\/ */
/* /\* pwr[] and freq[] are npts/2+1 long, and do not include Nyquist *\/ */

/*     complex *dat; */
/*     int i,j,npts,np2; */
/*     int nsamp=voltPtr->numSamples; */
/*     int nchan=voltPtr->numChannels; */
/*     np2=(int)(log10(voltPtr->numSamples)/log10(2));  */
/*     npts = 1 << np2; /\* An integer power of two (probably was anyhow) *\/ */

/*     /\* test for large nsamps and set the pwr=1.0 for these cases to avoid */
/*        long waits *\/ */
/*     if(nsamp>16384){ */
/* 	for(j=0;j<nchan;j++){ */
/* 	    for(i=0;i<npts/2;i++){ */
/* 		pwr[j][i] = 1.0; */
/* 	    } */
/* 	} */
/* 	for(i=0; i<npts/2; i++){ */
/* 	    freq[i] = (((double)(i)) / ((double)npts)) * fs1; */
/* 	} */
/*     } */
/*     else {  /\* normal situation, N<=16384 *\/	         */
/* /\*	fprintf(stderr,"npts= %d nsamp= %d, fs1=%g\n, begin power spec\n", npts, nsamp, fs1); *\/ */
/* /\* 	fflush(stderr); *\/ */

/* 	dat = (complex*)calloc(npts+2, sizeof(complex)); */

/* /\* funny order here (im,re) since NR routines ignore first C indexed element *\/ */
/* 	for(j=0;j<nchan;j++){ */

/* 	    for(i=0;i<npts;i++){ */
/* 		dat[i].im = voltPtr->channel[j].data[i]; */
/* 		dat[i+1].re = 0.; */
/* 	    } */

/* 	    /\* <<<<<<<<<<<<<<<  do fft  >>>>>>>>>>>>>>>>>>>>>>>*\/ */
/* 	    nrFFTForward(dat,npts); */
/* 	    /\*<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>*\/ */

/* 	    /\* DC to Nyquist-1 *\/ */
/* 	    for(i=0;i<npts/2;i++){ */
/* 		pwr[j][i] = dat[i+1].re*dat[i+1].re + */
/* 		    dat[i].im*dat[i].im; */
/* 	    } */

/* 	}  /\* end of channel loop *\/ */

/* 	/\* fill the frequency array *\/ */
/* 	for(i=0; i<npts/2; i++){ */
/* 	    freq[i] = (((double)(i)) / ((double)npts)) * fs1; */
/* /\*	   fprintf(stdout,"%e   %f\n", freq[i], pwr[0][i]); *\/ */
/* 	} */
/* 	free(dat); */
	
/*     } /\* end of else 16384 test *\/ */
/* } */


/* int checkPayloadFrequencies( float *power[], float freq[], int numChannels, int numFreqs) */
/* /\* At the moment this is a direct port of the old CheckPower routine from */
/*    the ANITA-lite code. The final version will probably be modified such that */
/*    both the veto frequencies and the number fo channels showing them are */
/*    configurable. */
/* *\/ */
/* { */

/* /\* if m of n channels have significant power near 401 MHz,  */
/*    reject this event *\/ */

/* 	int i,j, N401; */
/* 	float df; */
	
/* 	float maxpower[NUM_DIGITZED_CHANNELS], Peakpower[NUM_DIGITZED_CHANNELS], meanpower[NUM_DIGITZED_CHANNELS], npower[NUM_DIGITZED_CHANNELS], maxf[NUM_DIGITZED_CHANNELS]; */

/* /\*	fprintf(stderr, "%d  %d\n", numChannels, numFreqs); *\/ */

/* 	for(i=0;i<numChannels;i++){ */
/* 	    npower[i] = 0.0;  */
/* 	    meanpower[i]=0.0; */
/* 	} */


/* 	for(i=0; i<numChannels; i++){ */
/* 	   for(j=10;j<numFreqs;j++){ */
/* /\*	   fprintf(stdout,"freq=%e   %f\n", freq[j], power[i][j]); *\/ */
/* 		if( (freq[j]<=3.8e8) || (freq[j]>= 4.5e8)){ */
/* 		   meanpower[i] += power[i][j]; */
/* 		   npower[i] += 1.0; */
/* 		   } */
/* 		} */
/* 	} */
	

/* 	for(i=0;i<numChannels;i++){ */
/* 	        meanpower[i] /= npower[i]; */
/* /\*		fprintf(stdout,"%d   %e  %e\n", i, meanpower[i], npower[i]);*\/ */
/* 		} */

/* 	for(i=0;i<numChannels;i++){ */
/* 		 maxpower[i] = -999.; */
/* 		 Peakpower[i] = -999.; */
/* 		 maxf[i] = 0.0; */
/* 		 } */

/* /\* first check the power around 401 MHz *\/ */
/* 	for(i=0; i<numChannels; i++){ */
/* 	   for(j=0;j<numFreqs;j++){ */
/* 		if(freq[j]>396.e6 && freq[j]< 440e6){ */
/* 		   if(power[i][j] > maxpower[i]) maxpower[i] = power[i][j]; */
/* 		   } */
/* 		} */
/* 	} */

/* /\* now get highest power in the file *\/ */
/* 	for(i=0; i<numChannels; i++){ */
/* 	   for(j=0;j<numFreqs;j++){ */
/* 		   if(power[i][j] > Peakpower[i]){ */
/* 		    Peakpower[i] = power[i][j]; */
/* 		    maxf[i] = freq[j]; */
/* 		   } */
/* 		} */
/* 	} */
	
/* 	N401=0; */
/* 	for(i=0;i<numChannels;i++){ */
/* 	    if(maxpower[i]/meanpower[i] > 25.0) N401++; */
/* 	} */
		
/* 	df = freq[2]-freq[1]; */
/* /\*	fprintf(stderr,"df = %e\n", df); *\/ */
/* 	if(df==0.0) df= 1./(((float)(numFreqs))*.5e-9);  /\* default value in case of df error *\/ */

/* /\* 	for(i=0;i<numChannels;i++){ *\/ */
/* /\* 		header.meanpower[i] = (int)(10.*log10(meanpower[i])); *\/ */
/* /\* 		header.peakpower[i] = (int)(10.*log10(maxpower[i])); *\/ */
/* /\* 		header.peakf[i] = maxf[i]<1.0e9 ? (int)(maxf[i]/4.e6) : 255; *\/ */
/* /\* 	  } *\/ */
      
/* 	/\* orginally this was 3 but changed to 1 for testing *\/ */
/* 	if(N401 >= 1) return(9);  /\* lowest priority == reject *\/ */
/* 	else return(8); */
/* } */






/* /\* Below is numerical recipes code *\/ */

/* #define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr */

/* void nrFFTForward(complex data[],int nn) */
/* { */
/*     nrFFT((float*)data,nn,1); */
/* } */
/* void nrFFT(float data[],int nn,int isign) */
/* /\* numerical recipes code *\/ */
/* /\* data is of length 2*nn *\/ */
/* { */
/*     int n,mmax,m,j,istep,i; */
/*     double wtemp,wr,wpr,wpi,wi,theta; /\*double precision for the trig *\/ */
/*     float tempr,tempi; */
    
/*     n=nn << 1; */
/*     j=1; */
/*     for (i=1;i<n;i+=2) { */
/* 	if (j > i) { */
/* 	    SWAP(data[j],data[i]);   /\*Bit reversal section *\/ */
/* 	    SWAP(data[j+1],data[i+1]); */
/* 	} */
/* 	m=nn; */
/* 	while (m >= 2 && j > m) { */
/* 	    j -= m; */
/* 	    m >>= 1; */
/* 	} */
/* 	j += m; */
/*     } */
    
/*     /\* Start the Danielson-Lanczos section *\/ */
/*     mmax=2; */
/*     while (n > mmax) { */
/* 	istep=2*mmax; */
/* 	theta=6.28318530717959/(isign*mmax); */
/* 	wtemp=sin(0.5*theta); */
/* 	wpr = -2.0*wtemp*wtemp; */
/* 	wpi=sin(theta); */
/* 	wr=1.0; */
/* 	wi=0.0; */
/* 	for (m=1;m<mmax;m+=2) { */
/* 	    for (i=m;i<=n;i+=istep) { */
/* 		j=i+mmax; */
/* 		tempr=wr*data[j]-wi*data[j+1]; */
/* 		tempi=wr*data[j+1]+wi*data[j]; */
/* 		data[j]=data[i]-tempr; */
/* 		data[j+1]=data[i+1]-tempi; */
/* 		data[i] += tempr; */
/* 		data[i+1] += tempi; */
/* 	    } */
/* 	    wr=(wtemp=wr)*wpr-wi*wpi+wr; */
/* 	    wi=wi*wpr+wtemp*wpi+wi; */
/* 	} */
/* 	mmax=istep;  */
/*     } */
/* } */

#undef SWAP
