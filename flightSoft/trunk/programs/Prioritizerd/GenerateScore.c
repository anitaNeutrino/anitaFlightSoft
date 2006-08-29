#include "AnitaInstrument.h"
#include "GenerateScore.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int value;
    int index;
} SortStruct_t;

typedef struct {
    int value;
    int index;
    int phi;
    int ring;
} SortStruct2_t;

int compareStructs(const void *ptr1, const void *ptr2) {
    SortStruct_t *s1 = (SortStruct_t*) ptr1;
    SortStruct_t *s2 = (SortStruct_t*) ptr2;
    if(s1->value<s2->value) return -1;
    if(s1->value>s2->value) return 1;
    return 0;
} 

int compareStructs2(const void *ptr1,const void *ptr2) {
    SortStruct2_t *s1 = (SortStruct2_t*) ptr1;
    SortStruct2_t *s2 = (SortStruct2_t*) ptr2;
    if(s1->value<s2->value) return -1;
    if(s1->value>s2->value) return 1;
    return 0;
}

void AnalyseSectorLogic(AnitaSectorLogic_t *secLogPtr,
			AnitaSectorAnalysis_t *secAnaPtr)
{
    int phi,samp,index,uptoPeak;
    int summedValue;
    int top=0;
    int bot=1;
    int peakSeparation=10;

    int count=0;
    SortStruct_t sortArray[MAX_NUMBER_SAMPLES];
    SortStruct2_t overallArray[100];
    int numPeaks=0;
    
    memset(secAnaPtr,0,sizeof(AnitaSectorAnalysis_t));
    memset(sortArray,0,sizeof(SortStruct_t)*MAX_NUMBER_SAMPLES);
    memset(overallArray,0,sizeof(SortStruct2_t)*100);

    for(phi=0;phi<PHI_SECTORS;phi++) {	
	//Do top Ring first
	count=0;
	for(samp=2;samp<secLogPtr->topRing[phi].valid_samples-2;samp++) {
	    secAnaPtr->totalOccupancy[phi][top]+=secLogPtr->topRing[phi].data[samp];
	    summedValue=0;
	    for(index=samp-2;index<=samp+2;index++)
		summedValue+=secLogPtr->topRing[phi].data[index];
	    if(summedValue>0) {
		sortArray[count].value=summedValue;
		sortArray[count].index=samp;
		count++;
	    }
	}
	if(count) {
	    uptoPeak=0;
//	    printf("count %d\n",count);
	    qsort(sortArray,count,sizeof(SortStruct_t),compareStructs);
	    for(index=count-1;index>=0;index--) {
		if(uptoPeak==0) {
//		    printf("%d %d\n",sortArray[index].value,sortArray[index].index);
		    secAnaPtr->peakSize[uptoPeak][phi][top]=sortArray[index].value;
		    secAnaPtr->peakLocation[uptoPeak][phi][top]=sortArray[index].index;
		    uptoPeak++;
		    overallArray[numPeaks].value=sortArray[index].value;
		    overallArray[numPeaks].index=sortArray[index].index;
		    overallArray[numPeaks].phi=phi;
		    overallArray[numPeaks].ring=top;
		    numPeaks++;
		}
		else {
		    if(abs(secAnaPtr->peakLocation[0][phi][top]-
			   sortArray[index].index)<peakSeparation) continue;
		    if(uptoPeak==2)
			if(abs(secAnaPtr->peakLocation[1][phi][top]-
			       sortArray[index].index)<peakSeparation) continue;
		    
		    
		    secAnaPtr->peakSize[uptoPeak][phi][top]=sortArray[index].value;
		    secAnaPtr->peakLocation[uptoPeak][phi][top]=sortArray[index].index;
		    uptoPeak++;
		    overallArray[numPeaks].value=sortArray[index].value;
		    overallArray[numPeaks].index=sortArray[index].index;
		    overallArray[numPeaks].phi=phi;
		    overallArray[numPeaks].ring=top;
		    numPeaks++;
		}	    
		if(uptoPeak>2) break;
	    }
	    secAnaPtr->numPeaks[phi][top]=uptoPeak;
	}
	    
	
	//now do bottom
	count=0;
	for(samp=2;samp<secLogPtr->botRing[phi].valid_samples-2;samp++) {
	    secAnaPtr->totalOccupancy[phi][bot]+=secLogPtr->botRing[phi].data[samp];
	    summedValue=0;
	    for(index=samp-2;index<=samp+2;index++)
		summedValue+=secLogPtr->botRing[phi].data[index];
	    if(summedValue>0) {
		sortArray[count].value=summedValue;
		sortArray[count].index=samp;
		count++;
	    }
	}
	if(count) {
	    uptoPeak=0;
	    qsort(sortArray,count,sizeof(SortStruct_t),compareStructs);
	    for(index=count-1;index>=0;index--) {
		if(uptoPeak==0) {
		    secAnaPtr->peakSize[uptoPeak][phi][bot]=sortArray[index].value;
		    secAnaPtr->peakLocation[uptoPeak][phi][bot]=sortArray[index].index;
		    uptoPeak++;
		    overallArray[numPeaks].value=sortArray[index].value;
		    overallArray[numPeaks].index=sortArray[index].index;
		    overallArray[numPeaks].phi=phi;
		    overallArray[numPeaks].ring=bot;
		    numPeaks++;
		}
		else {

		    if(abs(secAnaPtr->peakLocation[0][phi][bot]-
			   sortArray[index].index)<peakSeparation) continue;
		    if(uptoPeak==2)
			if(abs(secAnaPtr->peakLocation[1][phi][bot]-
			       sortArray[index].index)<peakSeparation) continue;
		    
		    secAnaPtr->peakSize[uptoPeak][phi][bot]=sortArray[index].value;
		    secAnaPtr->peakLocation[uptoPeak][phi][bot]=sortArray[index].index;
		    uptoPeak++;
		    overallArray[numPeaks].value=sortArray[index].value;
		    overallArray[numPeaks].index=sortArray[index].index;
		    overallArray[numPeaks].phi=phi;
		    overallArray[numPeaks].ring=bot;
		    numPeaks++;
		    
		}
		if(uptoPeak>2) break;
	    }
	    secAnaPtr->numPeaks[phi][bot]=uptoPeak;
	}	    		       	

    }
    qsort(overallArray,numPeaks,sizeof(SortStruct2_t),compareStructs2);
    uptoPeak=0;
//    printf("\n");
    for(index=numPeaks-1;index>=0;index--) {
/* 	printf("%d %d %d %d\n",overallArray[index].value, */
/* 	       overallArray[index].index, */
/* 	       overallArray[index].phi, */
/* 	       overallArray[index].ring); */
	secAnaPtr->overallPeakSize[uptoPeak]=overallArray[index].value;
	secAnaPtr->overallPeakLoc[uptoPeak]=overallArray[index].index;
	secAnaPtr->overallPeakPhi[uptoPeak]=overallArray[index].phi;
	secAnaPtr->overallPeakRing[uptoPeak]=overallArray[index].ring;
	uptoPeak++;
	if(uptoPeak>2) break;
    }

}
		
