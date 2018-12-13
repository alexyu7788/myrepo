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
BOOL CImgProc::InitGB(int size)
{
    uint32_t r, c;

    if (m_gb_init == FALSE) {

        m_gk_weight = 0;

        if (size == 3)
            m_gk = gsl_matrix_view_array(gk_3by3, 3, 3);
        else if (size == 5)
            m_gk = gsl_matrix_view_array(gk_5by5, 5, 5);
        else {
            dbg("GB: size %d is not implemented yet.", size);
            return FALSE;
        }

        for (r=0 ; r<m_gk.matrix.size1 ; ++r) {
            for (c=0 ; c<m_gk.matrix.size2 ; ++c) {
                m_gk_weight += gsl_matrix_get(&m_gk.matrix, r, c);
            }
        }

        m_gb_init = TRUE;
    }

    return TRUE;
}

BOOL CImgProc::DeinitGB()
{
    FreeMatrix(&m_gb_src);
    FreeMatrix(&m_gb_dst);

    m_gb_init = FALSE;

    return TRUE;
}

BOOL CImgProc::GaussianBlue(const gsl_matrix* src, 
                            gsl_matrix* dst, 
                            int kernel_size)
{
    uint32_t r, c, i, j;
    uint32_t size;
    uint32_t row, col;
    double sum = 0.0;
    gsl_matrix_view submatrix_src;

    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        dbg();
        return FALSE;
    }

    if (InitGB(kernel_size) == FALSE || m_gk.matrix.size1 != m_gk.matrix.size2 || m_gk_weight == 0) {
        dbg();
        return FALSE;
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

    return TRUE;
}

// Sobel
BOOL CImgProc::InitSobel()
{
    if (m_sobel_init == FALSE) {
        m_dx    = gsl_matrix_view_array(dx_ary, 3, 3);
        m_dy    = gsl_matrix_view_array(dy_ary, 3, 3);
        m_dx1d  = gsl_vector_view_array(dx1d_ary, 3);
        m_dy1d  = gsl_vector_view_array(dx1d_ary, 3);

        m_gradient = NULL;
        m_dir   = NULL;

        m_sobel_init = TRUE;
    }

    return TRUE;
}

int CImgProc::GetRoundedDirection(int gx, int gy)
{
    /* reference angles:
     *   tan( pi/8) = sqrt(2)-1
     *   tan(3pi/8) = sqrt(2)+1
     * Gy/Gx is the tangent of the angle (theta), so Gy/Gx is compared against
     * <ref-angle>, or more simply Gy against <ref-angle>*Gx
     *
     * Gx and Gy bounds = [-1020;1020], using 16-bit arithmetic:
     *   round((sqrt(2)-1) * (1<<16)) =  27146
     *   round((sqrt(2)+1) * (1<<16)) = 158218
     */
    if (gx) {
        int tanpi8gx, tan3pi8gx;

        if (gx < 0)
            gx = -gx, gy = -gy;
        gy <<= 16;
        tanpi8gx  =  27146 * gx;
        tan3pi8gx = 158218 * gx;
        if (gy > -tan3pi8gx && gy < -tanpi8gx)  return DIRECTION_45UP;
        if (gy > -tanpi8gx  && gy <  tanpi8gx)  return DIRECTION_HORIZONTAL;
        if (gy >  tanpi8gx  && gy <  tan3pi8gx) return DIRECTION_45DOWN;
    }

    return DIRECTION_VERTICAL;
}

BOOL CImgProc::NonMaximumSuppression(gsl_matrix* dst,
                                   gsl_matrix_char* dir,
                                   gsl_matrix* gradient)
{
    uint32_t r, c;
    uint32_t row, col;
    uint32_t size;

    if (!gradient || !dst || !dir) {
        dbg();
        return FALSE;
    }

#define COPY_MAXIMA(ay, ax, by, bx) do {                                                    \
    if (gsl_matrix_get(gradient, r, c) > gsl_matrix_get(gradient, r+(ay), c+ax) &&      \
        gsl_matrix_get(gradient, r, c) > gsl_matrix_get(gradient, r+(by), c+bx))        \
        gsl_matrix_set(dst, r, c, (uint8_t)gsl_matrix_get(gradient, r, c));                \
} while (0)

    row = gradient->size1;
    col = gradient->size2;

    for (r=1 ; r < row - 1; r++) {
        for (c=1 ; c < col - 1; c++) {
            switch (gsl_matrix_char_get(dir, r, c)) {
            case DIRECTION_45UP:
                COPY_MAXIMA(1, -1, -1, 1);
                break;
            case DIRECTION_45DOWN:
                COPY_MAXIMA(-1, -1, 1, 1);
                break;
            case DIRECTION_HORIZONTAL:
                COPY_MAXIMA( 0, -1, 0, 1);
                break;
            case DIRECTION_VERTICAL:
                COPY_MAXIMA(-1,  0, 1, 0);
                break;
            }
        }
    }

    return TRUE;
}

