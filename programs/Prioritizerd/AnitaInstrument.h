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
     float time;
     float value;
} Peak_t;

typedef struct{
     Peak_t topRing[PHI_SECTORS][2];
     Peak_t botRing[PHI_SECTORS][2];
     Peak_t bicone[4];
     Peak_t discone[4];
} AnitaPeak_t;

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

     int RMS(TransientChannel3_t *in);

     float RMSF(TransientChannelF_t *in);

     void Discriminate(TransientChannel3_t *in,LogicChannel_t *out,
		       int threshold, int width);

     void DiscriminateF(TransientChannelF_t *in,LogicChannel_t *out,
		       float threshold, int width);

     void FormSectorMajority(AnitaChannelDiscriminator_t *in,
			     AnitaSectorLogic_t *out,
			     int sectorWidth);
			    
#ifdef __cplusplus
}
#endif
#endif /* ANITAINSTRUMENT_H */
