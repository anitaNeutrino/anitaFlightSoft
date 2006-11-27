#include <stdio.h>
#include <stdlib.h>

#include "bifurcate.h"

// perform the bifurcation mapping from integers to the positive
// integers as a prelude to applying prefix codes (e.g. fibonacci)
//
// chosen mapping is 0,-1,1,-2,2... => 1,2,3,4,5
// 
// posi integers map to 2*n+1
// negative integers map 2*abs(n)
//

unsigned short bifurcate(short input)
{
     unsigned short output;
     if (input>=0){
	  output=(2*(unsigned short) input + 1);
     }
     else{
	  output=(2* (unsigned short)abs(input));
     }
     return output;
}

short unbifurcate(unsigned short input)
{
     // note zero is not a valid input
     short output;
     if ((input % 2) == 1){//odd => non-negative
	  output=((input-1)/2);
     }
     else{ //even => negative
	  output=-(input/2);
     }
}
	  
	  
