/**
 * @file libICA.c
 * 
 * Main FastICA functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libICA.h"
#include "matrix.h"
#include "svdcmp.h"


/*
 * Functions for matrix elements transformations
 * used in mat_apply_fx().
 */
static scal fx_inv(scal x, scal par)
{
	return (1/x);
}

static scal fx_inv_sqrt(scal x, scal par)
{
	return (1/sqrt(x));	
}

static scal fx_div_c(scal x, scal par)
{
	return (x/par);
}

static scal fx_rand(scal x, scal par)
{
	return (scal)rand()/RAND_MAX; 
}

static scal fx_tanh(scal x, scal par)
{
	return tanh(ALPHA * x);
}

static scal fx_1sub_sqr(scal x, scal par)
{
	return (ALPHA * (1-x*x));
}


/**
 * ICA function. Computes the W matrix from the
 * preprocessed data.
 */
static mat ICA_compute(mat X, int rows, int cols)
{
	mat TXp, GWX, W, Wd, W1, D, TU, TMP;
	vect d, lim;
	int i, it;

	// matrix creation
	TXp = mat_create(cols, rows);
	GWX = mat_create(rows, cols);
	W = mat_create(rows, rows);
	Wd = mat_create(rows, rows);
	D = mat_create(rows, rows);
	TMP = mat_create(rows, rows);
	TU = mat_create(rows, rows);
	W1 = mat_create(rows, rows);
	d = vect_create(rows);

	// W rand init
	mat_apply_fx(W, rows, rows, fx_rand, 0);

	// sW <- La.svd(W)
	mat_copy(W, rows, rows, Wd);
	svdcmp(Wd, rows, rows, d, D);

	// W <- sW$u %*% diag(1/sW$d) %*% t(sW$u) %*% W
	mat_transpose(Wd, rows, rows, TU);
	vect_apply_fx(d, rows, fx_inv, 0);
	mat_diag(d, rows, D);
	mat_mult(Wd, rows, rows, D, rows, rows, TMP);
	mat_mult(TMP, rows, rows, TU, rows, rows, D);
	mat_mult(D, rows, rows, W, rows, rows, Wd); // W = Wd

	// W1 <- W 
	mat_copy(Wd, rows, rows, W1);

	// lim <- rep(1000, maxit); it = 1
	lim = vect_create(MAX_ITERATIONS);
	for (i=0; i<MAX_ITERATIONS; i++)
		lim[i] = 1000;
	it = 0;


	// t(X)/p
	mat_transpose(X, rows, cols, TXp);
	mat_apply_fx(TXp, cols, rows, fx_div_c, cols);

	while (lim[it] > TOLERANCE && it < MAX_ITERATIONS) {
		// wx <- W %*% X
		mat_mult(Wd, rows, rows, X, rows, cols, GWX);

		// gwx <- tanh(alpha * wx)
		mat_apply_fx(GWX, rows, cols, fx_tanh, 0);
		
		// v1 <- gwx %*% t(X)/p
		mat_mult(GWX, rows, cols, TXp, cols, rows, TMP); // V1 = TMP
		
		// g.wx <- alpha * (1 - (gwx)^2)
		mat_apply_fx(GWX, rows, cols, fx_1sub_sqr, 0);

		// v2 <- diag(apply(g.wx, 1, FUN = mean)) %*% W
		mat_mean_rows(GWX, rows, cols, d);
		mat_diag(d, rows, D);
		mat_mult(D, rows, rows, Wd, rows, rows, TU); // V2 = TU

		// W1 <- v1 - v2
		mat_sub(TMP, TU, rows, rows, W1);
		
		// sW1 <- La.svd(W1)
		mat_copy(W1, rows, rows, W);
		svdcmp(W, rows, rows, d, D);

		// W1 <- sW1$u %*% diag(1/sW1$d) %*% t(sW1$u) %*% W1
		mat_transpose(W, rows, rows, TU);
		vect_apply_fx(d, rows, fx_inv, 0);
		mat_diag(d, rows, D);
		mat_mult(W, rows, rows, D, rows, rows, TMP);
		mat_mult(TMP, rows, rows, TU, rows, rows, D);
		mat_mult(D, rows, rows, W1, rows, rows, W); // W1 = W
		
		// lim[it + 1] <- max(Mod(Mod(diag(W1 %*% t(W))) - 1))
		mat_transpose(Wd, rows, rows, TU);
		mat_mult(W, rows, rows, TU, rows, rows, TMP);

		lim[it] = fabs(mat_max_diag(TMP, rows, rows) - 1);

		// W <- W1
		mat_copy(W, rows, rows, Wd);

		it++;
	}

	// clean up
	mat_delete(TXp, cols, rows);
	mat_delete(GWX, rows, cols);
	mat_delete(W, rows, rows);
	mat_delete(D, rows, rows);
	mat_delete(TMP, rows, rows);
	mat_delete(TU, rows, rows);
	mat_delete(W1, rows, rows);
	vect_delete(d);	

	return Wd;
}

