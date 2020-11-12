#ifndef _CONTROLLOOP_H_
#define _CONTROLLOOP_H_

void controlLoopDataUpdate (void);
void calculateExpectedSgv (int *, int *, int, char);
int  calculateAdjustedSensitivity (void);
void calculateAdjustedValues (void);
//int linRegres(int numElements, int *sgvDeriv, float *instantIOB, 
//              float* sensibility, float* basal);

#endif
