#ifndef HILBERTMATH_H
#define HILBERTMATH_H

#include "Source.h"
#include <complex>
#include <valarray>

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

void fft(CArray& x);
void ifft(CArray& x);
int maxHilbert(int xIn[], int inLength);


#endif