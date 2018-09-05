#include "common.h"


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

bool CheckOrReallocVector(gsl_vector** v, int size, bool reset)
{
    if (!(*v) || (*v)->size != size) {
        if (*v) {
            gsl_vector_free(*v);
            *v = NULL;
        }

        if ((*v = gsl_vector_alloc(size)) == NULL)
            return false;
    }

    if (reset)
        gsl_vector_set_zero(*v);

    return true;
}

bool CheckOrReallocMatrix(gsl_matrix** m, int h, int w, bool reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_alloc(h, w)) == NULL)
            return false;
    }

    if (reset)
        gsl_matrix_set_zero(*m);

    return true;
}

bool CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w, bool reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_ushort_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_ushort_alloc(h, w)) == NULL)
            return false;
    }

    if (reset)
        gsl_matrix_ushort_set_zero(*m);

    return true;
}

bool CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w, bool reset)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_char_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_char_alloc(h, w)) == NULL)
            return false;
    }

    if (reset)
        gsl_matrix_char_set_zero(*m);

    return true;
}
