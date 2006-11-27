#ifndef PACKWAVE_H
#define PACKWAVE_H
#ifdef __cplusplus
extern "C"
{
#endif
unsigned short packwave(unsigned short nwords, 
			unsigned short width, unsigned short nstrip, 
			unsigned short npack, unsigned short *in,
			unsigned char *out);

unsigned short unpackwave(unsigned short nbytes, 
			  unsigned short width, unsigned short nstrip, 
			  unsigned short npack, unsigned char *in,
			  unsigned short *out);
#ifdef __cplusplus
}
#endif
#endif
