#ifndef __FCWS_H__
#define __FCWS_H__

#include "common.h"
#include "candidate.h"

#define NOT_SHADOW   255

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
    bool valid;
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
    struct {
        int r;
        int c;
    }point[ROI_TOTAL];
    int size;
}roi_t;

bool FCW_Init(void);

bool FCW_DeInit(void);

bool FCW_PixelInROI(uint32_t r, uint32_t c, const roi_t* roi);

bool FCW_DoDetection(
        uint8_t* img, 
        int linesize, 
        int w, 
        int h, 
        gsl_vector* vertical_hist, 
        gsl_vector* hori_hist, 
        gsl_vector* grayscale_hist, 
        VehicleCandidates *vcs,
        uint8_t* vedge,
        uint8_t* shadow,
        uint8_t* heatmap,
        const roi_t* roi);

bool FCW_Thresholding(gsl_matrix* src, gsl_matrix* dst, gsl_vector* grayscale_hist);

int  FCW_GetRounded_Direction(int gx, int gy);

bool FCW_NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src);

bool FCW_GaussianBlur(gsl_matrix* dst, const gsl_matrix* src);

bool FCW_CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);

uint8_t FCW_OtsuThreshold(gsl_vector* grayscale_hist, int pixel_count);

bool FCW_CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);

bool FCW_CalVerticalHist2(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist);

bool FCW_CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);

bool FCW_CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int direction, int crop_r, int crop_c, int crop_w, int crop_h);

bool FCW_BlobFindIdentical(blob** bhead, blob* nblob);

bool FCW_BlobRearrange(blob** bhead);

bool FCW_BlobAdd(blob** bhead, blob* blob);

void FCW_BlobClear(blob** bhead);

bool FCW_BlobGenerator(const gsl_matrix* imgy, uint32_t peak_idx, blob** bhead);

bool FCW_VehicleCandidateGenerate(
        const gsl_matrix* imgy, 
        const gsl_matrix* edged_imgy, 
        const gsl_vector* horizontal_hist, 
        VehicleCandidates* vcs);

bool FCW_UpdateBlobByEdge(const gsl_matrix* imgy, blob*  blob);

bool FCW_CheckBlobByArea(const gsl_matrix* imgy, blob* cur);

bool FCW_CheckBlobByVerticalEdge(const gsl_matrix* edged_imgy, blob* cur);

bool FCW_CheckBlobValid(const gsl_matrix* imgy, const gsl_matrix* edged_imgy, blob* cur);

bool FCW_UpdateVehicleHeatMap(gsl_matrix* heatmap, VehicleCandidates* vcs); 

bool FCW_UpdateVCStatus(gsl_matrix* heatmap, VehicleCandidates* vcs);

bool FCW_EdgeDetection(gsl_matrix* src, gsl_matrix* dst, gsl_matrix_ushort* gradient, gsl_matrix_char* dir, int direction);

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
//        bool DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist, Candidates& vcs);
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
//        bool NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src);
//
//        bool DoubleThreshold(int low, int high, gsl_matrix* dst, const gsl_matrix* src);
//
//        bool Sobel(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int crop_r = 0, int crop_c = 0, int crop_w = 0, int crop_h = 0);
//
//        bool GaussianBlur(gsl_matrix* dst, const gsl_matrix* src);
//
//        bool EdgeDetect(const gsl_matrix* src, gsl_matrix* temp_buf, gsl_matrix* edged, gsl_matrix_ushort* gradients, gsl_matrix_char* directions);
//
//
//        bool CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);
//
//        bool CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);
//
//        bool CalVerticalHist(const gsl_matrix* imgy, int start_r, int start_c, int w, int h, gsl_vector* vertical_hist);
//
//        bool CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);
//
//        bool CalGradient(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src, int crop_r = 0, int crop_c = 0, int crop_w = 0, int crop_h = 0);
//
//        bool VehicleCandidateGenerate(
//                const gsl_matrix* imgy, 
//                const gsl_matrix* edged_imgy, 
//                const gsl_vector* horizontal_hist, 
//                gsl_vector* vertical_hist, 
//                const gsl_matrix_ushort* gradient,
//                const gsl_matrix_char* direction,
//                Candidates& vcs);
//
//        bool UpdateVehicleCanidateByEdge(
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
