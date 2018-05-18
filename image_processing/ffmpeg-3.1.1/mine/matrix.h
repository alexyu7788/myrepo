#ifndef __MATRIX_H__
#define __MATRIX_H__

typedef double** MATRIX;
typedef double* VECTOR;

#if 0
MATRIX mat_new_mat(int col, int row);
VECTOR mat_new_vector(int n);
void mat_rel_mat(MATRIX m, int row);
void mat_rel_vector(VECTOR v);
double mat_det_3X3(MATRIX m);
double mat_def_2X2(MATRIX m);
void mat_copy(MATRIX src, MATRIX dst, int col, int row);
void mat_change_col(MATRIX m, MATRIX a, VECTOR b, int replace_col,int col, int row);
void mat_dump(MATRIX m,int col, int row);
void mat_solve_equation(MATRIX a, VECTOR x, VECTOR b, int col, int row);
#endif
#endif
