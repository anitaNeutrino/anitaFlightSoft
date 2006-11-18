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

extern int topRingIndex[PHI_SECTORS][2];
extern int botRingIndex[PHI_SECTORS][2];
extern int biconeIndex[4];
extern int disconeIndex[4];

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
