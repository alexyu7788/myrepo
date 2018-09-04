#include "fcws.h"

enum {
    DIRECTION_45UP = 0,
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

static double dx1d_ary[] = {
    -1, 0, 1
};

static double dy_ary[] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

// Global variables
gsl_matrix*         m_imgy = NULL;
gsl_matrix*         m_edged_imgy = NULL;
gsl_matrix*         m_temp_imgy = NULL;

gsl_matrix_ushort*  m_gradient = NULL;
gsl_matrix_char*    m_direction = NULL;

gsl_matrix_view     m_gk;
double              m_gk_weight = 0;

gsl_matrix_view     m_dx;
gsl_matrix_view     m_dy;
gsl_vector_view     m_dx1d;
gsl_vector_view     m_dy1d;

VehicleCandidates   m_vcs;

gsl_vector*         m_temp_hori_hist= NULL;

blob*               m_blob_head = NULL;






bool FCW_Init()
{
    // Gaussian Kernel
    m_gk = gsl_matrix_view_array(gk_5by5, 5, 5);    
    m_gk_weight = 0;

    for (uint32_t r=0 ; r<m_gk.matrix.size1 ; r++)
        for (uint32_t c=0 ; c<m_gk.matrix.size2 ; c++)
            m_gk_weight += gsl_matrix_get(&m_gk.matrix, r, c);

    // dx & dy
    m_dx    = gsl_matrix_view_array(dx_ary, 3, 3);
    m_dy    = gsl_matrix_view_array(dy_ary, 3, 3);
    m_dx1d  = gsl_vector_view_array(dx1d_ary, 3);
    m_dy1d  = gsl_vector_view_array(dx1d_ary, 3);

    memset(&m_vcs, 0x0, sizeof(m_vcs));

    return true;
}

bool FCW_DeInit()
{
    FreeVector(&m_temp_hori_hist);

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_edged_imgy);
    FreeMatrix(&m_temp_imgy);
    FreeMatrixUshort(&m_gradient);
    FreeMatrixChar(&m_direction);

    memset(&m_vcs, 0x0, sizeof(m_vcs));

    return true;
}

bool FCW_DoDetection(uint8_t* img,int linesize, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist, VehicleCandidates *vcs)
{
    if (!img || !vertical_hist || !hori_hist || !grayscale_hist) {
        dbg();
        return false;
    }

    uint32_t i, r, c;

    CheckOrReallocMatrix(&m_imgy, h, w);
    CheckOrReallocMatrix(&m_edged_imgy, h, w);
    CheckOrReallocMatrix(&m_temp_imgy, h, w);
    CheckOrReallocMatrixUshort(&m_gradient, h, w);
    CheckOrReallocMatrixChar(&m_direction, h, w);

    // Copy image array to image matrix
    for (r=0 ; r<m_imgy->size1 ; r++)
        for (c=0 ; c<m_imgy->size2 ; c++)
            gsl_matrix_set(m_imgy, r, c, img[r * linesize + c]); 

    gsl_matrix_memcpy(m_temp_imgy, m_imgy);

    FCW_CalGrayscaleHist(m_imgy, m_temp_imgy, grayscale_hist);

    FCW_CalHorizontalHist(m_temp_imgy, hori_hist);

    gsl_matrix_memcpy(m_edged_imgy, m_imgy);
    //FCW_GaussianBlur(m_edged_imgy, m_imgy); 

    FCW_CalGradient(m_gradient,
                    m_direction,
                    m_edged_imgy,
                    0, 0, 0, 0);

    gsl_matrix_set_zero(m_edged_imgy);

    FCW_NonMaximum_Suppression(m_edged_imgy, m_direction, m_gradient);

    FCW_VehicleCandidateGenerate(m_temp_imgy,
                                m_edged_imgy,
                                hori_hist,
                                vertical_hist,
                                m_gradient,
                                m_direction,
                                &m_vcs);

    memset(vcs, 0x0, sizeof(VehicleCandidates));

    vcs->vc_count = m_vcs.vc_count;
    for (i=0 ; i<m_vcs.vc_count ; i++) {
        vcs->vc[i].m_r = m_vcs.vc[i].m_r;
        vcs->vc[i].m_c = m_vcs.vc[i].m_c;
        vcs->vc[i].m_w = m_vcs.vc[i].m_w;
        vcs->vc[i].m_h = m_vcs.vc[i].m_h;
    }

    for (r=0 ; r<m_imgy->size1 ; r++)
        for (c=0 ; c<m_imgy->size2 ; c++)
            //img[r * linesize + c] = (uint8_t)gsl_matrix_get(m_imgy, r,c); 
            //img[r * linesize + c] = (uint8_t)gsl_matrix_get(m_temp_imgy, r,c); 
            img[r * linesize + c] = (uint8_t)gsl_matrix_get(m_edged_imgy, r,c); 

    return true;
}

