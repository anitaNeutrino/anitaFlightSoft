#include "BaselineAnalysis.h"
#include <math.h>

//arrays here are column index first, 
//but remember det(At) = det A


float det3(float a[3][3]){//3x3 determinant
                          //passing a single **float
return a[0][0]*a[1][1]*a[2][2]-a[0][2]*a[1][1]*a[2][0]
     + a[0][1]*a[1][2]*a[2][0]-a[0][1]*a[1][0]*a[2][2]
     + a[0][2]*a[1][0]*a[2][1]-a[0][0]*a[1][2]*a[2][1];
}

float det3p(float *a[3]){//3x3 determinant 
                         //using pointers to the columns
return a[0][0]*a[1][1]*a[2][2]-a[0][2]*a[1][1]*a[2][0]
     + a[0][1]*a[1][2]*a[2][0]-a[0][1]*a[1][0]*a[2][2]
     + a[0][2]*a[1][0]*a[2][1]-a[0][0]*a[1][2]*a[2][1];
}

int solve3(float a[3][3], float b[3], float x[3]){
     //solve Ax=b using Cramer's rule for a 3x3
     //the pointer arra u is used to sub b for
     //the columns of A one by one without too
     //much copying overhead.
     float *u[3],denom,num[3];
     int i,j;
     denom=det3(a);
     if (denom==0.) return -1;
     for (i=0;i<3;i++){
	  for (j=0; j<3; j++){
	       u[j]=a[j];    
	  }
	  u[i]=b;
	  num[i]=det3p(u);
	  x[i]=num[i]/denom;
     }
     return 0;
}

int fitBaselines(BaselineAnalysis_t *theBL){
     int nused,status;
     float statA[3][MAX_BASELINES],statB[MAX_BASELINES],
	  statATA[3][3],statATB[3];
     // direction in the struct is A transpose (column index first)
     // cosangle is the column vector b
     int i,j,k;
     nused=0;
     for (i=0; i<theBL->nbaselines; i++){
	  if (theBL->bad[i]==0){
	       for (j=0; j<3; j++){
		    statA[j][nused]=theBL->direction[i][j];
	       }
	       statB[nused]=theBL->cosangle[i];
	       nused++;
	  }
     }
     for (i=0; i<3; i++){
	  statATB[i]=0.;
	  for (k=0;k<nused;k++){
	       statATB[i]+=statA[i][k]*statB[k];
	  }
	  for (j=0; j<3; j++){
	       statATA[i][j]=0.;
	       for (k=0; k<nused; k++){
		    statATA[i][j]+=statA[i][k]*statA[j][k];
	       }
	  }
     }
     status=solve3(statATA,statB,theBL->arrival);
     if (status==0) { //normalization may be worth looking at 
	              // as a goodness of fit measure
	  theBL->norm=theBL->arrival[0]*theBL->arrival[0]
	       +theBL->arrival[1]*theBL->arrival[1]
	       +theBL->arrival[2]*theBL->arrival[2];
	  theBL->arrival[0]=theBL->arrival[0]/theBL->norm; 
	  theBL->arrival[1]=theBL->arrival[1]/theBL->norm; 
	  theBL->arrival[2]=theBL->arrival[2]/theBL->norm;
     }
     return status;
}
