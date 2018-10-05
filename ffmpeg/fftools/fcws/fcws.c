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
gsl_matrix*         m_shadow = NULL;
gsl_matrix*         m_vedge_imgy = NULL;
gsl_matrix*         m_heatmap = NULL;
gsl_matrix*         m_temp_imgy = NULL;
gsl_matrix*         m_intimg = NULL;    // integral image
gsl_matrix*         m_shadow2 = NULL;   // shadow by integral image

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
gsl_matrix_char*    m_heatmap_id = NULL;
Candidate*          m_cand_head = NULL;

uint64_t frame_count = 0;

#define VehicleWidth 1650   //mm
#define EFL          2.7    //mm
#define PixelSize    2.8    //um = 10^-3 mm


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
    FreeMatrix(&m_shadow);
    FreeMatrix(&m_vedge_imgy);
    FreeMatrix(&m_temp_imgy);
    FreeMatrix(&m_intimg);
    FreeMatrix(&m_shadow2);
    FreeMatrixUshort(&m_gradient);
    FreeMatrixChar(&m_direction);
    FreeMatrixChar(&m_heatmap_id);

    memset(&m_vcs, 0x0, sizeof(m_vcs));

    FCW_BlobClear(&m_blob_head);

    return true;
}

bool FCW_PixelInROI(uint32_t r, uint32_t c, const roi_t* roi)
{
    float slopl, slopr;

    if (!roi) {
        dbg();
        return false;
    }

    if (r < roi->point[ROI_LEFTTOP].r || r > roi->point[ROI_LEFTBOTTOM].r)
        return false;

    if (c < roi->point[ROI_LEFTBOTTOM].c || c > roi->point[ROI_RIGHTBOTTOM].c)
        return false;
    
    slopl = (roi->point[ROI_LEFTTOP].r - roi->point[ROI_LEFTBOTTOM].r) / (float)(roi->point[ROI_LEFTTOP].c - roi->point[ROI_LEFTBOTTOM].c);
    slopr = (roi->point[ROI_RIGHTTOP].r - roi->point[ROI_RIGHTBOTTOM].r) / (float)(roi->point[ROI_RIGHTTOP].c - roi->point[ROI_RIGHTBOTTOM].c);

    if (c >= roi->point[ROI_LEFTBOTTOM].c && c <= roi->point[ROI_LEFTTOP].c && 
        (((int)c - roi->point[ROI_LEFTBOTTOM].c == 0) || (((int)r - roi->point[ROI_LEFTBOTTOM].r) / (float)((int)c - roi->point[ROI_LEFTBOTTOM].c)) < slopl))
        return false;

    if (c >= roi->point[ROI_RIGHTTOP].c && c <= roi->point[ROI_RIGHTBOTTOM].c && 
        (((int)c - roi->point[ROI_RIGHTBOTTOM].c == 0) || (((int)r - roi->point[ROI_RIGHTBOTTOM].r) / (float)((int)c - roi->point[ROI_RIGHTBOTTOM].c)) > slopr))
        return false;

    return true;
}

bool FCW_Thresholding(
        gsl_matrix* src, 
        gsl_matrix* dst, 
        gsl_vector* grayscale_hist,
        uint8_t* hist_peak,
        uint8_t* otsu_th,
        uint8_t* final_th
        )
{
    uint8_t pixel_val, peak_idx, th, th2;
    uint32_t r, c, i;
    double val;

    if (!src || !dst || !grayscale_hist) {
        dbg();
        return false;
    }

    if (src->size1 != dst->size1 && src->size2 != dst->size2) {
        dbg();
        return false;
    }

    gsl_matrix_memcpy(dst, src);
    gsl_vector_set_zero(grayscale_hist);

    for (r=0 ; r<src->size1 ; ++r) {
        for (c=0 ; c<src->size2 ; ++c) {
            pixel_val = (uint8_t)gsl_matrix_get(src, r, c);

            if (pixel_val != NOT_SHADOW) {
                val = gsl_vector_get(grayscale_hist, pixel_val);
                gsl_vector_set(grayscale_hist, pixel_val, ++val);
            }
        }
    }

    th = FCW_OtsuThreshold(grayscale_hist, src->size1 * src->size2);
    peak_idx = gsl_vector_max_index(grayscale_hist);

    if (peak_idx > 100)
        th2 = (th * 0.7 + peak_idx * 0.3);
    else if (peak_idx > 80 && peak_idx <= 100)
        th2 = (th * 0.5 + peak_idx * 0.5);
    else
        th2 = (th * 0.3 + peak_idx * 0.7);

    for (r=0 ; r<src->size1 ; ++r) {
        for (c=0 ; c<src->size2 ; ++c) {
            if (gsl_matrix_get(dst, r, c) > th2)
                gsl_matrix_set(dst, r, c, NOT_SHADOW);
        }
    }

    *hist_peak = peak_idx;
    *otsu_th = th;
    *final_th = th2;

    return true;
}

bool FCW_ThresholdingByIntegralImage(
        gsl_matrix* src,
        gsl_matrix* intimg,
        gsl_matrix* dst,
        uint32_t s,
        float p
        )
{
    uint32_t r, c;
    uint32_t r1, c1, r2, c2;
    uint32_t count;
    double sum;
   
    if (!src || !intimg || !dst) {
        dbg();
        return false;
    }

    // Generate integral image for retrive average value of a rectange quickly.
    for (c=0 ; c<src->size2 ; ++c) {
        sum = 0;
        for (r=0 ; r<src->size1 ; ++r) {
            sum += gsl_matrix_get(src, r, c);

            if (c == 0)
                gsl_matrix_set(intimg, r, c, sum);
            else
                gsl_matrix_set(intimg, r, c, gsl_matrix_get(intimg, r, c-1) + sum);
        }
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
                    gsl_matrix_set(dst, r, c, NOT_SHADOW);
            }
            else
                gsl_matrix_set(dst, r, c, NOT_SHADOW);
        }
    }

    return true;
}

