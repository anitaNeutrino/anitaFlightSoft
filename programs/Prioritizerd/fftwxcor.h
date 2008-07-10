#ifndef FFTWXCOR_H
#define FFTWXCOR_H
#ifdef __cplusplus
extern "C"
{
#endif

int fftwcorr(float wfm[], float tmpl[], float cout[]);
int fftwcorr_anal_1x(float wfm[], float tmpl[], float cout[]);
int fftwcorr_anal_2x(float wfm[], float tmpl[], float cout[]);

#ifdef __cplusplus
}
#endif
#endif
