#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"
#include "simpleStructs.h"
//#include "../../../flightSoft/trunk/common/utilLib/utilLib.h"
#include <iostream>
#include <zlib.h>
#include "errno.h"
#define PRIORITIZER_EVENT_DIR "/tmp/anita/prioritizerd/event"


void prioritizerToRoot(int numFiles,int energy,int exponent);
int fillHeader(AnitaEventHeader_t *theEventHdPtr, char *filename)
{
    int fd; 
    fd = open(filename,O_RDONLY); 
    if(fd<=0){
      std::cout << "fd is 0" << std::endl;
      std::cout << strerror(errno) << std::endl;
      return 0;
    }
    int numBytes=read(fd,(unsigned char*)theEventHdPtr,
				   sizeof(AnitaEventHeader_t));
    close(fd);
    if(numBytes==sizeof(AnitaEventHeader_t)) return 0;
    if(numBytes==-1) std::cout << strerror(errno) << std::endl;
    //std::cout << "numBytes: " << numBytes << " fd: " << fd << std::endl;
    return numBytes;  
}


AnitaEventHeader_t theHeader;

int main(int argc,char **argv){
  int numFiles=250;
  int energy=1;
  int exponent=20;
  int extraNoise=180;
  if(argc>3){
    numFiles=atoi(argv[1]);
    energyOrNoise=atoi(argv[2]);
    exponent=atoi(argv[3]);
    extraNoise=atoi(argv[4]);
    prioritizerToRootEvent(numFiles,energy,exponent);
  }
  else{
    printf("Not doing anything!!!");
    return 0;
  }

}



void prioritizerToRootEvent(int numFiles,int energy,int exponent){


  char fileName[FILENAME_MAX];
  char treeName[FILENAME_MAX];

  sprintf(fileName,"/home/mottram/matt/AesopInterface/trunk/mattProgs/prioritizer.root");
  TFile *output = new TFile(fileName,"UPDATE");

  sprintf(treeName,"%de%dTree",energy,exponent);
  TTree *tree = new TTree(treeName,"priority tree");

  UInt_t eventNumber;
  UChar_t priority;
  int retVal;

  tree->Branch("eventNumber",&eventNumber,"eventNumber/i");
  tree->Branch("priority",&priority,"priority/b");

  for(int fileNumber=0;fileNumber<numFiles;fileNumber++){

    sprintf(fileName,"%s/hd_%d.dat",PRIORITIZER_EVENT_DIR,fileNumber);
    std::cout << fileNumber << "\t" << fileName << "\t";
    retVal=fillHeader(&theHeader,fileName);



    priority = theHeader.priority;
    eventNumber = theHeader.eventNumber;
    std::cout << eventNumber << "\t" << theHeader.eventNumber << " (" << priority << ")" << " " << retVal << std::endl;
    //printf("fileNumber %d, eventNumber %d, priority %d\n",fileNumber,theHeader.eventNumber,theHeader.priority);
    //printf("fileNumber %d, eventNumber %d, priority %d\n",fileNumber,eventNumber,priority);


    tree->Fill();
  }

  printf("output being written\n");
  output->Write();
  output->Close();

}
