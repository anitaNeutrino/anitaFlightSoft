#ifndef ANITAINSTRUMENT_H
#define ANITAINSTRUMENT_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include "pedestalLib/pedestalLib.h"
#include "utilLib/utilLib.h"
#include <math.h>
#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef struct{
     TransientChannelF_t topRing[PHI_SECTORS][2];
     TransientChannelF_t botRing[PHI_SECTORS][2];
     TransientChannelF_t nadir[8][2];
} AnitaInstrumentF_t; 

typedef struct{
     LogicChannel_t topRing[PHI_SECTORS][2];
     LogicChannel_t botRing[PHI_SECTORS][2];
     LogicChannel_t nadir[PHI_SECTORS][2]; //between nadirs,incherently add
} AnitaChannelDiscriminator_t;

typedef struct{
     LogicChannel_t topRing[PHI_SECTORS];
     LogicChannel_t botRing[PHI_SECTORS];
     LogicChannel_t nadir[PHI_SECTORS]; //between nadirs, incoherently add.
} AnitaSectorLogic_t;

typedef struct{
     LogicChannel_t sector[PHI_SECTORS];
} AnitaCoincidenceLogic_t;

typedef struct{
     float time;
     float value;
} Peak_t;

typedef struct{
     Peak_t topRing[PHI_SECTORS][2];
     Peak_t botRing[PHI_SECTORS][2];
     Peak_t nadir[8][2];
} AnitaPeak_t;

typedef struct{
     int nover;
     float topRing[PHI_SECTORS][2];
     float botRing[PHI_SECTORS][2];
     float nadir[8][2];
}RMScheck_t;

// this is the data structure for old SLAC trigger
typedef struct {
     int overallPeakSize[3]; //
     int overallPeakLoc[3]; //
     int overallPeakPhi[3]; //
     int overallPeakRing[3]; //
     int numPeaks[PHI_SECTORS][2]; //
     int peakSize[3][PHI_SECTORS][2]; // 0 is upper, 1 is lower
     int peakLocation[3][PHI_SECTORS][2]; //
     int totalOccupancy[PHI_SECTORS][2]; //
} AnitaSectorAnalysis_t;

#endif //DOXYGEN_SHOULD_SKIP_THIS
#include "GenerateScore.h"
#include "Filters.h"
    
extern const int topRingIndex[PHI_SECTORS][2];
extern const int botRingIndex[PHI_SECTORS][2];
#ifdef ANITA2
extern const int nadirIndex[8][2];
#else //ANITA2
extern const int biconeIndex[4]; //phi=2,6,10,14
extern const int disconeIndex[4]; //phi=4,8,12,16
#endif //not ANITA 2
extern const float topPhaseCenter[16][3]; //x,y,z in ANITA system and  meters
extern const float botPhaseCenter[16][3];
extern const float topDelay[16][2];//ns
extern const float botDelay[16][2];//ns
#ifdef ANITA2
extern const float nadirPhaseCenter[8][3];
#else //ANITA2
extern const float biconePhaseCenter[4][3];
extern const float disconePhaseCenter[4][3];
#endif //not ANITA2
//nominal view direction in ACS phi _degrees_ by phi sector
extern const float sectorPhiACS[16];
extern const float dipangle; 
extern const float samples_per_ns; //SURF nominal
extern const float meters_per_ns; //speed of light in meters per ns


//Global Control Variables
extern int printToScreen;
extern int verbosity;
extern float hornThresh;
extern int hornDiscWidth;
extern int hornSectorWidth;
extern float coneThresh;
extern int coneDiscWidth;
extern int holdoff;
extern int delay;
extern int hornGuardOffset;
extern int hornGuardWidth;
extern int hornGuardThresh;
extern int coneGuardOffset;
extern int coneGuardWidth;
extern int coneGuardThresh;
extern int FFTPeakMaxA;
extern int FFTPeakMaxB;
extern int FFTPeakWindowL;
extern int FFTPeakWindowR;
extern int FFTMaxChannels;
extern int RMSMax;
extern int RMSevents;
extern int WindowCut;
extern int BeginWindow;
extern int EndWindow;
extern int MethodMask;
extern int NuCut;