int FCW_GetRounded_Direction(int gx, int gy)
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

bool FCW_NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src)
{
    uint32_t r, c;
    uint32_t row, col;
    uint32_t size;

    if (!src || !dst || !dir) {
        dbg();
        return false;
    }

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

bool FCW_GaussianBlur(gsl_matrix* dst, const gsl_matrix* src)
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

    if (m_gk.matrix.size1 != m_gk.matrix.size2) {
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

bool FCW_CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int crop_r, int crop_c, int crop_w, int crop_h)
{
    int cr, cc, cw, ch;
    uint32_t i, j;
    uint32_t r, c, row, col;
    uint32_t size;
    gsl_matrix*     pmatrix;
    gsl_matrix_view submatrix_src;
    gsl_matrix_view cropmatrix_src;

    if (!src || !dst || !dir) {
        dbg();
        return false;
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

        CheckOrReallocMatrixUshort(&dst, ch, cw);
        CheckOrReallocMatrixChar(&dir, ch, cw);

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

            for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                    gx += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dx.matrix, i, j));
                }
            }

            //for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
            //    for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
            //        gy += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dy.matrix, i, j));
            //    }
            //}

            //gsl_matrix_ushort_set(dst, r, c, gsl_hypot(gx, gy));
            gsl_matrix_ushort_set(dst, r, c, abs(gx) + abs(gy));
            gsl_matrix_char_set(dir, r, c, FCW_GetRounded_Direction(gx, gy));
        }
    }

    return true;
}

bool FCW_CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist)
{
    uint32_t r, c, i;
    int cutoff_pixel_value = 70;
    int pixel_value_peak = 0;
    double hist_peak = 0;

    if (!imgy || !result_imgy || !grayscale_hist) {
        dbg();
        return false;
    }

    gsl_vector_set_zero(grayscale_hist);

    for (r=0 ; r<imgy->size1 ; r++) {
        for (c=0 ; c<imgy->size2 ; c++) {
            uint8_t pixel_val = (uint8_t)gsl_matrix_get(imgy, r, c);
            double val = gsl_vector_get(grayscale_hist, pixel_val);
            gsl_vector_set(grayscale_hist, pixel_val, ++val);
        }
    }

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

bool FCW_CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view column_view;

    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }

    gsl_vector_set_zero(vertical_hist);

    // skip border
    for (c=1 ; c<imgy->size2 - 1 ; c++) {
        column_view = gsl_matrix_column((gsl_matrix*)imgy, c);

        for (r=1 ; r<column_view.vector.size - 1; r++) {
            //dbg("[%d,%d] = %d", r, c, (int)gsl_vector_get(&column_view.vector, r));
            //if (gsl_vector_get(&column_view.vector, r) != NOT_SHADOW) {
            if (gsl_vector_get(&column_view.vector, r) != 0) {
                val = gsl_vector_get(vertical_hist, c);
                gsl_vector_set(vertical_hist, c, ++val);
            }
        }
    }

    return true;
}

bool FCW_CalVerticalHist2(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist)
{
    uint32_t r, c, sr, sc, size;
    double val = 0;
    gsl_matrix_view submatrix;
    gsl_vector_view column_view;

    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }

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
    CheckOrReallocVector(&vertical_hist, size);

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

bool FCW_CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view row_view;

    if (!imgy || !horizontal_hist) {
        dbg();
        return false;
    }

    gsl_vector_set_zero(horizontal_hist);

    // skip border
    for (r=1 ; r<imgy->size1 - 1 ; r++) {
        row_view = gsl_matrix_row((gsl_matrix*)imgy, r);

        for (c=1 ; c<row_view.vector.size - 1 ; c++) {
            if (gsl_vector_get(&row_view.vector, c) != NOT_SHADOW)
            {
                val = gsl_vector_get(horizontal_hist, r);
                gsl_vector_set(horizontal_hist, r, ++val);
            }
        }
    }

    return true;
}

bool FCW_BlobFindIdentical(blob** bhead, blob* nblob)
{
    blob* cur = NULL;

    cur = *bhead;

    while (cur != NULL) {
        if (cur->r == nblob->r && cur->c == nblob->c && cur->w == nblob->w && cur->h == nblob->h)
            return true;

        cur = cur->next;
    }

    return false;
}

