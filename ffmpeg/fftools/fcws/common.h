#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
//#include <string>
//#include <vector>
//#include <list>
//#include <algorithm>
#include <pthread.h>
#include <SDL.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>

#define DEBUG

#ifdef DEBUG
#define dbg(format, args...) printf("[%s][%s][%d] " format "\n", __FILE__, __func__, __LINE__, ##args)
#else
#define dbg(format, args...)
#endif

#ifndef bool
#define bool unsigned char
#endif

#ifndef true
#define true (1)
#endif

#ifndef false
#define false (0)
#endif

// Macro
#define FreeVector(m) do {                  \
    if ((m)) {                              \
        gsl_vector_free((m));               \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrix(m) do {                  \
    if ((m)) {                              \
        gsl_matrix_free((m));               \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrixUshort(m) do {            \
    if ((m)) {                              \
        gsl_matrix_ushort_free((m));        \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrixChar(m) do {              \
    if ((m)) {                              \
        gsl_matrix_char_free((m));          \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

bool CheckOrReallocVector(gsl_vector** v, int size);

bool CheckOrReallocMatrix(gsl_matrix** m, int h, int w);

bool CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w);

bool CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w);

#endif
