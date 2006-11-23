#include "BaselineAnalysis.h"
#include <math.h>
#include <stdio.h>

//arrays here are column index first, 
//but remember det(At) = det(A)
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

int FitBaselines(BaselineAnalysis_t *theBL){
     int nused,status,status2;
     float statA[3][MAX_BASELINES],statB[MAX_BASELINES],
	  statATA[3][3],statATB[3],
	  statATAL[3][3],lambda,oldlambda,oldnorm,tempfloat;
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
     theBL->ngood=nused; //eventually will need to deal with cutting baselines
     //weight baselines by the square of their length; 
     // cos(theta) has uniform contribution from timing errors
     for (i=0; i<3; i++){
	  statATB[i]=0.;
	  for (k=0;k<nused;k++){
	       statATB[i]+=statA[i][k]*statB[k]
		    *(theBL->length[k]*theBL->length[k]);
	  }
	  for (j=0; j<3; j++){
	       statATA[i][j]=0.;
	       for (k=0; k<nused; k++){
		    statATA[i][j]+=statA[i][k]*statA[j][k]
			 *(theBL->length[k]*theBL->length[k]);
	       }
	  }
     }
     status=solve3(statATA,statATB,theBL->arrival);
     theBL->norm=sqrtf(theBL->arrival[0]*theBL->arrival[0]
			    +theBL->arrival[1]*theBL->arrival[1]
			    +theBL->arrival[2]*theBL->arrival[2]);
     //need to use newtons method to find the Lagrange multiplier
     //to enforce the constraint that arrival[] is a unit vector
     theBL->niter=0;
     oldlambda=0.;
     theBL->lambda=0.;
     printf("Initial:%i %f\n",theBL->niter,theBL->norm);
     if (status==0) { 
	  while (fabsf(theBL->norm-1.)>0.001){
	       if (theBL->niter==0) {
		    theBL->lambda=theBL->norm-1.;
	       }
	       else {
		    tempfloat=theBL->lambda-
			 ((theBL->lambda-oldlambda)/(theBL->norm-oldnorm))
			 *(theBL->norm-1);
		    oldlambda=theBL->lambda;
		    theBL->lambda=tempfloat;
	       }
	       for (i=0; i<3; i++){
		    for (j=0; j<3; j++){
			 statATAL[i][j]=statATA[i][j];
		    }
		    statATAL[i][i]+=theBL->lambda;
	       }
	       status=solve3(statATAL,statATB,theBL->arrival);
	       theBL->niter++;
	       if (status==0){
		    oldnorm=theBL->norm;
		    theBL->norm=sqrtf(theBL->arrival[0]*theBL->arrival[0]
				      +theBL->arrival[1]*theBL->arrival[1]
				      +theBL->arrival[2]*theBL->arrival[2]);
		    printf("Newton:%i %f %f\n",theBL->niter,theBL->norm,
			   theBL->lambda);
	       }
	       else {
		    status=-2;
		    break;
	       }
	       if (theBL->niter==MAX_ITER){
		    status=-3;
		    break;
	       }
	  }
     }
     if (status==0 || status==-3){
	  theBL->arrival[0]=theBL->arrival[0]/theBL->norm; 
	  theBL->arrival[1]=theBL->arrival[1]/theBL->norm; 
	  theBL->arrival[2]=theBL->arrival[2]/theBL->norm;
     }
     theBL->validfit=status;
     return status;
}
