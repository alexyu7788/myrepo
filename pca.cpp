#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_blas.h>

#include "pca.h"
#include "utility.h"


//uint8_t* cal_mean_vector(const uint8_t **img, int rows, int cols)
//{
//    uint8_t *mean = NULL;
//    int r,c;
//
//    mean = (uint8_t*)malloc(sizeof(uint8_t)*rows);
//
//    if (mean == NULL)
//    {
//        fprintf(stdout, "Can not allocate mean vector\n");
//        exit(-1);
//    }
//
//    for (r=0 ; r<rows ; ++r)
//    {
//        double temp_mean = 0;
//        for (c=0 ; c<cols ; ++c)
//        {
//            temp_mean += img[r][c];
//        }
//        mean[r] = (uint8_t)(temp_mean / c);
//    }
//
//    return mean;
//}
//
//
//int16_t** cal_covariance_matrix(const uint8_t **src, const uint8_t *mean, int rows, int cols)
//{
//    int r,c,k;
//    int16_t **diff = NULL;
//    int16_t **covar = NULL;
//
//    diff    = (int16_t**)malloc(sizeof(int16_t*)*rows);
//    for (r=0 ; r<rows ; ++r)
//        diff[r]     = (int16_t*)malloc(sizeof(int16_t)*cols);
//
//    covar   = (int16_t**)malloc(sizeof(int16_t*)*rows);
//    for (r=0 ; r<rows ; ++r)
//        covar[r]    = (int16_t*)malloc(sizeof(int16_t)*rows);
//
//    /* calculate difference between src and mean. */
//    for (c=0 ; c<cols ; ++c)
//        for (r=0 ; r<rows ; ++r)
//            diff[r][c] = (src[r][c] - mean[r]);
//
//    /* calculate co-variance matrix. */
//    for (r=0 ; r<rows ; ++r)
//    {
//        for (c=0 ; c<rows ; ++c)
//        {
//            int32_t d=0;
//            for (k=0 ; k<cols ; ++k)
//            {
//                d += (diff[r][k] * diff[c][k]);
//            }
//
//            covar[r][c] = (d / cols);
//        }
//    }
//
//    for (r=0 ; r<rows ; ++r)
//        free(diff[r]);
//    free(diff);
//
//    return covar;
//}
//
///* P:first k eigenvectors corresponding to first k max. eigenvalues. */
//mat
//generate_pca_base_vector
//(
// gsl_vector *eval,
// gsl_matrix *evec,
// int first_k_component,
// int rows
//)
//{
//    int r,c;
//    mat p = NULL;
//
//    p = mat_create(rows, first_k_component);
//
//    for (c=0 ; c<first_k_component ; ++c)
//    {
//        //double eval_i;
//        gsl_vector_view evec_i;
//
//        //eval_i  = gsl_vector_get(eval, c);
//        evec_i  = gsl_matrix_column(evec, c);
//
//        if (evec_i.vector.block != NULL)
//        {
//            for (r=0 ; r<evec_i.vector.block->size ; ++r)
//            {
//                if (r%evec_i.vector.size == 0)
//                {
//                    p[r/evec_i.vector.size][c] = (scal)evec_i.vector.block->data[c+r];
//                }
//            }
//        }
//    }
//
//    return p;
//}
//
///* b[i] = (P-transport) * (x[i] - x-mean) */
//mat
//generate_pca_projection_vector
//(
// mat p,
// uint8_t **img,
// uint8_t *mean,
// int first_k_component,
// int rows,
// int cols
//)
//{
//    int r,c,k;
//    mat b = NULL;
//
//    b = mat_create(first_k_component, cols);
//
//    for (c=0 ; c<cols ; ++c)
//    {
//        for (r=0 ; r<first_k_component ; ++r)
//        {
//            scal temp = 0.0;
//            for (k=0 ; k<rows ; ++k)
//                temp += (p[k][r] * (img[k][c] - mean[k]));
//
//            b[r][c] = temp;
//        }
//    }
//
//    return b;
//}
//
///* X-reconstruct[i] = P * b[i] + X-mean */
//mat
//reconstruct_image_from_pca_parameter
//(
// mat p,
// mat b,
// int first_k_component,
// int rows,
// int cols
//)
//{
//    int     r,c,k;
//    mat     img_recon = NULL;
//    scal    mini = 1e+7;
//
//    img_recon = mat_create(rows, cols);
//
//    for (c=0 ; c<cols ; ++c)
//    {
//        for (r=0 ; r<rows ; ++r)
//        {
//            scal temp = 0.0;
//
//            for (k=0 ; k<first_k_component ; ++k)
//                temp += (p[r][k] * b[k][c]);
//
//            img_recon[r][c] = temp;
//        }
//    }
//
//    /* shifting. */
//    for (c=0 ; c<cols ; ++c)
//    {
//        mini = 1e+7;
//        /* find mini. */
//        for (r=0 ; r<rows ; ++r)
//            if (mini > img_recon[r][c])
//                mini = img_recon[r][c];
//
//        for (r=0 ; r<rows ; ++r)
//            img_recon[r][c] = (img_recon[r][c] + fabs(mini));
//    }
//
//    return img_recon;
//}
//
//mat
//generate_residual_image
//(
// uint8_t **img,
// mat img_recon,
// int rows,
// int cols
//)
//{
//    int r,c;
//    mat residual = NULL;
//
//    residual = mat_create(rows, cols);
//
//    for (c=0 ; c<cols ; ++c)
//        for (r=0 ; r<rows ; ++r)
//            residual[r][c] = ((scal)img[r][c] - img_recon[r][c]);
//
//    return residual;
//}