bool FCW_BlobRearrange(blob** bhead)
{
    int diff;
    bool need_add = true;
    blob *head = NULL, *cur = NULL, *next = NULL, *pre = NULL, *temp = NULL; 

    head = *bhead;

    if (!head)
        return false;

redo:

    cur = head;
    while (cur) {
        //dbg("=====checking cur blob(%d,%d,%d)======",
        //            cur->r,
        //            cur->c,
        //            cur->w);

        next = cur->next;
        while (next) {
            //dbg("next blob(%d,%d,%d) <==> cur blob(%d,%d,%d).",
            //        next->r,
            //        next->c,
            //        next->w,
            //        cur->r,
            //        cur->c,
            //        cur->w);

            // next blob is above cur blob.
            if (next->r < cur->r) {
                if ((next->c >= cur->c && next->c <= cur->c + cur->w) ||
                    (next->c < cur->c && next->c + next->w > cur->c && next->c + next->w <= cur->c + cur->w)) {
                    //dbg("next blob(%d,%d,%d) is above cur blob(%d,%d,%d).",
                    //        next->r,
                    //        next->c,
                    //        next->w,
                    //        cur->r,
                    //        cur->c,
                    //        cur->w);

                    pre = cur;
                    while (pre->next != next)
                        pre = pre->next;

                    pre->next = next->next;
                    free(next);
                    next = pre;
                } else if (cur->c > next->c && cur->c + cur->w <= next->c + next->w) {
                    //dbg("\033[1;33mnext blob(%d,%d,%d) is above cur blob(%d,%d,%d).\033[m",
                    //        next->r,
                    //        next->c,
                    //        next->w,
                    //        cur->r,
                    //        cur->c,
                    //        cur->w);

                    //dbg("Update cur blob.");
                    pre = cur;
                    while (pre->next != next)
                        pre = pre->next;

                    memcpy(cur, next, sizeof(blob));
                    pre->next = next->next;
                    free(next);
                    next = pre;

                    goto redo;
                }
            }
            // next blob is below cur blob.
            else if (next->r > cur->r) {
                if ((cur->c >= next->c && cur->c <= next->c + next->w) ||
                    (cur->c < next->c && cur->c + cur->w > next->c && cur->c + cur->w <= next->c + next->w)) {
                    //dbg("next blob(%d,%d,%d) is below cur blob(%d,%d,%d).",
                    //        next->r,
                    //        next->c,
                    //        next->w,
                    //        cur->r,
                    //        cur->c,
                    //        cur->w);

                    //dbg("Update cur blob.");
                    pre = cur;
                    while (pre->next != next)
                        pre = pre->next;

                    memcpy(cur, next, sizeof(blob));
                    pre->next = next->next;
                    free(next);
                    next = pre;

                    goto redo;
                }
            // next overlaps cur.
            } else if (next->r == cur->r) {
                if ((cur->c > next->c && cur->c <= next->c + next->w) || 
                    (cur->c > next->c && cur->c > next->c + next->w && abs(cur->c - next->c - next->w) < 3) || 
                    (cur->c < next->c && cur->c + cur->w >= next->c)  || 
                    (cur->c < next->c && cur->c + cur->w < next->c && abs(cur->c + cur->w - next->c) < 3)) {
                    //dbg("next blob(%d,%d,%d) overlaps cur blob(%d,%d,%d).",
                    //        next->r,
                    //        next->c,
                    //        next->w,
                    //        cur->r,
                    //        cur->c,
                    //        cur->w);

                    // Merge cur & next
                    if (cur->c > next->c) {
                        diff = next->c + next->w - cur->c;
                        cur->c = next->c;
                        cur->w = cur->w + next->w - diff;
                    } else {
                        diff = cur->c + cur->w - next->c;
                        cur->w = cur->w + next->w - diff;
                    }

                    pre = cur;
                    while (pre->next != next)
                        pre = pre->next;

                    pre->next = next->next;
                    free(next);
                    next = pre;

                    goto redo;
                }
            }

            next = next->next;
            //printf("\n");
        }

        cur = cur->next;
        //printf("\n");
    }

    //dbg("--------End of Rearrange----------");
    return true;
}

bool FCW_BlobAdd(blob** bhead, blob* nblob)
{
    blob *cur = NULL; 

    if (!nblob)
        return false;

    if (!*bhead) {
        *bhead = nblob;
        (*bhead)->next = NULL;
    } else {
        cur = *bhead;
        while (cur->next) {
            cur = cur->next;
        }

        cur->next = nblob;
        cur = nblob;
    }

    FCW_BlobRearrange(bhead);

    return true;
}

void FCW_BlobClear(blob** bhead)
{
    blob* cur;

    while ((cur = *bhead) != NULL) {
        *bhead = (*bhead)->next;
        free(cur);
        cur = NULL;
    }
}

#define BLOB_MARGIN 5

