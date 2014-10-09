#include "anitaGpu.h"


void getData();
void getDataLab3AnitaStyle(AnitaEventHeader_t* theHeader, PedSubbedEventBody_t* pedSubBody, int event);

int main(int argc, char* argv[]){

  /* All setup has been outsourced to this function */
  prepareGpuThings();

  /* Set up some Anita Data structures */
  AnitaEventHeader_t theHeader[NUM_EVENTS];
  PedSubbedEventBody_t pedSubBody[NUM_EVENTS];

  /*
    NOW LETS LOOP AND MEASURE HOW QUICKLY WE CAN COMPUTE OUR STUFF
  */

  uint loop = 0;
  uint numLoops = 100;
  for(loop = 0; loop<numLoops; loop++){

    /* /\* Read in num samples array *\/ */
    /* uint numSampsInd; */
    /* for(numSampsInd=0; numSampsInd<NUM_POLARIZATIONS*NUM_ANTENNAS*NUM_EVENTS; numSampsInd++){ */
    /*   numEventSamples[numSampsInd] = NUM_SAMPLES; */
    /* } */

    /* /\* Generate some trigger states *\/ */
    /* uint polInd; */
    /* for(polInd = 0; polInd < NUM_POLARIZATIONS; polInd++){ */
    /*   uint eventInd; */
    /*   for(eventInd = 0; eventInd < NUM_EVENTS; eventInd++){ */
    /* 	uint phiSector; */
    /* 	for(phiSector = 0; phiSector < NUM_PHI_SECTORS; phiSector++){ */
    /* 	  if(phiSector == 14 || phiSector == 15){ */
    /* 	    phiTrig[polInd*NUM_EVENTS*NUM_PHI_SECTORS + eventInd*NUM_PHI_SECTORS + phiSector] = 1; */
    /* 	  } */
    /* 	  else{ */
    /* 	    phiTrig[polInd*NUM_EVENTS*NUM_PHI_SECTORS + eventInd*NUM_PHI_SECTORS + phiSector] = 0; */
    /* 	  } */
    /* 	} */
    /*   } */
    /* } */

    /* Read in data from a text file, copy each event n-times for each loop */
    /* getData(eventDataVPol, loop%256); */
    /* getData(&eventDataVPol[NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES], (255-loop)%256); */
    /* getData(eventData, 65); */
    /* getData(&eventData[NUM_EVENTS*NUM_ANTENNAS*NUM_SAMPLES], 149); */

    getDataLab3AnitaStyle(theHeader, pedSubBody, 65);

    /* Then GPU the shit out of them */
    mainGpuLoop(pedSubBody, theHeader);
  }

  tidyUpGpuThings();

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
    int chanInd3 = -1;
    fscanf(input, "%d ", &chanInd3);
    int fhb = 0;
    fscanf(input, "%d ", &fhb);
    int lhb = 0;
    fscanf(input, "%d ", &lhb);
    int chipIdFlag = 0;
    fscanf(input, "%d ", &chipIdFlag);

    pedSubBody[0].channel[chanInd3].header.firstHitbus = (unsigned char) fhb;
    pedSubBody[0].channel[chanInd3].header.lastHitbus = (unsigned char) lhb;
    pedSubBody[0].channel[chanInd3].header.chipIdFlag = (unsigned char) chipIdFlag;

    /* printf("%d %d\n", chanInd, chanInd3); */
    /* printf("FHB = %d!!!!!!!!!!!!!!!!!!!\n", fhb); */
    /* printf("LHB = %d!!!!!!!!!!!!!!!!!!!\n", lhb); */
    /* if(fabs(fhb-lhb > 128)){ */
    /*   printf("wrap = %d!!!!!!!!!!!!!!!!!!!\n", event); */
    /*   pedSubBody[0].channel[chanInd3].header.chipIdFlag = 255; */
    /* } */

    /* printf("%d %d %d %hu %hu \n", chanInd3,  */
    /* 	   fhb, pedSubBody[0].channel[chanInd3].header.firstHitbus,  */
    /* 	   lhb, pedSubBody[0].channel[chanInd3].header.lastHitbus); */
    int samp=0;
    for(samp=0; samp<260; samp++){
      fscanf(input, "%hd ", &pedSubBody[0].channel[chanInd3].data[samp]);
      /* printf("%hd ", pedSubBody[0].channel[chanInd3].data[samp]); */
    }
    /* printf("\n\n"); */
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

  int eventInd=0;
  for(eventInd=0; eventInd < NUM_EVENTS; eventInd++){
    memcpy(&theHeader[eventInd], &theHeader[0], sizeof(AnitaEventHeader_t));
    memcpy(&pedSubBody[eventInd], &pedSubBody[0], sizeof(PedSubbedEventBody_t));
  }

  //  printf("finished reading in and copying data...\n");

}
