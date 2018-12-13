#ifndef __FCWS_CWRAPPER_H__
#define __FCWS_CWRAPPER_H__

//#include "common.h"
#ifdef __cplusplus
extern "C" {
#include "../utils/ma.h"
}
#endif

#include "../utils/common.hpp"
#include "candidate.h"

#define NOT_SHADOW      255

#define VHW_RATIO       0.6                         // ratio of height / width
#define AR_LB           (3.0 / (4.0 * VHW_RATIO))   // low bound of aspect ratio
#define AR_HB           (5.0 / (4.0 * VHW_RATIO))   // high bound of aspect ratio

#define MAX_PEAK_COUNT  16

typedef struct PEAK {
    uint32_t value;
    uint32_t idx;
    uint32_t vote_cnt;
}PEAK;

typedef struct PEAK_GROUP {
    uint32_t peak_count;
    PEAK peak[MAX_PEAK_COUNT];
}PEAK_GROUP;

typedef struct blob {
    BOOL valid;
    int number;
    int r;
    int c;
    int w;
    int h;
    float density;
    struct blob* next;
}blob;

enum {
    ROI_LEFTTOP = 0,
    ROI_RIGHTTOP,
    ROI_RIGHTBOTTOM,
    ROI_LEFTBOTTOM,
    ROI_TOTAL,
};

typedef struct roi_s {
    point_t point[ROI_TOTAL];
    int size;
}roi_t;

#ifdef __cplusplus
extern "C" {
#endif
BOOL FCW_Init(void);

BOOL FCW_DeInit(void);

BOOL FCW_PixelInROI(uint32_t r, uint32_t c, const roi_t* roi);

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
        VehicleCandidates *vcs,     // result of each frame
        VehicleCandidates *vcs2,    // result of heatmap
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
        );

BOOL FCW_Thresholding(
        gsl_matrix* src, 
        gsl_matrix* dst, 
        gsl_vector* grayscale_hist,
        uint8_t* hist_peak,
        uint8_t* otsu_th,
        uint8_t* final_th
        );

BOOL FCW_ThresholdingByIntegralImage(
        gsl_matrix* src,
        gsl_matrix* intimg,
        gsl_matrix* dst,
        uint32_t s,
        float p
        );

int  FCW_GetRounded_Direction(int gx, int gy);

BOOL FCW_NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src);

BOOL FCW_GaussianBlur(gsl_matrix* dst, const gsl_matrix* src);

BOOL FCW_CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);

uint8_t FCW_OtsuThreshold(gsl_vector* grayscale_hist, int pixel_count);

BOOL FCW_CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);

BOOL FCW_CalVerticalHist2(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist);

BOOL FCW_CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);

BOOL FCW_CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int direction, int crop_r, int crop_c, int crop_w, int crop_h);

BOOL FCW_BlobFindIdentical(blob** bhead, blob* nblob);

BOOL FCW_BlobRearrange(blob** bhead);

BOOL FCW_BlobAdd(blob** bhead, blob* blob);

void FCW_BlobClear(blob** bhead);

BOOL FCW_BlobGenerator(const gsl_matrix* imgy, uint32_t peak_idx, blob** bhead);

BOOL FCW_BlobRemoveLiteralShadow(
        const gsl_matrix* imgy,
        const gsl_matrix* intimg,
        const gsl_matrix* shadow_imgy,
        const blob* bhead);

BOOL FCW_VehicleCandidateGenerate(
        const gsl_matrix* imgy,
        const gsl_matrix* intimg,
        const gsl_matrix* shadow_imgy, 
        const gsl_matrix* edged_imgy, 
        const gsl_vector* horizontal_hist, 
        VehicleCandidates* vcs);

BOOL FCW_UpdateBlobByStrongVEdge(const gsl_matrix* imgy, blob*  blob);

BOOL FCW_CheckBlobByArea(const gsl_matrix* imgy, blob* cur);

BOOL FCW_CheckBlobByVerticalEdge(const gsl_matrix* edged_imgy, blob* cur);

BOOL FCW_CheckBlobAR(blob* blob);

BOOL FCW_CheckBlobValid(const gsl_matrix* imgy, const gsl_matrix* edged_imgy, blob* cur);

BOOL FCW_CheckSymmProperty(const gsl_matrix* imgy, VehicleCandidates* vcs, float th_pairwise, float th_symm);

BOOL FCW_UpdateVehicleHeatMap(gsl_matrix* heatmap, gsl_matrix_char* heatmap_id, VehicleCandidates* vcs); 

