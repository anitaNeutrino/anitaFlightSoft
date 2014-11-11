#include "anitaGpu.h"
#include "anitaTimingCalib.h"
#include "includes/anitaStructures.h"

int chan3ToChan2[12*9] = {-1};
int chan2ToChan3[12*9] = {-1};

void getData();
void getDataLab3AnitaStyle(AnitaEventHeader_t* theHeader, PedSubbedEventBody_t* pedSubBody, int event);
void printChannelMapping();

//const int alreadyUnwrappedAndCalibrated = 1;
const int alreadyUnwrappedAndCalibrated = 0;

//int main(int argc, char* argv[]){
int main(){

  /* All setup has been outsourced to this function */
  prepareGpuThings();
  prepareTimingCalibThings();

  /* Set up some Anita Data structures */
  AnitaEventHeader_t theHeader[NUM_EVENTS];
  PedSubbedEventBody_t pedSubBody[NUM_EVENTS];
  
  /*
    NOW LETS LOOP AND MEASURE HOW QUICKLY WE CAN COMPUTE OUR STUFF
  */

  /* printf("AnitaEventHeader_t is %d bytes.\n", sizeof(AnitaEventHeader_t)); */
  /* printf("PedSubbedEventBody_t is %d bytes.\n", sizeof(PedSubbedEventBody_t)); */
  /* printf("For reference, a float is %d bytes.\n", sizeof(float)); */

  uint loop = 0;
  uint numLoops = 10;

  giveChan3ToChan2(chan3ToChan2);

  int entry=0;
  for(entry=0; entry<128; entry++){
    getDataLab3AnitaStyle(&theHeader[entry], &pedSubBody[entry], 65);
  }

  int numEntries = 0;
  for(loop=0; loop<numLoops; loop++){
    for(entry=0; entry<128; entry++){
      double* finalVolts[12*9];
      doTimingCalibration(entry, theHeader[entry], pedSubBody[entry], finalVolts);
      int surf=0;
      int chan=0;

      //      addEventToGpuQueue(entry, pedSubBody[entry], theHeader[entry], 0);
      addEventToGpuQueue(entry, finalVolts, theHeader[entry]);
      numEntries++;

      int chanInd=0;
      for(chanInd=0; chanInd<12*CHANNELS_PER_SURF; chanInd++){
	free(finalVolts[chanInd]);
      }
    }
    /* Then GPU the shit out of them */
    mainGpuLoop(numEntries, theHeader);
    numEntries=0;
  }

  tidyUpGpuThings();
  tidyUpTimingCalibThings();

  return 0;
}


void getData(short* eventData, int event){

  /* read in from a file once */
  uint index=0;
  char fileName[1024];
  if(event >= 0){
    sprintf(fileName, "../rawEventFiles/data_file_165_%d.txt", event);
  }
  else if(event == -1){
    sprintf(fileName, "../rawEventFiles/pureImpulseWaveforms.txt");
  }
  else{ /* less than -1 */
    sprintf(fileName, "../rawEventFiles/pureSineWaveforms.txt");
  }

  //    printf("fileName = %s\n", fileName);
  FILE* input = NULL;
  input = fopen(fileName, "r");
  if(input == NULL){
    fprintf(stderr, "Error! Failed to open data file %s!\n", fileName);
    fprintf(stderr, "Exiting program.\n");
    exit (1);
  }
  while(fscanf(input, "%hd ", &eventData[index]) != EOF){
    index++;
  }
  fclose(input);

  uint eventInd;
  for(eventInd=0; eventInd<NUM_EVENTS; eventInd++){
    if(eventInd > 0){
      memcpy(&eventData[eventInd*NUM_SAMPLES*NUM_ANTENNAS],
	     &eventData[0], 
	     sizeof(short)*NUM_SAMPLES*NUM_ANTENNAS);
    }    
  }
}

void getDataLab3(short* eventData, short* numEventSamples){

  uint eventInd=0;
  uint index=0;

  for(eventInd=0; eventInd<NUM_EVENTS; eventInd++){
#ifdef DEBUG_MODE
    int event = 65;
#else
    //    int event = eventInd;
    int event = 65;
#endif
    char fileName[1024];
    sprintf(fileName, "../rawEventFiles/data_file_lab3_165_%d.txt", event);
    printf("fileName = %s\n", fileName);
    FILE* input = NULL;
    input = fopen(fileName, "r");
    if(input == NULL){
      fprintf(stderr, "Error! Failed to open data file %s!\n", fileName);
      fprintf(stderr, "Exiting program.\n");
      exit (1);
    }

    int ant=0;
    for(ant=0; ant<NUM_ANTENNAS; ant++){
      int numSamps = 0;
      fscanf(input, "%hd ", &numSamps);
      numEventSamples[eventInd*NUM_ANTENNAS + ant] = numSamps;
      //      printf("%d %d %d\n", event, ant, numSamps); // just to check
      int samp=0;
      for(samp=0; samp < numSamps; samp++){
	float temp = 0;
	fscanf(input, "%f ", &temp);
	//	printf("%f ", temp);
	eventData[(eventInd*NUM_ANTENNAS + ant)*NUM_SAMPLES + samp] = (short) temp;
      }
      //      printf("\n");

      // GPU should ignore these elements, but just to be safe lets zero them now
      // Later a good test might be to put crazy values in here...
      for(samp=numSamps; samp<NUM_SAMPLES; samp++){
	eventData[(eventInd*NUM_ANTENNAS + ant)*NUM_SAMPLES + samp] = 0; 
      }
    }
    
    fclose(input);
  }


}