gsl_vector*	CalMean(gsl_matrix *m)
{
	int i, j;
	int rows, cols;
	gsl_vector *mean = NULL;
	float value;

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

	return mean;
}

gsl_matrix* ICA_compute_GSL(gsl_matrix *X)
{
	gsl_matrix *TXp, *GWX, *W, *Wd, *W1, *D, *TU, *TMP, *SVD_WorkMat;
	gsl_vector *d, *lim, *SVD_WorkVect;
	int i, j, it, rows, cols;
	double max, val;

	rows = X->size1;
	cols = X->size2;

	TXp = gsl_matrix_alloc(cols, rows);
	GWX = gsl_matrix_alloc(rows, cols);
	W 	= gsl_matrix_alloc(rows, rows);
	Wd 	= gsl_matrix_alloc(rows, rows);
	D 	= gsl_matrix_alloc(rows, rows);
	TMP = gsl_matrix_alloc(rows, rows);
	TU 	= gsl_matrix_alloc(rows, rows);
	W1  = gsl_matrix_alloc(rows, rows);
	d 	= gsl_vector_alloc(rows);

	// W rand init
	srand(time(NULL));
	 
	for (i=0 ; i<W->size1 ; i++)
		for (j=0 ; j<W->size2 ; j++)
			gsl_matrix_set(W, i, j, rand()/(double)RAND_MAX);

	// sW <- La.svd(W)
	gsl_matrix_memcpy(Wd, W);
	// gsl_linalg_SV_decomp_jacobi(Wd, D, d);	//?????

	SVD_WorkMat = gsl_matrix_alloc(Wd->size2, Wd->size2);
	SVD_WorkVect = gsl_vector_alloc(Wd->size2);
	// printf("%s %d: %dx%d, %d\n", __func__, __LINE__, SVD_WorkMat->size1, SVD_WorkMat->size2, SVD_WorkVect->size);
	gsl_linalg_SV_decomp_mod(Wd, SVD_WorkMat, D, d, SVD_WorkVect);	

	if (SVD_WorkMat)
	{
		gsl_matrix_free(SVD_WorkMat);
		SVD_WorkMat = NULL;
	}

	if (SVD_WorkVect)
	{
		gsl_vector_free(SVD_WorkVect);
		SVD_WorkVect = NULL;
	}

	// W <- sW$u %*% diag(1/sW$d) %*% t(sW$u) %*% W
	gsl_matrix_transpose_memcpy(TU, Wd);
	
	for (i=0 ; i<d->size ; i++)
		gsl_vector_set(d, i, 1 / gsl_vector_get(d, i));

	gsl_matrix_set_zero(D);

	for (i=0 ; i<d->size ; i++)
		gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Wd, D, 0.0, TMP);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, TMP, TU, 0.0, D);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, W, 0.0, Wd);

	// W1 <- W 
	gsl_matrix_memcpy(W1, Wd);

	// lim <- rep(1000, maxit); it = 1
	lim = gsl_vector_alloc(MAX_ITERATIONS+1);
	gsl_vector_set_zero(lim);
	gsl_vector_add_constant(lim, 1000.0);
	it = 0;

	// t(X)/p
	gsl_matrix_transpose_memcpy(TXp, X);
	gsl_matrix_scale(TXp, 1 / (double)TXp->size1);

	while (gsl_vector_get(lim, it) > TOLERANCE && it < MAX_ITERATIONS)
	{
		// wx <- W %*% X
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, Wd, X, 0.0, GWX);

		// gwx <- tanh(alpha * wx)
		for (i=0 ; i<GWX->size1 ; i++)
			for (j=0 ; j<GWX->size2 ; j++)
				gsl_matrix_set(GWX, i, j, tanh(ALPHA * gsl_matrix_get(GWX, i, j)));

		// v1 <- gwx %*% t(X)/p
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, GWX, TXp, 0.0, TMP);

		// g.wx <- alpha * (1 - (gwx)^2)
		for (i=0 ; i<GWX->size1 ; i++)
			for (j=0 ; j<GWX->size2 ; j++)
				gsl_matrix_set(GWX, i, j, ALPHA * ( 1 - gsl_pow_2(gsl_matrix_get(GWX, i, j))));

		// v2 <- diag(apply(g.wx, 1, FUN = mean)) %*% W
		gsl_vector_set_zero(d);
		d = CalMean(GWX);

		gsl_matrix_set_zero(D);
		for (i=0 ; i<d->size ; i++)
			gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, Wd, 0.0, TU);

		// W1 <- v1 - v2
		gsl_matrix_memcpy(W1, TMP);
		gsl_matrix_sub(W1, TU);

		// sW1 <- La.svd(W1)
		gsl_matrix_memcpy(W, W1);
		// gsl_linalg_SV_decomp_jacobi(W, D, d);	//?????
		if (SVD_WorkMat == NULL)
			SVD_WorkMat = gsl_matrix_alloc(W->size2, W->size2);

		if (SVD_WorkVect == NULL)
			SVD_WorkVect = gsl_vector_alloc(W->size2);
		gsl_linalg_SV_decomp_mod(W, SVD_WorkMat, D, d, SVD_WorkVect);

		// W1 <- sW1$u %*% diag(1/sW1$d) %*% t(sW1$u) %*% W1
		gsl_matrix_transpose_memcpy(TU, W);
		for (i=0 ; i<d->size ; i++)
			gsl_vector_set(d, i, 1 / gsl_vector_get(d, i));

		gsl_matrix_set_zero(D);
		for (i=0 ; i<d->size ; i++)
			gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, W, D, 0.0, TMP);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, TMP, TU, 0.0, D);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, W1, 0.0, W);

		// lim[it + 1] <- max(Mod(Mod(diag(W1 %*% t(W))) - 1))
		gsl_matrix_transpose_memcpy(TU, Wd);
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, W, TU, 0.0, TMP);

		gsl_vector_view diagonal = gsl_matrix_diagonal(TMP);
		max = gsl_vector_get(&diagonal.vector, 0);
		for (i=1 ; i<diagonal.vector.size ; i++)
		{
			val = gsl_vector_get(&diagonal.vector, i);
			if (val > max)
				max = val;
		}

		gsl_vector_set(lim, it, fabs(max - 1));

		// W <- W1
		gsl_matrix_memcpy(Wd, W);

		it++;
	}

	if (TXp)
		gsl_matrix_free(TXp);

	if (GWX)
		gsl_matrix_free(GWX);

	if (W)
		gsl_matrix_free(W);

	if (D)
		gsl_matrix_free(D);

	if (TMP)
		gsl_matrix_free(TMP);

	if (TU)
		gsl_matrix_free(TU);

	if (W1)
		gsl_matrix_free(W1);

	if (d)
		gsl_vector_free(d);

	if (SVD_WorkMat)
		gsl_matrix_free(SVD_WorkMat);

	if (SVD_WorkVect)
		gsl_vector_free(SVD_WorkVect);
	
	return Wd;
}

