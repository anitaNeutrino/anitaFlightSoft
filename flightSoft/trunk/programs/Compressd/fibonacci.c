#include <stdio.h>
#include <stdlib.h>

unsigned short fn[23]={1,2,3,5,8,13,21,34,55,89,
		       144,233,377,610,987,1597,2584,4181,6765,10946,
		       17711,28657,46368};

unsigned short fibonacci(unsigned char input){
//     short fn[13]; //can represent numbers up to 609
     short i,output;
//     fn[0]=1;
//     fn[1]=2;
//     for(i=2;i<13;i++){
//	  fn[i]=fn[i-1]+fn[i-2];
//        printf("%u %u\n",i,fn[i]);
//     }	 
     i=0;
     while(input>=fn[i]){
	  i++;
     }
     // printf("%i\n",i);
     output=1<<(i);
     while(i>=0){
	  if (input>=fn[i]){
	       output += (1<<i);
	       input -= fn[i];
	       // printf("%i %u\n",i,fn[i]);
	  }
	  i--;
     }  
     return output;
}

unsigned char unfibonacci(unsigned short input)
{
     unsigned short i;
     unsigned char output;
     unsigned short curbit, lastbit;
     output=0;
     lastbit=0;
     for (i=0; i<13; i++){
	  curbit=(input>>i) & 0x0001;
	  if (curbit==1 && lastbit==1) break;
	  else if (curbit==1) output+=fn[i];
	  lastbit=curbit;
     }
     return output;
}

	  