bool FCW_DoDetection(
        uint8_t* img, 
        int linesize, 
        int w, 
        int h, 
        gsl_vector* vertical_hist, 
        gsl_vector* hori_hist, 
        gsl_vector* grayscale_hist, 
        VehicleCandidates *vcs,
        VehicleCandidates *vcs2,
        uint8_t* roi_img,
        uint8_t* vedge,
        uint8_t* shadow,
        uint8_t* shadow2,
        uint8_t* heatmap,
        const roi_t* roi,
        uint8_t* hist_peak,
        uint8_t* otsu_th,
        uint8_t* final_th
        )
{
    uint32_t i, r, c;

    if (!img || !vertical_hist || !hori_hist || !grayscale_hist || !vedge || !shadow || !heatmap) {
        dbg();
        return false;
    }

    CheckOrReallocMatrix(&m_imgy, h, w, true);
    CheckOrReallocMatrix(&m_shadow, h, w, true);
    CheckOrReallocMatrix(&m_vedge_imgy, h, w, true);
    CheckOrReallocMatrix(&m_temp_imgy, h, w, true);
    CheckOrReallocMatrix(&m_heatmap, h, w, false);
    CheckOrReallocMatrix(&m_intimg, h, w, true);
    CheckOrReallocMatrix(&m_shadow2, h, w, true);
    CheckOrReallocMatrixChar(&m_heatmap_id, h, w, false);
    CheckOrReallocMatrixUshort(&m_gradient, h, w, true);
    CheckOrReallocMatrixChar(&m_direction, h, w, true);

    // Integral Image-based thresholding-----------------------------------------------
    // Copy image array to image matrix
    for (r=0 ; r<m_imgy->size1 ; r++) {
        for (c=0 ; c<m_imgy->size2 ; c++) {
            gsl_matrix_set(m_imgy, r, c, img[r * linesize + c]); 
        }
    }

    // Get thresholding image
    FCW_ThresholdingByIntegralImage(m_imgy, m_intimg, m_temp_imgy, 50, 0.7);

    // Get horizontal histogram
    for (r=0 ; r<m_temp_imgy->size1 ; r++) {
        for(c=0 ; c<m_temp_imgy->size2 ; c++) {
            if (FCW_PixelInROI(r, c, roi) == true)
                gsl_matrix_set(m_shadow2, r, c, gsl_matrix_get(m_temp_imgy, r, c));
            else
                gsl_matrix_set(m_shadow2, r, c, NOT_SHADOW);
        }
    }
    FCW_CalHorizontalHist(m_shadow2, hori_hist);
    // Integral Image-based thresholding-----------------------------------------------

    // Otus-based threshold------------------------------------------------------------
    for (r=0 ; r<m_imgy->size1 ; r++) {
        for (c=0 ; c<m_imgy->size2 ; c++) {
            // Generate ROI image based roi
            if (FCW_PixelInROI(r, c, roi) == true)
                gsl_matrix_set(m_temp_imgy, r, c, img[r * linesize + c]);
            else
                gsl_matrix_set(m_temp_imgy, r, c, NOT_SHADOW);
        }
    }

    // Get thresholding image
    FCW_Thresholding(m_temp_imgy, m_shadow, grayscale_hist, hist_peak, otsu_th, final_th);

    // Otus-based threshold------------------------------------------------------------

    // Get vertical edge.
    FCW_EdgeDetection(m_imgy, m_vedge_imgy, m_gradient, m_direction, 1);

    // Get VC
    FCW_VehicleCandidateGenerate(m_shadow2,
                                m_vedge_imgy,
                                hori_hist,
                                &m_vcs);

    // Update HeatMap
    FCW_UpdateVehicleHeatMap(m_heatmap, m_heatmap_id, &m_vcs);

    FCW_UpdateVCStatus(m_heatmap, m_heatmap_id, &m_cand_head, &m_vcs, vcs2);

    memset(vcs, 0x0, sizeof(VehicleCandidates));

    vcs->vc_count = m_vcs.vc_count;
    for (i=0 ; i<m_vcs.vc_count ; i++) {
        vcs->vc[i].m_valid  = m_vcs.vc[i].m_valid;
        vcs->vc[i].m_id     = m_vcs.vc[i].m_id;
        vcs->vc[i].m_dist   = m_vcs.vc[i].m_dist;
        vcs->vc[i].m_r      = m_vcs.vc[i].m_r;
        vcs->vc[i].m_c      = m_vcs.vc[i].m_c;
        vcs->vc[i].m_w      = m_vcs.vc[i].m_w;
        vcs->vc[i].m_h      = m_vcs.vc[i].m_h;
    }

    for (i=0 ; i<vcs2->vc_count ; ++i) {
        if (vcs2->vc[i].m_valid) {
        dbg("\033[1;33mVehicle %d at (%d,%d) with (%d,%d), dist %.02lfm\033[m\n\n",
                vcs2->vc[i].m_id,
                vcs2->vc[i].m_r,
                vcs2->vc[i].m_c,
                vcs2->vc[i].m_w,
                vcs2->vc[i].m_h,
                vcs2->vc[i].m_dist);
        }
        
    }

    // Copy matrix to data buffer for showing.
    for (r=0 ; r<m_imgy->size1 ; r++) {
        for (c=0 ; c<m_imgy->size2 ; c++) {
            img     [r * linesize + c] = (uint8_t)gsl_matrix_get(m_imgy, r,c); 
            shadow  [r * linesize + c] = (uint8_t)gsl_matrix_get(m_shadow, r,c); 
            shadow2 [r * linesize + c] = (uint8_t)gsl_matrix_get(m_shadow2, r,c); 
            roi_img [r * linesize + c] = (uint8_t)gsl_matrix_get(m_temp_imgy, r,c); 
            vedge   [r * linesize + c] = (uint8_t)gsl_matrix_get(m_vedge_imgy, r,c); 
            heatmap [r * linesize + c] = (uint8_t)gsl_matrix_get(m_heatmap, r,c); 
        }
    }

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

bool FCW_CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int direction, int crop_r, int crop_c, int crop_w, int crop_h)
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

        CheckOrReallocMatrixUshort(&dst, ch, cw, true);
        CheckOrReallocMatrixChar(&dir, ch, cw, true);

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
            
    //dbg("%d %lf/%lf at %d", pixel_value_peak, hist_peak, gsl_vector_max(grayscale_hist), gsl_vector_max_index(grayscale_hist));

    for (r=0 ; r<imgy->size1 ; r++) {
        for (c=0 ; c<imgy->size2 ; c++) {
            if (gsl_matrix_get(result_imgy, r, c) > pixel_value_peak)
                gsl_matrix_set(result_imgy, r, c, NOT_SHADOW);
        }
    }

    return true;
}

uint8_t FCW_OtsuThreshold(gsl_vector* grayscale_hist, int pixel_count)
{
    uint8_t sumB=0, sum1=0;
    float wB=0, wF=0, mF=0, max_var=0, inter_var=0;
    uint8_t th=0;
    uint16_t index_histo=0;

    for (index_histo = 1 ; index_histo < grayscale_hist->size ; index_histo++)
        sum1 += index_histo * gsl_vector_get(grayscale_hist, index_histo);

    for (index_histo = 1 ; index_histo < grayscale_hist->size ; index_histo++) {
        wB = wB + gsl_vector_get(grayscale_hist, index_histo);
        wF = pixel_count - wB;

        if (wB == 0 || wF == 0)
            continue;

        sumB = sumB + index_histo * gsl_vector_get(grayscale_hist, index_histo);
        mF = (sum1 - sumB) / wF;
        inter_var = wB * wF * ((sumB / wB) - mF) * ((sumB / wB) - mF);

        if (inter_var >= max_var) {
            th = index_histo;
            max_var = inter_var;
        }
    }

    //dbg("th %d", th);
    return th;
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
    CheckOrReallocVector(&vertical_hist, size, true);

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
    int diff, number;
    blob *head = NULL, *cur = NULL, *next = NULL, *pre = NULL, *temp = NULL; 

    head = *bhead;

    if (!head)
        return false;

    //dbg("--------Start of Rearrange----------");
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
                /*
                 *  a:   next     ------
                 *       cur   ------------- (delete next)
                 *
                 *  b:   next  --------
                 *       cur      ---------- (delete next)
                 *
                 *  c:   next  -----------
                 *       cur     ------      (copy next to cur and delete next)
                 *
                 */
                if ((/*(a)*/next->c >= cur->c && next->c <= cur->c + cur->w) ||
                    (/*(b)*/next->c < cur->c && next->c + next->w > cur->c && next->c + next->w <= cur->c + cur->w)) {
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
                } else if (/*(c)*/cur->c > next->c && cur->c + cur->w <= next->c + next->w) {
                    if ((cur->r - next->r) < (cur->w / 2)) {
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

                        temp = cur->next;
                        memcpy(cur, next, sizeof(blob));
                        cur->next = temp;

                        pre->next = next->next;
                        free(next);
                        next = pre;

                        dbg("redo");
                        goto redo;
                    }
                }
            }
            // next blob is below cur blob.
            else if (next->r > cur->r) {
                /*
                 *  d:   cur     ------             -------
                 *       next   ------------- or  -------        (copy next to cur and delete next)
                 *
                 *  e:   cur     --------
                 *       next      ----------                    (copy next to cur and delete next)
                 */
                if (/*(d)*/(cur->c >= next->c && cur->c <= next->c + next->w) ||
                    /*(e)*/(cur->c < next->c && cur->c + cur->w > next->c && cur->c + cur->w <= next->c + next->w)) {
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

                    temp = cur->next;
                    memcpy(cur, next, sizeof(blob));
                    cur->next = temp;

                    pre->next = next->next;
                    free(next);
                    next = pre;

                    //dbg("redo");
                    goto redo;
                }
            // next overlaps cur.
            } else if (next->r == cur->r) {
                /*
                 *  f:        next       cur 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  g:        next    3 pixels   cur 
                 *       -----------|<------->|-------      (merge cur & next)
                 *
                 *  h:        cur       next 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  i:        cur    3 pixels   next 
                 *       -----------|<------->|-------      (merge cur & next)
                 */
                if (/*(f)*/(cur->c > next->c && cur->c <= next->c + next->w) || 
                    /*(g)*/(cur->c > next->c && cur->c > next->c + next->w && abs(cur->c - next->c - next->w) < 3) || 
                    /*(h)*/(cur->c < next->c && cur->c + cur->w >= next->c)  || 
                    /*(i)*/(cur->c < next->c && cur->c + cur->w < next->c && abs(cur->c + cur->w - next->c) < 3)) {
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

                    //dbg("redo");
                    goto redo;
                }
            }

            next = next->next;
            //printf("\n");
        }

        cur = cur->next;
        //printf("\n");
    }

    number = 0;
    cur = head;
    while (cur) {
        cur->number = number;
        number++;
        cur = cur->next;
    }

    //dbg("--------End of Rearrange----------");
    return true;
}