static void vect_print(char *name, int line, double *v, int dim)
{
	printf("mat_print: %s %d\n", name, line);

	int i;

	for (i=0 ; i<dim ; i++)
		printf("[%d] = %0.6f\n", i, v[i]);
}

static void mat_print(char *name, int line, double **M, int rows, int cols)
{
	printf("mat_print: %s %d\n", name, line);
	int i, j;

	for (i=0; i<rows; i++) {
		for (j=0; j<cols; j++) {
			printf("[%d][%d] = %0.6f", i, j, M[i][j]);
			if (j < cols - 1)
				printf(" ");
		}
		printf("\n");
	}
}

static gsl_vect_print(char *name, int line, gsl_vector *v)
{
	printf("gsl_vect_print: %s %d\n", name, line);

	int i, dim;

	dim = v->size;
	for (i=0 ; i<dim ; i++)
		printf("[%d] = %0.6f\n", i, gsl_vector_get(v, i));

}

static void gsl_mat_print(char *name, int line, gsl_matrix *m)
{
	printf("gsl_mat_print: %s %d\n", name, line);
	int i, j;
	int rows, cols;

	rows = m->size1;
	cols = m->size2;

	for (i=0; i<rows; i++) {
		for (j=0; j<cols; j++) {
			printf("[%d][%d] = %0.6f", i, j, gsl_matrix_get(m, i, j));
			if (j < cols - 1)
				printf(" ");
		}
		printf("\n");
	}
}
/**
 * Main FastICA function. Centers and whitens the input
 * matrix, calls the ICA computation function ICA_compute()
 * and computes the output matrixes.
 *
 * http://tumic.wz.cz/fel/online/libICA/
 *
 * X: 		pre-processed data matrix [rows, cols]
 * compc: 	number of components to be extracted
 * K: 		pre-whitening matrix that projects data onto the first compc principal components
 * W:		estimated un-mixing matrix
 * A: 		estimated mixing matrix
 * S:		estimated source matrix
 *
 * Ref: 	https://en.wikipedia.org/wiki/FastICA
 * 
 */
