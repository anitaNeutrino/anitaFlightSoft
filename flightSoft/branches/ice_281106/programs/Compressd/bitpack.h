#ifndef BITPACK_H
#define BITPACK_H
#ifdef __cplusplus
extern "C"
{
#endif
     unsigned short bitpack(unsigned short bits,unsigned short nwords, 
		  unsigned short *in, unsigned char *out);
#ifdef __cplusplus
}
#endif
#endif