bool FCW_BlobGenerator(const gsl_matrix* imgy, uint32_t peak_idx, blob** bhead)
{
    uint32_t i, r, c;
    uint32_t blob_pixel_cnt, max_blob_pixel_cnt;
    uint32_t blob_cnt;
    int blob_r, blob_c, blob_w, blob_h;
    int max_blob_r, max_blob_c, max_blob_w, max_blob_h;
    int sm_r, sm_c, sm_w, sm_h;
    float blob_pixel_desity;
    bool has_neighborhood = false;
    blob *newblob = NULL, *curblob = NULL;
    gsl_matrix_view submatrix;

    if (!imgy) {
        dbg();
        return false;
    }

    if (peak_idx > BLOB_MARGIN - 1 && peak_idx < (imgy->size1 - BLOB_MARGIN - 1)) {

        //dbg("peak index %d", peak_idx);

        sm_r = peak_idx - BLOB_MARGIN;
        sm_c = 0;
        sm_w = imgy->size2;
        sm_h = BLOB_MARGIN * 2;

        submatrix = gsl_matrix_submatrix((gsl_matrix*)imgy, sm_r, sm_c, sm_h, sm_w);

        blob_cnt = 0;
        blob_pixel_cnt = max_blob_pixel_cnt = 0;

        max_blob_r = max_blob_c = max_blob_w = max_blob_h = 
        blob_r = blob_c = blob_w = blob_h = -1;

        for (c=1 ; c<submatrix.matrix.size2-1 ; ++c) {
            for (r=1 ; r<submatrix.matrix.size1-1 ; ++r) {
                if ( r == 1)
                    has_neighborhood = false;

                if (gsl_matrix_get(&submatrix.matrix, r, c) != NOT_SHADOW && 
                    (gsl_matrix_get(&submatrix.matrix, r-1, c+1) != NOT_SHADOW || 
                    gsl_matrix_get(&submatrix.matrix, r, c+1) != NOT_SHADOW || 
                    gsl_matrix_get(&submatrix.matrix, r+1, c+1) != NOT_SHADOW))
                    has_neighborhood = true;

                if (gsl_matrix_get(&submatrix.matrix, r, c) != NOT_SHADOW) {
                    blob_pixel_cnt++;

                    if (blob_r <= r)
                        blob_r = r;

                    if (blob_c == -1)
                        blob_c = c;

                    if (gsl_matrix_get(&submatrix.matrix, r-1, c+1) == NOT_SHADOW && 
                        gsl_matrix_get(&submatrix.matrix,   r, c+1) == NOT_SHADOW &&
                        gsl_matrix_get(&submatrix.matrix, r+1, c+1) == NOT_SHADOW &&
                        has_neighborhood == false) {

                        blob_w = c - blob_c + 1;
                        blob_h = r - blob_r + 1;

                        if (blob_pixel_cnt > max_blob_pixel_cnt && blob_w > max_blob_w) {
                            max_blob_pixel_cnt = blob_pixel_cnt;
                            max_blob_r = peak_idx;//sm_r + blob_r;
                            max_blob_c = sm_c + blob_c;
                            max_blob_w = blob_w;
                            max_blob_h = 4;//blob_h;

                            blob_pixel_desity = max_blob_pixel_cnt / (float)(max_blob_w * max_blob_h);
                        }

                        if (max_blob_w >= 20 && blob_pixel_desity > 0.7) {

                            //dbg("Max blob %d %d %d %d %.02lf", max_blob_r, max_blob_c, max_blob_pixel_cnt, max_blob_w, blob_pixel_desity);

                            newblob = (blob*)malloc(sizeof(blob));         
                            newblob->r = max_blob_r;
                            newblob->c = max_blob_c;
                            newblob->w = max_blob_w;
                            newblob->h = max_blob_h;
                            newblob->number = blob_cnt;
                            newblob->next = NULL;

                            if (FCW_BlobFindIdentical(bhead, newblob) == false) {
                                FCW_BlobAdd(bhead, newblob);
                            } else {
                                free(newblob);
                                newblob = NULL;
                            }

                            blob_cnt++;
                            max_blob_pixel_cnt = 0;
                            max_blob_r = 
                            max_blob_c = 
                            max_blob_w = 
                            max_blob_h = -1;
                            blob_pixel_desity = 0;
                        }

                        blob_pixel_cnt = 0;
                        blob_r = blob_c = blob_w = blob_h = -1;
                    }
                }
            }
        }
    }

//    curblob = *bhead;
//    while (curblob!= NULL) {
//        dbg("[%d,%d] with [%d,%d]", curblob->r, curblob->c, curblob->w, curblob->h);
//        curblob = curblob->next;
//    }

    return true;
}

