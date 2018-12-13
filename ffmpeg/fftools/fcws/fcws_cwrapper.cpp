#include "fcws_cwrapper.h"
#include "fcws.hpp"
#include "../ldws/ldws_cwrapper.h"

//-------------------------------
static CFCWS* fcws_obj = NULL;
//-------------------------------
  
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
gsl_matrix*         m_imgu = NULL;
gsl_matrix*         m_imgv = NULL;
gsl_matrix*         m_vedge_imgy = NULL;
gsl_matrix*         m_heatmap = NULL;
gsl_matrix*         m_temp_imgy = NULL;
gsl_matrix*         m_intimg = NULL;    // integral image
gsl_matrix*         m_shadow = NULL;   // shadow by integral image

gsl_matrix_ushort*  m_gradient = NULL;
gsl_matrix_char*    m_direction = NULL;

gsl_matrix_view     m_gk;
double              m_gk_weight = 0;

gsl_matrix_view     m_dx;
gsl_matrix_view     m_dy;
gsl_vector_view     m_dx1d;
gsl_vector_view     m_dy1d;

gsl_vector*         m_temp_hori_hist= NULL;

blob*               m_blob_head = NULL;
gsl_matrix_char*    m_heatmap_id = NULL;
Candidate*          m_vc_tracker = NULL;

gsl_matrix*         m_hsv_imgy = NULL;
gsl_matrix*         m_hsv_imgu = NULL;
gsl_matrix*         m_hsv_imgv = NULL;
gsl_matrix*         m_hsv[3] = {NULL};

gsl_matrix*         m_rgb_imgy = NULL;
gsl_matrix*         m_rgb_imgu = NULL;
gsl_matrix*         m_rgb_imgv = NULL;
gsl_matrix*         m_rgb[3] = {NULL};

gsl_matrix*         m_lab_imgy = NULL;
gsl_matrix*         m_lab_imgu = NULL;
gsl_matrix*         m_lab_imgv = NULL;
gsl_matrix*         m_lab[3] = {NULL};

uint64_t frame_count = 0;

#define VehicleWidth 1650   //mm
#define EFL          2.7    //mm
#define PixelSize    2.8    //um = 10^-3 mm

extern CLDWS* LDW_GetObj(void);

inline uint8_t clip_uint8(int a)
{
    if (a&(~0xFF)) return (~a)>>31;
    else           return a;
}

inline double max2(double a, double b)
{
    return a > b ? a : b;
}

inline double max3(double a, double b, double c)
{
    return max2(a, b) > c ? max2(a, b) : c;
}

inline double min2(double a, double b)
{
    return a > b ? b : a;
}

inline double min3(double a, double b, double c)
{
    return min2(a, b) > c ? c : min2(a, b);
}

BOOL FCW_Init()
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

    fcws_obj = new CFCWS();
    if (fcws_obj)
        fcws_obj->Init();

    return TRUE;
}

BOOL FCW_DeInit()
{
    FreeVector(&m_temp_hori_hist);

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_imgu);
    FreeMatrix(&m_imgv);
    FreeMatrix(&m_shadow);
    FreeMatrix(&m_vedge_imgy);
    FreeMatrix(&m_temp_imgy);
    FreeMatrix(&m_intimg);
    FreeMatrix(&m_hsv[0]);
    FreeMatrix(&m_hsv[1]);
    FreeMatrix(&m_hsv[2]);
    FreeMatrix(&m_hsv_imgy);
    FreeMatrix(&m_hsv_imgu);
    FreeMatrix(&m_hsv_imgv);
    FreeMatrix(&m_rgb[0]);
    FreeMatrix(&m_rgb[1]);
    FreeMatrix(&m_rgb[2]);
    FreeMatrix(&m_rgb_imgy);
    FreeMatrix(&m_rgb_imgu);
    FreeMatrix(&m_rgb_imgv);
    FreeMatrix(&m_lab[0]);
    FreeMatrix(&m_lab[1]);
    FreeMatrix(&m_lab[2]);
    FreeMatrix(&m_lab_imgy);
    FreeMatrix(&m_lab_imgu);
    FreeMatrix(&m_lab_imgv);
    FreeMatrixUshort(&m_gradient);
    FreeMatrixChar(&m_direction);
    FreeMatrixChar(&m_heatmap_id);

    FCW_BlobClear(&m_blob_head);

    if (fcws_obj) {
        delete fcws_obj;
        fcws_obj = NULL;
    }

    return TRUE;
}
 
BOOL FCW_PixelInROI(uint32_t r, uint32_t c, const roi_t* roi)
{
    float slopl, slopr;

    if (!roi) {
        fcwsdbg();
        return FALSE;
    }

    if (r < roi->point[ROI_LEFTTOP].r || r > roi->point[ROI_LEFTBOTTOM].r)
        return FALSE;

    if (c < roi->point[ROI_LEFTBOTTOM].c || c > roi->point[ROI_RIGHTBOTTOM].c)
        return FALSE;
    
    slopl = (roi->point[ROI_LEFTTOP].r - roi->point[ROI_LEFTBOTTOM].r) / (float)(roi->point[ROI_LEFTTOP].c - roi->point[ROI_LEFTBOTTOM].c);
    slopr = (roi->point[ROI_RIGHTTOP].r - roi->point[ROI_RIGHTBOTTOM].r) / (float)(roi->point[ROI_RIGHTTOP].c - roi->point[ROI_RIGHTBOTTOM].c);

    if (c >= roi->point[ROI_LEFTBOTTOM].c && c <= roi->point[ROI_LEFTTOP].c && 
        (((int)c - roi->point[ROI_LEFTBOTTOM].c == 0) || (((int)r - roi->point[ROI_LEFTBOTTOM].r) / (float)((int)c - roi->point[ROI_LEFTBOTTOM].c)) < slopl))
        return FALSE;

    if (c >= roi->point[ROI_RIGHTTOP].c && c <= roi->point[ROI_RIGHTBOTTOM].c && 
        (((int)c - roi->point[ROI_RIGHTBOTTOM].c == 0) || (((int)r - roi->point[ROI_RIGHTBOTTOM].r) / (float)((int)c - roi->point[ROI_RIGHTBOTTOM].c)) > slopr))
        return FALSE;

    return TRUE;
}

BOOL FCW_Thresholding(
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
        fcwsdbg();
        return FALSE;
    }

    if (src->size1 != dst->size1 && src->size2 != dst->size2) {
        fcwsdbg();
        return FALSE;
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

    return TRUE;
}

BOOL FCW_ThresholdingByIntegralImage(
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
        fcwsdbg();
        return FALSE;
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

    return TRUE;
}
 