void fastICA(mat X, int rows, int cols, int compc, mat K, mat W, mat A, mat S)
{
	int i,j;
	mat XT, V, TU, D, X1, _A;
	vect scale, d;

	// matrix creation
	XT = mat_create(cols, rows);
	X1 = mat_create(compc, rows);
	V = mat_create(cols, cols);
	D = mat_create(cols, cols);
	TU = mat_create(cols, cols);
	scale = vect_create(cols);
	d = vect_create(cols);

	/*
	 * CENTERING
	 */
	mat_center(X, rows, cols, scale);


	/*
	 * WHITENING
	 */

	// X <- t(X); V <- X %*% t(X)/rows 
	mat_transpose(X, rows, cols, XT);
	mat_apply_fx(X, rows, cols, fx_div_c, rows);
	mat_mult(XT, cols, rows, X, rows, cols, V); // V is right

	// La.svd(V)
	// gsl_matrix *vv, *DD;
	// gsl_vector *dd;

	// vv = gsl_matrix_alloc(cols, cols);
	// DD = gsl_matrix_alloc(cols, cols);
	// dd = gsl_vector_alloc(cols);

	// mat_copy_gsl(V, cols, cols, vv);
	svdcmp(V, cols, cols, d, D);  // V = s$u, d = s$d, D = s$v
	vect_print("d", __LINE__, d, cols);
	// gsl_linalg_SV_decomp_jacobi(vv, DD, dd);

	// for (i=0 ; i<cols ; i++)
	// {
	// 	for (j=0 ; j<cols ; j++)
	// 	{
	// 		printf("V[%d][%d] = %.02lf, %.02lf\n", i,j, V[i][j], gsl_matrix_get(vv, i, j));
	// 	}
	// }

	// for (i=0 ; i<cols ; i++)
	// {
	// 	for (j=0 ; j<cols ; j++)
	// 	{
	// 		printf("D[%d][%d] = %.02lf, %.02lf\n", i,j, D[i][j], gsl_matrix_get(DD, i, j));
	// 	}
	// }

	// for (i=0 ; i<cols ; i++)
	// 	printf("d[%d]=%.02lf, %.02lf\n", i, d[i], gsl_vector_get(dd, i));

	// D <- diag(c(1/sqrt(d))
	vect_apply_fx(d, cols, fx_inv_sqrt, 0);	

	mat_diag(d, cols, D);

	// K <- D %*% t(U)
	mat_transpose(V, cols, cols, TU);
	mat_print("V", __LINE__, V, cols, cols);
	mat_print("TU", __LINE__, TU, cols, cols);
	mat_print("D", __LINE__, D, cols, cols);
	mat_mult(D, cols, cols, TU, cols, cols, V); // K = V 
	mat_print("V", __LINE__, V, cols, cols);
	// X1 <- K %*% X
	// gsl_matrix *VV = gsl_matrix_alloc(cols, cols);
	// gsl_matrix *XXTT = gsl_matrix_alloc(cols, rows);
	// gsl_matrix *XX1 = gsl_matrix_alloc(compc, rows);

	// mat_copy_gsl(V, cols, cols, VV);
	// mat_copy_gsl(XT, cols, rows, XXTT);

	mat_mult(V, compc, cols, XT, cols, rows, X1);
	// gsl_matrix_view sub_VV = gsl_matrix_submatrix(VV, 0, 0, compc, VV->size2);
	// gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &sub_VV.matrix, XXTT, 0.0, XX1);

	// for (i=0 ; i<compc ; i++)
	// {
	// 	for (j=0 ; j<rows ; j++)
	// 	{
	// 		printf("X1[%d][%d] = %lf, %lf\n", i, j, X1[i][j], gsl_matrix_get(XX1, i, j));
	// 	}
	// }


	/*
	 * FAST ICA
	 */
	
	// gsl_matrix *XX1 = gsl_matrix_alloc(compc, rows);
	// gsl_matrix *_AA;

	// mat_copy_gsl(X1, compc, rows, XX1);

	printf("====%d:%d====\n", compc, rows);

	_A = ICA_compute(X1, compc, rows);
	// _AA = ICA_compute_GSL(XX1);
	
	// printf("%d:%d %d:%d\n", compc, rows, _AA->size1, _AA->size2);
	// for (i=0 ; i<_AA->size1 ; i++)
	// 	for(j=0 ; j<_AA->size2 ; j++)
	// 		printf("_A[%d][%d] = %lf, %lf\n", i, j, _A[i][j], gsl_matrix_get(_AA, i, j));

	/*
	 * OUTPUT
	 */

	// X <- t(x)
	mat_transpose(XT, cols, rows, X);
	mat_decenter(X, rows, cols, scale);
	// mat_print(X, rows, cols);

	// K
	mat_transpose(V, compc, cols, K);
	// printf("%s %d:\n", __func__, __LINE__);
	// mat_print(V, compc, cols);
	// mat_print(K, cols, compc);

	// w <- a %*% K; S <- w %*% X
	mat_mult(_A, compc, compc, V, compc, cols, D);	
	mat_mult(D, compc, cols, XT, cols, rows, X1);
	// S
	mat_transpose(X1, compc, rows, S);

	// A <- t(w) %*% solve(w * t(w))
	mat_transpose(D, compc, compc, TU);
	mat_mult(D, compc, compc, TU, compc, compc, V);
	mat_inverse(V, compc, D);
	mat_mult(TU, compc, compc, D, compc, compc, V);
	// A
	mat_transpose(V, compc, compc, A);

	// W
	mat_transpose(_A, compc, compc, W);

	// cleanup
	mat_delete(XT, cols, rows);
	mat_delete(X1, compc, rows);
	mat_delete(V, cols, cols);
	mat_delete(D, cols, cols);
	mat_delete(TU,cols, cols);
	vect_delete(scale);
	vect_delete(d);
}