// global event thingees
/*Event object*/
extern AnitaEventHeader_t theHeader;
extern AnitaEventBody_t theBody;
extern PedSubbedEventBody_t pedSubBody;
extern AnitaTransientBodyF_t unwrappedBody;
extern AnitaInstrumentF_t theInstrument;
extern AnitaInstrumentF_t theXcorr;
//original analysis
extern AnitaChannelDiscriminator_t theDiscriminator;
extern AnitaSectorLogic_t theMajority;
extern AnitaSectorAnalysis_t sectorAna;
//nonupdating discriminator method
extern AnitaChannelDiscriminator_t theNonupdating;
extern AnitaSectorLogic_t theMajorityNoUp;
extern AnitaSectorLogic_t theHorizontal;
extern AnitaSectorLogic_t theVertical;
extern AnitaCoincidenceLogic_t theCoincidenceAll;
extern AnitaCoincidenceLogic_t theCoincidenceH;
extern AnitaCoincidenceLogic_t theCoincidenceV;
extern int MaxAll, MaxH, MaxV;
//xcorr peak boxcar method
extern AnitaChannelDiscriminator_t theBoxcar;
extern AnitaChannelDiscriminator_t theBoxcarNoGuard;
//extern AnitaSectorLogic_t theMajorityBoxcar;
extern AnitaSectorLogic_t theMajorityBoxcarH;
extern AnitaSectorLogic_t theMajorityBoxcarV;
//extern AnitaCoincidenceLogic_t theCoincidenceBoxcarAll;
extern AnitaCoincidenceLogic_t theCoincidenceBoxcarH;
extern AnitaCoincidenceLogic_t theCoincidenceBoxcarV;
//extern AnitaSectorLogic_t theMajorityBoxcar2;
extern AnitaSectorLogic_t theMajorityBoxcarH2;
extern AnitaSectorLogic_t theMajorityBoxcarV2;
//extern AnitaCoincidenceLogic_t theCoincidenceBoxcarAll2;
extern AnitaCoincidenceLogic_t theCoincidenceBoxcarH2;
extern AnitaCoincidenceLogic_t theCoincidenceBoxcarV2;
extern int /*MaxBoxAll,*/MaxBoxH,MaxBoxV;
extern int /*MaxBoxAll2,*/MaxBoxH2,MaxBoxV2;
extern LogicChannel_t HornCounter,ConeCounter;
extern int HornMax;
extern int RMSnum;
extern int LowRMSChan;
extern int MidRMSChan;
extern int HighRMSChan;
extern int CutRMS;

#ifdef __cplusplus
extern "C"
{
#endif

     void BuildInstrumentF(AnitaTransientBodyF_t  *surfData,
			   AnitaInstrumentF_t  *antennaData);

     void DiscriminateFChannels(AnitaInstrumentF_t *theData,
			       AnitaChannelDiscriminator_t *theDisc,
			       int ringthresh,int ringwidth,
			       int conethresh,int conewidth);

     void DiscriminateFChannels_noup(AnitaInstrumentF_t *in,
				     AnitaChannelDiscriminator_t *out,
				     int hornthresh,int hornwidth,
				     int conethresh,int conewidth,
				     int holdoff);

     float RMSF(TransientChannelF_t *in);

     float MSRatio(TransientChannelF_t *in,int begwindow,int endwindow);

     void DiscriminateF(TransientChannelF_t *in,LogicChannel_t *out,
		       float threshold, int width);

     void DiscriminateF_noup(TransientChannelF_t *in,LogicChannel_t *out,
			     float threshold, int width,int holdoff);

     int PeakBoxcarOne(TransientChannelF_t *in,LogicChannel_t *out,
		       int width, int guardOffset, 
		       int guardWidth,int guardThresh);

     void PeakBoxcarAll(AnitaInstrumentF_t *in,
		   AnitaChannelDiscriminator_t *out,
		   int hornwidth,int hornGuardOffset, 
			int hornGuardWidth,int hornGuardThresh,
			int conewidth,int coneGuardOffset, 
			int coneGuardWidth,int coneGuardThresh);

     int RMSCountAll(AnitaInstrumentF_t *in,int thresh, 
		     int begwindow,int endwindow);

     int MeanCountAll(AnitaInstrumentF_t *in,int thresh, 
		     int begwindow,int endwindow);

     int GlobalMajority(AnitaChannelDiscriminator_t *in,
			 LogicChannel_t *horns,
			 LogicChannel_t *cones,
			 int delay);
     
     void FormSectorMajority(AnitaChannelDiscriminator_t *in,
			     AnitaSectorLogic_t *out,
			     int sectorWidth);

     void FormSectorMajorityPol(AnitaChannelDiscriminator_t *in,
				AnitaSectorLogic_t *out,
				int sectorWidth,int pol);

     int FormSectorCoincidence(AnitaSectorLogic_t *majority,
			       AnitaCoincidenceLogic_t *coinc,  
			       int delay,
			       int topthresh, int botthresh);

     void FindPeaks(AnitaInstrumentF_t *theInst, AnitaPeak_t *thePeak);

     int determinePriority();

     float RMSRatio(TransientChannelF_t *theChan,int low, int mid, int high);

     float CheckRMSRatio(AnitaInstrumentF_t *theInst, 
			 int low, int mid, int high);
#ifdef __cplusplus
}
#endif
#endif /* ANITAINSTRUMENT_H */
