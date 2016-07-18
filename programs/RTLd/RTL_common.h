#ifndef ANITA_FLIGHT_RTL_COMMON_H
#define ANITA_FLIGHT_RTL_COMMON_H

#include "anitaStructures.h" 


/** Open a shared RTL region from serial number 
 *  Use parent = 1 for RTLd (to create the memory region and share it with children) 
 *  Or parent = 0 to just read one that as been shared by a parent
 *
 * */ 

RtlSdrPowerSpectraStruct_t * open_shared_RTL_region(const char * serial, int parent); 

/** Unlink a shared RLT region */ 
void unlink_shared_RTL_region(const char * serial); 

/** Human readable version of RTL power spectrum struct */ 
void dumpSpectrum(const RtlSdrPowerSpectraStruct_t * dump); 

#endif 