bool FCW_VehicleCandidateGenerate(
        const gsl_matrix* imgy, 
        const gsl_matrix* edged_imgy, 
        const gsl_vector* horizontal_hist, 
        gsl_vector* vertical_hist, 
        const gsl_matrix_ushort* gradient,
        const gsl_matrix_char* direction,
        VehicleCandidates* vcs)
{
    uint32_t r, c, row, col;
    //uint32_t size;
    //uint32_t peak_count;
    //size_t peak_idx;
    //CCandidate* vc = NULL;

    uint32_t vote_success, vote_fail;
    uint32_t cur_peak, cur_peak_idx, max_peak, max_peak_idx;
    PEAK_GROUP pg; 
    //PEAK* ppeak = NULL;
    //vector<PEAK*> pg;
    //vector<PEAK*>::iterator pg_it;
    gsl_vector* temp_hh = NULL;

    if (!imgy || !vertical_hist || !horizontal_hist || !gradient || !direction) {
        dbg();
        return false;
    }

    row = imgy->size1;
    col = imgy->size2;
    memset(&pg, 0x0, sizeof(PEAK_GROUP));
    memset(vcs, 0x0, sizeof(VehicleCandidates));
    //for (CandidatesIT it = vcs.begin() ; it != vcs.end(); ++it) {
    //    if ((*it)) {
    //        delete (*it);
    //        (*it) = NULL;
    //    }
    //}
    //vcs.clear();

    // find possible blob underneath vehicle
    CheckOrReallocVector(&m_temp_hori_hist, horizontal_hist->size);
    CheckOrReallocVector(&temp_hh, horizontal_hist->size);
    gsl_vector_memcpy(m_temp_hori_hist, horizontal_hist);

    max_peak = gsl_vector_max(m_temp_hori_hist);
    max_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

    if (max_peak == 0)
        return false;

    //dbg("==========max peak %d at %d===========", max_peak, max_peak_idx);

    FCW_BlobClear(&m_blob_head);

#if 1
    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        if (cur_peak < (max_peak * 0.4))
            break;

        //printf("\n");
        //dbg("Search blobs for current peak %d at %d", cur_peak, cur_peak_idx);

        gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

        FCW_BlobGenerator(imgy, cur_peak_idx, &m_blob_head);
    }
    
    if (m_blob_head) {

        blob *cur = m_blob_head;
        int left_idx, right_idx, bottom_idx;
        int vehicle_startr, vehicle_startc;
        int vehicle_width, vehicle_height;
        gsl_matrix_view imgy_submatrix;

        while (cur != NULL) {

            //scale up searching region of blob with 1/4 time of width on the left & right respectively.
            bottom_idx      = cur->r;
            left_idx        = (cur->c - (cur->w >> 2));
            right_idx       = (cur->c + cur->w + (cur->w >> 2));

            left_idx        = (left_idx < 0 ? 0 : left_idx);
            right_idx       = (right_idx >= edged_imgy->size2 ? edged_imgy->size2 - 1 : right_idx);

            vehicle_width   = (right_idx - left_idx + 1);
            vehicle_height  = vehicle_width * 0.5;

            //dbg("Scale up left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);

            vehicle_startr  = (bottom_idx - vehicle_height < 0 ? 0 : bottom_idx - vehicle_height);
            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= imgy->size2) {
                right_idx       = edged_imgy->size2 - 1;
                vehicle_width   = right_idx - left_idx + 1;
            } 

            //dbg("Vehicle at (%d,%d) with (%d,%d)", 
            //            vehicle_startr,
            //            vehicle_startc,
            //            vehicle_width,
            //            vehicle_height);

            cur->r = vehicle_startr;
            cur->c = vehicle_startc;
            cur->w = vehicle_width;
            cur->h = vehicle_height;

            imgy_submatrix = gsl_matrix_submatrix((gsl_matrix*)edged_imgy,
                                                                vehicle_startr,
                                                                vehicle_startc,
                                                                vehicle_height,
                                                                vehicle_width);

            FCW_UpdateVehicleCanidateByEdge2(&imgy_submatrix.matrix, cur);

            vcs->vc[vcs->vc_count].m_r = cur->r;
            vcs->vc[vcs->vc_count].m_c = cur->c;
            vcs->vc[vcs->vc_count].m_w = cur->w;
            vcs->vc[vcs->vc_count].m_h = cur->h;

            vcs->vc_count++;
            cur = cur->next;
        }
    }


    return true;