void getDataLab3AnitaStyle(AnitaEventHeader_t* theHeader, PedSubbedEventBody_t* pedSubBody, int event){
  
  char fileName[1024];
  sprintf(fileName, "../rawEventFiles/data_file_raw_lab3_165_%d.txt", event);
  FILE* input = NULL;
  input = fopen(fileName, "r");
  if(input == NULL){
    fprintf(stderr, "Error! Failed to open data file %s!\n", fileName);
    fprintf(stderr, "Exiting program.\n");
    exit (-1);
  }
  int chanInd=0;

  for(chanInd=0; chanInd<12*9; chanInd++){
    int chanInd2 = -1;
    fscanf(input, "%d ", &chanInd2);
    int chanInd3 = -1;
    fscanf(input, "%d ", &chanInd3);
    int rco = -1;
    fscanf(input, "%d ", &rco);
    int lab = -1;
    fscanf(input, "%d ", &lab);
    int fhb = 0;
    fscanf(input, "%d ", &fhb);
    int lhb = 0;
    fscanf(input, "%d ", &lhb);
    int chipIdFlag = 0;
    fscanf(input, "%d ", &chipIdFlag);

    if(chanInd2 > -1){
      chan2ToChan3[chanInd2] = chanInd3;
    }
    chan3ToChan2[chanInd3] = chanInd2;

    printf("%d %d %d\n", chanInd3, chan3ToChan2[chanInd3], chanInd2);

    pedSubBody[0].channel[chanInd3].header.firstHitbus = (unsigned char) fhb;
    pedSubBody[0].channel[chanInd3].header.lastHitbus = (unsigned char) lhb;
    pedSubBody[0].channel[chanInd3].header.chipIdFlag = (unsigned char) chipIdFlag;


	/* wrappedHitbus= */
	/*     (pedSubBdPtr->channel[chan].header.chipIdFlag&0x8)>>3; */
  //!  chip id bitmask
  /*!
    0:1  LABRADOR chip
    2 RCO
    3 HITBUS wrap
    4-7 HITBUS offset
  */

    int labBitMask = pedSubBody[0].channel[chanInd3].header.chipIdFlag & 0x3;
    /* printf("a3 = %d, a2 = %d, lab from labBitMask = %d, lab from root function = %d\n", */
    /* 	   chanInd3, chanInd2, labBitMask, lab); */
    /* OK the lab bit mask works, so now insert true RCO phase into chipIdFlag... */
    /* (just to decode it again later) */
    if(rco==1){
      pedSubBody[0].channel[chanInd3].header.chipIdFlag |= (1 << 2);
    }
    else if(rco==0){
      pedSubBody[0].channel[chanInd3].header.chipIdFlag |= (0 << 2);
    }
    /* printf("a3 = %d, a2 = %d, rco from bitMask = %d, rco from root function = %d\n", */
    /* 	   chanInd3, chanInd2, (int)(pedSubBody[0].channel[chanInd3].header.chipIdFlag&0x4)/4, rco); */


    int samp=0;
    for(samp=0; samp<260; samp++){
      fscanf(input, "%hd ", &pedSubBody[0].channel[chanInd3].data[samp]);
    }
    
    /* if( chanInd3 % 9 == 8){ */
    /*   for(samp=0; samp<260; samp++){ */
    /* 	printf("%hd ", pedSubBody[0].channel[chanInd3].data[samp]); */
    /*   } */
    /*   printf("\n\n"); */
    /* } */
  }

  theHeader[0].turfio.l3TrigPattern = 0;
  theHeader[0].turfio.l3TrigPatternH = 0;


  int phiInd=0;
  for(phiInd=0; phiInd<NUM_PHI_SECTORS; phiInd++){
    if( phiInd == 14 || phiInd == 13){
      theHeader[0].turfio.l3TrigPattern += pow(2, phiInd);
      theHeader[0].turfio.l3TrigPatternH += pow(2, phiInd);
    }
  }
  fclose(input);

  //  printf("finished reading in and copying data...\n");

  printChannelMapping();

}


void printChannelMapping(){

  /* Testing mapping makes sense. */
  FILE* channelMapping = NULL;
  channelMapping = fopen("channelMapping.txt", "w");
  int chanInd3 = 0;
  fprintf(channelMapping, "ANITA-3\tANITA-2\n");
  for(chanInd3=0; chanInd3<12*9; chanInd3++){
    int chanInd2 = chan3ToChan2[chanInd3];
    fprintf(channelMapping, "%d\t%d\n", chanInd3, chanInd2);
  }
  fclose(channelMapping);
}



