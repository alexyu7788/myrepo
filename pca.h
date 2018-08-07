#ifndef __PCA_H__
#define __PCA_H__

#include <stdint.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>

//#include "matrix.h"

#define FIRST_K_COMPONENTS 100

/* function definition */
//uint8_t* cal_mean_vector(const uint8_t **img, int rows, int cols);
//int16_t** cal_covariance_matrix(const uint8_t **src, const uint8_t *mean, int rows, int cols);
//mat generate_pca_base_vector(gsl_vector *eval, gsl_matrix *evec, int first_k_component, int rows);
//mat generate_pca_projection_vector(mat p, uint8_t **img, uint8_t *mean, int first_k_component, int rows, int cols);
//mat reconstruct_image_from_pca_parameter(mat p, mat b, int first_k_component, int rows, int cols);
//mat generate_residual_image(uint8_t **img, mat img_recon, int rows, int cols);



typedef struct pca_s
{
	gsl_matrix *mean;
	gsl_matrix *covar;
	gsl_matrix *principal_component;
	gsl_matrix *projection;
	gsl_matrix *reconstruction;
	gsl_matrix *residual;
}pca_t;

gsl_vector* cal_mean(gsl_matrix *m);
gsl_matrix* cal_covariance(gsl_matrix *m, gsl_vector *mean);
int cal_eigenvalue_eigenvector(gsl_matrix *m, gsl_vector **eval, gsl_matrix **evec);
gsl_matrix_view get_first_k_components(gsl_matrix *m, int firstk);
gsl_matrix* generate_projection_matrix(gsl_matrix *p, gsl_matrix *x);
gsl_matrix* generate_reconstruct_image_matrix_from_pca(gsl_matrix *p, gsl_matrix *r, gsl_vector *mean);
gsl_matrix* generate_residual_image_matrix(gsl_matrix *orignal_image_matrix, gsl_matrix *reconstruct_image_matrix);
#endif