gsl_vector* cal_mean(gsl_matrix *m)
{
	int i, j;
	int rows, cols;
	gsl_vector *mean = NULL;
	float value;
	unsigned long long start_time, end_time;

	start_time = GetTime();

	assert(m != NULL);
	rows = m->size1;
	cols = m->size2;
//	printf("rows:%d, cols:%d\n", rows, cols);

	mean = gsl_vector_alloc(rows);
	gsl_vector_set_zero(mean);

	for (i=0 ; i<rows ; i++)
	{
		value = gsl_stats_mean(m->data + i * cols, 1, cols);
//		printf("mean[%d]=%f\n", i, value);
		gsl_vector_set(mean, i, value);
	}

	end_time = GetTime();
	printf("time costs of %s %llu ms\n", __func__, end_time - start_time);

	return mean;
}

gsl_matrix* cal_covariance(gsl_matrix *m, gsl_vector *mean)
{
	assert(m != NULL);
	assert(mean != NULL);

	int i;
	int rows, cols;
	unsigned long long start_time, end_time;

	start_time = GetTime();

	rows = m->size1;
	cols = m->size2;

//	printf("%s: rows:%d, cols: %d\n", __func__, rows, cols);
	gsl_matrix *mean_subtracted_m, *covariance_matrix;
	gsl_vector_view mean_subtracted_m_cloumn_view;

	mean_subtracted_m = gsl_matrix_alloc(rows, cols);
	gsl_matrix_memcpy(mean_subtracted_m, m);

	for (i=0 ; i<cols ; i++)
	{
		mean_subtracted_m_cloumn_view = gsl_matrix_column(mean_subtracted_m, i);
		gsl_vector_sub(&mean_subtracted_m_cloumn_view.vector, mean);
	}

	covariance_matrix = gsl_matrix_alloc(rows, rows);
	gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0 / (double)(cols - 1), mean_subtracted_m, mean_subtracted_m, 0.0, covariance_matrix);

	gsl_matrix_free(mean_subtracted_m);

//	double max, min;
//	gsl_matrix_minmax(covariance_matrix, &min, &max);
//	printf("%s: max: %f, min: %f\n", __func__, max, min);

	end_time = GetTime();
	printf("time costs of %s %llu ms\n", __func__, end_time - start_time);

	return covariance_matrix;
}

