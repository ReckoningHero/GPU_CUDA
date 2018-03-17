#ifndef FFT_H_
#define FFT_H_


#include "stdafx.h"


void seedInitialFFTData(void);
void updateFFTPhases(void);
void performFFTforHeights(void);
void performFFTforChop(void);
void extractNormals(void);
#endif
