#include "fcws.h"

enum {
    DIRECTION_45UP,
    DIRECTION_45DOWN,
    DIRECTION_HORIZONTAL,
    DIRECTION_VERTICAL,
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

static double dy_ary[] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

// Macro
#define FreeVector(m) do {                  \
    if ((m)) {                              \
        gsl_vector_free((m));               \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrix(m) do {                  \
    if ((m)) {                              \
        gsl_matrix_free((m));               \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrixUshort(m) do {            \
    if ((m)) {                              \
        gsl_matrix_ushort_free((m));        \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define FreeMatrixChar(m) do {              \
    if ((m)) {                              \
        gsl_matrix_char_free((m));          \
        ((m)) = NULL;                       \
    }                                       \
}while (0)

#define CheckOrReallocVector(v, size)   do {            \
    if (!(v) || (v)->size != size) {                    \
        if ((v)) {                                      \
            gsl_vector_free((v));                       \
            (v) = NULL;                                 \
        }                                               \
        if ((v = gsl_vector_alloc(size)) == NULL)       \
            return false;                               \
    }                                                   \
    gsl_vector_set_zero((v));                           \
} while (0)

#define CheckOrReallocMatrix(m, h, w)   do {            \
    if (!(m) || (m)->size1 != h || (m)->size2 != w) {   \
        if ((m)) {                                      \
            gsl_matrix_free((m));                       \
            (m) = NULL;                                 \
        }                                               \
        if ((m = gsl_matrix_alloc(h, w)) == NULL)       \
            return false;                               \
    }                                                   \
    gsl_matrix_set_zero((m));                           \
} while (0)

#define CheckOrReallocMatrixUshort(m, h, w)   do {      \
    if (!(m) || (m)->size1 != h || (m)->size2 != w) {   \
        if ((m)) {                                      \
            gsl_matrix_ushort_free((m));                \
            (m) = NULL;                                 \
        }                                               \
        if ((m = gsl_matrix_ushort_alloc(h, w)) == NULL)\
            return false;                               \
    }                                                   \
    gsl_matrix_ushort_set_zero((m));                           \
} while (0)

#define CheckOrReallocMatrixChar(m, h, w)   do {        \
    if (!(m) || (m)->size1 != h || (m)->size2 != w) {   \
        if ((m)) {                                      \
            gsl_matrix_char_free((m));                  \
            (m) = NULL;                                 \
        }                                               \
        if ((m = gsl_matrix_char_alloc(h, w)) == NULL)  \
            return false;                               \
    }                                                   \
    gsl_matrix_char_set_zero((m));                           \
} while (0)









CFCWS::CFCWS()
{
    m_imgy              = 
    m_edged_imgy        = 
    m_temp_imgy         = NULL;

    m_gradient          = NULL;
    m_direction         = NULL;
    m_temp_hori_hist    = NULL;
    // Gaussian Kernel
    m_gk = gsl_matrix_view_array(gk_5by5, 5, 5);    
    
    m_gk_weight = 0;

    for (uint32_t r=0 ; r<m_gk.matrix.size1 ; r++)
        for (uint32_t c=0 ; c<m_gk.matrix.size2 ; c++)
            m_gk_weight += gsl_matrix_get(&m_gk.matrix, r, c);

    // dx & dy
    m_dx = gsl_matrix_view_array(dx_ary, 3, 3);
    m_dy = gsl_matrix_view_array(dy_ary, 3, 3);

    m_vcs.clear();
}

CFCWS::~CFCWS()
{
    FreeVector(m_temp_hori_hist);

    FreeMatrix(m_imgy);
    FreeMatrix(m_edged_imgy);
    FreeMatrix(m_temp_imgy);
    FreeMatrixUshort(m_gradient);
    FreeMatrixChar(m_direction);

    for (CandidatesIT it = m_vcs.begin() ; it != m_vcs.end(); ++it) {
        if ((*it)) {
            delete (*it);
            (*it) = NULL;
        }
    }
    m_vcs.clear();

}

bool CFCWS::DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist, Candidates& vcs)
{
    if (!img || !vertical_hist || !hori_hist || !grayscale_hist) {
        dbg();
        return false;
    }

    CheckOrReallocMatrix(m_imgy, h, w);
    CheckOrReallocMatrix(m_edged_imgy, h, w);
    CheckOrReallocMatrix(m_temp_imgy, h, w);
    CheckOrReallocMatrixUshort(m_gradient, h, w);
    CheckOrReallocMatrixChar(m_direction, h, w);

    // Copy image array to image matrix
    for (uint32_t r=0 ; r<m_imgy->size1 ; r++)
        for (uint32_t c=0 ; c<m_imgy->size2 ; c++)
            gsl_matrix_set(m_imgy, r, c, img[r * m_imgy->size2 + c]); 

    gsl_matrix_memcpy(m_temp_imgy, m_imgy);

#if 0
    EdgeDetect(m_imgy, m_temp_imgy, m_edged_imgy, m_gradient, m_direction);

    // Copy image matrix to image array
    for (uint32_t r=0 ; r<m_imgy->size1 ; r++)
        for (uint32_t c=0 ; c<m_imgy->size2 ; c++)
            img[r * m_imgy->size2 + c] = (uint8_t)gsl_matrix_get(m_edged_imgy, r, c); 
#else
    CalGrayscaleHist(m_imgy, m_temp_imgy, grayscale_hist);

    //CalVerticalHist(m_temp_imgy, vertical_hist);

    CalHorizontalHist(m_temp_imgy, hori_hist);

    VehicleCandidateGenerate(m_temp_imgy, hori_hist, vertical_hist, m_vcs);

    vcs.clear();
    vcs = m_vcs;

    // Copy image matrix to image array
    for (uint32_t r=0 ; r<m_imgy->size1 ; r++)
        for (uint32_t c=0 ; c<m_imgy->size2 ; c++)
            img[r * m_imgy->size2 + c] = (uint8_t)gsl_matrix_get(m_temp_imgy, r, c); 
#endif

    return true;
}

//------------------------------------------------------------------------------------
int  CFCWS::GetRounded_Direction(int gx, int gy)
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

bool CFCWS::NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src)
{
    if (!src || !dst || !dir) {
        dbg();
        return false;
    }

    uint32_t r, c;
    uint32_t row, col;
    uint32_t size;

#define COPY_MAXIMA(ay, ax, by, bx) do {                                                    \
    if (gsl_matrix_ushort_get(src, r, c) > gsl_matrix_ushort_get(src, r+(ay), c+ax) &&      \
        gsl_matrix_ushort_get(src, r, c) > gsl_matrix_ushort_get(src, r+(by), c+bx))        \
        gsl_matrix_set(dst, r, c, (uint8_t)gsl_matrix_ushort_get(src, r, c));                \
} while (0)

    row = src->size1;
    col = src->size2;

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

    return true;
}