#else
    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        if (cur_peak < (max_peak_idx * 0.8))
            break;

        printf("\n");
        dbg("vote for current peak %d at %d", cur_peak, cur_peak_idx);

        gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);
        gsl_vector_memcpy(temp_hh, m_temp_hori_hist);

        pg.peak[pg.peak_count].value    = cur_peak;
        pg.peak[pg.peak_count].idx      = cur_peak_idx;
        pg.peak[pg.peak_count].vote_cnt = 0;

        //ppeak = (PEAK*)malloc(sizeof(PEAK));
        //ppeak->value = cur_peak;
        //ppeak->idx = cur_peak_idx;
        //ppeak->vote_cnt = 0;

        vote_success = vote_fail = 0;
        while (1) {
            cur_peak_idx = gsl_vector_max_index(temp_hh);
            gsl_vector_set(temp_hh, cur_peak_idx, 0);
            //dbg("peak at %d, delta %d", cur_peak_idx, abs((int)ppeak->idx - (int)cur_peak_idx));
            if (abs((int)pg.peak[pg.peak_count].idx - (int)cur_peak_idx) <= 20) {
                gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

                if (++vote_success >= 10) {
                    pg.peak[pg.peak_count++].vote_cnt = vote_success;
                    dbg("vote_success %d", vote_success);
                    //ppeak->vote_cnt = vote_success;
                    //pg.push_back(ppeak);
                    break;
                }
            } else {
                if (++vote_fail > 5) {
                    dbg("vote_fail %d", vote_fail);
                    //free(ppeak);
                    //ppeak = NULL;
                    pg.peak[pg.peak_count].value    = cur_peak;
                    pg.peak[pg.peak_count].idx      = cur_peak_idx;
                    pg.peak[pg.peak_count].vote_cnt = 0;
                    break;
                }
            }
        }
    }

#endif
    printf("\n");
    dbg("Guessing left & right boundary....");
    // Guessing left boundary and right boundary of this blob
    int bottom_idx;
    int left_idx, right_idx;
    uint32_t max_vh, max_vh_idx;

    //for (pg_it = pg.begin() ; pg_it != pg.end(); ++pg_it) {
    //    bottom_idx = (*pg_it)->idx; 
    for (cur_peak_idx = 0 ; cur_peak_idx < pg.peak_count && cur_peak_idx < MAX_PEAK_COUNT ; ++cur_peak_idx) {
        bottom_idx = pg.peak[cur_peak_idx].idx;
        FCW_CalVerticalHist2(imgy, bottom_idx - 20, 0, imgy->size2, 20, vertical_hist);

        max_vh = gsl_vector_max(vertical_hist);
        max_vh_idx = gsl_vector_max_index(vertical_hist);

        printf("\n");
        dbg("VH max is %d at %d for bottom %d", (int)max_vh, (int)max_vh_idx, bottom_idx);

        left_idx = 0;
        //Find left boundary
        for (c=5 ; c<max_vh_idx ; ++c) {
            if (gsl_matrix_get(imgy, bottom_idx, c) != NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c-1) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c-2) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c-3) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c-4) == NOT_SHADOW &&
                (gsl_matrix_get(imgy, bottom_idx, c+1) != NOT_SHADOW || gsl_matrix_get(imgy, bottom_idx, c+2) != NOT_SHADOW) &&
                (gsl_vector_get(vertical_hist, c+1) >= gsl_vector_get(vertical_hist, c) || 
                 gsl_vector_get(vertical_hist, c+2) >= gsl_vector_get(vertical_hist, c))) {
                left_idx = c;
                dbg("Find left boundary");
                break;
            }
        }

        right_idx = 0;
        //Find right boundary
        for (c=max_vh_idx+1 ; c<imgy->size2 - 5 ; ++c) {
            if (gsl_matrix_get(imgy, bottom_idx, c) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c+1) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c+2) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c+3) == NOT_SHADOW &&
                gsl_matrix_get(imgy, bottom_idx, c+4) == NOT_SHADOW &&
                (gsl_matrix_get(imgy, bottom_idx, c-1) != NOT_SHADOW || gsl_matrix_get(imgy, bottom_idx, c-2) != NOT_SHADOW) &&
                (gsl_vector_get(vertical_hist, c-1) >= gsl_vector_get(vertical_hist, c) || 
                gsl_vector_get(vertical_hist, c-2) >= gsl_vector_get(vertical_hist, c))) {
                right_idx = c;
                dbg("Find right boundary");
                break;
            }
        }

#if 0
        for (c=0 ; c<col ; c++) {
            if (gsl_matrix_get(imgy, bottom_idx, c) != NOT_SHADOW) {
                dbg("imgy[%d][%d]=%d, vh=%d(max. %d)", bottom_idx, c, 
                                                (int)gsl_matrix_get(imgy, bottom_idx, c), 
                                                (int)gsl_vector_get(vertical_hist, c),
                                                (gsl_vector_get(vertical_hist, c) == gsl_vector_max(vertical_hist) ? 1 : 0));
            }
        }
