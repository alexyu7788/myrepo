#include <stdio.h>
#include <stdlib.h>
#include "libICA.h"

/*
 * Creates a matrix
 */
static double **mat_create(int rows, int cols)
{
	double **M; int i;
	
	M = (double**) malloc(rows * sizeof(double*));
	for (i=0; i<rows; i++)
		M[i] = (double*) malloc(cols * sizeof(double));
	
	return M;
}

/*
 * Reads matrix M from fp
 */
static double **mat_read(FILE *fp, int *rows, int *cols)
{
	int i, j; double **M;

	fscanf(fp, "%d %d", rows, cols);
	M = mat_create(*rows, *cols);
	
	for (i=0; i<*rows; i++) {
		for (j=0; j<*cols; j++)
			fscanf(fp, "%lf ", &(M[i][j]));	
	}
	
	return M;	
}

/*
 * Prints matrix M to stdout
 */
static void mat_print(double **M, int rows, int cols)
{
	int i, j;

	for (i=0; i<rows; i++) {
		for (j=0; j<cols; j++) {
			printf("[%d][%d]=%0.6f", i, j, M[i][j]);
			if (j < cols - 1)
				printf(" ");
		}
		printf("\n");
	}
}


/*
 * Main program
 */
int main(int argc, char *argv[])
{
	double **X, **K, **W, **A, **S;
	int rows, cols, compc;
	FILE *fp;

	// Input parameters check
	if (argc == 2) {
		if ((fp = fopen(argv[1], "r")) == NULL) {
			perror("Error opening input file");
			exit(-1);
		}
	} else {	
		printf("usage: %s data_file\n", argv[0]);
		exit(-1);
	}

	compc = 2;
	
	// Matrix creation
	X = mat_read(fp, &rows, &cols);
	W = mat_create(compc, compc);
	A = mat_create(compc, compc);
	K = mat_create(cols, compc);
	S = mat_create(rows, cols);	
	
	int i, j;
	gsl_matrix *XX, *WW, *AA, *KK, *SS;

	WW = gsl_matrix_alloc(compc, compc);
	AA = gsl_matrix_alloc(compc, compc);
	KK = gsl_matrix_alloc(cols, compc);
	SS = gsl_matrix_alloc(rows, cols);

	XX = gsl_matrix_alloc(rows, cols);
	mat_copy_gsl(X, rows, cols, XX);

	// ICA computation
	fastICA(X, rows, cols, compc, K, W, A, S);
	FastICA_GSL(XX, compc, KK, WW, AA, SS);
	// Output
	printf("$K\n");	
	mat_print(K, cols, compc);

	for (i=0 ; i<KK->size1 ; i++)
		for (j=0 ; j<KK->size2 ; j++)
			printf(j==KK->size2 - 1 ? "%6.09lf\n" : "%6.09lf ", gsl_matrix_get(KK, i, j));

	printf("\n$W\n");
	mat_print(W, compc, compc);
	for (i=0 ; i<WW->size1 ; i++)
		for (j=0 ; j<WW->size2 ; j++)
			printf(j==WW->size2 - 1 ? "%6.09lf\n" : "%6.09lf ", gsl_matrix_get(WW, i, j));

	printf("\n$A\n");
	mat_print(A, compc, compc);
	for (i=0 ; i<AA->size1 ; i++)
		for (j=0 ; j<AA->size2 ; j++)
			printf(j==AA->size2 - 1 ? "%6.09lf\n" : "%6.09lf ", gsl_matrix_get(AA, i, j));
	// printf("\n$S\n");
	// mat_print(S, rows, compc);
	// for (i=0 ; i<SS->size1 ; i++)
	// 	for (j=0 ; j<SS->size2 ; j++)
	// 		printf(j==SS->size2 - 1 ? "[%d][%d]=%6.09lf\n" : "[%d][%d]=%6.09lf ", i, j, gsl_matrix_get(SS, i, j));
	if (XX)
		gsl_matrix_free(XX);

	if (WW)
		gsl_matrix_free(WW);

	if (AA)
		gsl_matrix_free(AA);

	if (KK)
		gsl_matrix_free(KK);

	if (SS)
		gsl_matrix_free(SS);

	return 0;
}
