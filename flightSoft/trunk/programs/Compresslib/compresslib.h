#ifndef COMPRESSLIB_H
#define COMPRESSLIB_H
#ifdef __cplusplus
extern "C"
{
#endif
unsigned short bifurcate(short input);
short unbifurcate(unsigned short input);
unsigned short bitpack(unsigned short bits,unsigned short nwords, 
		  unsigned short *in, unsigned char *out);
void bitstrip(unsigned short nbits,unsigned short nwords,unsigned short *data);
int bytepack(int n, unsigned int *in, unsigned char *out);
int byteunpack(int m, unsigned char *in, unsigned int *out);
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
#ifdef __cplusplus
}
#endif
#endif
