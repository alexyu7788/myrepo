/**
 * @file libICA.h
 * 
 * Main FastICA functions.
 */

#ifndef LIBICA_H_
#define LIBICA_H_

// #include "matrix.h"

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>

#define MAX_ITERATIONS	1000
#define TOLERANCE		0.0001
#define ALPHA			1

#define gsl_matrix mat

void fastICA(mat X, int rows, int cols, int compc, mat K, mat W, mat A, mat S);

#endif /*LIBICA_H_*/