BOOL CImgProc::Sobel(gsl_matrix* grad, 
        gsl_matrix_char* dir, 
        const gsl_matrix* src,
        int direction, 
        BOOL double_edge,
        int crop_r, 
        int crop_c, 
        int crop_w, 
        int crop_h)
{
    int cr, cc, cw, ch;
    uint32_t i, j;
    uint32_t r, c, row, col;
    uint32_t size;
    gsl_matrix*     pmatrix;
    gsl_matrix_view submatrix_src;
    gsl_matrix_view cropmatrix_src;

    if (!src || !grad || !dir) {
        dbg();
        return FALSE;
    }

    if (InitSobel() == FALSE) {
        dbg();
        return FALSE;
    }

    if (crop_r && crop_c && crop_w && crop_h) {
        //Check boundary
        if (crop_r < 0)
            cr = 0;
        else if (crop_r > src->size1)
            cr = src->size1 - 1;
        else
            cr = crop_r;

        if (crop_c < 0)
            cc = 0;
        else if (crop_c > src->size2)
            cc = src->size2 - 1;
        else
            cc = crop_c;

        if (crop_r + crop_h >= src->size1)
            ch = src->size1 - crop_r;
        else 
            ch = crop_h;

        if (crop_c + crop_w >= src->size2)
            cw = src->size2 - crop_c;
        else
            cw = crop_w;

        CheckOrReallocMatrix(&grad, ch, cw, TRUE);
        CheckOrReallocMatrixChar(&dir, ch, cw, TRUE);

        cropmatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, cr, cc, ch, cw);
        pmatrix = &cropmatrix_src.matrix;
    } else {
        pmatrix = (gsl_matrix*)src;
    }

    row = pmatrix->size1;
    col = pmatrix->size2;
    size = m_dx.matrix.size1;

    // Convolution operation
    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            int gx=0, gy=0;

            submatrix_src = gsl_matrix_submatrix(pmatrix, r - (size/2), c - (size/2), size, size); 

            // direction : 
            // 0 : vertical & horizontal edge
            // 1 : vertical edge
            // 2 : horizontal edge
            if (direction != 2) {
                for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                    for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                        gx += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dx.matrix, i, j));
                    }
                }
            }

            if (direction != 1) {
                for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                    for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                        gy += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dy.matrix, i, j));
                    }
                }
            }

            //gsl_matrix_ushort_set(grad, r, c, gsl_hypot(gx, gy));
            if (double_edge)
                gsl_matrix_set(grad, r, c, abs(gx) + abs(gy));
            else
                gsl_matrix_set(grad, r, c, gx + gy);

            gsl_matrix_char_set(dir, r, c, GetRoundedDirection(gx, gy));
        }
    }

    return TRUE;
}





































//--------------------------------------------------------------------------
CImgProc::CImgProc() {
    // Gaussian Blur
    m_gb_init   = FALSE;
    m_gk_weight = 0;

    m_gb_src    =
    m_gb_dst    = NULL;
    
    // Sobel 
    m_sobel_init    = FALSE;
    m_gradient      = NULL;
    m_dir           = NULL;
}

CImgProc::~CImgProc()
{
    FreeMatrix(&m_gb_src);
    FreeMatrix(&m_gb_dst);

    FreeMatrix(&m_gradient);
    FreeMatrixChar(&m_dir);
}

BOOL CImgProc::Init() 
{

    return TRUE;
}

BOOL CImgProc::EdgeDetectForLDW(const gsl_matrix* src, 
                                gsl_matrix* dst,
                                int threshold,
                                double* dir,
                                int double_edge)
{
    size_t r, c;
    gsl_matrix* temp = NULL; 

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if (InitSobel() == FALSE) {
        dbg("Fail to init sobel");
        return FALSE;
    }

    CheckOrReallocMatrix(&temp, src->size1, src->size2, TRUE);
    CheckOrReallocMatrix(&m_gradient, src->size1, src->size2, TRUE);
    CheckOrReallocMatrixChar(&m_dir, src->size1, src->size2, TRUE);

    GaussianBlue(src, temp, 3);
    Sobel(m_gradient, m_dir, temp, 1, FALSE);

    for (r=0 ; r < m_gradient->size1 ; ++r) {
        for (c=0 ; c < m_gradient->size2 ; ++c) {
            if (gsl_matrix_get(m_gradient, r, c) < threshold)
                gsl_matrix_set(dst, r, c, 255);
            else 
                gsl_matrix_set(dst, r, c, 0);
        }
    }

    FreeMatrix(&temp);

    return TRUE;
}