int FastICA_GSL
(
	gsl_matrix *X,
	int compc,
	gsl_matrix *K,
	gsl_matrix *W,
	gsl_matrix *A,
	gsl_matrix *S
)
{
	int i, j;
	int ret= 0, rows, cols;
	gsl_matrix *XT, *V, *TU, *D, *X1, *_A, *InvMat;
	gsl_vector *scale, *d, *mean;
	gsl_matrix_view subMat1, subMat2, subMat3;

	rows = X->size1;
	cols = X->size2;

	XT = gsl_matrix_alloc(cols, rows);
	X1 = gsl_matrix_alloc(compc, rows);
	V  = gsl_matrix_alloc(cols, cols);
	D  = gsl_matrix_alloc(cols, cols);
	TU = gsl_matrix_alloc(cols, cols);
	scale = gsl_vector_alloc(cols);
	d  = gsl_vector_alloc(cols);

	/*
	 * CENTERING
	 */
	MatrixCenter(X, &mean);
	
	/*
	 * WHITENING
	 */
	gsl_matrix_transpose_memcpy(XT, X);
	// X[i][j] = X[i][j] * (1/rows)
	gsl_matrix_scale(X, 1 / (double)rows);

	gsl_matrix_set_zero(V);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, XT, X, 0.0, V); // V is right

	// SVD on V
	// refer to  https://www.gnu.org/software/gsl/doc/html/linalg.html#singular-value-decomposition
	gsl_linalg_SV_decomp_jacobi(V, D, d);	//?????
	gsl_vect_print("d", __LINE__, d);

	// D <- diag(c(1/sqrt(d))
	for (i=0 ; i<d->size ; i++)
		gsl_vector_set(d, i, 1 / sqrt(gsl_vector_get(d, i)));

	gsl_matrix_set_zero(D);			// D is right
	for (i=0 ; i<d->size ; i++)
		gsl_matrix_set(D, i, i, gsl_vector_get(d, i));

	// K <- D %*% t(U)
	gsl_matrix_transpose_memcpy(TU, V);
	gsl_mat_print("V", __LINE__, V);
	gsl_mat_print("TU", __LINE__, TU);
	gsl_mat_print("D", __LINE__, D);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, D, TU, 0.0, V);
	gsl_mat_print("V", __LINE__, V);

	// X1 <- K %*% X
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, XT, 0.0, X1);

	/*
	 * FAST ICA
	 */
	_A = ICA_compute_GSL(X1);

	/*
	 * OUTPUT
	 */
	// X <- t(x)
	gsl_matrix_transpose_memcpy(X, XT);
	MatrixDeCenter(X, mean); // right

	// K: colsXcompc
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	gsl_matrix_transpose_memcpy(K, &subMat1.matrix);
	// gsl_mat_print(K);

	// w <- a %*% K; S <- w %*% X
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, V->size2);
	subMat2 = gsl_matrix_submatrix(D, 0, 0, compc, D->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, _A, &subMat1.matrix, 0.0, &subMat2.matrix);
	
	subMat1 = gsl_matrix_submatrix(D, 0, 0, compc, D->size2);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, XT, 0.0, X1);

	// S
	subMat1 = gsl_matrix_submatrix(X1, 0, 0, compc, X1->size2);
	subMat2 = gsl_matrix_submatrix(S, 0, 0, S->size1, compc);
	gsl_matrix_transpose_memcpy(&subMat2.matrix, &subMat1.matrix);

	// A <- t(w) %*% solve(w * t(w))
	subMat1 = gsl_matrix_submatrix(D, 0, 0, compc, compc);
	subMat2 = gsl_matrix_submatrix(TU, 0, 0, compc, compc);
	gsl_matrix_transpose_memcpy(&subMat2.matrix, &subMat1.matrix);
	subMat3 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, &subMat2.matrix, 0.0, &subMat3.matrix);

	// Inverse matrix of V with dim of compc by compc.
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	InvMat = MatrixInvert(&subMat1.matrix);

	if (InvMat == NULL)
	{
		ret = -1;
		goto end;
	}

	subMat1 = gsl_matrix_submatrix(TU, 0, 0, compc, compc);
	subMat2 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &subMat1.matrix, InvMat, 0.0, &subMat2.matrix);

	// A
	subMat1 = gsl_matrix_submatrix(V, 0, 0, compc, compc);
	gsl_matrix_transpose_memcpy(A, &subMat1.matrix);

	// W
	gsl_matrix_transpose_memcpy(W, _A);

end:
	// cleanup
	if (XT)
		gsl_matrix_free(XT);

	if (X1)
		gsl_matrix_free(X1);

	if (V)
		gsl_matrix_free(V);

	if (D)
		gsl_matrix_free(D);

	if (TU)
		gsl_matrix_free(TU);

	if (InvMat)
		gsl_matrix_free(InvMat);

	if (scale)
		gsl_vector_free(scale);

	if (d)
		gsl_vector_free(d);

	if (mean)
		gsl_vector_free(mean);

	return ret;
}