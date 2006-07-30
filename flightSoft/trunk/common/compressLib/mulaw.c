/*! \file mulaw.c
    \brief Mulaw packing routines 
    
    Some functions that mulaw and unmulaw data
    June 2006  rjn@mps.ohio-state.edu
*/
#include "compressLib/compressLib.h"
#include "mulawTables.h"
#include "mulaw.h"


unsigned char convertToMuLawUC(short input, int inputBits,
			       int mulawBits) 
{
    return charbifurcate(convertToMuLaw(input,inputBits,mulawBits));
}

char convertToMuLaw(short input, int inputBits, int mulawBits) 
{    
    //It is the responsibilty of the caller to ensure input is within the range specified.
    //Might add checks as I don't trust the user.
    int index=input+(1<<(inputBits-1));
    switch (inputBits) {
	case 11:
	    switch (mulawBits) {
		case 8:
		    return linear11toMuLaw8[index];
		case 7:
		    return linear11toMuLaw7[index];
		case 6:
		    return linear11toMuLaw6[index];
		case 5:
		    return linear11toMuLaw5[index];
		case 4:
		    return linear11toMuLaw4[index];
		case 3:
		    return linear11toMuLaw3[index];
		default:
		    return linear11toMuLaw8[index];
	    }
	case 10:
	    switch (mulawBits) {
		case 8:
		    return linear10toMuLaw8[index];
		case 7:
		    return linear10toMuLaw7[index];
		case 6:
		    return linear10toMuLaw6[index];
		case 5:
		    return linear10toMuLaw5[index];
		case 4:
		    return linear10toMuLaw4[index];
		case 3:
		    return linear10toMuLaw3[index];
		default:
		    return linear10toMuLaw8[index];
	    }
	case 9:
	    switch (mulawBits) {
		case 7:
		    return linear9toMuLaw7[index];
		case 6:
		    return linear9toMuLaw6[index];
		case 5:
		    return linear9toMuLaw5[index];
		case 4:
		    return linear9toMuLaw4[index];
		case 3:
		    return linear9toMuLaw3[index];
		default:
		    return linear9toMuLaw7[index];
	    }
	case 8:
	    switch (mulawBits) {
		case 6:
		    return linear8toMuLaw6[index];
		case 5:
		    return linear8toMuLaw5[index];
		case 4:
		    return linear8toMuLaw4[index];
		case 3:
		    return linear8toMuLaw3[index];
		default:
		    return linear8toMuLaw6[index];
	    }
	case 7:
	    switch (mulawBits) {
		case 5:
		    return linear7toMuLaw5[index];
		case 4:
		    return linear7toMuLaw4[index];
		case 3:
		    return linear7toMuLaw3[index];
		default:
		    return linear7toMuLaw5[index];
	    }
	case 6:
	    switch (mulawBits) {
		case 4:
		    return linear6toMuLaw4[index];
		case 3:
		    return linear6toMuLaw3[index];
		default:
		    return linear6toMuLaw4[index];
	    }
	default:
	    switch (mulawBits) {
		case 8:
		    return linear11toMuLaw8[index];
		case 7:
		    return linear11toMuLaw7[index];
		case 6:
		    return linear11toMuLaw6[index];
		case 5:
		    return linear11toMuLaw5[index];
		case 4:
		    return linear11toMuLaw4[index];
		case 3:
		    return linear11toMuLaw3[index];
		default:
		    return linear11toMuLaw8[index];
	    }
    }
    return 0;
}

short convertFromMuLawUC(unsigned char input, int outputBits, int mulawBits) 
{
    return convertFromMuLaw(charunbifurcate(input),outputBits,mulawBits);
}

short convertFromMuLaw(char input, int outputBits, int mulawBits) 
{    
    int index=input+(1<<(mulawBits-1));
    switch (outputBits) {
	case 11:
	    switch (mulawBits) {
		case 8:
		    return mulaw8toLinear11[index];
		case 7:
		    return mulaw7toLinear11[index];
		case 6:
		    return mulaw6toLinear11[index];
		case 5:
		    return mulaw5toLinear11[index];
		case 4:
		    return mulaw4toLinear11[index];
		case 3:
		    return mulaw3toLinear11[index];
		default:
		    return mulaw8toLinear11[index];
	    }
	case 10:
	    switch (mulawBits) {
		case 8:
		    return mulaw8toLinear10[index];
		case 7:
		    return mulaw8toLinear10[index];
		case 6:
		    return mulaw8toLinear10[index];
		case 5:
		    return mulaw8toLinear10[index];
		case 4:
		    return mulaw8toLinear10[index];
		case 3:
		    return mulaw8toLinear10[index];
		default:
		    return mulaw8toLinear10[index];
	    }
	case 9:
	    switch (mulawBits) {
		case 7:
		    return mulaw7toLinear9[index];
		case 6:
		    return mulaw6toLinear9[index];
		case 5:
		    return mulaw5toLinear9[index];
		case 4:
		    return mulaw4toLinear9[index];
		case 3:
		    return mulaw3toLinear9[index];
		default:
		    return mulaw7toLinear9[index];
	    }
	case 8:
	    switch (mulawBits) {
		case 6:
		    return mulaw6toLinear8[index];
		case 5:
		    return mulaw5toLinear8[index];
		case 4:
		    return mulaw4toLinear8[index];
		case 3:
		    return mulaw3toLinear8[index];
		default:
		    return mulaw6toLinear8[index];
	    }
	case 7:
	    switch (mulawBits) {
		case 5:
		    return mulaw5toLinear7[index];
		case 4:
		    return mulaw4toLinear7[index];
		case 3:
		    return mulaw3toLinear7[index];
		default:
		    return mulaw5toLinear7[index];
	    }
	case 6:
	    switch (mulawBits) {
		case 4:
		    return mulaw4toLinear6[index];
		case 3:
		    return mulaw3toLinear6[index];
		default:
		    return mulaw4toLinear6[index];
	    }
	default:
	    switch (mulawBits) {
		case 8:
		    return mulaw8toLinear11[index];
		case 7:
		    return mulaw7toLinear11[index];
		case 6:
		    return mulaw6toLinear11[index];
		case 5:
		    return mulaw5toLinear11[index];
		case 4:
		    return mulaw4toLinear11[index];
		case 3:
		    return mulaw3toLinear11[index];
		default:
		    return mulaw8toLinear11[index];
	    }
    }
    return 0;
}
    
