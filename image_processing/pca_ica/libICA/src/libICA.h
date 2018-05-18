/**
 * @file libICA.h
 * 
 * Main FastICA functions.
 */

#ifndef LIBICA_H_
#define LIBICA_H_

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#define MAX_ITERATIONS	1000
#define TOLERANCE		0.0001
#define ALPHA			1

void fastICA(double** X, int rows, int cols, int compc, double** K, double** W, double** A, double** S);
int FastICA_GSL(gsl_matrix *X,int compc,gsl_matrix *K,gsl_matrix *W,gsl_matrix *A,gsl_matrix *S);
#endif /*LIBICA_H_*/
