/*! \file mulaw.c
    \brief Mulaw packing routines 
    
    Some functions that mulaw and unmulaw data
    June 2006  rjn@mps.ohio-state.edu
*/

char convertToMuLaw(short input, int inputBits, int mulawBits);
unsigned char convertToMuLawUC(short input, int inputBits,int mulawBits);
short convertFromMuLawUC(unsigned char input, int outputBits, int mulawBits);
short convertFromMuLaw(char input, int outputBits, int mulawBits);
