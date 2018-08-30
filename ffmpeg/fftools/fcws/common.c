#include "common.h"




bool CheckOrReallocVector(gsl_vector** v, int size)
{
    if (!(*v) || (*v)->size != size) {
        if (*v) {
            gsl_vector_free(*v);
            *v = NULL;
        }

        if ((*v = gsl_vector_alloc(size)) == NULL)
            return false;
    }

    gsl_vector_set_zero(*v);

    return true;
}

bool CheckOrReallocMatrix(gsl_matrix** m, int h, int w)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_alloc(h, w)) == NULL)
            return false;
    }

    gsl_matrix_set_zero(*m);

    return true;
}

bool CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_ushort_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_ushort_alloc(h, w)) == NULL)
            return false;
    }

    gsl_matrix_ushort_set_zero(*m);

    return true;
}

bool CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w)
{
    if (!*m || (*m)->size1 != h || (*m)->size2 != w) {
        if (*m) {
            gsl_matrix_char_free(*m);
            *m = NULL;
        }

        if ((*m = gsl_matrix_char_alloc(h, w)) == NULL)
            return false;
    }

    gsl_matrix_char_set_zero(*m);

    return true;
}