BOOL FCW_DoDetection(
        uint8_t* img, 
        int linesize, 
        uint8_t* imgu, 
        int linesize_u, 
        uint8_t* imgv, 
        int linesize_v, 
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
        uint8_t* heatmap,
        uint8_t* hsv_imgy,
        uint8_t* hsv_imgu,
        uint8_t* hsv_imgv,
        uint8_t* rgb_imgy,
        uint8_t* rgb_imgu,
        uint8_t* rgb_imgv,
        uint8_t* lab_imgy,
        uint8_t* lab_imgu,
        uint8_t* lab_imgv,
        const roi_t* roi
        )
{
    uint32_t i, r, c;
    Candidate* cur_vc = NULL;

    if (!img || !vertical_hist || !hori_hist || !grayscale_hist || !vedge || !shadow || !heatmap) {
        fcwsdbg();
        return FALSE;
    }

#if 0
    CheckOrReallocMatrix(&m_imgy, h, w, TRUE);
    CheckOrReallocMatrix(&m_imgu, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_imgv, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_shadow, h, w, TRUE);
    CheckOrReallocMatrix(&m_vedge_imgy, h, w, TRUE);
    CheckOrReallocMatrix(&m_temp_imgy, h, w, TRUE);
    CheckOrReallocMatrix(&m_heatmap, h, w, FALSE);
    CheckOrReallocMatrix(&m_intimg, h, w, TRUE);
    CheckOrReallocMatrixChar(&m_heatmap_id, h, w, FALSE);
    CheckOrReallocMatrixUshort(&m_gradient, h, w, TRUE);
    CheckOrReallocMatrixChar(&m_direction, h, w, TRUE);

    // HSV-based colour segmentation.
    CheckOrReallocMatrix(&m_hsv[0], h, w, TRUE); // H
    CheckOrReallocMatrix(&m_hsv[1], h, w, TRUE); // S
    CheckOrReallocMatrix(&m_hsv[2], h, w, TRUE); // V
    CheckOrReallocMatrix(&m_hsv_imgy, h, w, TRUE); // hsv image y
    CheckOrReallocMatrix(&m_hsv_imgu, h/2, w/2, TRUE); // hsv image u
    CheckOrReallocMatrix(&m_hsv_imgv, h/2, w/2, TRUE); // hsv image v

    // RGB-based colour segmentation.
    CheckOrReallocMatrix(&m_rgb[0], h, w, TRUE); // R
    CheckOrReallocMatrix(&m_rgb[1], h, w, TRUE); // G
    CheckOrReallocMatrix(&m_rgb[2], h, w, TRUE); // B
    CheckOrReallocMatrix(&m_rgb_imgy, h, w, TRUE); // rgb image y
    CheckOrReallocMatrix(&m_rgb_imgu, h/2, w/2, TRUE); // rgb image u
    CheckOrReallocMatrix(&m_rgb_imgv, h/2, w/2, TRUE); // rgb image v

    // L*a*b*-based colour segmentation.
    CheckOrReallocMatrix(&m_lab[0], h, w, TRUE); // L*
    CheckOrReallocMatrix(&m_lab[1], h, w, TRUE); // a*
    CheckOrReallocMatrix(&m_lab[2], h, w, TRUE); // b*
    CheckOrReallocMatrix(&m_lab_imgy, h, w, TRUE); // L*a*b* image y
    CheckOrReallocMatrix(&m_lab_imgu, h/2, w/2, TRUE); // L*a*b* image u
    CheckOrReallocMatrix(&m_lab_imgv, h/2, w/2, TRUE); // L*a*b* image v
    
    // Copy image array to image matrix
    for (r=0 ; r<m_imgy->size1 ; r++) {
        for (c=0 ; c<m_imgy->size2 ; c++) {
            gsl_matrix_set(m_imgy, r, c, img[r * linesize + c]); 
            // Generate ROI image based roi
#if 1
            gsl_matrix_set(m_temp_imgy, r, c, img[r * linesize + c]); 
#else
            if (FCW_PixelInROI(r, c, roi) == TRUE)
                gsl_matrix_set(m_temp_imgy, r, c, img[r * linesize + c]);
            else
                gsl_matrix_set(m_temp_imgy, r, c, NOT_SHADOW);
#endif
        }
    }

    for (r=0 ; r<m_imgu->size1 ; r++) {
        for (c=0 ; c<m_imgu->size2 ; c++) {
            gsl_matrix_set(m_imgu, r, c, imgu[r * linesize_u + c]);
            gsl_matrix_set(m_imgv, r, c, imgv[r * linesize_u + c]);
        }
    }

    // Get thresholding image
    FCW_ThresholdingByIntegralImage(m_imgy, m_intimg, m_shadow, 50, 0.7);

    if (LDW_ApplyDynamicROI(m_temp_imgy) == FALSE) {
        for (r=0 ; r<m_temp_imgy->size1 ; r++) {
            for(c=0 ; c<m_temp_imgy->size2 ; c++) {
                if (FCW_PixelInROI(r, c, roi) == FALSE)
                    gsl_matrix_set(m_temp_imgy, r, c, NOT_SHADOW);
            }
        }
    }

    if (LDW_ApplyDynamicROI(m_shadow) == FALSE) {
        for (r=0 ; r<m_shadow->size1 ; r++) {
            for(c=0 ; c<m_shadow->size2 ; c++) {
                if (FCW_PixelInROI(r, c, roi) == FALSE)
                    gsl_matrix_set(m_shadow, r, c, NOT_SHADOW);
            }
        }
    }

    // Get horizontal histogram
    FCW_CalHorizontalHist(m_shadow, hori_hist);

    // Get vertical edge.
    FCW_EdgeDetection(m_imgy, m_vedge_imgy, m_gradient, m_direction, 1);

    // Get VC
    FCW_VehicleCandidateGenerate(
            m_imgy,
            m_intimg,
            m_shadow,
            m_vedge_imgy,
            hori_hist,
            vcs);

    // Check symmetry property.
    //FCW_CheckSymmProperty(m_imgy, vcs, 0.1, 0.5);

    // Update HeatMap
    FCW_UpdateVehicleHeatMap(m_heatmap, m_heatmap_id, vcs);

    FCW_UpdateVCStatus(m_heatmap, m_heatmap_id, &m_vc_tracker, vcs);

#if 0
    // Convert IYUV to HSV
    FCW_ConvertIYUV2HSV(FALSE, m_imgy, m_imgu, m_imgv, m_vc_tracker, m_hsv);

    FCW_SegmentTaillightByHSV(
            m_imgy, 
            m_imgu, 
            m_imgv, 
            m_hsv_imgy, 
            m_hsv_imgu, 
            m_hsv_imgv, 
            (const gsl_matrix**)m_hsv, 
            m_vc_tracker, 
            15.0, 
            345.0, 
            0.2, 
            0.25);

    FCW_ConvertIYUV2RGB(FALSE, m_imgy, m_imgu, m_imgv, m_vc_tracker, m_rgb);

    FCW_SegmentTaillightByRGB(
            m_imgy,
            m_imgu,
            m_imgv,
            m_rgb_imgy,
            m_rgb_imgu,
            m_rgb_imgv,
            (const gsl_matrix**)m_rgb,
            m_vc_tracker,
            180,
            100);

    FCW_ConvertIYUV2Lab(FALSE, m_imgy, m_imgu, m_imgv, m_vc_tracker, m_lab);

    FCW_SegmentTaillightByLab(
            m_imgy,
            m_imgu,
            m_imgv,
            m_lab_imgy,
            m_lab_imgu,
            m_lab_imgv,
            (const gsl_matrix**)m_lab,
            m_vc_tracker);

#endif

    memset(vcs2, 0x0, sizeof(VehicleCandidates));
    
    cur_vc = m_vc_tracker;

    while (cur_vc) {
        vcs2->vc[vcs2->vc_count].m_updated    = cur_vc->m_updated;
        vcs2->vc[vcs2->vc_count].m_valid      = cur_vc->m_valid;
        vcs2->vc[vcs2->vc_count].m_id         = cur_vc->m_id;
        vcs2->vc[vcs2->vc_count].m_r          = cur_vc->m_r;
        vcs2->vc[vcs2->vc_count].m_c          = cur_vc->m_c;
        vcs2->vc[vcs2->vc_count].m_w          = cur_vc->m_w;
        vcs2->vc[vcs2->vc_count].m_h          = cur_vc->m_h;
        vcs2->vc[vcs2->vc_count].m_dist       = cur_vc->m_dist;
        vcs2->vc[vcs2->vc_count].m_next       = NULL;
        vcs2->vc_count++;

        if (cur_vc->m_valid) {
            fcwsdbg("\033[1;33mVehicle %d at (%d,%d) with (%d,%d), dist %.02lfm\033[m\n\n",
                    cur_vc->m_id,
                    cur_vc->m_r,
                    cur_vc->m_c,
                    cur_vc->m_w,
                    cur_vc->m_h,
                    cur_vc->m_dist);
        }

        cur_vc = cur_vc->m_next;
    }

    // Copy matrix to data buffer for display.
    for (r=0 ; r<m_imgy->size1 ; r++) {
        for (c=0 ; c<m_imgy->size2 ; c++) {
            //img     [r * linesize + c] = (uint8_t)gsl_matrix_get(m_imgy, r,c); 
            //shadow  [r * linesize + c] = (uint8_t)gsl_matrix_get(m_shadow, r,c); 
            //roi_img [r * linesize + c] = (uint8_t)gsl_matrix_get(m_temp_imgy, r,c); 
            //vedge   [r * linesize + c] = (uint8_t)gsl_matrix_get(m_vedge_imgy, r,c); 
            //heatmap [r * linesize + c] = (uint8_t)gsl_matrix_get(m_heatmap, r,c); 
            hsv_imgy[r * linesize + c] = (uint8_t)gsl_matrix_get(m_hsv_imgy, r,c); 
            rgb_imgy[r * linesize + c] = (uint8_t)gsl_matrix_get(m_rgb_imgy, r,c); 
            lab_imgy[r * linesize + c] = (uint8_t)gsl_matrix_get(m_lab_imgy, r,c); 
        }
    }

    for (r=0 ; r<m_imgu->size1 ; r++) {
        for (c=0 ; c<m_imgu->size2 ; c++) {
            hsv_imgu[r * linesize_u + c] = gsl_matrix_get(m_hsv_imgu, r, c);
            hsv_imgv[r * linesize_v + c] = gsl_matrix_get(m_hsv_imgv, r, c);
            rgb_imgu[r * linesize_u + c] = gsl_matrix_get(m_rgb_imgu, r, c);
            rgb_imgv[r * linesize_v + c] = gsl_matrix_get(m_rgb_imgv, r, c);
            lab_imgu[r * linesize_u + c] = gsl_matrix_get(m_lab_imgu, r, c);
            lab_imgv[r * linesize_v + c] = gsl_matrix_get(m_lab_imgv, r, c);
        }
    }
#endif

    // --------------------------------------------------
    if (fcws_obj) {
        fcwsdbg(LIGHT_RED "=============New Frame=============" NONE);

        fcws_obj->DoDetection(img,
                              w,
                              h,
                              linesize,
                              LDW_GetObj(),
                              roi
                              );

        // For debug prupose.
        memset(vcs, 0x0, sizeof(VehicleCandidates));
        memset(vcs2, 0x0, sizeof(VehicleCandidates));

        fcws_obj->GetInternalData(w,
                                  h,
                                  linesize,
                                  img,
                                  shadow,
                                  roi_img,
                                  vedge,
                                  heatmap,
                                  vcs,
                                  vcs2
                                  );
    }
    // --------------------------------------------------

    return TRUE;
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

BOOL FCW_NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src)
{
    uint32_t r, c;
    uint32_t row, col;
    uint32_t size;

    if (!src || !dst || !dir) {
        fcwsdbg();
        return FALSE;
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

    return TRUE;
}

BOOL FCW_GaussianBlur(gsl_matrix* dst, const gsl_matrix* src)
{
    uint32_t r, c, i, j;
    uint32_t size;
    uint32_t row, col;
    double sum = 0.0;
    gsl_matrix_view submatrix_src;

    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        fcwsdbg();
        return FALSE;
    }

    if (m_gk.matrix.size1 != m_gk.matrix.size2) {
        fcwsdbg();
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

BOOL FCW_CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int direction, int crop_r, int crop_c, int crop_w, int crop_h)
{
    int cr, cc, cw, ch;
    uint32_t i, j;
    uint32_t r, c, row, col;
    uint32_t size;
    gsl_matrix*     pmatrix;
    gsl_matrix_view submatrix_src;
    gsl_matrix_view cropmatrix_src;

    if (!src || !dst || !dir) {
        fcwsdbg();
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

        CheckOrReallocMatrixUshort(&dst, ch, cw, TRUE);
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

            //gsl_matrix_ushort_set(dst, r, c, gsl_hypot(gx, gy));
            gsl_matrix_ushort_set(dst, r, c, abs(gx) + abs(gy));
            gsl_matrix_char_set(dir, r, c, FCW_GetRounded_Direction(gx, gy));
        }
    }

    return TRUE;
}

BOOL FCW_CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist)
{
    uint32_t r, c, i;
    int cutoff_pixel_value = 70;
    int pixel_value_peak = 0;
    double hist_peak = 0;

    if (!imgy || !result_imgy || !grayscale_hist) {
        fcwsdbg();
        return FALSE;
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
            
    //fcwsdbg("%d %lf/%lf at %d", pixel_value_peak, hist_peak, gsl_vector_max(grayscale_hist), gsl_vector_max_index(grayscale_hist));

    for (r=0 ; r<imgy->size1 ; r++) {
        for (c=0 ; c<imgy->size2 ; c++) {
            if (gsl_matrix_get(result_imgy, r, c) > pixel_value_peak)
                gsl_matrix_set(result_imgy, r, c, NOT_SHADOW);
        }
    }

    return TRUE;
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

    //fcwsdbg("th %d", th);
    return th;
}

BOOL FCW_CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view column_view;

    if (!imgy || !vertical_hist) {
        fcwsdbg();
        return FALSE;
    }

    gsl_vector_set_zero(vertical_hist);

    // skip border
    for (c=1 ; c<imgy->size2 - 1 ; c++) {
        column_view = gsl_matrix_column((gsl_matrix*)imgy, c);

        for (r=1 ; r<column_view.vector.size - 1; r++) {
            //fcwsdbg("[%d,%d] = %d", r, c, (int)gsl_vector_get(&column_view.vector, r));
            //if (gsl_vector_get(&column_view.vector, r) != NOT_SHADOW) {
            if (gsl_vector_get(&column_view.vector, r) != 0) {
                val = gsl_vector_get(vertical_hist, c);
                gsl_vector_set(vertical_hist, c, ++val);
            }
        }
    }

    return TRUE;
}