#endif

        dbg("Guessing left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);
#if 1
        if (left_idx && right_idx) {
            int temp_h;

            vcs->vc[vcs->vc_count].m_c = left_idx;
            vcs->vc[vcs->vc_count].m_w = (right_idx - left_idx + 1);

            temp_h = vcs->vc[vcs->vc_count].m_w * 0.5;
            vcs->vc[vcs->vc_count].m_r = (bottom_idx - temp_h < 0 ? 0 : bottom_idx - temp_h);
            vcs->vc[vcs->vc_count].m_h = (bottom_idx - temp_h < 0 ? bottom_idx : temp_h);

            dbg("[%d] => [%d, %d] with [%d, %d]", 
                    vcs->vc_count,
                    vcs->vc[vcs->vc_count].m_r,
                    vcs->vc[vcs->vc_count].m_c,
                    vcs->vc[vcs->vc_count].m_w,
                    vcs->vc[vcs->vc_count].m_h);

            vcs->vc_count++;
        }

        continue;
#endif

        int vehicle_startr, vehicle_startc;
        int vehicle_width, vehicle_height;
        gsl_matrix_view imgy_submatrix;
        gsl_matrix_ushort_view gradient_submatrix;
        gsl_matrix_char_view direction_submatrix;

        vehicle_width = (right_idx - left_idx + 1);

        if (left_idx && right_idx && vehicle_width >= 20 && vehicle_width < (imgy->size2 * 3.0 / 4.0)) {
            //scale up searching region of blob with 1/4 time of width on the left & right respectively.
            left_idx        = (left_idx - (vehicle_width >> 2));
            right_idx       = (right_idx + (vehicle_width >> 2));

            left_idx        = (left_idx < 0 ? 0 : left_idx);
            right_idx       = (right_idx >= imgy->size2 ? imgy->size2 - 1 : right_idx);

            vehicle_width   = (right_idx - left_idx + 1);
            vehicle_height  = vehicle_width * 0.3;

            dbg("Scale up left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);

            vehicle_startr  = (bottom_idx - vehicle_height < 0 ? 0 : bottom_idx - vehicle_height);
            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= imgy->size2) {
                right_idx       = imgy->size2 - 1;
                vehicle_width   = right_idx - left_idx + 1;
            } 

            dbg("Vehicle at (%d,%d) with (%d,%d)", 
                        vehicle_startr,
                        vehicle_startc,
                        vehicle_width,
                        vehicle_height);

            imgy_submatrix = gsl_matrix_submatrix((gsl_matrix*)edged_imgy,
                                                                vehicle_startr,
                                                                vehicle_startc,
                                                                vehicle_height,
                                                                vehicle_width);

            gradient_submatrix = gsl_matrix_ushort_submatrix((gsl_matrix_ushort*)gradient,
                                                                vehicle_startr,
                                                                vehicle_startc,
                                                                vehicle_height,
                                                                vehicle_width);

            direction_submatrix = gsl_matrix_char_submatrix((gsl_matrix_char*)gradient,
                                                                vehicle_startr,
                                                                vehicle_startc,
                                                                vehicle_height,
                                                                vehicle_width);

#if 1 
            FCW_UpdateVehicleCanidateByEdge(&imgy_submatrix.matrix, 
                                       &gradient_submatrix.matrix, 
                                       &direction_submatrix.matrix,
                                       &vehicle_startr,
                                       &vehicle_startc,
                                       &vehicle_width,
                                       &vehicle_height);
#endif

            dbg("Vehicle at (%d,%d) with (%d,%d)", 
                        vehicle_startr,
                        vehicle_startc,
                        vehicle_width,
                        vehicle_height);

            vcs->vc[vcs->vc_count].m_r = vehicle_startr;
            vcs->vc[vcs->vc_count].m_c = vehicle_startc;
            vcs->vc[vcs->vc_count].m_w = vehicle_width;
            vcs->vc[vcs->vc_count].m_h = vehicle_height;
            vcs->vc_count++;
            //vc = new CCandidate();
            //vc->SetPos(vehicle_startr, vehicle_startc);
            //vc->SetWH(vehicle_width, vehicle_height);
            //vcs.push_back(vc);
        }
    }

    FreeVector(&temp_hh);

    //for (pg_it = pg.begin() ; pg_it != pg.end(); ++pg_it) {
    //    if (*pg_it) {
    //        free(*pg_it);
    //        (*pg_it) = NULL;
    //    }
    //}

    //pg.clear();

    return true;
}