bool CFCWS::DoubleThreshold(int low, int high, gsl_matrix* dst, const gsl_matrix* src)
{
    if (!dst || !src) {
        dbg();
        return false;
    }

    uint32_t r, c;
    uint32_t row, col;

    row = src->size1;
    col = src->size2;
    
    for (r=0 ; r<row ; r++) {
        for (c=0 ; c<col ; c++) {
            if (gsl_matrix_get(src, r, c) > high) {
                gsl_matrix_set(dst, r, c, gsl_matrix_get(src, r, c));
                continue;
            }

            if ((!c || c == col - 1 || !r || r == row -1) &&
                    gsl_matrix_get(src, r, c) > low &&
                    (gsl_matrix_get(src, r-1, c-1) > high ||
                     gsl_matrix_get(src, r-1, c  ) > high ||
                     gsl_matrix_get(src, r-1, c+1) > high ||
                     gsl_matrix_get(src, r  , c-1) > high ||
                     gsl_matrix_get(src, r  , c+1) > high ||
                     gsl_matrix_get(src, r+1, c-1) > high ||
                     gsl_matrix_get(src, r+1, c  ) > high ||
                     gsl_matrix_get(src, r+1, c+1) > high))
                gsl_matrix_set(dst, r, c, gsl_matrix_get(src, r, c));
            else
                gsl_matrix_set(dst, r, c, 0);
        }
    }

    return true;
}

