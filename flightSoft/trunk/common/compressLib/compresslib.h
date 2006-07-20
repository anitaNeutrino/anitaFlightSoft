#ifndef COMPRESSLIB_H
#define COMPRESSLIB_H



#ifdef __cplusplus
extern "C" {
#endif


#include "anitaStructures.h"

#define MAX_WAVE_BUFFER NUM_DIGITZED_CHANNELS*MAX_NUMBER_SAMPLES*4


typedef enum {
    COMPRESS_E_OK = 0,
    COMPRESS_E_PACK = 0x100,
    COMPRESS_E_UNPACK,
    COMPRESS_E_NO_PEDS,
    COMPRESS_E_BAD_PEDS,
    COMPRESS_E_TOO_BIG
} CompressErrorCode_t ;

    unsigned short bifurcate(short input);
    short unbifurcate(unsigned short input);
    unsigned short bitpack(unsigned short bits,unsigned short nwords, 
			   unsigned short *in, unsigned char *out);
    void bitstrip(unsigned short nbits,unsigned short nwords,unsigned short *data);
    int codepack(int n, unsigned int *in, unsigned char *out);
    int codeunpack(int m, unsigned char *in, unsigned int *out);
    unsigned int fibonacci(unsigned short input);
    unsigned short unfibonacci(unsigned int input);
    unsigned short findmedian(unsigned short nwords, unsigned short *in);
    unsigned short packwave(unsigned short nwords, 
			    unsigned short width, unsigned short nstrip, 
			    unsigned short npack, unsigned short *in,
			    unsigned char *out);
    unsigned short unpackwave(unsigned short nbytes, 
			      unsigned short width, unsigned short nstrip, 
			      unsigned short npack, unsigned char *in,
			      unsigned short *out);
    
    //Now we'll add the wrapper routines that will actually be called
    CompressErrorCode_t packEvent(AnitaEventBody_t *bdPtr,
				  EncodeControlStruct_t *cntlPtr,
				  unsigned char *output,
				  int *numBytes);
    CompressErrorCode_t unpackEvent(AnitaEventBody_t *bdPtr,
				    unsigned char *input,
				    int numBytes);
				  
	
//And here are it's worker routines
    unsigned short simpleCrcShort(unsigned short *p, unsigned long n);
    int encodeChannel(ChannelEncodingType_t encType, SurfChannelFull_t *chanPtr, unsigned char *buffer);
    int encodeWaveNone(unsigned char *buffer,SurfChannelFull_t *chanPtr);


#ifdef __cplusplus
}
#endif
#endif
