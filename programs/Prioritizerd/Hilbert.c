/* Flat Hilbert Filtering of Transients up to 10th order. 
   see JJB Mathematica notebook Hilbert.nb for tests.
*/

double HilbertCoefficient[10][10];
int IntHilbertCoefficient[10][10]; /*these are in 12-bit scale */

int makeHilbertCoefficients()
{
     int kmax,i;
     for (kmax=1;kmax<=10;kmax++){
	  for (i=1;i<=kmax;kmax++){
	       HilbertCoefficient[i-1][kmax-1]=
		    ((double)(kmax-i+1)/ (double) kmax)*
		    (1./(double)(2*i-1))*sqrt(2./3.14159265358979323);
	       IntHilbertCoefficient[i-1][kmax-1]=
		    (int)(HilbertCoefficient[i-1][kmax-1]*4096.+0.5);
	  }
     }
}

double hilbertFilter(int k,double *a)
{
/* the user is responsible for validity of the pointers 
 *(a-k) to *(a+(k-1)), so for a[i] in a[n] call this 
 for k < i < n-k  and reduce the order beyond here.

 The pointer points to the data item just to the right of
 the filter center point. (i.e. filter is antisymmetric about (a-0.5)).
*/
     int i;
     double result;
     if (k>10) k=10;
     result=0;
     for (i=0; i<k; i++){
	  result+=HilbertCoefficient[i][k]* (*(a+i) - *(a-(i+1)));
     }
     return result;
}

double hilbertSymFilter(int k,double *a)
{
/* This is a zero phase shift filter with the sam degree of (non)locality
   as the Hilbert filter.  The frequency response is conjugate (swap Nyquist 
   and zero frequencies.)  Same caveats as hilbertFilter re: pointers.
*/
     int i;
     double result, sign;
     if (k>10) k=10;
     result=0;
     sign=1.
     for (i=0; i<k; i++){
	  result+=HilbertCoefficient[i][k]* (*(a+i) + *(a-(i+1)))*sign;
	  sign *=-1.;
     }
     return result;
}

/* below add scaled int versions of last two functions */
