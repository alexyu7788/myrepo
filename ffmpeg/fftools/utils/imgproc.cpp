#include "imgproc.hpp"

enum {
    DIRECTION_45UP = 0,
    DIRECTION_45DOWN,
    DIRECTION_HORIZONTAL,
    DIRECTION_VERTICAL,
};

static double gk_3by3[] = {
    1,  2,  1,
    2,  4,  2,
    1,  2,  1,
};

static double gk_5by5[] = {
    2,  4,  5,  4, 2,
    4,  9, 12,  9, 4,
    5, 12, 15, 12, 5,
    4,  9, 12,  9, 4,
    2,  4,  5,  4, 2,
};

static double dx_ary[] = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

static double dx1d_ary[] = {
    -1, 0, 1
};

static double dy_ary[] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

//--------------------------------------------------------------------------
// Gaussian Blur
bool CImgProc::InitGB(int size)
{
    uint32_t r, c;

    if (m_gb_init == false) {

        m_gk_weight = 0;

        if (size == 3)
            m_gk = gsl_matrix_view_array(gk_3by3, 3, 3);
        else if (size == 5)
            m_gk = gsl_matrix_view_array(gk_5by5, 5, 5);
        else {
            dbg("GB: size %d is not implemnted yet.");
            return false;
        }

        for (r=0 ; r<m_gk.matrix.size1 ; ++r) {
            for (c=0 ; c<m_gk.matrix.size2 ; ++c) {
                m_gk_weight += gsl_matrix_get(&m_gk.matrix, r, c);
            }
        }

        m_gb_init = true;
    }

    return true;
}

bool CImgProc::DeinitGB()
{
    FreeMatrix(&m_gb_src);
    FreeMatrix(&m_gb_dst);

    m_gb_init = false;

    return true;
}
// Gradient
bool CImgProc::InitGradient()
{
    if (m_gradient_init == false) {
        m_dx    = gsl_matrix_view_array(dx_ary, 3, 3);
        m_dy    = gsl_matrix_view_array(dy_ary, 3, 3);
        m_dx1d  = gsl_vector_view_array(dx1d_ary, 3);
        m_dy1d  = gsl_vector_view_array(dx1d_ary, 3);

        m_gradient_init = true;
    }

    return true;
}




//--------------------------------------------------------------------------
CImgProc::CImgProc() {
    // Gaussian Blur
    m_gb_init   = false;
    m_gk_weight = 0;

    m_gb_src    =
    m_gb_dst    = NULL;
    
    // Gradient
    m_gradient_init  = false;
}

CImgProc::~CImgProc()
{
        
}

bool CImgProc::Init() 
{

    return true;
}
// Gaussian Blur
bool CImgProc::GaussianBlue(gsl_matrix* src, gsl_matrix* dst)
{
    uint32_t r, c, i, j;
    uint32_t size;
    uint32_t row, col;
    double sum = 0.0;
    gsl_matrix_view submatrix_src;

    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        dbg();
        return false;
    }

    if (InitGB() == false || m_gk.matrix.size1 != m_gk.matrix.size2) {
        dbg();
        return false;
    }

    row = src->size1;
    col = src->size2;
    size = m_gk.matrix.size1;

    gsl_matrix_memcpy(dst, src);

    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            submatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, r - (size/2), c - (size/2), size, size); 

            sum = 0;
            for (i=0 ; i<submatrix_src.matrix.size1 ; i++)
                for (j=0 ; j<submatrix_src.matrix.size2 ; j++)
                    sum += (gsl_matrix_get((const gsl_matrix*)&submatrix_src.matrix, i, j) * gsl_matrix_get((const gsl_matrix*)&m_gk.matrix, i, j));
            
            sum /= m_gk_weight;
            gsl_matrix_set(dst, r, c, sum);
        }
    }

    return true;
}