bool FCW_BlobAdd(blob** bhead, blob* nblob)
{
    blob *cur = NULL, *newblob = NULL; 

    if (!nblob)
        return false;

    newblob = (blob*)malloc(sizeof(blob));
    memcpy(newblob, nblob, sizeof(blob));

    if (!*bhead) {
        *bhead = newblob;
        (*bhead)->next = NULL;
    } else {
        cur = *bhead;
        while (cur->next) {
            cur = cur->next;
        }

        cur->next = newblob;
        cur = newblob;
    }

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

#define BLOB_MARGIN 3

bool FCW_BlobGenerator(const gsl_matrix* imgy, uint32_t peak_idx, blob** bhead)
{
    int i, r, c;
    int blob_r_top, blob_r_bottom, blob_c, blob_w, blob_h;
    int max_blob_r, max_blob_c, max_blob_w, max_blob_h;
    int sm_r, sm_c, sm_w, sm_h;
    int stop_wall_cnt;
    uint32_t blob_cnt;
    uint32_t blob_pixel_cnt, max_blob_pixel_cnt;
    float blob_pixel_density;
    bool has_neighborhood = false;
    blob *curblob = NULL, tblob;
    gsl_matrix_view submatrix;

    if (!imgy) {
        dbg();
        return false;
    }

    if (peak_idx > BLOB_MARGIN + 1 && peak_idx < (imgy->size1 - BLOB_MARGIN - 1)) {

        //dbg("peak index %d", peak_idx);

        sm_r = peak_idx - BLOB_MARGIN;
        sm_c = 0;
        sm_w = imgy->size2;
        sm_h = BLOB_MARGIN * 2;

        submatrix = gsl_matrix_submatrix((gsl_matrix*)imgy, sm_r, sm_c, sm_h, sm_w);

        blob_cnt = 0;
        blob_pixel_cnt = max_blob_pixel_cnt = 0;

        max_blob_r = max_blob_c = max_blob_w = max_blob_h = 
        blob_c = blob_w = blob_h = -1;
        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
        blob_c = INT_MIN;

        for (c=1 ; c<submatrix.matrix.size2-1 ; ++c) {
            for (r=1 ; r<submatrix.matrix.size1-1 ; ++r) {
                if (r == 1) {
                    stop_wall_cnt = sm_h - 2;
                    has_neighborhood = false;
                }

                //dbg("(%d,%d)=> %d %d %d %d, n %d, cnt %d, w %d",
                //        r,c,
                //        (uint8_t)gsl_matrix_get(&submatrix.matrix, r, c),
                //        (uint8_t)gsl_matrix_get(&submatrix.matrix, r-1, c+1),
                //        (uint8_t)gsl_matrix_get(&submatrix.matrix, r, c+1),
                //        (uint8_t)gsl_matrix_get(&submatrix.matrix, r+1, c+1),
                //        has_neighborhood,
                //        blob_pixel_cnt,
                //        stop_wall_cnt
                //   );

                if (gsl_matrix_get(&submatrix.matrix, r, c) != NOT_SHADOW) {

                    if (gsl_matrix_get(&submatrix.matrix, r-1, c+1) != NOT_SHADOW ||
                        gsl_matrix_get(&submatrix.matrix,   r, c+1) != NOT_SHADOW ||
                        gsl_matrix_get(&submatrix.matrix, r+1, c+1) != NOT_SHADOW)
                        has_neighborhood = true;

                    blob_pixel_cnt++;

                    if (r <= blob_r_top)
                        blob_r_top = r;

                    if (r >= blob_r_bottom)
                        blob_r_bottom = r;

                    if (blob_c == INT_MIN)
                        blob_c = c;

                    if (gsl_matrix_get(&submatrix.matrix, r-1, c+1) == NOT_SHADOW && 
                        gsl_matrix_get(&submatrix.matrix,   r, c+1) == NOT_SHADOW &&
                        gsl_matrix_get(&submatrix.matrix, r+1, c+1) == NOT_SHADOW &&
                        has_neighborhood == false) {

examine:
                        if (blob_pixel_cnt > 1) {
                            blob_w = c - blob_c + 1;
                            blob_h = blob_r_bottom - blob_r_top + 1;

                            if (1 || (blob_pixel_cnt > max_blob_pixel_cnt && blob_w > max_blob_w)) {
                                max_blob_pixel_cnt = blob_pixel_cnt;
                                max_blob_r = peak_idx;
                                max_blob_c = sm_c + blob_c;
                                max_blob_w = blob_w;
                                max_blob_h = blob_h;

                                blob_pixel_density = max_blob_pixel_cnt / (float)(max_blob_w * max_blob_h);
                            }

                            //dbg("Max blob (%d, %d) with  (%d, %d) %d %.02lf", 
                            //        max_blob_r, max_blob_c, max_blob_w, max_blob_h,
                            //        max_blob_pixel_cnt, blob_pixel_density);

                            if (max_blob_h >= 1 &&
                                    ((peak_idx <= imgy->size1 / 3.0 && max_blob_w >= 10 && blob_pixel_density >= 0.2) ||
                                     (peak_idx > imgy->size1 / 3.0 && max_blob_w >= 20 && blob_pixel_density >= 0.55))) {

                                //dbg("Valid Max blob (%d, %d) with  (%d, %d) %d %.02lf", 
                                //        max_blob_r, max_blob_c, max_blob_w, max_blob_h,
                                //        max_blob_pixel_cnt, blob_pixel_density);

                                tblob.r = max_blob_r;
                                tblob.c = max_blob_c;
                                tblob.w = max_blob_w;
                                tblob.h = max_blob_h;
                                tblob.number = blob_cnt;
                                tblob.next = NULL;

                                if (FCW_BlobFindIdentical(bhead, &tblob) == false) {
                                    blob_cnt++;
                                    FCW_BlobAdd(bhead, &tblob);
                                } else {
                                    memset(&tblob, 0x0, sizeof(blob));
                                }
                            }
                        }

                        blob_pixel_cnt = 0;
                        blob_c = blob_w = blob_h = -1;
                        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
                        blob_c = INT_MIN;
                        max_blob_pixel_cnt = 0;

                        max_blob_r = 
                        max_blob_c = 
                        max_blob_w = 
                        max_blob_h = -1;
                        blob_pixel_density = 0;
                    }
                } else {
                    if (has_neighborhood == false && --stop_wall_cnt <= 1) {
                        goto examine;
                    }
                }
            }
        }
    }

    return true;
}

bool FCW_VehicleCandidateGenerate(
        const gsl_matrix* imgy, 
        const gsl_matrix* vedge_imgy, 
        const gsl_vector* horizontal_hist, 
        VehicleCandidates* vcs)
{
    uint32_t r, c, row, col;
    //uint32_t size;
    //uint32_t peak_count;
    //size_t peak_idx;
    //CCandidate* vc = NULL;

    //uint32_t vote_success, vote_fail;
    uint32_t cur_peak, cur_peak_idx, max_peak;//, max_peak_idx;
    //PEAK_GROUP pg; 
    //PEAK* ppeak = NULL;
    //vector<PEAK*> pg;
    //vector<PEAK*>::iterator pg_it;
    gsl_vector* temp_hh = NULL;

    if (!imgy || !horizontal_hist || !vedge_imgy || !vcs) {
        dbg();
        return false;
    }

    row = imgy->size1;
    col = imgy->size2;
    //memset(&pg, 0x0, sizeof(PEAK_GROUP));
    memset(vcs, 0x0, sizeof(VehicleCandidates));
    //for (CandidatesIT it = vcs.begin() ; it != vcs.end(); ++it) {
    //    if ((*it)) {
    //        delete (*it);
    //        (*it) = NULL;
    //    }
    //}
    //vcs.clear();

    // find possible blob underneath vehicle
    CheckOrReallocVector(&m_temp_hori_hist, horizontal_hist->size, true);
    //CheckOrReallocVector(&temp_hh, horizontal_hist->size, true);
    gsl_vector_memcpy(m_temp_hori_hist, horizontal_hist);

    max_peak = gsl_vector_max(m_temp_hori_hist);
    //max_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

    if (max_peak == 0)
        return false;

    dbg("\033[1;33m========== max peak %d frame %u ===========\033[m", max_peak, frame_count++);

    FCW_BlobClear(&m_blob_head);

#if 1
    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        if (cur_peak < (max_peak * 0.6))
            break;

        //printf("\n");
        //dbg("Search blobs for current peak %d at %d", cur_peak, cur_peak_idx);

        gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

        FCW_BlobGenerator(imgy, cur_peak_idx, &m_blob_head);
    }
    
    if (m_blob_head) {
        blob *curblob = NULL;

        //dbg("Before arrange");
        //curblob = m_blob_head;
        //while (curblob!= NULL) {
        //    dbg("[%d,%d] with [%d,%d]", curblob->r, curblob->c, curblob->w, curblob->h);
        //    curblob = curblob->next;
        //}

        FCW_BlobRearrange(&m_blob_head);

        //dbg("After arrange");
        //curblob = m_blob_head;
        //while (curblob!= NULL) {
        //    dbg("[%d,%d] with [%d,%d]", curblob->r, curblob->c, curblob->w, curblob->h);
        //    curblob = curblob->next;
        //}
        //printf("\n");

        blob *cur = m_blob_head;
        int left_idx, right_idx, bottom_idx;
        int vehicle_startr, vehicle_startc;
        int vehicle_width, vehicle_height;
        gsl_matrix_view imgy_submatrix;

        while (cur != NULL) {

            //dbg("Before scaleup: blob at (%d,%d) with (%d,%d)",
            //        cur->r,
            //        cur->c,
            //        cur->w,
            //        cur->h);

            //scale up searching region of blob with 1/4 time of width on the left & right respectively.
            bottom_idx      = cur->r;
            left_idx        = (cur->c - (cur->w >> 2));
            right_idx       = (cur->c + cur->w + (cur->w >> 2));

            left_idx        = (left_idx < 0 ? 0 : left_idx);
            right_idx       = (right_idx >= vedge_imgy->size2 ? vedge_imgy->size2 - 1 : right_idx);

            vehicle_width   = (right_idx - left_idx + 1);
            vehicle_height  = vehicle_width * 0.5;

            //dbg("Scale up left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);

            vehicle_startr  = (bottom_idx - vehicle_height < 0 ? 0 : bottom_idx - vehicle_height);
            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= imgy->size2) {
                right_idx       = vedge_imgy->size2 - 1;
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

            imgy_submatrix = gsl_matrix_submatrix((gsl_matrix*)vedge_imgy,
                                                   vehicle_startr,
                                                   vehicle_startc,
                                                   vehicle_height,
                                                   vehicle_width);

            //dbg("Before Edge: Vehicle at (%d,%d) with (%d,%d)",
            //        cur->r,
            //        cur->c,
            //        cur->w,
            //        cur->h);

            FCW_UpdateBlobByEdge(&imgy_submatrix.matrix, cur);

            //dbg("Before Valid: Vehicle at (%d,%d) with (%d,%d)",
            //        cur->r,
            //        cur->c,
            //        cur->w,
            //        cur->h);

            FCW_CheckBlobValid(imgy, vedge_imgy, cur);

            if (cur->valid) {

                vcs->vc[vcs->vc_count].m_valid  = true;
                vcs->vc[vcs->vc_count].m_dist   = FCW_GetObjDist(cur->w);
                vcs->vc[vcs->vc_count].m_id     = cur->number;

                vcs->vc[vcs->vc_count].m_r      = cur->r;
                vcs->vc[vcs->vc_count].m_c      = cur->c;
                vcs->vc[vcs->vc_count].m_w      = cur->w;
                vcs->vc[vcs->vc_count].m_h      = cur->h;

                vcs->vc[vcs->vc_count].m_st     = Disappear;

//                dbg("\033[1;33mVehicle %d at (%d,%d) with (%d,%d), dist %.02lfm\033[m\n\n",
//                        vcs->vc[vcs->vc_count].m_id,
//                        vcs->vc[vcs->vc_count].m_r,
//                        vcs->vc[vcs->vc_count].m_c,
//                        vcs->vc[vcs->vc_count].m_w,
//                        vcs->vc[vcs->vc_count].m_h,
//                        vcs->vc[vcs->vc_count].m_dist);

                vcs->vc_count++;
            }
            else {
                dbg("Fail Vehicle at (%d,%d) with (%d,%d)\n\n",
                        cur->r,
                        cur->c,
                        cur->w,
                        cur->h);

            }

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

            imgy_submatrix = gsl_matrix_submatrix((gsl_matrix*)vedge_imgy,
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
#endif

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

bool FCW_UpdateBlobByEdge(const gsl_matrix* imgy, blob*  blob)
{
    int left_idx, right_idx, bottom_idx;
    uint32_t r, c;
    uint32_t max_vedge_c;
    double magnitude;
    double vedge_strength, max_vedge_strength;
    gsl_matrix_view imgy_block;

    if (!imgy || !blob) {
        dbg();
        return false;
    }

    //dbg("==================");

    //dbg("Before (%d, %d) with (%d, %d)",
    //    blob->r,
    //    blob->c,
    //    blob->w,
    //    blob->h);

    bottom_idx  = blob->r + blob->h;
    left_idx    = blob->c;
    right_idx   = blob->c + blob->w - 1;

    // update left idx
    //dbg("Update left idx");
    max_vedge_c = 0;
    max_vedge_strength = 0;

    for (c=0 ; c<((imgy->size2 / 4) - 2) ; ++c) {
        imgy_block = gsl_matrix_submatrix((gsl_matrix*)imgy, 
                                                        0, 
                                                        c, 
                                                        imgy->size1, 
                                                        2);

        magnitude = 0;

        for (uint32_t rr=0 ; rr<imgy_block.matrix.size1 ; ++rr) {
            for (uint32_t cc=0 ; cc<imgy_block.matrix.size2 ; ++cc) {
                magnitude += gsl_matrix_get(&imgy_block.matrix, rr, cc);
            }
        }

        vedge_strength = (magnitude / (double)(imgy_block.matrix.size1*imgy_block.matrix.size2));

        if (vedge_strength > 30 && vedge_strength > max_vedge_strength) {
            max_vedge_strength = vedge_strength;
            max_vedge_c = c;
            //dbg("%.02lf at %d", max_vedge_strength, c);
        }
    }

    if (max_vedge_c == 0) {
        blob->valid = false;
        return false;
    }

    blob->c += max_vedge_c;
//    dbg("blob->c %d", blob->c);

    // upate right idx
    //dbg("Update right idx");
    max_vedge_c = 0;
    max_vedge_strength = 0;

    for (c=imgy->size2 - 2 ; c>((imgy->size2 * 3 / 4) - 2) ; --c) {
        imgy_block = gsl_matrix_submatrix((gsl_matrix*)imgy, 
                                                        0, 
                                                        c, 
                                                        imgy->size1, 
                                                        2);
        magnitude = 0;

        for (uint32_t rr=0 ; rr<imgy_block.matrix.size1 ; ++rr) {
            for (uint32_t cc=0 ; cc<imgy_block.matrix.size2 ; ++cc) {
                magnitude += gsl_matrix_get(&imgy_block.matrix, rr, cc);
            }
        }

        vedge_strength = (magnitude / (double)(imgy_block.matrix.size1*imgy_block.matrix.size2));

        if (vedge_strength > 30 && vedge_strength > max_vedge_strength) {
            max_vedge_strength = vedge_strength;
            max_vedge_c = c;
            //dbg("%.02lf at %d", max_vedge_strength, c);
        }
    }

    if (max_vedge_c == 0) {
        blob->valid = false;
        return false;
    }

    right_idx = left_idx +  max_vedge_c;

    blob->w = right_idx - blob->c + 1;
    blob->h = blob->w * 0.5;
    blob->r = bottom_idx - blob->h;
    blob->valid = true;

    //dbg("After (%d, %d) with (%d, %d)",
    //    blob->r,
    //    blob->c,
    //    blob->w,
    //    blob->h);

    return true;
}

bool FCW_CheckBlobByArea(const gsl_matrix* imgy, blob* cur)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float area_ratio;
    gsl_matrix_view submatrix;

    if (!imgy || !cur) {
        dbg();
        return false;
    }
    
    if (cur && cur->valid == false)
        return false;

    area = cur->w * cur->h;
    submatrix = gsl_matrix_submatrix((gsl_matrix*)imgy, cur->r, cur->c, cur->h, cur->w);

    for (r=0 ; r<submatrix.matrix.size1 ; ++r) {
        for (c=0 ; c<submatrix.matrix.size2 ; ++c) {
            if (gsl_matrix_get(&submatrix.matrix, r, c) != NOT_SHADOW) {
                pixel_cnt++;
            }
        }
    }

    area_ratio = pixel_cnt / (float)area;

    //dbg("Area: %d / %d = %.02f", pixel_cnt, area, area_ratio);

    if (/*area_ratio < 0.1 || */area_ratio > 0.8)
        cur->valid = false;

    return true;
}

bool FCW_CheckBlobByVerticalEdge(const gsl_matrix* vedge_imgy, blob* cur)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float percentage;
    gsl_matrix_view submatrix;

    if (!vedge_imgy || !cur) {
        dbg();
        return false;
    }

    if (cur && cur->valid == false)
        return false;

    area = cur->w * cur->h;
    submatrix = gsl_matrix_submatrix((gsl_matrix*)vedge_imgy, cur->r, cur->c, cur->h, cur->w);

    for (r=0 ; r<submatrix.matrix.size1 ; ++r) {
        for (c=0 ; c<submatrix.matrix.size2 ; ++c) {
            if (gsl_matrix_get(&submatrix.matrix, r, c)) {
                pixel_cnt++;
            }
        }
    }

    percentage = pixel_cnt / (float)area;

    //dbg("VerticalEdge: %d / %d = %.04f", pixel_cnt, area, percentage);

    if (percentage < 0.175)
        cur->valid = false;

    return true;
}

bool FCW_CheckBlobValid(const gsl_matrix* imgy, const gsl_matrix* vedge_imgy, blob* cur)
{
    if (!imgy || !cur) {
        dbg();
        return false;
    }
    
    if (cur && cur->valid == false)
        return false;

    //FCW_CheckBlobByArea(imgy, cur);

    FCW_CheckBlobByVerticalEdge(vedge_imgy, cur);

    return true;
}

#define HeatMapIncrease 10.0
#define HeatMapDecrease 10.0
#define HeatMapAppearThreshold   100 


bool FCW_UpdateVehicleHeatMap(gsl_matrix* heatmap, gsl_matrix_char* heatmap_id, VehicleCandidates* vcs)
{
    bool pixel_hit = false;
    uint32_t i, r, c;
    double val;

    if (!heatmap || !vcs) {
        dbg();
        return false;
    }

    CheckOrReallocMatrixChar(&heatmap_id, heatmap->size1, heatmap->size2, false); 

    // decrease/increase HeapMap
    for (r=0 ; r<heatmap->size1 ; ++r) {
        for (c=0 ; c<heatmap->size2 ; ++c) {

            pixel_hit = false;

            // this pixel belongs to certain vehicle candidate.
            for (i=0 ; i<vcs->vc_count ; ++i) {
                if (r >= vcs->vc[i].m_r && r< vcs->vc[i].m_r + vcs->vc[i].m_h && c >= vcs->vc[i].m_c && c < vcs->vc[i].m_c + vcs->vc[i].m_w) {
                    pixel_hit = true;
                    break;
                }
            }

            val = gsl_matrix_get(heatmap, r, c);

            if (pixel_hit) {
                val += HeatMapIncrease;
                if (val >= 255)
                    val = 255;
            } else {
                val -= HeatMapDecrease;
                if (val <= 0)
                    val = 0;
            }

            gsl_matrix_set(heatmap, r, c, val);

            if (val < HeatMapAppearThreshold /*&& gsl_matrix_char_get(heatmap_id, r, c) != -1*/) {
                //dbg("[%d,%d] %d disappear", r, c, gsl_matrix_char_get(heatmap_id, r, c));
                gsl_matrix_char_set(heatmap_id, r, c, -1);
            }
        }
    }

    return true;
}

typedef enum {
    DIR_RIGHTUP = 0,
    DIR_RIGHT,
    DIR_RIGHTDOWN,
    DIR_DOWN,
    DIR_LEFTDOWN,
    DIR_LEFT,
    DIR_LEFTUP,
    DIR_UP,
    DIR_TOTAL
}DIR;

bool FCW_GetContour(
    const gsl_matrix_char* heatmap_id,
    char id,
    const point* start,
    rect* rect
    )
{
    bool ret = false;
    bool find_pixel;
    uint32_t r, c;
    uint32_t left, right, top, bottom;
    uint32_t find_fail_cnt;
    uint32_t count;
    DIR nextdir;
    float aspect_ratio;
    point tp; // test point

    if (!heatmap_id || id < 0 || !start || !rect) {
        dbg();
        return ret;
    }

    nextdir = DIR_RIGHTUP;
    find_pixel = false;
    find_fail_cnt = 0;

    r = top = bottom = start->r;
    c = left = right = start->c;

    memset(&tp, 0, sizeof(tp));

    count = 0;
    while (1) {// Get contour of VC. (start point is equal to end point)
        while (1) {// Scan different direction to get neighbour.
            switch (nextdir) {
                case DIR_RIGHTUP:
                    if (gsl_matrix_char_get(heatmap_id, r-1, c+1) == id) {
                        --r;
                        ++c;
                        find_pixel = true;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                        right   = right < c ? c : right; 
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHT;
                    }
                    break;
                case DIR_RIGHT:
                    if (gsl_matrix_char_get(heatmap_id, r, c+1) == id) {
                        ++c;
                        find_pixel = true;
                        nextdir = DIR_RIGHTUP;

                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTDOWN;
                    }
                    break;
                case DIR_RIGHTDOWN:
                    if (gsl_matrix_char_get(heatmap_id, r+1, c+1) == id) {
                        ++r;
                        ++c;
                        find_pixel = true;
                        nextdir = DIR_RIGHTUP;

                        bottom  = bottom < r ? r : bottom;
                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_DOWN;
                    }
                    break;
                case DIR_DOWN:
                    if (gsl_matrix_char_get(heatmap_id, r+1, c) == id) {
                        ++r;
                        find_pixel = true;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTDOWN;
                    }
                    break;
                case DIR_LEFTDOWN:
                    if (gsl_matrix_char_get(heatmap_id, r+1, c-1) == id) {
                        ++r;
                        --c;
                        find_pixel = true;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                        left    = left > c ? c : left;
                    } else { 
                        find_fail_cnt++;
                        nextdir = DIR_LEFT;
                    }
                    break;
                case DIR_LEFT:
                    if (gsl_matrix_char_get(heatmap_id, r, c-1) == id) {
                        --c;
                        find_pixel = true;
                        nextdir = DIR_LEFTDOWN;

                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTUP;
                    }
                    break;
                case DIR_LEFTUP:
                    if (gsl_matrix_char_get(heatmap_id, r-1, c-1) == id) {
                        --r;
                        --c;
                        find_pixel = true;
                        nextdir = DIR_LEFTDOWN;

                        top     = top > r ? r : top;
                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_UP;
                    }
                    break;
                case DIR_UP:
                    if (gsl_matrix_char_get(heatmap_id, r-1, c) == id) {
                        --r;
                        find_pixel = true;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTUP;
                    }
                    break;
            } // switch

            if (find_pixel) {
                find_pixel = false;
                find_fail_cnt = 0;
                break;
            }

            if (find_fail_cnt >= DIR_TOTAL) {
                dbg();
                break;
            }
        }// Scan different direction to get neighbour.

        if (r == start->r && c == start->c) {
           rect->r  = top; 
           rect->c  = left;
           rect->w  = right - left + 1;
           rect->h  = bottom - top + 1;

           // aspect ratio checking (2 * 3/4 < ar < 2 * 5/4)
           aspect_ratio = (rect->w / (float)rect->h);

           //dbg("ratio of aspect is %.02f", rect->w / (float)rect->h);
           if (1.5 < aspect_ratio && aspect_ratio < 2.5)
               ret = true;

           break;
        } else {
            if (r != start->r && c != start->c) {
                if (tp.r == 0 && tp.c == 0) {
                    tp.r = r;
                    tp.c = c;
                } else {
                    if (r == tp.r && c == tp.c) {
                        //dbg("start(%d,%d), tp(%d,%d) <->(%d,%d)",
                        //        start->r,
                        //        start->c,
                        //        tp.r,
                        //        tp.c,
                        //        r,
                        //        c);
                        dbg("Get incorrect contour.");
                        break;
                    }
                }
            }
        }

        if (++count >= 1000) {
            dbg("id %d (%d,%d) <-> id %d (%d,%d), next dir %d",
                id,
                start->r,
                start->c,
                gsl_matrix_char_get(heatmap_id, r, c),
                r, 
                c,
                nextdir
               );
        }
    }// Get contour of VC. (start point is equal to end point)

    return ret;
}

bool FCW_UpdateVCStatus(
    gsl_matrix* heatmap, 
    gsl_matrix_char* heatmap_id, 
    Candidate** cand_head,
    VehicleCandidates* vcs,
    VehicleCandidates* vcs2
    )
{
    char vc_id, max_vc_id;
    unsigned char which_vc;
    uint32_t i;
    uint32_t r, c, rr, cc; 
    bool find_pixel;
    bool gen_new_cand;
    double dist;
    Candidate *cur_cand, *nearest_cand, *new_cand;
    point midpoint_newcand, midpoint_existedcand;
    point contour_sp;
    rect contour_rect;
    gsl_matrix_view heatmap_sm;
    gsl_matrix_char_view heatmapid_sm;


    if (!heatmap || !heatmap_id || !vcs) {
        dbg();
        return false;
    }

    // Reset each existed candidate as need to update.
    cur_cand = *cand_head;
    while (cur_cand) {
        dbg("cur_cand[%d] at (%d, %d) with (%d,%d)", 
                cur_cand->m_id,
                cur_cand->m_r,
                cur_cand->m_c,
                cur_cand->m_w,
                cur_cand->m_h);
        cur_cand->m_updated = false;
        cur_cand = cur_cand->m_next;
    }

    //dbg("vc count %d", vcs->vc_count);
    for (i=0 ; i<vcs->vc_count ; ++i) {
        if (vcs->vc[i].m_valid) {
            dbg("New vc[%d] at (%d,%d) with (%d,%d)",
                    i,
                    vcs->vc[i].m_r,
                    vcs->vc[i].m_c,
                    vcs->vc[i].m_w,
                    vcs->vc[i].m_h);
        }
    }

    //Assign an ID to each pixel which exceeds the threshold and does not have an ID yet.
    for (r=0 ; r<heatmap->size1 ; ++r) {
        for (c=0 ; c<heatmap->size2 ; ++c) {
            if (gsl_matrix_get(heatmap, r, c) < HeatMapAppearThreshold)
                continue;

            if (gsl_matrix_char_get(heatmap_id, r, c) != -1)
                continue;

            // which new candidate it belongs to.
            for (i=0 ; i<vcs->vc_count ; ++i) {
                if (r >= vcs->vc[i].m_r && r < vcs->vc[i].m_r + vcs->vc[i].m_h && 
                    c >= vcs->vc[i].m_c && c < vcs->vc[i].m_c + vcs->vc[i].m_w) {
                    which_vc = i;
                    break;
                }
            }

            if (vcs->vc[which_vc].m_valid == false)
                continue;

            dbg("New point at (%d,%d) belongs to %d", r, c, which_vc);
            gen_new_cand = false;

            // Find the nearest existed candidate.
            nearest_cand = NULL;
#if 1
            midpoint_newcand.r = vcs->vc[which_vc].m_r + vcs->vc[which_vc].m_h/2;
            midpoint_newcand.c = vcs->vc[which_vc].m_c + vcs->vc[which_vc].m_w/2;

            cur_cand = *cand_head;

            while (cur_cand) {
                if (midpoint_newcand.r > cur_cand->m_r && midpoint_newcand.r < cur_cand->m_r + cur_cand->m_h &&
                    midpoint_newcand.c > cur_cand->m_c && midpoint_newcand.c < cur_cand->m_c + cur_cand->m_w) {
                    nearest_cand = cur_cand;
                    break;
                }

                cur_cand = cur_cand->m_next;
            }
#else
            cur_cand = *cand_head;

            while (cur_cand) {
                midpoint_newcand.r = vcs->vc[which_vc].m_r + vcs->vc[which_vc].m_h/2;
                midpoint_newcand.c = vcs->vc[which_vc].m_c + vcs->vc[which_vc].m_w/2;

                midpoint_existedcand.r = cur_cand->m_r + cur_cand->m_h/2;
                midpoint_existedcand.c = cur_cand->m_c + cur_cand->m_w/2;

                dist = sqrt(pow(midpoint_existedcand.r - midpoint_newcand.r, 2.0) + pow(midpoint_existedcand.c - midpoint_newcand.c, 2.0));
                dbg("dist %lf between cur_cand[%d] and which_vc[%d]", dist, cur_cand->m_id, which_vc);
                //if (dist <= cur_cand->m_w/2/* || dist <= cur_cand->m_h/2*//* || vcs->vc[which_vc].m_w/2 || vcs->vc[which_vc].m_h/2*/) {
                if (midpoint_newcand.r > cur_cand->m_r && midpoint_newcand.r < cur_cand->m_r + cur_cand->m_h &&
                       midpoint_newcand.c > cur_cand->m_c && midpoint_newcand.c < cur_cand->m_c + cur_cand->m_w
                        ) {
                    nearest_cand = cur_cand;
                    break;
                }

                cur_cand = cur_cand->m_next;
            }
#endif

            if (nearest_cand) {
                vc_id = nearest_cand->m_id;
                dbg("Find nearest candidate[%d]", vc_id);
                //dbg("Find nearest candidate %d(%d,%d,%d,%d), (%d,%d), id %d, which_vc %d", 
                //        vc_id, 
                //        nearest_cand->m_r,
                //        nearest_cand->m_c,
                //        nearest_cand->m_w,
                //        nearest_cand->m_h,
                //        r, 
                //        c, 
                //        gsl_matrix_char_get(heatmap_id, r, c), 
                //        which_vc);

                //dbg("new vc %d valid %d at (%d,%d) with (%d,%d)",
                //        which_vc,
                //        vcs->vc[which_vc].m_valid,
                //        vcs->vc[which_vc].m_r,
                //        vcs->vc[which_vc].m_c,
                //        vcs->vc[which_vc].m_w,
                //        vcs->vc[which_vc].m_h);


                heatmap_sm = gsl_matrix_submatrix(heatmap, 
                        vcs->vc[which_vc].m_r,
                        vcs->vc[which_vc].m_c,
                        vcs->vc[which_vc].m_h,
                        vcs->vc[which_vc].m_w);

                heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id, 
                        vcs->vc[which_vc].m_r,
                        vcs->vc[which_vc].m_c,
                        vcs->vc[which_vc].m_h,
                        vcs->vc[which_vc].m_w);

                for (rr=0 ; rr<heatmap_sm.matrix.size1 ; ++rr) {
                    for (cc=0 ; cc<heatmap_sm.matrix.size2 ; ++cc) {
                        if (gsl_matrix_get(&heatmap_sm.matrix, rr, cc) < HeatMapAppearThreshold)
                            continue;

                        //if (gsl_matrix_char_get(&heatmapid_sm.matrix, rr, cc) != -1)
                        //    continue;

                        gsl_matrix_char_set(&heatmapid_sm.matrix, rr, cc, vc_id);
                    }
                }
            } else {
                vc_id = 0;
                gen_new_cand = true;

                // New candidate needs a new ID.
                if (*cand_head) {
                    max_vc_id = 0;
                    cur_cand = *cand_head;
                    while (cur_cand) {
                        if (cur_cand->m_id > max_vc_id)
                            max_vc_id = cur_cand->m_id;

                        cur_cand = cur_cand->m_next;
                    }

                    vc_id = max_vc_id + 1;
                }

                dbg("Create a new candidate at (%d,%d) id %d, which_vc %d", r, c, vc_id, which_vc);
                //dbg("vc %d at (%d,%d) with (%d,%d)", which_vc,
                //        vcs->vc[which_vc].m_r,
                //        vcs->vc[which_vc].m_c,
                //        vcs->vc[which_vc].m_w,
                //        vcs->vc[which_vc].m_h);

                heatmap_sm = gsl_matrix_submatrix(heatmap, 
                        vcs->vc[which_vc].m_r,
                        vcs->vc[which_vc].m_c,
                        vcs->vc[which_vc].m_h,
                        vcs->vc[which_vc].m_w);

                heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id, 
                        vcs->vc[which_vc].m_r,
                        vcs->vc[which_vc].m_c,
                        vcs->vc[which_vc].m_h,
                        vcs->vc[which_vc].m_w);

                for (rr=0 ; rr<heatmap_sm.matrix.size1 ; ++rr) {
                    for (cc=0 ; cc<heatmap_sm.matrix.size2 ; ++cc) {
                        if (gsl_matrix_get(&heatmap_sm.matrix, rr, cc) < HeatMapAppearThreshold)
                            continue;

                        gsl_matrix_char_set(&heatmapid_sm.matrix, rr, cc, vc_id);
                    }
                }
            }

            // Generate/Update existed candidate.
            if (gen_new_cand == false && nearest_cand) {
                //r > nearest_cand->m_r && r < nearest_cand->m_r + nearest_cand->m_h &&
                //c > nearest_cand->m_c && c < nearest_cand->m_c + nearest_cand->m_w) {
                // if start point is inside nearest vc.
               // dbg("(%d,%d) is inside vc[%d]:(%d,%d) (%d,%d)",
               //         r,
               //         c,
               //         nearest_cand->m_id,
               //         nearest_cand->m_r,
               //         nearest_cand->m_c,
               //         nearest_cand->m_w,
               //         nearest_cand->m_h);
               // for (rr=nearest_cand->m_r ; rr<nearest_cand->m_r + nearest_cand->m_h ; ++rr) {
               //     for (cc=nearest_cand->m_c ; cc<nearest_cand->m_c + nearest_cand->m_w ; ++cc) {
               //         dbg("(%d,%d): %lf, %d",
               //                 rr,
               //                 cc,
               //                 gsl_matrix_get(heatmap, rr, cc),
               //                 gsl_matrix_char_get(heatmap_id, rr, cc)
               //            );
               //     }
               // }

                if (r > nearest_cand->m_r && c > nearest_cand->m_c) {
                    find_pixel = false;
                    for (rr=nearest_cand->m_r ; rr<nearest_cand->m_r + nearest_cand->m_h ; ++rr) {
                        for (cc=nearest_cand->m_c ; cc<nearest_cand->m_c + nearest_cand->m_w ; ++cc) {
                            if (gsl_matrix_char_get(heatmap_id, rr, cc) == nearest_cand->m_id && gsl_matrix_get(heatmap, rr, cc) >= HeatMapAppearThreshold) {
                                find_pixel = true;
                                break;
                            }
                        }

                        if (find_pixel)
                            break;
                    }

                    //dbg("rr %d cc %d", rr, cc);
                    contour_sp.r = rr;
                    contour_sp.c = cc;
                    dbg("new top_left is inside cur top_left, using (%d,%d) instead of (%d,%d)", rr, cc, r, c);
                } else {
                    contour_sp.r = r;
                    contour_sp.c = c;
                    dbg("new top_left is outside cur top_left, using (%d,%d)", r, c);
                }
            } else {
                contour_sp.r = r;
                contour_sp.c = c;
            }

            if (FCW_GetContour(heatmap_id, vc_id, &contour_sp, &contour_rect) == true) {
                if (gen_new_cand) {
                    new_cand = (Candidate*)malloc(sizeof(Candidate));
                    if (!new_cand) {
                        dbg();
                        return false;
                    }

                    memset(new_cand, 0x0, sizeof(Candidate));
                    new_cand->m_next = NULL;

                    new_cand->m_updated     = true;
                    new_cand->m_valid       = true;
                    new_cand->m_id          = vc_id;
                    new_cand->m_r           = contour_rect.r;
                    new_cand->m_c           = contour_rect.c;
                    new_cand->m_w           = contour_rect.w;
                    new_cand->m_h           = contour_rect.h;
                    new_cand->m_dist        = FCW_GetObjDist(new_cand->m_w);

                    // Add new candidate to candidate list.
                    if (!*cand_head) {
                        *cand_head = cur_cand = new_cand;
                    } else {
                        cur_cand = *cand_head;
                        while (cur_cand) {
                            if (!cur_cand->m_next) {
                                cur_cand->m_next = new_cand;
                                new_cand->m_next = NULL;
                                cur_cand = new_cand;
                                break;
                            }

                            cur_cand = cur_cand->m_next;
                        }
                    }
                } else {
                    if (nearest_cand) {
                        nearest_cand->m_updated     = true;
                        nearest_cand->m_valid       = true;
                        nearest_cand->m_id          = vc_id;
                        nearest_cand->m_r           = contour_rect.r;
                        nearest_cand->m_c           = contour_rect.c;
                        nearest_cand->m_w           = contour_rect.w;
                        nearest_cand->m_h           = contour_rect.h;
                        nearest_cand->m_dist        = FCW_GetObjDist(nearest_cand->m_w);

                        cur_cand                    = nearest_cand;
                    } else 
                        dbg();
                }

                if (cur_cand) {
                    dbg("vc %d is at (%d,%d) with (%d,%d)",
                            cur_cand->m_id,
                            cur_cand->m_r,
                            cur_cand->m_c,
                            cur_cand->m_w,
                            cur_cand->m_h
                       );

                    heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id,
                            cur_cand->m_r,
                            cur_cand->m_c,
                            cur_cand->m_h,
                            cur_cand->m_w);

                    gsl_matrix_char_set_all(&heatmapid_sm.matrix, cur_cand->m_id);
                }
            } else {
                if (gen_new_cand) {
                    dbg("Fail to generate new vc.");
                }
            }
        }
    }
     
    // Update existed candidate that has not been updated yet.
    cur_cand = *cand_head;
    while (cur_cand) {
        if (cur_cand->m_updated == false) {
            //dbg("vc %d is at (%d,%d) with (%d,%d) but has not updated.",
            //        cur_cand->m_id,
            //        cur_cand->m_r,
            //        cur_cand->m_c,
            //        cur_cand->m_w,
            //        cur_cand->m_h);

            // Find a pixel inside existed vc which still has an id.
            find_pixel = false;
            for (r=cur_cand->m_r ; r<cur_cand->m_r+cur_cand->m_h ; ++r) {
                for (c=cur_cand->m_c ; c<cur_cand->m_c+cur_cand->m_w ; ++c) {
                    if (gsl_matrix_char_get(heatmap_id, r, c) == cur_cand->m_id) {
                        find_pixel      = true;
                        contour_sp.r    = r;
                        contour_sp.c    = c;
                        vc_id           = cur_cand->m_id; 
                    }
                }

                if (find_pixel)
                    break;
            }

            if (find_pixel == true && FCW_GetContour(heatmap_id, vc_id, &contour_sp, &contour_rect) == true) {
                cur_cand->m_updated     = true;
                cur_cand->m_valid       = true;
                cur_cand->m_id          = vc_id;
                cur_cand->m_r           = contour_rect.r;
                cur_cand->m_c           = contour_rect.c;
                cur_cand->m_w           = contour_rect.w;
                cur_cand->m_h           = contour_rect.h;
                cur_cand->m_dist        = FCW_GetObjDist(cur_cand->m_w);
            }

            if (cur_cand->m_updated == true) {
                dbg("vc %d (%d,%d) with (%d,%d) is self-updated.",
                        cur_cand->m_id,
                        cur_cand->m_r,
                        cur_cand->m_c,
                        cur_cand->m_w,
                        cur_cand->m_h);
                cur_cand = cur_cand->m_next;
            } else {
                dbg("vc %d is removed.", cur_cand->m_id);
                // This existed candidate does not been updated anymore. Remove it from the list.
                if (cur_cand == *cand_head) {
                    *cand_head = cur_cand->m_next;
                    free (cur_cand);
                    cur_cand = *cand_head;
                } else {
                    Candidate *temp = NULL;

                    temp  = *cand_head;
                    while (temp) {
                        if (temp->m_next == cur_cand) {
                            temp->m_next = cur_cand->m_next;
                            free (cur_cand);
                            cur_cand = temp->m_next;
                            break;
                        }

                        temp = temp->m_next;
                    }
                }
            }
        } else {
            cur_cand = cur_cand->m_next;
        }
    }
    
    memset(vcs2, 0x0, sizeof(VehicleCandidates));
    
    cur_cand = *cand_head;

    while (cur_cand) {
        vcs2->vc[vcs2->vc_count].m_updated    = true;
        vcs2->vc[vcs2->vc_count].m_valid      = true;
        vcs2->vc[vcs2->vc_count].m_id         = cur_cand->m_id;
        vcs2->vc[vcs2->vc_count].m_r          = cur_cand->m_r;
        vcs2->vc[vcs2->vc_count].m_c          = cur_cand->m_c;
        vcs2->vc[vcs2->vc_count].m_w          = cur_cand->m_w;
        vcs2->vc[vcs2->vc_count].m_h          = cur_cand->m_h;
        vcs2->vc[vcs2->vc_count].m_dist       = cur_cand->m_dist;
        vcs2->vc[vcs2->vc_count].m_next       = NULL;
        vcs2->vc_count++;

        cur_cand = cur_cand->m_next;
    }
    
    return true;
}

bool FCW_EdgeDetection(gsl_matrix* src, gsl_matrix* dst, gsl_matrix_ushort* gradient, gsl_matrix_char* dir, int direction)
{
    if (!src || !dst || !gradient || !dir) {
        dbg();
        return false;
    }

    FCW_CalGradient(gradient, dir, src, direction, 0, 0, 0, 0);
    gsl_matrix_set_zero(dst);
    FCW_NonMaximum_Suppression(dst, dir, gradient);

    return true;
}

double FCW_GetObjDist(double pixel)
{
    // d = W * f / w 
    return (VehicleWidth * (EFL / 2.0)) / (pixel * PixelSize);
}