BOOL CImgProc::EdgeDetectForFCW(const gsl_matrix* src,
                                gsl_matrix* dst,
                                gsl_matrix* gradient,
                                gsl_matrix_char* dir,
                                int direction)
{
    if (!src || !dst || !gradient || !dir) {
        dbg();
        return FALSE;
    }

    Sobel(gradient, dir, src, direction, TRUE);
    gsl_matrix_set_zero(dst);
    NonMaximumSuppression(dst, dir, gradient);

    return TRUE;
}

BOOL CImgProc::CropImage(uint8_t* src, 
                       gsl_matrix* dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize,
                       uint32_t rowoffset)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if ((dst->size1 + rowoffset) != h || dst->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < h - rowoffset ; ++r) {
        for (c=0 ; c < w ; ++c) {
            gsl_matrix_set(dst, r, c, src[(r + rowoffset) * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyImage(uint8_t* src, 
                          gsl_matrix* dst, 
                          uint32_t w, 
                          uint32_t h, 
                          uint32_t linesize)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if (dst->size1 != h || dst->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < h ; ++r) {
        for (c=0 ; c < w ; ++c) {
            gsl_matrix_set(dst, r, c, src[r * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyBackImage(gsl_matrix* src, 
                             uint8_t* dst, 
                             uint32_t w, 
                             uint32_t h, 
                             uint32_t linesize, 
                             uint32_t rowoffset)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if ((src->size1 + rowoffset) != h || src->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < src->size1 ; ++r) {
        for (c=0 ; c < src->size2 ; ++c) {
            dst[(r + rowoffset) * linesize + c] = (uint8_t)gsl_matrix_get(src, r, c);
        }
    }

    return TRUE;
}

BOOL CImgProc::GenIntegralImage(gsl_matrix* src, gsl_matrix* dst)
{
    uint32_t r, c;
    double sum;

    if (!src || !dst || src->size1 != dst->size1 || src->size2 != dst->size2) {
        dbg();
        return FALSE;
    }

    // Generate integral image for retrive average value of a rectange quickly.
    for (c=0 ; c<src->size2 ; ++c) {
        sum = 0;
        for (r=0 ; r<src->size1 ; ++r) {
            sum += gsl_matrix_get(src, r, c);

            if (c == 0)
                gsl_matrix_set(dst, r, c, sum);
            else
                gsl_matrix_set(dst, r, c, gsl_matrix_get(dst, r, c-1) + sum);
        }
    }

    return TRUE;
}

BOOL CImgProc::ThresholdingByIntegralImage(gsl_matrix* src, 
                                           gsl_matrix* intimg, 
                                           gsl_matrix* dst, 
                                           uint32_t s, 
                                           float p)
{
    uint32_t r, c;
    uint32_t r1, c1, r2, c2;
    uint32_t count;
    double sum;

    if (!src || !intimg || !dst) {
        dbg();
        return FALSE;
    }

    // Move a sxs sliding window pixel by pixel. The center pixel of this sliding window is (r, c).
    // If center pixel is p percentage lower than average value of sliding window, set it as black. Otherwise, set as white.
    for (r=0 ; r<src->size1 ; ++r) {
        for (c=0 ; c<src->size2 ; ++c) {
            if (r >= s/2+1 && r<src->size1 - s/2 - 1 && c>=s/2+1 && c<src->size2 - s/2 -1) { // boundary checking.
                r1 = r - s/2;
                c1 = c - s/2;
                r2 = r + s/2;
                c2 = c + s/2;

                count = (r2 - r1) * (c2 - c1);
                sum = gsl_matrix_get(intimg, r2, c2) - gsl_matrix_get(intimg, r1-1, c2) - gsl_matrix_get(intimg, r2, c1-1) + gsl_matrix_get(intimg, r1-1, c1-1);

                if ((gsl_matrix_get(src, r, c) * count) <= (sum * p))
                    gsl_matrix_set(dst, r, c, 0);
                else
                    gsl_matrix_set(dst, r, c, 255);
            }
            else
                gsl_matrix_set(dst, r, c, 255);
        }
    }

    return TRUE;
}

BOOL CImgProc::CalHorizonProject(const gsl_matrix* const src,
                                 gsl_vector* proj)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view row_view;

    if (!src || !proj || src->size1 != proj->size) {
        dbg();
        return FALSE;
    }

    gsl_vector_set_zero(proj);

    // skip border
    for (r=1 ; r<src->size1 - 1 ; ++r) {
        row_view = gsl_matrix_row((gsl_matrix*)src, r);

        for (c=1 ; c<row_view.vector.size - 1 ; c++) {
            if (gsl_vector_get(&row_view.vector, c) != 255)
            {
                val = gsl_vector_get(proj, r);
                gsl_vector_set(proj, r, ++val);
            }
        }
    }

    return TRUE;
}
        
BOOL CImgProc::RemoveNoisyBlob(const gsl_matrix* src, 
                            gsl_matrix* dst)
{

    if (!src || !dst) {
        ldwsdbg();
        return FALSE;
    }



    return TRUE;
}