int cal_eigenvalue_eigenvector(gsl_matrix *m, gsl_vector **eval, gsl_matrix **evec)
{
	assert(m != NULL);

	int i, rows, cols, ret = 0;
	gsl_vector *pEValue;
	gsl_matrix *pEVctor;
    gsl_eigen_symmv_workspace* workspace;

	unsigned long long start_time, end_time;

	start_time = GetTime();

	rows = m->size1;
	cols = m->size2;
//	printf("%s: rows:%d, cols: %d\n", __func__, rows, cols);

	pEValue = gsl_vector_alloc(rows);
	gsl_vector_set_zero(pEValue);

	pEVctor = gsl_matrix_alloc(rows, rows);
	gsl_matrix_set_zero(pEVctor);

	workspace = gsl_eigen_symmv_alloc(rows);
	ret = gsl_eigen_symmv(m, pEValue, pEVctor, workspace);
    gsl_eigen_symmv_free(workspace);

    if (ret == GSL_SUCCESS)
    {
    	gsl_eigen_symmv_sort(pEValue, pEVctor, GSL_EIGEN_SORT_VAL_DESC);

		*eval = pEValue;
		*evec = pEVctor;
    }
    else
    {
    	gsl_vector_free(pEValue);
    	gsl_matrix_free(pEVctor);
    }

	end_time = GetTime();
	printf("time costs of %s %llu ms\n", __func__, end_time - start_time);

	return ret;
}

/*
 * m is matrix of eigen-vectors, [ E1 E2 ... En ]
 *
 */
gsl_matrix_view get_first_k_components(gsl_matrix *m, int firstk)
{
	return gsl_matrix_submatrix(m, 0, 0, m->size1, firstk);
}

/*
 * R = P-Transpose * X, where R is KxM, P is NxK, X is NxM
 */
gsl_matrix* generate_projection_matrix(gsl_matrix *p, gsl_matrix *x)
{
	assert(p != NULL);
	assert(x != NULL);

	int rows, cols;
	gsl_matrix *r = NULL;

	rows = p->size2;	// K
	cols = x->size2;	// M

	r = gsl_matrix_alloc(rows, cols);
	gsl_matrix_set_zero(r);

	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, p, x, 0.0, r);

	return r;
}

// X` = P X R, where X` is NxM, P is NxK, R is KxM
gsl_matrix* generate_reconstruct_image_matrix_from_pca(gsl_matrix *p, gsl_matrix *r, gsl_vector *mean)
{
	assert(p != NULL);
	assert(r != NULL);
	assert(mean != NULL);

	int i, j;
	int rows, cols;
	unsigned char pix_datat;
	double max, min, temp_data;
	gsl_matrix *x = NULL;
	gsl_vector_view x_col_view;
//	gsl_matrix_uchar *x = NULL;

	rows = p->size1;
	cols = r->size2;

	x = gsl_matrix_alloc(rows, cols);
	gsl_matrix_set_zero(x);

//	x = gsl_matrix_uchar_alloc(rows, cols);
//	gsl_matrix_uchar_set_zero(x);

	// X`= P X R.
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, p, r, 0.0, x);

	// Add mean
	for (i=0 ; i<cols ; i++)
	{
		x_col_view = gsl_matrix_column(x, i);
		gsl_vector_add(&x_col_view.vector, mean);
	}

	// Get max and min value for normalization.
	gsl_matrix_minmax(x, &min, &max);
//	printf("min %lf, max %lf\n", min, max);

	for (i=0 ; i<rows ; i++)
	{
		for (j=0 ; j<cols ; j++)
		{
			temp_data = gsl_matrix_get(x, i, j);
			temp_data = ((temp_data - min) / (max - min)) * 255;
			gsl_matrix_set(x, i, j, temp_data);
		}
	}

	gsl_matrix_minmax(x, &min, &max);
//	printf("min %lf, max %lf\n", min, max);