BOOL FCW_CalVerticalHist2(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist)
{
    uint32_t r, c, sr, sc, size;
    double val = 0;
    gsl_matrix_view submatrix;
    gsl_vector_view column_view;

    if (!imgy || !vertical_hist) {
        fcwsdbg();
        return FALSE;
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
    CheckOrReallocVector(&vertical_hist, size, TRUE);

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

    return TRUE;
}

BOOL FCW_CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view row_view;

    if (!imgy || !horizontal_hist) {
        fcwsdbg();
        return FALSE;
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

    return TRUE;
}

BOOL FCW_BlobFindIdentical(blob** bhead, blob* nblob)
{
    blob* cur = NULL;

    cur = *bhead;

    while (cur != NULL) {
        if (cur->r == nblob->r && cur->c == nblob->c && cur->w == nblob->w && cur->h == nblob->h)
            return TRUE;

        cur = cur->next;
    }

    return FALSE;
}

BOOL FCW_BlobRearrange(blob** bhead)
{
    int diff, number;
    blob *head = NULL, *cur = NULL, *next = NULL, *pre = NULL, *temp = NULL; 

    head = *bhead;

    if (!head)
        return FALSE;

    //fcwsdbg("--------Start of Rearrange----------");
redo:

    cur = head;
    while (cur) {
        //fcwsdbg("=====checking cur blob(%d,%d,%d)======",
        //            cur->r,
        //            cur->c,
        //            cur->w);

        next = cur->next;
        while (next) {
            //fcwsdbg("next blob(%d,%d,%d) <==> cur blob(%d,%d,%d).",
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
                    //fcwsdbg("next blob(%d,%d,%d) is above cur blob(%d,%d,%d).",
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
                        //fcwsdbg("\033[1;33mnext blob(%d,%d,%d) is above cur blob(%d,%d,%d).\033[m",
                        //        next->r,
                        //        next->c,
                        //        next->w,
                        //        cur->r,
                        //        cur->c,
                        //        cur->w);

                        //fcwsdbg("Update cur blob.");
                        pre = cur;
                        while (pre->next != next)
                            pre = pre->next;

                        temp = cur->next;
                        memcpy(cur, next, sizeof(blob));
                        cur->next = temp;

                        pre->next = next->next;
                        free(next);
                        next = pre;

                        fcwsdbg("redo");
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
                    //fcwsdbg("next blob(%d,%d,%d) is below cur blob(%d,%d,%d).",
                    //        next->r,
                    //        next->c,
                    //        next->w,
                    //        cur->r,
                    //        cur->c,
                    //        cur->w);

                    //fcwsdbg("Update cur blob.");
                    pre = cur;
                    while (pre->next != next)
                        pre = pre->next;

                    temp = cur->next;
                    memcpy(cur, next, sizeof(blob));
                    cur->next = temp;

                    pre->next = next->next;
                    free(next);
                    next = pre;

                    //fcwsdbg("redo");
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
                    //fcwsdbg("next blob(%d,%d,%d) overlaps cur blob(%d,%d,%d).",
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

                    //fcwsdbg("redo");
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

    //fcwsdbg("--------End of Rearrange----------");
    return TRUE;
}

BOOL FCW_BlobAdd(blob** bhead, blob* nblob)
{
    blob *cur = NULL, *newblob = NULL; 

    if (!nblob)
        return FALSE;

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

    return TRUE;
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

BOOL FCW_BlobGenerator(const gsl_matrix* imgy, uint32_t peak_idx, blob** bhead)
{
    int i, r, c;
    int blob_r_top, blob_r_bottom, blob_c, blob_w, blob_h;
    int max_blob_r, max_blob_c, max_blob_w, max_blob_h;
    int sm_r, sm_c, sm_w, sm_h;
    int stop_wall_cnt;
    uint32_t blob_cnt;
    uint32_t blob_pixel_cnt, max_blob_pixel_cnt;
    float blob_pixel_density;
    BOOL has_neighborhood = FALSE;
    blob *curblob = NULL, tblob;
    gsl_matrix_view submatrix;

    if (!imgy) {
        fcwsdbg();
        return FALSE;
    }

    if (peak_idx > BLOB_MARGIN + 1 && peak_idx < (imgy->size1 - BLOB_MARGIN - 1)) {

        //fcwsdbg("peak index %d", peak_idx);

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
                    has_neighborhood = FALSE;
                }

                //fcwsdbg("(%d,%d)=> %d %d %d %d, n %d, cnt %d, w %d",
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
                        has_neighborhood = TRUE;

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
                        has_neighborhood == FALSE) {

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

                            //fcwsdbg("Max blob (%d, %d) with  (%d, %d) %d %.02lf", 
                            //        max_blob_r, max_blob_c, max_blob_w, max_blob_h,
                            //        max_blob_pixel_cnt, blob_pixel_density);

                            if (max_blob_h >= 1 &&
                                    ((peak_idx <= imgy->size1 / 3.0 && max_blob_w >= 10 && blob_pixel_density >= 0.2) ||
                                     (peak_idx > imgy->size1 / 3.0 && max_blob_w >= 20 && blob_pixel_density >= 0.55))) {

                                //fcwsdbg("Valid Max blob (%d, %d) with  (%d, %d) %d %.02lf", 
                                //        max_blob_r, max_blob_c, max_blob_w, max_blob_h,
                                //        max_blob_pixel_cnt, blob_pixel_density);

                                tblob.valid = TRUE;
                                tblob.r     = max_blob_r;
                                tblob.c     = max_blob_c;
                                tblob.w     = max_blob_w;
                                tblob.h     = max_blob_h;
                                tblob.number = blob_cnt;
                                tblob.next  = NULL;

                                if (FCW_BlobFindIdentical(bhead, &tblob) == FALSE) {
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
                    if (has_neighborhood == FALSE && --stop_wall_cnt <= 1) {
                        goto examine;
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL FCW_BlobRemoveLiteralShadow(
        const gsl_matrix* imgy,
        const gsl_matrix* intimg,
        const gsl_matrix* shadow_imgy,
        const blob* bhead)
{
    BOOL below_th;
    uint32_t r, c;
    uint32_t lcs, rcs;  // left- and right- column of shadow
    double mean, sd, samples, sum;
    blob* cur_blob = NULL;
    gsl_matrix_view imgy_sm;

    if (!imgy || !intimg || !shadow_imgy || !bhead) {
        fcwsdbg();
        return FALSE;
    }

    cur_blob = (blob*)bhead;

    while (cur_blob) {
        fcwsdbg("cur_blob is at (%d, %d) with (%d, %d)", cur_blob->r, cur_blob->c, cur_blob->w, cur_blob->h);

        imgy_sm = gsl_matrix_submatrix((gsl_matrix*)imgy, cur_blob->r, cur_blob->c, cur_blob->h, cur_blob->w);
        mean = sd = samples = sum = 0;

        for (r=0 ; r<imgy_sm.matrix.size1 ; ++r) {
            for (c=0 ; c<imgy_sm.matrix.size2 ; ++c) {
                mean += gsl_matrix_get(&imgy_sm.matrix, r, c);
                samples++;
            }
        }

        if (samples > 1) {
            mean /= samples;

            for (r=0 ; r<imgy_sm.matrix.size1 ; ++r) {
                for (c=0 ; c<imgy_sm.matrix.size2 ; ++c) {
                    sum += gsl_pow_2(mean - gsl_matrix_get(&imgy_sm.matrix, r, c));
                }
            }

            sd = sqrt(sum / (samples - 1));
            fcwsdbg("mean %.02lf, sd %.02lf", mean, sd);

            lcs = rcs = UINT_MAX;
            for (c=0 ; c<imgy_sm.matrix.size2 ; ++c) {
                below_th = TRUE;

                for (r=0 ; r<imgy_sm.matrix.size1 ; ++r) {
                    if (gsl_matrix_get(&imgy_sm.matrix, r, c) > (mean + sd / 2.0)) {
                        below_th = FALSE;
                        break;
                    }

                    if (below_th == TRUE) {
                        if (lcs == UINT_MAX)
                            lcs = c;
                        if (rcs == UINT_MAX || rcs < c)
                            rcs = c;
                    }
                }
            }

            if (rcs > lcs) {
                cur_blob->c += lcs;
                cur_blob->w = (rcs - lcs + 1);
                fcwsdbg("lcs %d, rcs %d, width %d", lcs, rcs, rcs - lcs + 1);
                fcwsdbg("cur_blob is at (%d, %d) with (%d, %d)", cur_blob->r, cur_blob->c, cur_blob->w, cur_blob->h);

                if (cur_blob->w <= 10)
                    cur_blob->valid = FALSE;
            }
        }

        cur_blob = cur_blob->next;
    }

    fcwsdbg("========================");

    return TRUE;
}

BOOL FCW_VehicleCandidateGenerate(
        const gsl_matrix* imgy,
        const gsl_matrix* intimg,
        const gsl_matrix* shadow_imgy, 
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

    if (!shadow_imgy || !horizontal_hist || !vedge_imgy || !vcs) {
        fcwsdbg();
        return FALSE;
    }

    row = shadow_imgy->size1;
    col = shadow_imgy->size2;
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
    CheckOrReallocVector(&m_temp_hori_hist, horizontal_hist->size, TRUE);
    //CheckOrReallocVector(&temp_hh, horizontal_hist->size, TRUE);
    gsl_vector_memcpy(m_temp_hori_hist, horizontal_hist);

    max_peak = gsl_vector_max(m_temp_hori_hist);
    //max_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

    if (max_peak == 0)
        return FALSE;

    fcwsdbg("\033[1;33m========== max peak %d frame %u ===========\033[m", max_peak, frame_count++);

    FCW_BlobClear(&m_blob_head);

#if 1
    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        if (cur_peak < (max_peak * 0.5))
            break;

        //printf("\n");
        //fcwsdbg("Search blobs for current peak %d at %d", cur_peak, cur_peak_idx);

        gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

        FCW_BlobGenerator(shadow_imgy, cur_peak_idx, &m_blob_head);
    }
    
    if (m_blob_head) {
        blob *curblob = NULL;

        //fcwsdbg("Before arrange");
        //curblob = m_blob_head;
        //while (curblob!= NULL) {
        //    fcwsdbg("[%d,%d] with [%d,%d]", curblob->r, curblob->c, curblob->w, curblob->h);
        //    curblob = curblob->next;
        //}

        FCW_BlobRearrange(&m_blob_head);

        //fcwsdbg("After arrange");
        //curblob = m_blob_head;
        //while (curblob!= NULL) {
        //    fcwsdbg("[%d,%d] with [%d,%d]", curblob->r, curblob->c, curblob->w, curblob->h);
        //    curblob = curblob->next;
        //}
        //printf("\n");

        //FCW_BlobRemoveLiteralShadow(imgy,
        //                            intimg,
        //                            shadow_imgy,
        //                            m_blob_head);

        blob *cur = m_blob_head;
        int left_idx, right_idx, bottom_idx;
        int vehicle_startr, vehicle_startc;
        int vehicle_width, vehicle_height;
        gsl_matrix_view imgy_submatrix;

        while (cur != NULL) {

            if (cur->valid == TRUE) {
                //fcwsdbg("Before scaleup: blob at (%d,%d) with (%d,%d)",
                //        cur->r,
                //        cur->c,
                //        cur->w,
                //        cur->h);

                bottom_idx      = cur->r;

                //scale up searching region of blob with 1/4 time of width on the left & right respectively.
                left_idx        = (cur->c - (cur->w >> 2));
                right_idx       = (cur->c + cur->w + (cur->w >> 2));

                left_idx        = (left_idx < 0 ? 0 : left_idx);
                right_idx       = (right_idx >= vedge_imgy->size2 ? vedge_imgy->size2 - 1 : right_idx);

                vehicle_width   = (right_idx - left_idx + 1);

                if ((bottom_idx - (cur->w * 0.5)) < 0) {
                    vehicle_startr = 0;
                    vehicle_height = bottom_idx;
                } else {
                    vehicle_height = cur->w * 0.5;
                    vehicle_startr = bottom_idx - vehicle_height;
                }

                vehicle_startc  = left_idx;

                if (left_idx + vehicle_width >= shadow_imgy->size2) {
                    right_idx       = vedge_imgy->size2 - 1;
                    vehicle_width   = right_idx - left_idx + 1;
                } 


                //fcwsdbg("Vehicle at (%d,%d) with (%d,%d)", 
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

                //fcwsdbg("Before Edge: Vehicle at (%d,%d) with (%d,%d)",
                //        cur->r,
                //        cur->c,
                //        cur->w,
                //        cur->h);

                FCW_UpdateBlobByStrongVEdge(&imgy_submatrix.matrix, cur);

                //fcwsdbg("Before Valid: Vehicle at (%d,%d) with (%d,%d)",
                //        cur->r,
                //        cur->c,
                //        cur->w,
                //        cur->h);

                FCW_CheckBlobValid(shadow_imgy, vedge_imgy, cur);

                if (cur->valid) {

                    vcs->vc[vcs->vc_count].m_valid  = TRUE;
                    vcs->vc[vcs->vc_count].m_dist   = FCW_GetObjDist(cur->w);
                    vcs->vc[vcs->vc_count].m_id     = cur->number;

                    vcs->vc[vcs->vc_count].m_r      = (cur->r == 0 ? 1 : cur->r);
                    vcs->vc[vcs->vc_count].m_c      = cur->c;
                    vcs->vc[vcs->vc_count].m_w      = cur->w;
                    vcs->vc[vcs->vc_count].m_h      = (cur->r == 0 ? cur->h - 1 : cur->h);

                    vcs->vc[vcs->vc_count].m_st     = Disappear;

                    //                fcwsdbg("\033[1;33mVehicle %d at (%d,%d) with (%d,%d), dist %.02lfm\033[m\n\n",
                    //                        vcs->vc[vcs->vc_count].m_id,
                    //                        vcs->vc[vcs->vc_count].m_r,
                    //                        vcs->vc[vcs->vc_count].m_c,
                    //                        vcs->vc[vcs->vc_count].m_w,
                    //                        vcs->vc[vcs->vc_count].m_h,
                    //                        vcs->vc[vcs->vc_count].m_dist);

                    vcs->vc_count++;
                }
                else {
                    fcwsdbg("Fail Vehicle at (%d,%d) with (%d,%d)\n\n",
                            cur->r,
                            cur->c,
                            cur->w,
                            cur->h);

                }
            }

            cur = cur->next;
        }
    }

    return TRUE;
#else
    while (1) {
        cur_peak = gsl_vector_max(m_temp_hori_hist);
        cur_peak_idx = gsl_vector_max_index(m_temp_hori_hist);

        if (cur_peak < (max_peak_idx * 0.8))
            break;

        printf("\n");
        fcwsdbg("vote for current peak %d at %d", cur_peak, cur_peak_idx);

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
            //fcwsdbg("peak at %d, delta %d", cur_peak_idx, abs((int)ppeak->idx - (int)cur_peak_idx));
            if (abs((int)pg.peak[pg.peak_count].idx - (int)cur_peak_idx) <= 20) {
                gsl_vector_set(m_temp_hori_hist, cur_peak_idx, 0);

                if (++vote_success >= 10) {
                    pg.peak[pg.peak_count++].vote_cnt = vote_success;
                    fcwsdbg("vote_success %d", vote_success);
                    //ppeak->vote_cnt = vote_success;
                    //pg.push_back(ppeak);
                    break;
                }
            } else {
                if (++vote_fail > 5) {
                    fcwsdbg("vote_fail %d", vote_fail);
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
    fcwsdbg("Guessing left & right boundary....");
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
        fcwsdbg("VH max is %d at %d for bottom %d", (int)max_vh, (int)max_vh_idx, bottom_idx);

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
                fcwsdbg("Find left boundary");
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
                fcwsdbg("Find right boundary");
                break;
            }
        }

#if 0
        for (c=0 ; c<col ; c++) {
            if (gsl_matrix_get(imgy, bottom_idx, c) != NOT_SHADOW) {
                fcwsdbg("imgy[%d][%d]=%d, vh=%d(max. %d)", bottom_idx, c, 
                                                (int)gsl_matrix_get(imgy, bottom_idx, c), 
                                                (int)gsl_vector_get(vertical_hist, c),
                                                (gsl_vector_get(vertical_hist, c) == gsl_vector_max(vertical_hist) ? 1 : 0));
            }
        }
#endif

        fcwsdbg("Guessing left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);
#if 1
        if (left_idx && right_idx) {
            int temp_h;

            vcs->vc[vcs->vc_count].m_c = left_idx;
            vcs->vc[vcs->vc_count].m_w = (right_idx - left_idx + 1);

            temp_h = vcs->vc[vcs->vc_count].m_w * VHW_RATIO;
            vcs->vc[vcs->vc_count].m_r = (bottom_idx - temp_h < 0 ? 0 : bottom_idx - temp_h);
            vcs->vc[vcs->vc_count].m_h = (bottom_idx - temp_h < 0 ? bottom_idx : temp_h);

            fcwsdbg("[%d] => [%d, %d] with [%d, %d]", 
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

            fcwsdbg("Scale up left %d right %d at bottom %d", left_idx, right_idx, bottom_idx);

            vehicle_startr  = (bottom_idx - vehicle_height < 0 ? 0 : bottom_idx - vehicle_height);
            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= imgy->size2) {
                right_idx       = imgy->size2 - 1;
                vehicle_width   = right_idx - left_idx + 1;
            } 

            fcwsdbg("Vehicle at (%d,%d) with (%d,%d)", 
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

            fcwsdbg("Vehicle at (%d,%d) with (%d,%d)", 
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

    return TRUE;
}

BOOL FCW_UpdateBlobByStrongVEdge(const gsl_matrix* imgy, blob*  blob)
{
    int left_idx, right_idx, bottom_idx;
    uint32_t r, c;
    uint32_t max_vedge_c, max_vedge_c_left;
    double magnitude;
    double vedge_strength, max_vedge_strength;
    gsl_matrix_view imgy_block;

    if (!imgy || !blob) {
        fcwsdbg();
        return FALSE;
    }

//    fcwsdbg("==================");
//
//    fcwsdbg("Before (%d, %d) with (%d, %d)",
//        blob->r,
//        blob->c,
//        blob->w,
//        blob->h);

#if 1
    {
        uint32_t pix_cnt = 0;
        uint32_t column_cnt = 0;
        uint32_t max_edge_idx = 0;
        double max_edge_value = 0;
        double mean = 0, sd = 0, delta = 0;
        gsl_vector* strong_edge_value = NULL;

        strong_edge_value = gsl_vector_alloc(imgy->size2);

        bottom_idx  = blob->r + blob->h;

        for (c=0 ; c<imgy->size2 - 2 ; ++c) {
            imgy_block = gsl_matrix_submatrix((gsl_matrix*)imgy, 
                    0, 
                    c, 
                    imgy->size1, 
                    2);

            pix_cnt = magnitude = 0;

            for (uint32_t rr= (imgy_block.matrix.size1/2); rr<imgy_block.matrix.size1 ; ++rr) {
                for (uint32_t cc=0 ; cc<imgy_block.matrix.size2 ; ++cc) {
                    pix_cnt++;
                    magnitude += gsl_matrix_get(&imgy_block.matrix, rr, cc);
                }
            }

            vedge_strength = (magnitude / (double)(pix_cnt));
            gsl_vector_set(strong_edge_value, c, vedge_strength);
            //fcwsdbg("strong edge[%d] = %lf", c, vedge_strength);

            column_cnt++;
            mean += vedge_strength;
        }

        // mean & standart deviation
        mean /= column_cnt;

        for (c=0 ; c<imgy->size2 - 2 ; ++c) {
            delta = (mean - gsl_vector_get(strong_edge_value, c));
            sd += (delta * delta);
        }

        sd = sqrt(sd / (double)(column_cnt - 1));

//        fcwsdbg("mean %lf, sd %lf", mean, sd);

        // Find left max
        max_edge_idx = 0;
        max_edge_value = 0;
        for (c=0 ; c<strong_edge_value->size / 2 ; ++c) {
            if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) && 
                (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                max_edge_value = gsl_vector_get(strong_edge_value, c);
                max_edge_idx = c;
            }
        }

        if (max_edge_idx == 0) {
            blob->valid = FALSE;
            return FALSE;
        }

//        fcwsdbg("Left max %lf at %d", max_edge_value, max_edge_idx);
        left_idx = max_edge_idx;
        blob->c += max_edge_idx;

        // Find right max
        max_edge_idx = 0;
        max_edge_value = 0;
        for (c=strong_edge_value->size / 2 ; c<(strong_edge_value->size - 2); ++c) {
            if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) && 
                (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                max_edge_value = gsl_vector_get(strong_edge_value, c);
                max_edge_idx = c;
            }
        }

        if (max_edge_idx == 0) {
            blob->valid = FALSE;
            return FALSE;
        }

//        fcwsdbg("Right max %lf at %d", max_edge_value, max_edge_idx);

        blob->w = max_edge_idx - left_idx + 1;
        blob->h = (blob->w * VHW_RATIO > bottom_idx) ? bottom_idx : blob->w * VHW_RATIO;
        blob->r = bottom_idx - blob->h;
        blob->valid = TRUE;

        gsl_vector_free(strong_edge_value);
        strong_edge_value = NULL;

//        fcwsdbg("After (%d, %d) with (%d, %d)",
//                blob->r,
//                blob->c,
//                blob->w,
//                blob->h);

        return TRUE;
    }

#else
    bottom_idx  = blob->r + blob->h;
    left_idx    = blob->c;
    right_idx   = blob->c + blob->w - 1;

    // update left idx
    fcwsdbg("Update left idx");
    max_vedge_c = 0;
    max_vedge_strength = 0;

    //for (c=0 ; c<((imgy->size2 / 2) - 2) ; ++c) {
    for (c=0 ; c<((imgy->size2 / 4) - 2) ; ++c) {
    //for (c=((imgy->size2 / 4) - 2) ; c>2 ; --c) {
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
            //fcwsdbg("%.02lf at %d", max_vedge_strength, c);
        }
    }

    if (max_vedge_c == 0) {
        blob->valid = FALSE;
        return FALSE;
    }

    fcwsdbg("left max = %d", max_vedge_c);
    blob->c += max_vedge_c;
//    fcwsdbg("blob->c %d", blob->c);

    // upate right idx
    fcwsdbg("Update right idx");
    max_vedge_c_left = max_vedge_c;
    max_vedge_c = 0;
    max_vedge_strength = 0;

    //for (c=imgy->size2 - 2 ; c>(max_vedge_c_left + 2) ; --c) {
    for (c=imgy->size2 - 2 ; c>((imgy->size2 * 3 / 4) - 2) ; --c) {
    //for (c=((imgy->size2 * 3 / 4) - 2) ; c<(imgy->size2 - 2) ; ++c) {
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
            //fcwsdbg("%.02lf at %d", max_vedge_strength, c);
        }
    }

    if (max_vedge_c == 0) {
        blob->valid = FALSE;
        return FALSE;
    }

    fcwsdbg("right max = %d", max_vedge_c);
    right_idx = left_idx +  max_vedge_c;

    blob->w = right_idx - blob->c + 1;
    blob->h = (blob->w * VHW_RATIO > bottom_idx) ? bottom_idx : blob->w * VHW_RATIO;
    blob->r = bottom_idx - blob->h;
    blob->valid = TRUE;

    fcwsdbg("After (%d, %d) with (%d, %d)",
        blob->r,
        blob->c,
        blob->w,
        blob->h);

    return TRUE;
#endif
}

BOOL FCW_CheckBlobByArea(const gsl_matrix* imgy, blob* cur)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float area_ratio;
    gsl_matrix_view submatrix;

    if (!imgy || !cur) {
        fcwsdbg();
        return FALSE;
    }
    
    if (cur && cur->valid == FALSE)
        return FALSE;

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

    //fcwsdbg("Area: %d / %d = %.02f", pixel_cnt, area, area_ratio);

    if (/*area_ratio < 0.1 || */area_ratio > 0.8)
        cur->valid = FALSE;

    return TRUE;
}

BOOL FCW_CheckBlobByVerticalEdge(const gsl_matrix* vedge_imgy, blob* cur)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float percentage;
    gsl_matrix_view submatrix;

    if (!vedge_imgy || !cur) {
        fcwsdbg();
        return FALSE;
    }

    if (cur && cur->valid == FALSE)
        return FALSE;

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

    //fcwsdbg("VerticalEdge: %d / %d = %.04f", pixel_cnt, area, percentage);

    if (percentage < 0.2)
        cur->valid = FALSE;

    return TRUE;
}

BOOL FCW_CheckBlobAR(blob* blob)
{
    double ar;

    if (!blob) {
        fcwsdbg();
        return FALSE;
    }

    if (blob->valid == TRUE) {
        ar = blob->w / (double)blob->h;

        //fcwsdbg("ar %.03lf", ar);

        if (AR_LB < ar && ar < AR_HB)
            blob->valid = TRUE;
        else blob->valid = FALSE;
    }

    return TRUE;
}

BOOL FCW_CheckBlobValid(const gsl_matrix* imgy, const gsl_matrix* vedge_imgy, blob* cur)
{
    if (!imgy || !cur) {
        fcwsdbg();
        return FALSE;
    }
    
    if (cur && cur->valid == FALSE)
        return FALSE;

    //FCW_CheckBlobByArea(imgy, cur);

    FCW_CheckBlobByVerticalEdge(vedge_imgy, cur);

    FCW_CheckBlobAR(cur);

    return TRUE;
}

BOOL FCW_CheckSymmProperty(const gsl_matrix* imgy, VehicleCandidates* vcs, float th_pairwise, float th_symm)
{
    BOOL ret = FALSE; 
    uint32_t i, r, c;
    double grade;
    gsl_matrix_view imgy_sm, grademap_sm;
    gsl_matrix* grademap = NULL;

    if (!imgy || !vcs || th_pairwise <= 0 || th_symm <= 0) {
        fcwsdbg();
        return ret;
    }

    CheckOrReallocMatrix(&grademap, imgy->size1, imgy->size2, TRUE);

    if (!grademap) {
        fcwsdbg();
        return ret;
    }

    for (i=0 ; i<vcs->vc_count ; ++i) {

        gsl_matrix_set_all(grademap, 0);

        if (vcs->vc[i].m_valid == TRUE) {

            imgy_sm     = gsl_matrix_submatrix((gsl_matrix*)imgy, vcs->vc[i].m_r, vcs->vc[i].m_c, vcs->vc[i].m_h, vcs->vc[i].m_w);
            grademap_sm = gsl_matrix_submatrix(grademap, vcs->vc[i].m_r, vcs->vc[i].m_c, vcs->vc[i].m_h, vcs->vc[i].m_w);

            for (r=0 ; r<grademap_sm.matrix.size1 ; ++r) {
                for (c=0 ; c<grademap_sm.matrix.size2/2 ; ++c) {
                    //fcwsdbg("[%d,%d][%d,%d][%d,%d]", r, c, r, imgy_sm.matrix.size2 - 1 - c, grademap_sm.matrix.size1, grademap_sm.matrix.size2);
                    //fcwsdbg("%lf", gsl_matrix_get(&imgy_sm.matrix, r, c));
                    //fcwsdbg("%lf", gsl_matrix_get(&imgy_sm.matrix, r, imgy_sm.matrix.size2 - 1 - c));
                    grade = fabs(((gsl_matrix_get(&imgy_sm.matrix, r, c) - gsl_matrix_get(&imgy_sm.matrix, r, imgy_sm.matrix.size2 - 1 - c)) / gsl_matrix_get(&imgy_sm.matrix, r, c))); 

                    //fcwsdbg("[%d,%d] grade %.04lf", r, c, grade);
                    if (grade < th_pairwise)
                        gsl_matrix_set(&grademap_sm.matrix, r, c, 1);
                }
            }

            grade = 0;

            for (r=0 ; r<grademap_sm.matrix.size1 ; ++r) {
                for (c=0 ; c<grademap_sm.matrix.size2/2 ; ++c) {
                    grade += gsl_matrix_get(&grademap_sm.matrix, r, c);
                }
            }

            grade = (grade / (grademap_sm.matrix.size1 * grademap_sm.matrix.size2 / 2.0));
            fcwsdbg("grade[%d] %.02lf", i, grade);

            if (grade < th_symm)
                vcs->vc[i].m_valid = FALSE;
        }
    }


    FreeMatrix(&grademap);

    return ret;
}

BOOL FCW_UpdateVehicleHeatMap(gsl_matrix* heatmap, gsl_matrix_char* heatmap_id, VehicleCandidates* vcs)
{
    BOOL pixel_hit = FALSE;
    uint32_t i, r, c;
    double val;

    if (!heatmap || !vcs) {
        fcwsdbg();
        return FALSE;
    }
 
    CheckOrReallocMatrixChar(&heatmap_id, heatmap->size1, heatmap->size2, FALSE); 
 
    // decrease/increase HeapMap
    for (r=0 ; r<heatmap->size1 ; ++r) {
        for (c=0 ; c<heatmap->size2 ; ++c) {

            pixel_hit = FALSE;

            // this pixel belongs to certain vehicle candidate.
            for (i=0 ; i<vcs->vc_count ; ++i) {
                if (vcs->vc[i].m_valid == FALSE)
                    continue;

                if (r >= vcs->vc[i].m_r && r< vcs->vc[i].m_r + vcs->vc[i].m_h && c >= vcs->vc[i].m_c && c < vcs->vc[i].m_c + vcs->vc[i].m_w) {
                    pixel_hit = TRUE;
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
                //fcwsdbg("[%d,%d] %d disappear", r, c, gsl_matrix_char_get(heatmap_id, r, c));
                gsl_matrix_char_set(heatmap_id, r, c, -1);
            }
        }
    }

    return TRUE;
}
 
BOOL FCW_GetContour(
    const gsl_matrix_char* m,
    char id,
    const point_t* start,
    rect* rect
    )
{
    BOOL ret = FALSE;
    BOOL find_pixel;
    uint32_t r, c;
    uint32_t left, right, top, bottom;
    uint32_t find_fail_cnt;
    uint32_t count;
    DIR nextdir;
    float aspect_ratio;
    point_t tp; // test point to avoid infinite loop.

    if (!m || id < 0 || !start || !rect) {
        fcwsdbg();
        return ret;
    }

    if (start->r == 0) 
        return ret;

    nextdir = DIR_RIGHTUP;
    find_pixel = FALSE;
    find_fail_cnt = 0;

    r = top = bottom = start->r;
    c = left = right = start->c;

    memset(&tp, 0, sizeof(tp));

    count = 0;
    while (1) {// Get contour of VC. (start point is equal to end point)
        while (1) {// Scan different direction to get neighbour.
            switch (nextdir) {
                case DIR_RIGHTUP:
                    if (gsl_matrix_char_get(m, r-1, c+1) == id) {
                        --r;
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                        right   = right < c ? c : right; 
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHT;
                    }
                    break;
                case DIR_RIGHT:
                    if (gsl_matrix_char_get(m, r, c+1) == id) {
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTUP;

                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTDOWN;
                    }
                    break;
                case DIR_RIGHTDOWN:
                    if (gsl_matrix_char_get(m, r+1, c+1) == id) {
                        ++r;
                        ++c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTUP;

                        bottom  = bottom < r ? r : bottom;
                        right   = right < c ? c : right;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_DOWN;
                    }
                    break;
                case DIR_DOWN:
                    if (gsl_matrix_char_get(m, r+1, c) == id) {
                        ++r;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTDOWN;
                    }
                    break;
                case DIR_LEFTDOWN:
                    if (gsl_matrix_char_get(m, r+1, c-1) == id) {
                        ++r;
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_RIGHTDOWN;

                        bottom  = bottom < r ? r : bottom;
                        left    = left > c ? c : left;
                    } else { 
                        find_fail_cnt++;
                        nextdir = DIR_LEFT;
                    }
                    break;
                case DIR_LEFT:
                    if (gsl_matrix_char_get(m, r, c-1) == id) {
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTDOWN;

                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_LEFTUP;
                    }
                    break;
                case DIR_LEFTUP:
                    if (gsl_matrix_char_get(m, r-1, c-1) == id) {
                        --r;
                        --c;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTDOWN;

                        top     = top > r ? r : top;
                        left    = left > c ? c : left;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_UP;
                    }
                    break;
                case DIR_UP:
                    if (gsl_matrix_char_get(m, r-1, c) == id) {
                        --r;
                        find_pixel = TRUE;
                        nextdir = DIR_LEFTUP;

                        top     = top > r ? r : top;
                    } else {
                        find_fail_cnt++;
                        nextdir = DIR_RIGHTUP;
                    }
                    break;
            } // switch

            if (find_pixel) {
                find_pixel = FALSE;
                find_fail_cnt = 0;
                break;
            }

            if (find_fail_cnt >= DIR_TOTAL) {
                fcwsdbg();
                break;
            }
        }// Scan different direction to get neighbour.

        if (r == start->r && c == start->c) {
           rect->r  = top; 
           rect->c  = left;
           rect->w  = right - left + 1;
           rect->h  = bottom - top + 1;

           // *** Assume previously height is half of weight. *** 
           // aspect ratio checking (VHW_RATIO * 3/4 < ar < VHW_RATIO * 5/4)
           aspect_ratio = (rect->w / (float)rect->h);

           //fcwsdbg("ratio of aspect is %.02f", rect->w / (float)rect->h);
           if (AR_LB < aspect_ratio && aspect_ratio < AR_HB)
               ret = TRUE;

           break;
        } else {
            if (r != start->r && c != start->c) {
                if (tp.r == 0 && tp.c == 0) {
                    tp.r = r;
                    tp.c = c;
                } else {
                    if (r == tp.r && c == tp.c) {
                        //fcwsdbg("start(%d,%d), tp(%d,%d) <->(%d,%d)",
                        //        start->r,
                        //        start->c,
                        //        tp.r,
                        //        tp.c,
                        //        r,
                        //        c);
                        fcwsdbg("Get incorrect contour.");
                        break;
                    }
                }
            }
        }

        if (++count >= (m->size1 * m->size2) >> 4) { // (w/2 * h/2)
            fcwsdbg("Can not generate appropriate contour.");
            break;
        }
    }// Get contour of VC. (start point is equal to end point)

    return ret;
}

Candidate* FCW_NewCandidate(void)
{
    Candidate* vc = NULL;

    vc = (Candidate*)malloc(sizeof(Candidate));

    if (vc) {
        vc->m_updated   = FALSE;
        vc->m_valid     = FALSE;
        vc->m_id        = 0;
        vc->m_dist      = 0.0;
        vc->m_r         = 0;
        vc->m_c         = 0;
        vc->m_w         = 0;
        vc->m_h         = 0;
        vc->m_st        = Disappear;
        vc->m_next      = NULL;
    }

    return vc;
}

BOOL FCW_UpdateVCStatus(
    gsl_matrix* heatmap, 
    gsl_matrix_char* heatmap_id, 
    Candidate** vc_tracker,
    VehicleCandidates* vcs
    )
{
    char vc_id, max_vc_id;
    unsigned char which_vc;
    uint32_t i;
    uint32_t r, c, rr, cc; 
    BOOL find_pixel;
    BOOL gen_new_vc;
    Candidate *cur_vc, *nearest_vc, *new_vc;
    point_t midpoint_newcand;
    point_t contour_sp;
    rect contour_rect;
    gsl_matrix_view heatmap_sm;
    gsl_matrix_char_view heatmapid_sm;


    if (!heatmap || !heatmap_id || !vcs) {
        fcwsdbg();
        return FALSE;
    }

    // Reset each existed candidate as need to update.
    cur_vc = *vc_tracker;
    while (cur_vc) {
        fcwsdbg("cur_vc[%d] at (%d, %d) with (%d,%d)", 
                cur_vc->m_id,
                cur_vc->m_r,
                cur_vc->m_c,
                cur_vc->m_w,
                cur_vc->m_h);
        cur_vc->m_updated = FALSE;
        cur_vc = cur_vc->m_next;
    }

    //fcwsdbg("vc count %d", vcs->vc_count);
    for (i=0 ; i<vcs->vc_count ; ++i) {
        if (vcs->vc[i].m_valid) {
            fcwsdbg("New vc[%d] at (%d,%d) with (%d,%d)",
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

            if (vcs->vc[which_vc].m_valid == FALSE)
                continue;

            fcwsdbg("New point at (%d,%d) belongs to new vc[%d]", r, c, which_vc);
            gen_new_vc = FALSE;

            // Find the nearest existed candidate.
            nearest_vc = NULL;
#if 1
            midpoint_newcand.r = vcs->vc[which_vc].m_r + vcs->vc[which_vc].m_h/2;
            midpoint_newcand.c = vcs->vc[which_vc].m_c + vcs->vc[which_vc].m_w/2;

            cur_vc = *vc_tracker;

            while (cur_vc) {
                if (midpoint_newcand.r > cur_vc->m_r && midpoint_newcand.r < cur_vc->m_r + cur_vc->m_h &&
                    midpoint_newcand.c > cur_vc->m_c && midpoint_newcand.c < cur_vc->m_c + cur_vc->m_w) {
                    nearest_vc = cur_vc;
                    break;
                }

                cur_vc = cur_vc->m_next;
            }
#else
            cur_vc = *vc_tracker;

            while (cur_vc) {
                midpoint_newcand.r = vcs->vc[which_vc].m_r + vcs->vc[which_vc].m_h/2;
                midpoint_newcand.c = vcs->vc[which_vc].m_c + vcs->vc[which_vc].m_w/2;

                midpoint_existedcand.r = cur_vc->m_r + cur_vc->m_h/2;
                midpoint_existedcand.c = cur_vc->m_c + cur_vc->m_w/2;

                dist = sqrt(pow(midpoint_existedcand.r - midpoint_newcand.r, 2.0) + pow(midpoint_existedcand.c - midpoint_newcand.c, 2.0));
                fcwsdbg("dist %lf between cur_vc[%d] and which_vc[%d]", dist, cur_vc->m_id, which_vc);
                //if (dist <= cur_vc->m_w/2/* || dist <= cur_vc->m_h/2*//* || vcs->vc[which_vc].m_w/2 || vcs->vc[which_vc].m_h/2*/) {
                if (midpoint_newcand.r > cur_vc->m_r && midpoint_newcand.r < cur_vc->m_r + cur_vc->m_h &&
                       midpoint_newcand.c > cur_vc->m_c && midpoint_newcand.c < cur_vc->m_c + cur_vc->m_w
                        ) {
                    nearest_vc = cur_vc;
                    break;
                }

                cur_vc = cur_vc->m_next;
            }
#endif

            if (nearest_vc) {
                vc_id = nearest_vc->m_id;
                fcwsdbg("Find nearest candidate[%d]", vc_id);
                //fcwsdbg("Find nearest candidate %d(%d,%d,%d,%d), (%d,%d), id %d, which_vc %d", 
                //        vc_id, 
                //        nearest_vc->m_r,
                //        nearest_vc->m_c,
                //        nearest_vc->m_w,
                //        nearest_vc->m_h,
                //        r, 
                //        c, 
                //        gsl_matrix_char_get(heatmap_id, r, c), 
                //        which_vc);

                //fcwsdbg("new vc %d valid %d at (%d,%d) with (%d,%d)",
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
                gen_new_vc = TRUE;

                // New candidate needs a new ID.
                if (*vc_tracker) {
                    max_vc_id = 0;
                    cur_vc = *vc_tracker;
                    while (cur_vc) {
                        if (cur_vc->m_id > max_vc_id)
                            max_vc_id = cur_vc->m_id;

                        cur_vc = cur_vc->m_next;
                    }

                    vc_id = max_vc_id + 1;
                }

                fcwsdbg("Create a new candidate at (%d,%d) with id %d for new vc[%d]", r, c, vc_id, which_vc);
                fcwsdbg("vc %d at (%d,%d) with (%d,%d)", which_vc,
                        vcs->vc[which_vc].m_r,
                        vcs->vc[which_vc].m_c,
                        vcs->vc[which_vc].m_w,
                        vcs->vc[which_vc].m_h);

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
            if (gen_new_vc == FALSE && nearest_vc) {
                //r > nearest_vc->m_r && r < nearest_vc->m_r + nearest_vc->m_h &&
                //c > nearest_vc->m_c && c < nearest_vc->m_c + nearest_vc->m_w) {
                // if start point is inside nearest vc.
               // fcwsdbg("(%d,%d) is inside vc[%d]:(%d,%d) (%d,%d)",
               //         r,
               //         c,
               //         nearest_vc->m_id,
               //         nearest_vc->m_r,
               //         nearest_vc->m_c,
               //         nearest_vc->m_w,
               //         nearest_vc->m_h);
               // for (rr=nearest_vc->m_r ; rr<nearest_vc->m_r + nearest_vc->m_h ; ++rr) {
               //     for (cc=nearest_vc->m_c ; cc<nearest_vc->m_c + nearest_vc->m_w ; ++cc) {
               //         fcwsdbg("(%d,%d): %lf, %d",
               //                 rr,
               //                 cc,
               //                 gsl_matrix_get(heatmap, rr, cc),
               //                 gsl_matrix_char_get(heatmap_id, rr, cc)
               //            );
               //     }
               // }

                if (r > nearest_vc->m_r && c > nearest_vc->m_c) {
                    find_pixel = FALSE;
                    for (rr=nearest_vc->m_r ; rr<nearest_vc->m_r + nearest_vc->m_h ; ++rr) {
                        for (cc=nearest_vc->m_c ; cc<nearest_vc->m_c + nearest_vc->m_w ; ++cc) {
                            if (gsl_matrix_char_get(heatmap_id, rr, cc) == nearest_vc->m_id && gsl_matrix_get(heatmap, rr, cc) >= HeatMapAppearThreshold) {
                                find_pixel = TRUE;
                                break;
                            }
                        }

                        if (find_pixel)
                            break;
                    }

                    //fcwsdbg("rr %d cc %d", rr, cc);
                    contour_sp.r = rr;
                    contour_sp.c = cc;
                    fcwsdbg("new top_left is inside cur top_left, using (%d,%d) instead of (%d,%d) as start point of contour.", rr, cc, r, c);
                } else {
                    contour_sp.r = r;
                    contour_sp.c = c;
                    fcwsdbg("new top_left is outside cur top_left, using (%d,%d) as start point of contour.", r, c);
                }
            } else {
                contour_sp.r = r;
                contour_sp.c = c;
            }

            if (FCW_GetContour(heatmap_id, vc_id, &contour_sp, &contour_rect) == TRUE) {
                if (gen_new_vc) {
                    new_vc = FCW_NewCandidate();
                    if (!new_vc) {
                        fcwsdbg();
                        return FALSE;
                    }

                    new_vc->m_updated     = TRUE;
                    new_vc->m_valid       = TRUE;
                    new_vc->m_id          = vc_id;
                    new_vc->m_r           = contour_rect.r;
                    new_vc->m_c           = contour_rect.c;
                    new_vc->m_w           = contour_rect.w;
                    new_vc->m_h           = contour_rect.h;
                    new_vc->m_dist        = FCW_GetObjDist(new_vc->m_w);

                    // Add new candidate to candidate list.
                    if (!*vc_tracker) {
                        *vc_tracker = cur_vc = new_vc;
                    } else {
                        cur_vc = *vc_tracker;
                        while (cur_vc) {
                            if (!cur_vc->m_next) {
                                cur_vc->m_next = new_vc;
                                new_vc->m_next = NULL;
                                cur_vc = new_vc;
                                break;
                            }

                            cur_vc = cur_vc->m_next;
                        }
                    }
                } else {
                    if (nearest_vc) {
                        nearest_vc->m_updated     = TRUE;
                        nearest_vc->m_valid       = TRUE;
                        nearest_vc->m_id          = vc_id;
                        nearest_vc->m_r           = contour_rect.r;
                        nearest_vc->m_c           = contour_rect.c;
                        nearest_vc->m_w           = contour_rect.w;
                        nearest_vc->m_h           = contour_rect.h;
                        nearest_vc->m_dist        = FCW_GetObjDist(nearest_vc->m_w);

                        cur_vc                    = nearest_vc;
                    } else 
                        fcwsdbg();
                }

                if (cur_vc) {
                    fcwsdbg("vc[%d] locates at (%d,%d) with (%d,%d)",
                            cur_vc->m_id,
                            cur_vc->m_r,
                            cur_vc->m_c,
                            cur_vc->m_w,
                            cur_vc->m_h
                       );

                    heatmapid_sm = gsl_matrix_char_submatrix(heatmap_id,
                                                                cur_vc->m_r,
                                                                cur_vc->m_c,
                                                                cur_vc->m_h,
                                                                cur_vc->m_w);

                    gsl_matrix_char_set_all(&heatmapid_sm.matrix, cur_vc->m_id);
                }
            } else {
                if (gen_new_vc) {
                    fcwsdbg("Fail to generate new vc.");
                }
            }
        }
    }
     
    // Update existed candidate that has not been updated yet.
    cur_vc = *vc_tracker;
    while (cur_vc) {
        if (cur_vc->m_updated == FALSE) {
            //fcwsdbg("vc %d is at (%d,%d) with (%d,%d) but has not updated.",
            //        cur_vc->m_id,
            //        cur_vc->m_r,
            //        cur_vc->m_c,
            //        cur_vc->m_w,
            //        cur_vc->m_h);

            // Find a pixel inside existed vc which still has an id.
            find_pixel = FALSE;
            for (r=cur_vc->m_r ; r<cur_vc->m_r+cur_vc->m_h ; ++r) {
                for (c=cur_vc->m_c ; c<cur_vc->m_c+cur_vc->m_w ; ++c) {
                    if (gsl_matrix_char_get(heatmap_id, r, c) == cur_vc->m_id) {
                        find_pixel      = TRUE;
                        contour_sp.r    = r;
                        contour_sp.c    = c;
                        vc_id           = cur_vc->m_id; 
                        break;
                    }
                }

                if (find_pixel)
                    break;
            }

            if (find_pixel == TRUE && FCW_GetContour(heatmap_id, vc_id, &contour_sp, &contour_rect) == TRUE) {
                cur_vc->m_updated     = TRUE;
                cur_vc->m_valid       = TRUE;
                cur_vc->m_id          = vc_id;
                cur_vc->m_r           = contour_rect.r;
                cur_vc->m_c           = contour_rect.c;
                cur_vc->m_w           = contour_rect.w;
                cur_vc->m_h           = contour_rect.h;
                cur_vc->m_dist        = FCW_GetObjDist(cur_vc->m_w);
            }

            if (cur_vc->m_updated == TRUE) {
                fcwsdbg("vc[%d] locates at (%d,%d) with (%d,%d) is self-updated.",
                        cur_vc->m_id,
                        cur_vc->m_r,
                        cur_vc->m_c,
                        cur_vc->m_w,
                        cur_vc->m_h);
                cur_vc = cur_vc->m_next;
            } else {
                fcwsdbg("vc[%d] is removed.", cur_vc->m_id);
                // This existed candidate does not been updated anymore. Remove it from the list.
                if (cur_vc == *vc_tracker) {
                    *vc_tracker = cur_vc->m_next;
                    free (cur_vc);
                    cur_vc = *vc_tracker;
                } else {
                    Candidate *temp = NULL;

                    temp  = *vc_tracker;
                    while (temp) {
                        if (temp->m_next == cur_vc) {
                            temp->m_next = cur_vc->m_next;
                            free (cur_vc);
                            cur_vc = temp->m_next;
                            break;
                        }

                        temp = temp->m_next;
                    }
                }
            }
        } else {
            cur_vc = cur_vc->m_next;
        }
    }
    
    return TRUE;
}

BOOL FCW_EdgeDetection(gsl_matrix* src, gsl_matrix* dst, gsl_matrix_ushort* gradient, gsl_matrix_char* dir, int direction)
{
    if (!src || !dst || !gradient || !dir) {
        fcwsdbg();
        return FALSE;
    }

    FCW_CalGradient(gradient, dir, src, direction, 0, 0, 0, 0);
    gsl_matrix_set_zero(dst);
    FCW_NonMaximum_Suppression(dst, dir, gradient);

    return TRUE;
}

double FCW_GetObjDist(double pixel)
{
    // d = W * f / w 
    return (VehicleWidth * (EFL / 2.0)) / (pixel * PixelSize);
}

void FCW_ConvertYUV2RGB(int y, int u, int v, uint8_t* r, uint8_t* g, uint8_t* b)
{
    *r = clip_uint8(1.164 * (y - 16) + 1.596 * (v - 128));
    *g = clip_uint8(1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128));
    *b = clip_uint8(1.164 * (y - 16) + 2.018 * (u - 128));
}

// refer to https://www.rapidtables.com/convert/color/rgb-to-hsv.html
void FCW_ConvertRGB2HSV(uint8_t r, uint8_t g, uint8_t b, double* h, double* s, double* v)
{
    double nr, ng, nb; 
    double max, min;
    double delta;
    
    // normalized to 0 ~ 1
    nr = r / 255.0;
    ng = g / 255.0;
    nb = b / 255.0;

    max = max3(nr, ng, nb);
    min = min3(nr, ng, nb);

    delta = max - min;
    *v = max;

    if (delta > 0) {
        if (max)
            *s = delta / max;
        else
            *s = 0;

        if (max == nr)
            *h = 60 * (fmod(((ng - nb) / delta), 6));
        else if (max == ng)
            *h = 60 * (((nb - nr) / delta) + 2);
        else if (max == nb)
            *h = 60 * (((nr - ng) / delta) + 4);

        if (*h < 0.0)
            *h += 360.0;
    } else {
        *h = *s = 0.0;
    }
}

// sRGB8 -> sRGB' -> sRGB -> XYZ
// refer to https://en.wikipedia.org/wiki/SRGB#The_reverse_transformation &
//          http://davengrace.com/dave/cspace/ and http://colorizer.org/ for checking conversion
//
void FCW_ConvertRGB2XYZ(uint8_t r, uint8_t g, uint8_t b, double* x, double* y, double* z)
{
    double nlr, nlg, nlb;   //sRGB', non-linear rgb.(divided by 255.0)
    double lr, lg, lb;      //sRGB,  linear sRGB

    // R
    nlr = (double) r / (double)255;

    if (nlr > 0.04045)
        lr = pow((nlr + 0.055) / (1 + 0.055), 2.4);
    else 
        lr = nlr / 12.92;

    // G
    nlg = (double) g / (double)255;

    if (nlg > 0.04045)
        lg = pow((nlg + 0.055) / (1 + 0.055), 2.4);
    else 
        lg = nlg / 12.92;

    // B
    nlb = (double) b / (double)255;

    if (nlb > 0.04045)
        lb = pow((nlb + 0.055) / (1 + 0.055), 2.4);
    else 
        lb = nlb / 12.92;

//    fcwsdbg("RGB(%u,%u,%u), non-linear sRGB(%lf, %lf, %lf), linear sRGB(%lf, %lf, %lf)",
//            r, g, b,
//            nlr, nlg, nlb,
//            lr, lg, lb);

    *x = 0.412453 * lr + 0.357580 * lg + 0.180423 * lb;
    *y = 0.212671 * lr + 0.715160 * lg + 0.072169 * lb;
    *z = 0.019334 * lr + 0.119193 * lg + 0.950227 * lb;
}

#define param_13    1.0 / 3.0
#define param_16116 16.0 / 116.0
#define Xn          0.950456
#define Yn          1.0
#define Zn          1.088754

void FCW_ConvertXYZ2Lab(double x, double y, double z, double* l, double* a, double* b)
{
    double fx, fy, fz;

    x /= Xn;
    y /= Yn;
    z /= Zn;

    if (y > 0.008856) {
        fy = pow(y, param_13); 
        *l = 116.0 * fy - 16.0;
    } else {
        fy = 7.787 * y + param_16116;
        *l = 903.3 * y;
    }

    *l = *l > 0. ? *l : 0.;

    if (x > 0.008856)
        fx = pow(x, param_13);
    else
        fx = 7.787 * x + param_16116;

    if (z > 0.008856)
        fz = pow(z, param_13);
    else
        fz = 7.787 * z + param_16116;

    *a = 500 * (fx - fy);
    *b = 200 * (fy - fz);
}

BOOL FCW_ConvertIYUV2RGB(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* rgb[3])
{
    BOOL ret = FALSE;
    int y, u, v;
    uint32_t rr, cc;
    uint8_t r, g, b;
    const Candidate *cur_vc = vc_tracker;

    if (!imgy || !imgu || !imgv || !rgb) {
        fcwsdbg();
        return ret;
    }

    ret = TRUE;

    if (night_mode == TRUE) {
        for (rr=0 ; rr<imgy->size1 ; ++rr) {
            for (cc=0 ; cc<imgy->size2 ; ++cc) {
                y = gsl_matrix_get(imgy, rr, cc);
                u = gsl_matrix_get(imgu, rr/2, cc/2);
                v = gsl_matrix_get(imgv, rr/2, cc/2);

                FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);

                gsl_matrix_set(rgb[0], rr, cc, r);
                gsl_matrix_set(rgb[1], rr, cc, g);
                gsl_matrix_set(rgb[2], rr, cc, b);
            }
        }
    } else {
        while (cur_vc) {
            if (cur_vc->m_valid == TRUE) {
                for (rr=cur_vc->m_r ; rr<cur_vc->m_r + cur_vc->m_h ; ++rr) {
                    for (cc=cur_vc->m_c ; cc<cur_vc->m_c + cur_vc->m_w ; ++cc) {
                        y = gsl_matrix_get(imgy, rr, cc);
                        u = gsl_matrix_get(imgu, rr/2, cc/2);
                        v = gsl_matrix_get(imgv, rr/2, cc/2);

                        FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);

                        gsl_matrix_set(rgb[0], rr, cc, r);
                        gsl_matrix_set(rgb[1], rr, cc, g);
                        gsl_matrix_set(rgb[2], rr, cc, b);
                    }
                }
            }

            cur_vc = cur_vc->m_next;
        }
    } 

    return ret;
}

//#define DUMP_RAW 
BOOL FCW_ConvertIYUV2HSV(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* hsv[3])
{
    BOOL ret = FALSE;
    int y, u, v;
    uint32_t rr, cc;
    uint8_t r, g, b;
    double hsv_h, hsv_s, hsv_v;
    const Candidate *cur_vc = vc_tracker;

#ifdef DUMP_RAW
    uint8_t *dst = NULL, *rgb = NULL;
    FILE *fp = NULL;
    char fn[512];
    static int cnt = 0;
#endif

    if (!imgy || !imgu || !imgv || !hsv) {
        fcwsdbg();
        return ret;
    }

#ifdef DUMP_RAW
    dst = (uint8_t*)malloc(sizeof(uint8_t)* width * height * 3);

    if (!dst) {
        fcwsdbg();
        return ret;
    }

    rgb = dst;
#endif

    ret = TRUE;

    // Depend on argument hight_mode to convert full image or region within specified vc.
    if (night_mode == TRUE) {
        for (rr=0 ; rr<imgy->size1 ; ++rr) {
            for (cc=0 ; cc<imgy->size2 ; ++cc) {
                y = gsl_matrix_get(imgy, rr, cc);
                u = gsl_matrix_get(imgu, rr/2, cc/2);
                v = gsl_matrix_get(imgv, rr/2, cc/2);

                FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);

                FCW_ConvertRGB2HSV(r, g, b, &hsv_h, &hsv_s, &hsv_v);
#ifdef DUMP_RAW
                *rgb++ = r;
                *rgb++ = g;
                *rgb++ = b;
#endif

                gsl_matrix_set(hsv[0], rr, cc, hsv_h);
                gsl_matrix_set(hsv[1], rr, cc, hsv_s);
                gsl_matrix_set(hsv[2], rr, cc, hsv_v);
            }
        }
    } else {
        while (cur_vc) {
            if (cur_vc->m_valid == TRUE) {
                for (rr=cur_vc->m_r ; rr<cur_vc->m_r + cur_vc->m_h ; ++rr) {
                    for (cc=cur_vc->m_c ; cc<cur_vc->m_c + cur_vc->m_w ; ++cc) {
                        y = gsl_matrix_get(imgy, rr, cc);
                        u = gsl_matrix_get(imgu, rr/2, cc/2);
                        v = gsl_matrix_get(imgv, rr/2, cc/2);

                        FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);

                        FCW_ConvertRGB2HSV(r, g, b, &hsv_h, &hsv_s, &hsv_v);

#ifdef DUMP_RAW
                        *rgb++ = r;
                        *rgb++ = g;
                        *rgb++ = b;
#endif

                        gsl_matrix_set(hsv[0], rr, cc, hsv_h);
                        gsl_matrix_set(hsv[1], rr, cc, hsv_s);
                        gsl_matrix_set(hsv[2], rr, cc, hsv_v);
                    }
                }
            }

            cur_vc = cur_vc->m_next;
        }
    }

#ifdef DUMP_RAW
    snprintf(fn, sizeof(fn), "rgb/rgb%03d.rgb", cnt++);
    fp = fopen(fn, "w");
    if (fp) {
        fwrite(dst, 1, imgy->size1 * imgy->size2 * 3, fp);
        fclose(fp);
    }

    free (dst);
#endif

    return ret;
}

BOOL FCW_ConvertIYUV2Lab(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* lab[3])
{
    BOOL ret = FALSE;
    int y, u, v;
    uint32_t rr, cc;
    uint8_t r, g, b;
    double X, Y, Z;
    double L, A, B;
    const Candidate *cur_vc = vc_tracker;

    if (!imgy || !imgu || !imgv || !lab) {
        fcwsdbg();
        return ret;
    }

    ret = TRUE;
    
    if (night_mode == TRUE) {
        for (rr=0 ; rr<imgy->size1 ; ++rr) {
            for (cc=0 ; cc<imgy->size2 ; ++cc) {
                y = gsl_matrix_get(imgy, rr, cc);
                u = gsl_matrix_get(imgu, rr/2, cc/2);
                v = gsl_matrix_get(imgv, rr/2, cc/2);

                FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);
                FCW_ConvertRGB2XYZ(r, g, b, &X, &Y, &Z);
                FCW_ConvertXYZ2Lab(X, Y, Z, &L, &A, &B);

                gsl_matrix_set(lab[0], rr, cc, L);
                gsl_matrix_set(lab[1], rr, cc, A);
                gsl_matrix_set(lab[2], rr, cc, B);
            }
        }
    } else {
        while (cur_vc) {
            if (cur_vc->m_valid == TRUE) {
                for (rr=cur_vc->m_r ; rr<cur_vc->m_r + cur_vc->m_h ; ++rr) {
                    for (cc=cur_vc->m_c ; cc<cur_vc->m_c + cur_vc->m_w ; ++cc) {
                        y = gsl_matrix_get(imgy, rr, cc);
                        u = gsl_matrix_get(imgu, rr/2, cc/2);
                        v = gsl_matrix_get(imgv, rr/2, cc/2);

                        FCW_ConvertYUV2RGB(y, u, v, &r, &g, &b);
                        FCW_ConvertRGB2XYZ(r, g, b, &X, &Y, &Z);
                        FCW_ConvertXYZ2Lab(X, Y, Z, &L, &A, &B);

                        //fcwsdbg("(%u, %u, %u), (%.04lf, %.04lf, %.04lf), (%.02lf, %.02lf, %.02lf)",
                        //        r, g, b,
                        //        X, Y, Z,
                        //        L, A, B);

                        gsl_matrix_set(lab[0], rr, cc, L);
                        gsl_matrix_set(lab[1], rr, cc, A);
                        gsl_matrix_set(lab[2], rr, cc, B);
                    }
                }
            }

            cur_vc = cur_vc->m_next;
        }
    }

    return ret;
}

BOOL FCW_SegmentTaillightByHSV(
        const gsl_matrix* src_y, 
        const gsl_matrix* src_u, 
        const gsl_matrix* src_v, 
        gsl_matrix* dst_y, 
        gsl_matrix* dst_u, 
        gsl_matrix* dst_v, 
        const gsl_matrix* hsv[3], 
        const Candidate* vc_tracker, 
        double hue_th1, 
        double hue_th2, 
        double sat_th,
        double val_th)
{
    uint32_t r, c;
    const gsl_matrix *hue = NULL, *sat = NULL, *val = NULL;
    const Candidate* cur_vc = vc_tracker;

    hue = hsv[0];
    sat = hsv[1];
    val = hsv[2];

    if (!src_y || !src_u || !src_v || 
        !dst_y || !dst_u || !dst_v ||
        !hsv || !hue || !sat || !val) {
        fcwsdbg();
        return FALSE;
    }

    // Clear destnation yuv .
    gsl_matrix_set_all(dst_y, 0);
    gsl_matrix_set_all(dst_u, 128);
    gsl_matrix_set_all(dst_v, 128);

    while (cur_vc) {
        if (cur_vc->m_valid == TRUE) {
            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r) {
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c) {
//                    fcwsdbg("[%d,%d], %lf, %lf",
//                            r, c,
//                            gsl_matrix_get(hue, r, c),
//                            gsl_matrix_get(intensity, r, c));


                    if (gsl_matrix_get(val, r, c) > val_th && 
                        gsl_matrix_get(sat, r, c) > sat_th &&
                        (gsl_matrix_get(hue, r, c) <  hue_th1 || gsl_matrix_get(hue, r, c) > hue_th2)) {
                        //gsl_matrix_set(dst_y, r, c, 255);
                        gsl_matrix_set(dst_y, r, c, gsl_matrix_get(src_y, r, c));
                        gsl_matrix_set(dst_u, r/2, c/2, gsl_matrix_get(src_u, r/2, c/2));
                        gsl_matrix_set(dst_v, r/2, c/2, gsl_matrix_get(src_v, r/2, c/2));
                    } else {
                        gsl_matrix_set(dst_y, r, c, 0);
                    }
                }
            }
        }

        cur_vc = cur_vc->m_next;
    }

    return TRUE;
}

BOOL FCW_SegmentTaillightByRGB(
        const gsl_matrix* src_y, 
        const gsl_matrix* src_u, 
        const gsl_matrix* src_v, 
        gsl_matrix* dst_y, 
        gsl_matrix* dst_u, 
        gsl_matrix* dst_v, 
        const gsl_matrix* rgb[3], 
        const Candidate* vc_tracker, 
        double r_th,
        double rb_th
        )
{
    uint32_t r, c;
    const gsl_matrix *rm = NULL, *gm = NULL , *bm = NULL;   // Xmatrix
    gsl_matrix *rb_map = NULL, *rg_map = NULL;
    const Candidate* cur_vc = vc_tracker;
    double max, min;
    double delta;
    uint32_t pix_cnt = 0;

    rm = rgb[0];
    gm = rgb[1];
    bm = rgb[2];

    if (!src_y || !src_u || !src_v ||
        !dst_y || !dst_u || !dst_v ||
        !rgb || !rm || !gm || !bm) {
        fcwsdbg();
        return FALSE;
    }

    // Clear destnation yuv .
    gsl_matrix_set_all(dst_y, 0);
    gsl_matrix_set_all(dst_u, 128);
    gsl_matrix_set_all(dst_v, 128);

    while (cur_vc) { 
        if (cur_vc->m_valid == TRUE) { 
            CheckOrReallocMatrix(&rb_map, rm->size1, rm->size2, TRUE); 
            CheckOrReallocMatrix(&rg_map, rm->size1, rm->size2, TRUE);

            if (!rb_map) {
                fcwsdbg();
                return FALSE;
            }

            // Get matrix of subtracting r with b.
            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r) {
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c) {
                    gsl_matrix_set(rb_map, r, c, gsl_matrix_get(rm, r, c) - gsl_matrix_get(bm, r, c));
                    gsl_matrix_set(rg_map, r, c, gsl_matrix_get(rm, r, c) - gsl_matrix_get(gm, r, c));
                }
            }

            max = gsl_matrix_max(rb_map);
            min = gsl_matrix_min(rb_map);
            delta = max - min;

            //fcwsdbg("%.03lf, %.03lf", max, min);

            // Normalize to 0~255
            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r) 
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c) 
                    gsl_matrix_set(rb_map, r, c,
                            (gsl_matrix_get(rb_map, r, c) * 255 / delta));

            max = gsl_matrix_max(rg_map);
            min = gsl_matrix_min(rg_map);
            delta = max - min;

            //fcwsdbg("%.03lf, %.03lf", max, min);

            // Normalize to 0~255
            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r)
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c)
                    gsl_matrix_set(rg_map, r, c,
                            (gsl_matrix_get(rg_map, r, c) * 255 / delta));

            pix_cnt = 0;

            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r) {
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c) {
                    if (gsl_matrix_get(rg_map, r, c) > 128)// &&
                        //gsl_matrix_get(rb_map, r, c) > rb_th) 
                    {
                    //if (gsl_matrix_get(rm, r, c) > r_th && gsl_matrix_get(rb_map, r, c) > rb_th) {
                        gsl_matrix_set(dst_y, r, c, gsl_matrix_get(src_y, r, c));
                        //gsl_matrix_set(dst_y, r, c, 255);
                        gsl_matrix_set(dst_u, r/2, c/2, gsl_matrix_get(src_u, r/2, c/2));
                        gsl_matrix_set(dst_v, r/2, c/2, gsl_matrix_get(src_v, r/2, c/2));
                        ++pix_cnt;
                    } else {
                        gsl_matrix_set(dst_y, r, c, 0);
                    }
                }
            }

            fcwsdbg("vc[%d]: %d", cur_vc->m_id, pix_cnt);
        }

        cur_vc = cur_vc->m_next;
    }

    FreeMatrix(&rb_map);
    FreeMatrix(&rg_map);

    return TRUE;
}

