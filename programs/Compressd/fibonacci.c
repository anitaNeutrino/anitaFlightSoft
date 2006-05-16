#include <stdio.h>
#include <stdlib.h>

#include "fibonacci.h"

unsigned int fn[24]={1,2,3,5,8,13,21,34,55,89,
		     144,233,377,610,987,1597,2584,4181,6765,10946,
		     17711,28657,46368,75025}; //these are enough to encode unsigned short

//convert a short to its fibonacci representation with a 11 prefix
unsigned int fibonacci(unsigned short input){
     unsigned int output;
     int i;
     i=0;
     // find the first fibonacci number greater than input
     while(input>=fn[i]){
	  i++;
     }
     // set the prefix bit
     output=1<<(i);
     i--; // we are now at the largest fibonacci number less than input
     //now set the bits for the minimal fibonacci representation
     while(i>=0){
	  if (input>=fn[i]){
	       output += (1<<i);
	       input -= fn[i];
	  }
	  i--;
     }  
     return output;
}

// reverse the above operation
unsigned short unfibonacci(unsigned int input)
{
     unsigned int i;
     unsigned short output;
     unsigned int curbit, lastbit;
     output=0;
     lastbit=0;
     for (i=0; i<23; i++){
	  curbit=(input>>i) & 0x0001;
	  if (curbit==1 && lastbit==1) break;  // done--found the prefix bit
	  else if (curbit==1) output+=fn[i]; 
	  lastbit=curbit;
     }
     return output;
}

	  