bool CFCWS::Sobel(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src)
{
    if (!src || !dst || !dir) {
        dbg();
        return false;
    }

    uint32_t i, j;
    uint32_t r, c, row, col;
    uint32_t size;
    gsl_matrix_view submatrix_src;

    row = src->size1;
    col = src->size2;
    size = m_dx.matrix.size1;

    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            int gx=0, gy=0;

            submatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, r - (size/2), c - (size/2), size, size); 

            for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                    gx += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dx.matrix, i, j));
                }
            }

            for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                    gy += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dy.matrix, i, j));
                }
            }

            gsl_matrix_ushort_set(dst, r, c, abs(gx) + abs(gy));
            gsl_matrix_char_set(dir, r, c, GetRounded_Direction(gx, gy));
        }
    }

    return true;
}

bool CFCWS::GaussianBlur(gsl_matrix* dst, const gsl_matrix* src)
{
    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        dbg();
        return false;
    }

    if (m_gk.matrix.size1 != m_gk.matrix.size2) {
        dbg();
        return false;
    }

    uint32_t r, c, i, j;
    uint32_t size;
    uint32_t row, col;
    double sum = 0.0;
    gsl_matrix_view submatrix_src;

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

bool CFCWS::EdgeDetect(const gsl_matrix* src, gsl_matrix* temp_buf, gsl_matrix* edged, gsl_matrix_ushort* gradients, gsl_matrix_char* directions)
{
    GaussianBlur(temp_buf, src);
    Sobel(gradients, directions, temp_buf);
    gsl_matrix_set_zero(temp_buf);
    NonMaximum_Suppression(temp_buf, directions, gradients);
    DoubleThreshold(20, 50, edged, temp_buf);

    return true;
}

bool CFCWS::CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist)
{
    if (!imgy || !result_imgy || !grayscale_hist) {
        dbg();
        return false;
    }

    uint32_t r, c, i;

    gsl_vector_set_zero(grayscale_hist);

    for (r=0 ; r<imgy->size1 ; r++) {
        for (c=0 ; c<imgy->size2 ; c++) {
            uint8_t pixel_val = (uint8_t)gsl_matrix_get(imgy, r, c);
            double val = gsl_vector_get(grayscale_hist, pixel_val);
            //dbg("(%d,%d) = %d, val %d", r, c, pixel_val, (int)val);
            gsl_vector_set(grayscale_hist, pixel_val, ++val);
        }
    }

    int cutoff_pixel_value = 70;
    int pixel_value_peak = 0;
    double hist_peak = 0;

    for (i=0 ; i<cutoff_pixel_value ; i++) {
        if (gsl_vector_get(grayscale_hist, i) > hist_peak) {
            hist_peak = gsl_vector_get(grayscale_hist, i);
            pixel_value_peak = i;
        }
    }
            
    //dbg("%d %lf\n", pixel_value_peak, hist_peak);

    for (r=0 ; r<imgy->size1 ; r++) {
        for (c=0 ; c<imgy->size2 ; c++) {
            if (gsl_matrix_get(result_imgy, r, c) > pixel_value_peak)
                gsl_matrix_set(result_imgy, r, c, NOT_SHADOW);
        }
    }

    return true;
}

bool CFCWS::CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist)
{
    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }

    uint32_t r, c;
    double val = 0;
    gsl_vector_view column_view;

    gsl_vector_set_zero(vertical_hist);

    // skip border
    for (c=1 ; c<imgy->size2 - 1 ; c++) {
        column_view = gsl_matrix_column((gsl_matrix*)imgy, c);

        for (r=1 ; r<column_view.vector.size - 1; r++) {
            if (gsl_vector_get(&column_view.vector, r) != NOT_SHADOW) {
                val = gsl_vector_get(vertical_hist, c);
                gsl_vector_set(vertical_hist, c, ++val);
            }
        }
    }

    return true;
}