int GetSecAnaScore(AnitaSectorAnalysis_t *secAnaPtr)
{
    int peakPhi,peakRing,peakInd,compInd;
    int score,tempScore[3]={0},compScore[3]={0};
    for(peakInd=0;peakInd<3;peakInd++) {
	tempScore[peakInd]=0;
	if(secAnaPtr->overallPeakSize[peakInd]<10) break; //No peaks
	tempScore[peakInd]=300-(100*peakInd);    
	peakPhi=secAnaPtr->overallPeakPhi[peakInd];
	peakRing=secAnaPtr->overallPeakRing[peakInd];
	if(peakPhi<0 || peakPhi>15 || peakRing<0 || peakRing>1) break;


	for(compInd=0;compInd<3;compInd++) {	    
	    compScore[compInd]=tempScore[peakInd];
	    if(secAnaPtr->peakSize[compInd][peakPhi][1-peakRing]>9) {
		//Complimentary ring has a peak
		compScore[compInd]+=200-(50*compInd);
		if(abs(secAnaPtr->overallPeakLoc[peakInd]-secAnaPtr->peakLocation[compInd][peakPhi][1-peakRing])<15) {
		    //peaks coincide
		    compScore[compInd]+=300;
		    if(abs(secAnaPtr->overallPeakLoc[peakInd]-secAnaPtr->peakLocation[compInd][peakPhi][1-peakRing])<5) {
			compScore[compInd]+=100; //Peaks really coincide
		    }
		}				
	    }
	}	
	tempScore[peakInd]=compScore[0];
	if(tempScore[peakInd]<compScore[1])tempScore[peakInd]=compScore[1];
	if(tempScore[peakInd]<compScore[2])tempScore[peakInd]=compScore[2];
	
    }
    score=tempScore[0];
    if(secAnaPtr->overallPeakSize[1]>9 && 
       abs(secAnaPtr->overallPeakPhi[0]-secAnaPtr->overallPeakPhi[1])<3 &&
       abs(secAnaPtr->overallPeakLoc[0]-secAnaPtr->overallPeakLoc[1])<15) {
	score+=tempScore[1];
    }
    if(secAnaPtr->overallPeakSize[2]>9 && 
       abs(secAnaPtr->overallPeakPhi[0]-secAnaPtr->overallPeakPhi[2])<3 &&
       abs(secAnaPtr->overallPeakLoc[0]-secAnaPtr->overallPeakLoc[2])<15) {
	score+=tempScore[2];
    }
    
    if(score<tempScore[1]) {
	score=tempScore[1];
	if(secAnaPtr->overallPeakSize[2]>9 && 
	   abs(secAnaPtr->overallPeakPhi[1]-secAnaPtr->overallPeakPhi[2])<3 &&
	   abs(secAnaPtr->overallPeakLoc[1]-secAnaPtr->overallPeakLoc[2])<15) {
	    score+=tempScore[2];
	}
    }
    if(score<tempScore[2]) score=tempScore[2];

    return score;
}