/* void getDataLab3AnitaStyle(AnitaEventHeader_t* theHeader, PedSubbedEventBody_t* pedSubBody, int event){ */
  


/*   char fileName[1024]; */
/*   if( alreadyUnwrappedAndCalibrated ){ */
/*     sprintf(fileName, "../rawEventFiles/data_file_interp_lab3_165_%d.txt", event); */
/*     //    sprintf(fileName, "../rawEventFiles/pureImpulseWaveforms.txt"); */
/*     printf("%s\n", fileName); */
/*   } */
/*   else{ */
/*     sprintf(fileName, "../rawEventFiles/data_file_raw_lab3_165_%d.txt", event); */
/*   } */
/*   FILE* input = NULL; */
/*   input = fopen(fileName, "r"); */
/*   if(input == NULL){ */
/*     fprintf(stderr, "Error! Failed to open data file %s!\n", fileName); */
/*     fprintf(stderr, "Exiting program.\n"); */
/*     exit (-1); */
/*   } */
/*   int chanInd=0; */

/*   for(chanInd=0; chanInd<12*9; chanInd++){ */
/*     int chanInd3 = -1; */
/*     fscanf(input, "%d ", &chanInd3); */
/*     int fhb = 0; */
/*     fscanf(input, "%d ", &fhb); */
/*     int lhb = 0; */
/*     fscanf(input, "%d ", &lhb); */
/*     int chipIdFlag = 0; */
/*     fscanf(input, "%d ", &chipIdFlag); */

/*     pedSubBody[0].channel[chanInd3].header.firstHitbus = (unsigned char) fhb; */
/*     pedSubBody[0].channel[chanInd3].header.lastHitbus = (unsigned char) lhb; */
/*     pedSubBody[0].channel[chanInd3].header.chipIdFlag = (unsigned char) chipIdFlag; */

/*     /\* printf("%d %d\n", chanInd, chanInd3); *\/ */
/*     /\* printf("FHB = %d!!!!!!!!!!!!!!!!!!!\n", fhb); *\/ */
/*     /\* printf("LHB = %d!!!!!!!!!!!!!!!!!!!\n", lhb); *\/ */
/*     /\* if(fabs(fhb-lhb > 128)){ *\/ */
/*     /\*   printf("wrap = %d!!!!!!!!!!!!!!!!!!!\n", event); *\/ */
/*     /\*   pedSubBody[0].channel[chanInd3].header.chipIdFlag = 255; *\/ */
/*     /\* } *\/ */

/*     /\* printf("%d %d %d %hu %hu \n", chanInd3,  *\/ */
/*     /\* 	   fhb, pedSubBody[0].channel[chanInd3].header.firstHitbus,  *\/ */
/*     /\* 	   lhb, pedSubBody[0].channel[chanInd3].header.lastHitbus); *\/ */
/*     int samp=0; */
/*     for(samp=0; samp<260; samp++){ */
/*       /\* if( alreadyUnwrappedAndCalibrated ){ *\/ */
/*       /\* 	float tempVal = 0; *\/ */
/*       /\* 	fscanf(input, "%f ", &tempVal); *\/ */
/*       /\* 	pedSubBody[0].channel[chanInd3].data[samp] = (short)tempVal; *\/ */
/*       /\* } *\/ */
/*       /\* else{ *\/ */
/*       fscanf(input, "%hd ", &pedSubBody[0].channel[chanInd3].data[samp]); */
/*       /\* } *\/ */
/*       /\* printf("%hd ", pedSubBody[0].channel[chanInd3].data[samp]); *\/ */
/*     } */
/*     /\* printf("\n\n"); *\/ */
/*   } */

/*   theHeader[0].turfio.l3TrigPattern = 0; */
/*   theHeader[0].turfio.l3TrigPatternH = 0; */

/*   int phiInd=0; */
/*   for(phiInd=0; phiInd<NUM_PHI_SECTORS; phiInd++){ */
/*     //    if( phiInd == 14 || phiInd == 13){ */
/*       theHeader[0].turfio.l3TrigPattern += pow(2, phiInd); */
/*       theHeader[0].turfio.l3TrigPatternH += pow(2, phiInd); */
/*       //    } */
/*   } */


/*   fclose(input); */

/*   int eventInd=0; */
/*   for(eventInd=0; eventInd < NUM_EVENTS; eventInd++){ */
/*     memcpy(&theHeader[eventInd], &theHeader[0], sizeof(AnitaEventHeader_t)); */
/*     memcpy(&pedSubBody[eventInd], &pedSubBody[0], sizeof(PedSubbedEventBody_t)); */
/*   } */

/*   //  printf("finished reading in and copying data...\n"); */

/* } */