bool CFCWS::CalVerticalHist(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist)
{
    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }

    uint32_t r, c, sr, sc, size;
    double val = 0;
    gsl_matrix_view submatrix;
    gsl_vector_view column_view;

    // Boundary check.
    if (start_r < 0)
        sr = 0;
    else if (start_r >= imgy->size1)
        sr = imgy->size1 - 1;
    else
        sr = start_r;

    if (start_c < 0)
        sc = 0;
    else if (start_c >= imgy->size2)
        sc = imgy->size2 - 1;
    else
        sc = start_c;

    if (sr + h >= imgy->size1)
        submatrix = gsl_matrix_submatrix((gsl_matrix*)imgy, sr, sc, imgy->size1 - sr, w);
    else
        submatrix = gsl_matrix_submatrix((gsl_matrix*)imgy, sr, sc, h, w);

    size = w;
    CheckOrReallocVector(vertical_hist, size);

    // skip border
    for (c=1 ; c<submatrix.matrix.size2 - 1 ; c++) {
        column_view = gsl_matrix_column((gsl_matrix*)&submatrix.matrix, c);

        for (r=1 ; r<column_view.vector.size - 1; r++) {
            if (gsl_vector_get(&column_view.vector, r) != NOT_SHADOW) {
                val = gsl_vector_get(vertical_hist, c);
                gsl_vector_set(vertical_hist, c, ++val);
            }
        }
    }

    return true;
}

bool CFCWS::CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist)
{
    if (!imgy || !horizontal_hist) {
        dbg();
        return false;
    }

    uint32_t r, c;
    double val = 0;
    gsl_vector_view row_view;

    gsl_vector_set_zero(horizontal_hist);

    // skip border
    for (r=1 ; r<imgy->size1 - 1 ; r++) {
        row_view = gsl_matrix_row((gsl_matrix*)imgy, r);

        for (c=1 ; c<row_view.vector.size - 1 ; c++) {
            if (gsl_vector_get(&row_view.vector, c) != 255)
            {
                val = gsl_vector_get(horizontal_hist, r);
                gsl_vector_set(horizontal_hist, r, ++val);
            }
        }
    }

    return true;
}

typedef struct PEAK {
    uint32_t value;
    uint32_t idx;
    uint32_t vote_cnt;
};

