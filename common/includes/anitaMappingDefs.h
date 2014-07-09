/*! \file anitaMappingDefs.h
    \brief The ANITA mapping enum and structure definition include file.
    
    This is where the SURF and Channel is mapped to antenna, polarization, tier and phi.
    July 2014  r.nichol@ucl.ac.uk
*/

#ifndef ANITA_MAPPING_DEFS_H
#define ANITA_MAPPING_DEFS_H



typedef enum {    
    kUpperTier=1,
    kMiddleTier=2,
    kLowerTier=4,
    kDisconeTier=8,
    kBiconeTier=16,
    kUnknownTier=-1
} AntennaTier_t;
 
typedef enum {
    kHorizontal=0,
    kVertical=1,
    kUnknownPol=-1
} AntennaPol_t;

typedef struct {
    int surf;
    int channel;
    int index;
    int phi;
    AntennaTier_t tier;
    int antenna;
    AntennaPol_t pol;    
} SurfAntMapStruct_t;


#endif //ANITA_MAPPING_DEFS_H
