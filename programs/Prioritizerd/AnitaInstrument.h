#ifndef ANITAINSTRUMENT_H
#define ANITAINSTRUMENT_H
#include <anitaStructures.h>
#include <anitaFlight.h>
#include <math.h>

typedef struct{
     TransientChannel3_t* topRing[PHI_SECTORS][2];
     TransientChannel3_t* botRing[PHI_SECTORS][2];
     TransientChannel3_t* bicone[4];
     TransientChannel3_t* discone[4];
} AnitaInstrument3_t; //use pointers to contents of an anita transient body;

typedef struct{
     TransientChannelF_t topRing[PHI_SECTORS][2];
     TransientChannelF_t botRing[PHI_SECTORS][2];
     TransientChannelF_t bicone[4];
     TransientChannelF_t discone[4];
} AnitaInstrumentF_t; 

typedef struct {
     int sample[MAX_NUMBER_SAMPLES];
     float data[MAX_NUMBER_SAMPLES];
     short npoints;
} EnvelopeF_t;

typedef struct{
     EnvelopeF_t topRing[PHI_SECTORS][2];
     EnvelopeF_t botRing[PHI_SECTORS][2];
     EnvelopeF_t bicone[4];
     EnvelopeF_t discone[4];
} AnitaEnvelopeF_t;

typedef struct{
     LogicChannel_t topRing[PHI_SECTORS][2];
     LogicChannel_t botRing[PHI_SECTORS][2];
     LogicChannel_t bicone[4];
     LogicChannel_t discone[4];
} AnitaChannelDiscriminator_t;

typedef struct{
     LogicChannel_t topRing[PHI_SECTORS];
     LogicChannel_t botRing[PHI_SECTORS];
     LogicChannel_t bicone;
     LogicChannel_t discone;
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
     Peak_t bicone[4];
     Peak_t discone[4];
} AnitaPeak_t;

typedef struct{
     int nover;
     float topRing[PHI_SECTORS][2];
     float botRing[PHI_SECTORS][2];
     float bicone[4];
     float discone[4];
}RMScheck_t;

#define MAX_BASELINES 32
#define MAX_ITER 10

typedef struct{
     int nbaselines; //filled by builder
     int ngood; //count the baselines that are not bad.  
                //If there are too few to perform the fit, 
                //don't fit.  Filled by fitter.
     int validfit; //zero for good fit; otherwise an error code
     //-1 : a singular matrix was found in the initial fit
     //-2 : a singular matrix was found in the Lagrange multiplier search
     //-3 : too many iterations in the Lagrange search
     //-4 : too few baselines to fit
     int niter; //number of itereations to get norm right
     float lambda; //Lagrange multiplier for norm constraint
     float arrival[3]; //best fit to vector from instrument to
                       //source, normalized on the unit sphere
     float norm; //fitted length of direction vector
     float position[MAX_BASELINES][2][3]; //convenience copies of locations
     float delay[MAX_BASELINES][2]; //time in meters
     float value[MAX_BASELINES][2];
     float direction[MAX_BASELINES][3]; //unit vector from 
                                        //late to early antenna
     float length[MAX_BASELINES]; //length of baseline
     float sinangle[MAX_BASELINES]; //sine has errors which 
                                    //are linear in delay
     float cosangle[MAX_BASELINES]; //used for statB
     int bad[MAX_BASELINES]; //nonzero for 'bad' baselines
                             //integer gives pass on which baseline
                             //was rejected
}BaselineAnalysis_t;

extern const int topRingIndex[PHI_SECTORS][2];
extern const int botRingIndex[PHI_SECTORS][2];
extern const int biconeIndex[4]; //phi=2,6,10,14
extern const int disconeIndex[4]; //phi=4,8,12,16
extern const float topPhaseCenter[16][3]; //x,y,z in ANITA system and  meters
extern const float botPhaseCenter[16][3];
extern const float topDelay[16][2];//ns
extern const float botDelay[16][2];//ns
extern const float biconePhaseCenter[4][3];
extern const float disconePhaseCenter[4][3];
//nominal view direction in ACS phi _degrees_ by phi sector
extern const float sectorPhiACS[16];
extern const float dipangle; 
extern const float samples_per_ns; //SURF nominal
extern const float meters_per_ns; //speed of light in meters per ns

#ifdef __cplusplus
extern "C"
{
#endif

     void BuildInstrument3(AnitaTransientBody3_t  *surfData,
			   AnitaInstrument3_t  *antennaData);

     void BuildInstrumentF(AnitaTransientBodyF_t  *surfData,
			   AnitaInstrumentF_t  *antennaData);

     void Instrument3toF(AnitaInstrument3_t  *antennaData,
			    AnitaInstrumentF_t  *floatData);

     void DiscriminateChannels(AnitaInstrument3_t *theData,
			       AnitaChannelDiscriminator_t *theDisc,
			       int ringthresh,int ringwidth,
			       int conethresh,int conewidth);

     void DiscriminateFChannels(AnitaInstrumentF_t *theData,
			       AnitaChannelDiscriminator_t *theDisc,
			       int ringthresh,int ringwidth,
			       int conethresh,int conewidth);

     void DiscriminateFChannels_noup(AnitaInstrumentF_t *in,
				     AnitaChannelDiscriminator_t *out,
				     int hornthresh,int hornwidth,
				     int conethresh,int conewidth,
				     int holdoff);

     int RMS(TransientChannel3_t *in);

     float RMSF(TransientChannelF_t *in);

     void Discriminate(TransientChannel3_t *in,LogicChannel_t *out,
		       int threshold, int width);

     void DiscriminateF(TransientChannelF_t *in,LogicChannel_t *out,
		       float threshold, int width);

     void DiscriminateF_noup(TransientChannelF_t *in,LogicChannel_t *out,
			     float threshold, int width,int holdoff);

     int PeakBoxcarOne(TransientChannelF_t *in,LogicChannel_t *out,
		       int width, int guardOffset, int guardWidth,int guardThresh);

     void PeakBoxcarAll(AnitaInstrumentF_t *in,
		   AnitaChannelDiscriminator_t *out,
		   int hornwidth,int hornGuardOffset, int hornGuardWidth,int hornGuardThresh,
			int conewidth,int coneGuardOffset, int coneGuardWidth,int coneGuardThresh);
     
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

     void MakeBaselinesFour(AnitaPeak_t *thePeak,BaselineAnalysis_t *theBA,
			    int phi,int dphi);			    
#ifdef __cplusplus
}
#endif
#endif /* ANITAINSTRUMENT_H */