bool FCW_UpdateVehicleCanidateByEdge2(const gsl_matrix* imgy, blob*  blob)
{
    char dir;
    uint32_t r, c;
    uint32_t right_idx, bottom_idx;
    uint32_t magnitude, max_magnitude = 0;
    uint32_t max_magnitude_idx;
    gsl_matrix_view imgy_block;
    gsl_matrix_ushort_view gradient_block;
    gsl_matrix_char_view direction_block;

    if (!imgy) {
        dbg();
        return false;
    }














    return true;
}

bool FCW_UpdateVehicleCanidateByEdge(
        const gsl_matrix* imgy,
        const gsl_matrix_ushort* gradient,
        const gsl_matrix_char* direction,
        int*  vsr,
        int*  vsc,
        int*  vw,
        int*  vh)
{
    char dir;
    uint32_t r, c;
    uint32_t right_idx, bottom_idx;
    uint32_t magnitude, max_magnitude = 0;
    uint32_t max_magnitude_idx;
    gsl_matrix_view imgy_block;
    gsl_matrix_ushort_view gradient_block;
    gsl_matrix_char_view direction_block;

    if (!imgy || !gradient || !direction) {
        dbg();
        return false;
    }

    bottom_idx = *vsr + *vh;
    right_idx = *vsc + *vw - 1;

    // update left idx
    dbg("Update left idx");
    max_magnitude = 0;
    max_magnitude_idx = 0;

    for (c=0 ; c<((gradient->size2 / 4) - 2) ; ++c) {
        imgy_block = gsl_matrix_submatrix((gsl_matrix*)imgy, 
                                                        0, 
                                                        c, 
                                                        gradient->size1, 
                                                        2);

        gradient_block = gsl_matrix_ushort_submatrix((gsl_matrix_ushort*)gradient, 
                                                        0, 
                                                        c, 
                                                        gradient->size1, 
                                                        2);

        direction_block = gsl_matrix_char_submatrix((gsl_matrix_char*)direction, 
                                                        0, 
                                                        c, 
                                                        direction->size1, 
                                                        2);

        magnitude = 0;

        for (uint32_t rr=0 ; rr<gradient_block.matrix.size1 ; ++rr) {
            for (uint32_t cc=0 ; cc<gradient_block.matrix.size2 ; ++cc) {
                dir = gsl_matrix_char_get(&direction_block.matrix, rr, cc);
                if (1 || dir == DIRECTION_HORIZONTAL) {
                    //if ( direction == 0 || direction == 1 || direction == 11) {
                    //magnitude += gsl_matrix_ushort_get(&gradient_block.matrix, rr, cc);
                    magnitude += gsl_matrix_get(&imgy_block.matrix, rr, cc);
                }
            }
        }

        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_magnitude_idx = c;
            dbg("max %d at %d", max_magnitude, max_magnitude_idx);
        }
    }

    vsc += max_magnitude_idx;

    // upate right idx
    dbg("Update right idx");
    max_magnitude = 0;
    max_magnitude_idx = 0;

    for (c=gradient->size2 - 2 ; c>((gradient->size2 * 3 / 4) - 2) ; --c) {
        imgy_block = gsl_matrix_submatrix((gsl_matrix*)imgy, 
                                                        0, 
                                                        c, 
                                                        gradient->size1, 
                                                        2);

        gradient_block = gsl_matrix_ushort_submatrix((gsl_matrix_ushort*)gradient, 
                                                        0, 
                                                        c, 
                                                        gradient->size1, 
                                                        2);

        direction_block = gsl_matrix_char_submatrix((gsl_matrix_char*)direction, 
                                                        0, 
                                                        c, 
                                                        direction->size1, 
                                                        2);

        magnitude = 0;

        for (uint32_t rr=0 ; rr<gradient_block.matrix.size1 ; ++rr) {
            for (uint32_t cc=0 ; cc<gradient_block.matrix.size2 ; ++cc) {
                dir = gsl_matrix_char_get(&direction_block.matrix, rr, cc);
                if (1 || dir == DIRECTION_HORIZONTAL) {
                    //if ( direction == 0 || direction == 1 || direction == 11) {
                    //magnitude += gsl_matrix_ushort_get(&gradient_block.matrix, rr, cc);
                    magnitude += gsl_matrix_get(&imgy_block.matrix, rr, cc);
                }
            }
        }

        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_magnitude_idx = c;
            dbg("max %d at %d", max_magnitude, max_magnitude_idx);
        }
    }

    right_idx = right_idx - (*vw - max_magnitude_idx);

    *vw = right_idx - *vsc + 1;
    *vh = *vw * 0.5;
    *vsr = bottom_idx - *vh;

    return true;
}