bool CFCWS::VehicleCandidateGenerate(
        const gsl_matrix* imgy, 
        const gsl_vector* horizontal_hist, 
        gsl_vector* vertical_hist, 
        Candidates& vcs)
{
    if (!imgy || !vertical_hist || !horizontal_hist) {
        dbg();
        return false;
    }

    uint32_t r, c, row, col;
    uint32_t size;
    uint32_t peak_count;
    size_t peak_idx;
    CCandidate* vc = NULL;

    uint32_t vote_success, vote_fail;
    uint32_t cur_peak, cur_peak_idx, max_peak, max_peak_idx;
    PEAK* ppeak = NULL;
    vector<PEAK*> pg;
    vector<PEAK*>::iterator pg_it;
    gsl_vector* temp_hh = NULL;

    row = imgy->size1;
    col = imgy->size2;

    for (CandidatesIT it = vcs.begin() ; it != vcs.end(); ++it) {
        if ((*it)) {
            delete (*it);
            (*it) = NULL;
        }
    }
    vcs.clear();

    // find possible blob underneath vehicle
    size = horizontal_hist->size;
    CheckOrReallocVector(m_temp_hori_hist, size);
    CheckOrReallocVector(temp_hh, size);
    gsl_vector_memcpy(m_temp_hori_hist, horizontal_hist);

    max_peak = gsl_vector_max(m_temp_hori_hist);
    max_peak_idx = gsl_vector_max_index(m_temp_hori_hist);
    dbg("==========max peak %d at %d===========", max_peak, max_peak_idx);

    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        dbg("current peak %d at %d", cur_peak, cur_peak_idx);
        if (cur_peak < (max_peak_idx * 0.6))
            break;

        gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);
        gsl_vector_memcpy(temp_hh, m_temp_hori_hist);

        ppeak = (PEAK*)malloc(sizeof(PEAK));
        ppeak->value = cur_peak;
        ppeak->idx = cur_peak_idx;
        ppeak->vote_cnt = 0;

        vote_success = vote_fail = 0;
        dbg("Voting....");
        while (1) {
            cur_peak_idx = gsl_vector_max_index(temp_hh);
            gsl_vector_set(temp_hh, cur_peak_idx, 0);
            dbg("peak at %d, delta %d", cur_peak_idx, abs((int)ppeak->idx - (int)cur_peak_idx));
            if (abs((int)ppeak->idx - (int)cur_peak_idx) <= 20) {
                gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

                if (++vote_success >= 10) {
                    ppeak->vote_cnt = vote_success;
                    pg.push_back(ppeak);
                    break;
                }
            } else {
                if (++vote_fail > 5) {
                    free(ppeak);
                    ppeak = NULL;
                    break;
                }
            }
        }
    }

    // find left boundary and right boundary of this blob
    uint32_t index;
    uint32_t max_vh, max_vh_idx;
    uint32_t left_idx, right_idx;

    for (pg_it = pg.begin() ; pg_it != pg.end(); ++pg_it) {
        index = (*pg_it)->idx; 
        CalVerticalHist(imgy, index - 20, 0, imgy->size2, 40, vertical_hist);

        max_vh = gsl_vector_max(vertical_hist);
        max_vh_idx = gsl_vector_max_index(vertical_hist);
        dbg("VH max is %d at %d", (int)max_vh, (int)max_vh_idx);

        left_idx = 0;
        //Find left boundary
        for (c=5 ; c<max_vh_idx ; ++c) {
            if (gsl_matrix_get(imgy, index, c) != NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c-1) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c-2) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c-3) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c-4) == NOT_SHADOW &&
                (gsl_matrix_get(imgy, index, c+1) != NOT_SHADOW || gsl_matrix_get(imgy, index, c+2) != NOT_SHADOW) &&
                (gsl_vector_get(vertical_hist, c+1) >= gsl_vector_get(vertical_hist, c) || 
                 gsl_vector_get(vertical_hist, c+2) >= gsl_vector_get(vertical_hist, c))) {
                left_idx = c;
                dbg("Find left");
                break;
            }
        }

        right_idx = 0;
        //Find right boundary
        for (c=max_vh_idx+1 ; c<imgy->size2 - 5 ; ++c) {
            if (gsl_matrix_get(imgy, index, c) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c+1) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c+2) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c+3) == NOT_SHADOW &&
                gsl_matrix_get(imgy, index, c+4) == NOT_SHADOW &&
                (gsl_matrix_get(imgy, index, c-1) != NOT_SHADOW || gsl_matrix_get(imgy, index, c-2) != NOT_SHADOW) &&
                (gsl_vector_get(vertical_hist, c-1) >= gsl_vector_get(vertical_hist, c) || 
                gsl_vector_get(vertical_hist, c-2) >= gsl_vector_get(vertical_hist, c))) {
                right_idx = c;
                dbg("Find right");
                break;
            }
        }

        for (c=0 ; c<col ; c++) {
            if (gsl_matrix_get(imgy, index, c) != NOT_SHADOW) {
                dbg("imgy[%d][%d]=%d, vh=%d(max. %d)", index, c, 
                                                (int)gsl_matrix_get(imgy, index, c), 
                                                (int)gsl_vector_get(vertical_hist, c),
                                                (gsl_vector_get(vertical_hist, c) == gsl_vector_max(vertical_hist) ? 1 : 0));
            }
        }

        dbg("left %d right %d", left_idx, right_idx);

        uint32_t vehicle_width, vehicle_height;

        if (left_idx && right_idx) {
            vehicle_width = (right_idx - left_idx + 1);
            vehicle_height = vehicle_width * 0.8;

            if (index - vehicle_height > 0) {
                vc = new CCandidate();
                vc->SetPos(index - vehicle_height, left_idx);
                vc->SetWH(vehicle_width, vehicle_height);
                vcs.push_back(vc);
            }
        }
    }

    FreeVector(temp_hh);

    for (pg_it = pg.begin() ; pg_it != pg.end(); ++pg_it) {
        if (*pg_it) {
            dbg("free peak at %d", (*pg_it)->idx);
            free(*pg_it);
            (*pg_it) = NULL;
        }
    }

    pg.clear();

    return true;
}