BOOL FCW_SegmentTaillightByLab(
        const gsl_matrix* src_y, 
        const gsl_matrix* src_u, 
        const gsl_matrix* src_v, 
        gsl_matrix* dst_y, 
        gsl_matrix* dst_u, 
        gsl_matrix* dst_v, 
        const gsl_matrix* lab[3],
        const Candidate* vc_tracker
        )
{
    uint32_t r, c;
    const gsl_matrix *lm = NULL, *am = NULL, *bm = NULL;
    const Candidate* cur_vc = vc_tracker;

    lm = lab[0];
    am = lab[1];
    bm = lab[2];

    if (!src_y || !src_u || !src_v ||
        !dst_y || !dst_u || !dst_v ||
        !lab || !lm || !am || !bm) {
        fcwsdbg();
        return FALSE;
    }

    // Clear destnation yuv .
    gsl_matrix_set_all(dst_y, 0);
    gsl_matrix_set_all(dst_u, 128);
    gsl_matrix_set_all(dst_v, 128);

    while (cur_vc) {
        if (cur_vc->m_valid == TRUE) {
            for (r=cur_vc->m_r ; r<cur_vc->m_r + cur_vc->m_h ; ++r) {
                for (c=cur_vc->m_c ; c<cur_vc->m_c + cur_vc->m_w ; ++c) {

                    if (gsl_matrix_get(am, r, c) > 15) {
                        //gsl_matrix_set(dst_y, r, c, 255);
                        gsl_matrix_set(dst_y, r, c, gsl_matrix_get(src_y, r, c));
                        gsl_matrix_set(dst_u, r/2, c/2, gsl_matrix_get(src_u, r/2, c/2));
                        gsl_matrix_set(dst_v, r/2, c/2, gsl_matrix_get(src_v, r/2, c/2));
                    } else {
                        gsl_matrix_set(dst_y, r, c, 0);
                    }
                }
            }
        }

        cur_vc = cur_vc->m_next;
    }

    return TRUE;
}

