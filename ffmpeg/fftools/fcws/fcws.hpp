#ifndef _FCWS_H_
#define _FCWS_H_

#include <string>
#include <list>
#include <iterator>
#include "../utils/common.hpp"
#include "../utils/imgproc.hpp"
#include "../ldws/ldws.hpp"

#ifdef __cplusplus
extern "C" {
#include "../utils/ma.h"
#include "fcws_cwrapper.h"
#include "candidate.h"
}
#endif


#define VHW_RATIO       0.6                         // ratio of height / width
#define AR_LB           (3.0 / (4.0 * VHW_RATIO))   // low bound of aspect ratio
#define AR_HB           (5.0 / (4.0 * VHW_RATIO))   // high bound of aspect ratio

#define HeatMapIncrease 10.0
#define HeatMapDecrease 10.0
#define HeatMapAppearThreshold   100 

#define VehicleWidth 1650   //mm
#define EFL          2.7    //mm
#define PixelSize    2.8    //um = 10^-3 mm

using namespace std;

class CFCWS;

typedef enum _CandidateStatus{
    _Disappear = 0,
    _Appear,
}_CandidateStatus;

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

typedef struct blob_s {
    BOOL valid;
    int number;
    int r;
    int c;
    int w;
    int h;
    float density;
}blob_t;

typedef struct candidate_s {
    BOOL     m_updated;
    BOOL     m_valid;
    int      m_id;
    float    m_dist;
    uint32_t m_r;
    uint32_t m_c;
    uint32_t m_w;
    uint32_t m_h;
    _CandidateStatus m_st;
}candidate_t;

//enum {
//    ROI_LEFTTOP = 0,
//    ROI_RIGHTTOP,
//    ROI_RIGHTBOTTOM,
//    ROI_LEFTBOTTOM,
//    ROI_TOTAL,
//};
//
//typedef struct roi_s {
//    point_t point[ROI_TOTAL];
//    int size;
//}roi_t;

class CFCWS {
    protected:
        BOOL                m_terminate;

        // Image Process object
        CImgProc*           m_ip; 

        pthread_mutex_t     m_mutex;
        pthread_cond_t      m_cond; 

        gsl_matrix*         m_imgy;
        gsl_matrix*         m_imgu;
        gsl_matrix*         m_imgv;
        gsl_matrix*         m_vedgeimg;
        gsl_matrix*         m_heatmap;
        gsl_matrix*         m_tempimg;
        gsl_matrix*         m_intimg;       // integral image
        gsl_matrix*         m_shadowimg;

        gsl_matrix*         m_gradient;

        gsl_matrix_char*    m_direction;
        gsl_matrix_char*    m_heatmapid;

        gsl_vector*         m_horizonproject;
        gsl_vector*         m_temp_horizonproject;

        list<blob_t>        m_blobs;   
        list<candidate_t>   m_candidates;
        list<candidate_t>   m_vc_tracker;

    protected:
        BOOL PixelInROI(uint32_t r, uint32_t c, const roi_t* roi);

        BOOL ApplyStaticROI(gsl_matrix* src, const roi_t* roi);

        void BlobDump(string description, list<blob_t>& blobs);

        BOOL BlobConvertToVC(uint32_t cols, list<blob_t>& blobs, list<candidate_t>& cands);

        BOOL BlobRearrange(list<blob_t>& blobs);

        BOOL BlobFindIdentical(list<blob_t>& blobs, 
                               blob_t& temp);

        BOOL BlobGenerate(const gsl_matrix* src,
                          uint32_t peak_idx,
                          list<blob_t>& blobs);

        void VCDump(string description, list<candidate_t>& cands);

        double VCGetDist(double pixel);

        BOOL VCCheckByAR(list<candidate_t>& cands);

        BOOL VCCheckByVerticalEdge(const gsl_matrix* vedgeimg,
                                        list<candidate_t>& cands);

        BOOL VCCheck(const gsl_matrix* imgy,
                          const gsl_matrix* vedgeimg,
                          list<candidate_t>& cands);

        BOOL VCUpdateShapeByStrongVerticalEdge(const gsl_matrix* vedgeimg, list<candidate_t>& cands);

        BOOL VCUpdateHeatMap(gsl_matrix* map,
                             gsl_matrix_char* id,
                             list<candidate_t>& cands);

        BOOL HeatMapUpdateID(const gsl_matrix* heatmap,
                             gsl_matrix_char* heatmap_id,
                             list<candidate_t>::iterator it,
                             char id);

        BOOL HeatMapGetContour(const gsl_matrix_char* m,
                               char  id,
                               const point_t& start,
                               rect& rect);

        BOOL VCTrackerAddOrUpdateExistTarget(const gsl_matrix* heatmap,
                                             gsl_matrix_char* heatmap_id,
                                             list<candidate_t>& tracker,
                                             list<candidate_t>& cands);

        BOOL VCTrackerUpdateExistTarget(const gsl_matrix_char* heatmap_id,
                                        list<candidate_t>& tracker);

        BOOL VCTrackerUpdate(gsl_matrix* heatmap,
                             gsl_matrix_char* heatmap_id,
                             list<candidate_t>& tracker,
                             list<candidate_t>& cands);

        BOOL HypothesisGenerate(const gsl_matrix* imgy,
                                const gsl_matrix* intimg,
                                const gsl_matrix* shadowimg,
                                const gsl_matrix* vedgeimg,
                                const gsl_vector* horizonproject,
                                gsl_vector* temp_horizonproject,
                                gsl_matrix* heatmap,
                                gsl_matrix_char* heatmapid,
                                list<blob_t>& blobs,
                                list<candidate_t>& cands,
                                list<candidate_t>& tracker
                                );

    public:
        CFCWS();

        ~CFCWS();

        BOOL Init();

        BOOL Deinit();

        BOOL DoDetection(uint8_t* imgy,
                         int w,
                         int h,
                         int linesize,
                         CLDWS* ldws_obj,
                         const roi_t* roi
                         );

        BOOL GetInternalData(uint32_t w, 
                             uint32_t h,
                             int linesize,
                             uint8_t* img,
                             uint8_t* shadow,
                             uint8_t* roi,
                             uint8_t* vedge,
                             uint8_t* heatmap,
                             VehicleCandidates* vcs,
                             VehicleCandidates* vcs2
                            );
};
#endif
