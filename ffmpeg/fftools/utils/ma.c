#include "ma.h"

void FreeVector(gsl_vector** v)
{
    if (*v) {
        gsl_vector_free(*v);
        *v = NULL;
    }
}

void FreeMatrix(gsl_matrix** m)
{
    if (*m) {
        gsl_matrix_free(*m);
        *m = NULL;
    }
}

void FreeMatrixUshort(gsl_matrix_ushort** m)
{
    if (*m) {
        gsl_matrix_ushort_free(*m);
        *m = NULL;
    }
}

void FreeMatrixChar(gsl_matrix_char** m)
{
    if (*m) {
        gsl_matrix_char_free(*m);
        *m = NULL;
    }
}

BOOL CheckOrReallocVector(gsl_vector** v, int size, BOOL reset)
{
    if (!(*v) || (*v)->size != size) {
        if (*v) {
            gsl_vector_free(*v);
            *v = NULL;
        }

        if ((*v = gsl_vector_alloc(size)) == NULL)
            return FALSE;
    }

    if (reset)
        gsl_vector_set_zero(*v);

    return TRUE;
}

BOOL CheckOrReallocMatrix(gsl_matrix** m, int h, int w, BOOL reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_alloc(h, w)) == NULL)
            return FALSE;
    }

    if (reset)
        gsl_matrix_set_zero(*m);

    return TRUE;
}

BOOL CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w, BOOL reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_ushort_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_ushort_alloc(h, w)) == NULL)
            return FALSE;
    }

    if (reset)
        gsl_matrix_ushort_set_zero(*m);

    return TRUE;
}

BOOL CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w, BOOL reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_char_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_char_alloc(h, w)) == NULL)
            return FALSE;
    }

    if (reset)
        gsl_matrix_char_set_zero(*m);

    return TRUE;
}
