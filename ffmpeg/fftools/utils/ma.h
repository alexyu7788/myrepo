#ifndef __MA_H__
#define __MA_H__

#include "common.hpp"

void FreeVector(gsl_vector** v);

void FreeMatrix(gsl_matrix** m);

void FreeMatrixUshort(gsl_matrix_ushort** m);

void FreeMatrixChar(gsl_matrix_char** m);

bool CheckOrReallocVector(gsl_vector** v, int size, bool reset);

bool CheckOrReallocMatrix(gsl_matrix** m, int h, int w, bool reset);

bool CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w, bool reset);

bool CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w, bool reset);

#endif