//	for (i=0 ; i<rows ; i++)
//	{
//		for (j=0 ; j<cols ; j++)
//		{
//			pix_datat = (unsigned char)gsl_matrix_get(x_temp, i, j);
//			gsl_matrix_uchar_set(x, i, j, pix_datat);
//		}
//	}
//
//	if (x_temp)
//		gsl_matrix_free(x_temp);

	return x;
}

gsl_matrix* generate_residual_image_matrix(gsl_matrix *orignal_image_matrix, gsl_matrix *reconstruct_image_matrix)
{
	assert(orignal_image_matrix != NULL);
	assert(reconstruct_image_matrix != NULL);
	assert(orignal_image_matrix->size1 == reconstruct_image_matrix->size1);
	assert(orignal_image_matrix->size2 == reconstruct_image_matrix->size2);

	int i, j, rows, cols;
	double min, max, pixel_data;

#if 1
	gsl_matrix *output_im = NULL;

	rows = orignal_image_matrix->size1;
	cols = orignal_image_matrix->size2;

	output_im = gsl_matrix_alloc(rows, cols);
	gsl_matrix_memcpy(output_im, orignal_image_matrix);

	// Residual = Orignal - Reconstruct.
	gsl_matrix_sub(output_im , reconstruct_image_matrix);

	// Get max and min value for normalization.
	gsl_matrix_minmax(output_im, &min, &max);

	for (i=0 ; i<rows ; i++)
	{
		for (j=0 ; j<cols ; j++)
		{
			pixel_data = gsl_matrix_get(output_im, i, j);
			pixel_data = ((pixel_data - min) / (max - min)) * 255;
			gsl_matrix_set(output_im, i, j, pixel_data);
		}
	}

	return output_im;
#else
	gsl_matrix *d_temp_im = NULL, *d_temp_im2 = NULL;
	gsl_vector_view d_im_column_view;
	gsl_matrix_uchar *s_output_im = NULL;
	gsl_vector_uchar_view s_im_column_view;

	rows = orignal_image_matrix->size1;
	cols = orignal_image_matrix->size2;

	// Get image matrix of type of unsigned char. Store to residual image matrix.
	d_temp_im = gsl_matrix_alloc(rows, cols);
	d_temp_im2 = gsl_matrix_alloc(rows, cols);
	s_output_im = gsl_matrix_uchar_alloc(rows, cols);

	gsl_matrix_memcpy(d_temp_im, orignal_image_matrix);

	for (i=0 ; i<cols ; i++)
	{
		d_im_column_view = gsl_matrix_column(d_temp_im2, i);
		s_im_column_view = gsl_matrix_uchar_column(reconstruct_image_matrix, i);

		for (j=0 ; j<rows ; j++)
		{
			gsl_vector_set(&d_im_column_view.vector, j, gsl_vector_uchar_get(&s_im_column_view.vector, j));
		}
	}

	// Residual = Orignal - Reconstruct.
	gsl_matrix_sub(d_temp_im, d_temp_im2);

	// Get max and min value for normalization.
	gsl_matrix_minmax(d_temp_im, &min, &max);

	for (i=0 ; i<rows ; i++)
	{
		for (j=0 ; j<cols ; j++)
		{
			pixel_data = gsl_matrix_get(d_temp_im, i, j);
			pixel_data = ((pixel_data - min) / (max - min)) * 255;
			gsl_matrix_set(d_temp_im, i, j, pixel_data);
		}
	}

	// Convert double to uchar
	for (i=0 ; i<cols ; i++)
	{
		d_im_column_view = gsl_matrix_column(d_temp_im, i);
		s_im_column_view = gsl_matrix_uchar_column(s_output_im, i);

		for (j=0 ; j<rows ; j++)
		{
			gsl_vector_uchar_set(&s_im_column_view.vector, j, gsl_vector_get(&d_im_column_view.vector, j));
		}
	}


	gsl_matrix_free(d_temp_im);
	gsl_matrix_free(d_temp_im2);

	return s_output_im;
#endif
}