BOOL FCW_GetContour(
    const gsl_matrix_char* m,
    char id,
    const point_t* start,
    rect* rect
    );

Candidate* FCW_NewCandidate(void);

BOOL FCW_UpdateVCStatus(
    gsl_matrix* heatmap, 
    gsl_matrix_char* heatmap_id, 
    Candidate** vc_tracker,
    VehicleCandidates* vcs
    );

BOOL FCW_EdgeDetection(gsl_matrix* src, gsl_matrix* dst, gsl_matrix_ushort* gradient, gsl_matrix_char* dir, int direction);

double FCW_GetObjDist(double pixel);

double FCW_GetObjWidth(double objdist);

void FCW_ConvertYUV2RGB(int y, int u, int v, uint8_t* r, uint8_t* g, uint8_t* b);

void FCW_ConvertRGB2HSV(uint8_t r, uint8_t g, uint8_t b, double* h, double* s, double* v);

void FCW_ConvertRGB2XYZ(uint8_t r, uint8_t g, uint8_t b, double* x, double* y, double* z);

void FCW_ConvertXYZ2Lab(double x, double y, double z, double* l, double* a, double* b);

BOOL FCW_ConvertIYUV2RGB(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* rgb[3]);

BOOL FCW_ConvertIYUV2HSV(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* hsv[3]);

BOOL FCW_ConvertIYUV2Lab(
        BOOL night_mode,
        const gsl_matrix* imgy,
        const gsl_matrix* imgu,
        const gsl_matrix* imgv,
        const Candidate* vc_tracker,
        gsl_matrix* lab[3]);

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
        double val_th);

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
        );

BOOL FCW_SegmentTaillightByLab(
        const gsl_matrix* src_y, 
        const gsl_matrix* src_u, 
        const gsl_matrix* src_v, 
        gsl_matrix* dst_y, 
        gsl_matrix* dst_u, 
        gsl_matrix* dst_v, 
        const gsl_matrix* lab[3],
        const Candidate* vc_tracker
        );
#ifdef __cplusplus
}
#endif
//class CFCWS {
//    protected:
//        gsl_matrix*         m_imgy;
//        gsl_matrix*         m_edged_imgy;
//        gsl_matrix*         m_temp_imgy;
//
//        gsl_matrix_ushort*  m_gradient;
//        gsl_matrix_char*    m_direction;
//
//        gsl_matrix_view     m_gk;
//        double              m_gk_weight;
//
//        gsl_matrix_view     m_dx;
//        gsl_matrix_view     m_dy;
//        gsl_vector_view     m_dx1d;
//        gsl_vector_view     m_dy1d;
//
//        Candidates          m_vcs;
//
//        gsl_vector*         m_temp_hori_hist;
//
//    public:
//        CFCWS();
//
//        ~CFCWS();
//
//        BOOL DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist, Candidates& vcs);
//
//
//
//
//
//
//
//
//
//    protected:
//        int  GetRounded_Direction(int gx, int gy);
//
//        int  GetRounded_Direction2(int gx, int gy);
//
//        BOOL NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src);
//
//        BOOL DoubleThreshold(int low, int high, gsl_matrix* dst, const gsl_matrix* src);
//
//        BOOL Sobel(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int crop_r = 0, int crop_c = 0, int crop_w = 0, int crop_h = 0);
//
//        BOOL GaussianBlur(gsl_matrix* dst, const gsl_matrix* src);
//
//        BOOL EdgeDetect(const gsl_matrix* src, gsl_matrix* temp_buf, gsl_matrix* edged, gsl_matrix_ushort* gradients, gsl_matrix_char* directions);
//
//
//        BOOL CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);
//
//        BOOL CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);
//
//        BOOL CalVerticalHist(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist);
//
//        BOOL CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);
//
//        BOOL CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int crop_r = 0, int crop_c = 0, int crop_w = 0, int crop_h = 0);
//
//        BOOL VehicleCandidateGenerate(
//                const gsl_matrix* imgy, 
//                const gsl_matrix* edged_imgy, 
//                const gsl_vector* horizontal_hist, 
//                gsl_vector* vertical_hist, 
//                const gsl_matrix_ushort* gradient,
//                const gsl_matrix_char* direction,
//                Candidates& vcs);
//
//        BOOL UpdateVehicleCanidateByEdge(
//                const gsl_matrix* imgy,
//                const gsl_matrix_ushort* gradient,
//                const gsl_matrix_char* direction,
//                int&  vsr,
//                int&  vsc,
//                int&  vw,
//                int&  vh);
//
//};

#endif
