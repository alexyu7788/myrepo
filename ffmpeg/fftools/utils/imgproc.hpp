#ifndef __IMGPROC_HPP__
#define __IMGPROC_HPP__

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#include "ma.h"
}
#endif

#define WHITE   255
#define BLACK   0

class CImgProc {
    protected:
        // Gaussian Blur
        BOOL            m_gb_init;
        double          m_gk_weight;
        gsl_matrix_view m_gk; //Gaussian kernel
        gsl_matrix*     m_gb_src;
        gsl_matrix*     m_gb_dst;
        
        // Sobel
        BOOL                m_sobel_init;
        gsl_matrix_view     m_dx;
        gsl_matrix_view     m_dy;
        gsl_vector_view     m_dx1d;
        gsl_vector_view     m_dy1d;
        gsl_matrix*         m_gradient;
        gsl_matrix_char*    m_dir;

    protected:
        // Gaussian Blur
        BOOL InitGB(int size = 5);   
        
        BOOL DeinitGB();

        BOOL GaussianBlue(const gsl_matrix* src, 
                          gsl_matrix* dst, 
                          int kernel_size);

        // Sobel
        BOOL InitSobel();

        int GetRoundedDirection(int gx, int gy);

        BOOL NonMaximumSuppression(gsl_matrix* dst,
                                   gsl_matrix_char* dir,
                                   gsl_matrix* gradient);

        BOOL Sobel(gsl_matrix* grad, 
                   gsl_matrix_char* dir, 
                   const gsl_matrix* src,
                   int direction, 
                   BOOL double_edge,
                   int crop_r = 0, 
                   int crop_c = 0, 
                   int crop_w = 0, 
                   int crop_h = 0);
    public:
        CImgProc();

        ~CImgProc();

        BOOL Init();

        BOOL EdgeDetectForLDW(const gsl_matrix* src, 
                               gsl_matrix* dst,
                               int threshold,
                               double* dir,
                               int double_edge);

        BOOL EdgeDetectForFCW(const gsl_matrix* src,
                              gsl_matrix* dst,
                              gsl_matrix* gradient,
                              gsl_matrix_char* dir,
                              int direction);

        BOOL CropImage(uint8_t* src, 
                       gsl_matrix* dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize,
                       uint32_t rowoffset);

        BOOL CopyImage(uint8_t* src, 
                       gsl_matrix* dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize);

        BOOL CopyBackImage(gsl_matrix* src, 
                           uint8_t* dst, 
                           uint32_t w, 
                           uint32_t h, 
                           uint32_t linesize, 
                           uint32_t rowoffset = 0);

        BOOL GenIntegralImage(gsl_matrix* src, gsl_matrix* dst);

        BOOL ThresholdingByIntegralImage(gsl_matrix* src, 
                                         gsl_matrix* intimg, 
                                         gsl_matrix* dst, 
                                         uint32_t s, 
                                         float p);

        BOOL CalHorizonProject(const gsl_matrix* const src,
                               gsl_vector* proj);
};
#endif
