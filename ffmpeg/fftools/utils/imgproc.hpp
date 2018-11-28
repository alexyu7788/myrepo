#ifndef __IMGPROC_HPP__
#define __IMGPROC_HPP__

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#include "ma.h"
}
#endif

class CImgProc {
    protected:
        // Gaussian Blur
        bool            m_gb_init;
        double          m_gk_weight;
        gsl_matrix_view m_gk; //Gaussian kernel
        gsl_matrix*     m_gb_src;
        gsl_matrix*     m_gb_dst;
        
        // Sobel
        bool                m_sobel_init;
        gsl_matrix_view     m_dx;
        gsl_matrix_view     m_dy;
        gsl_vector_view     m_dx1d;
        gsl_vector_view     m_dy1d;
        gsl_matrix_ushort*  m_gradient;
        gsl_matrix_char*    m_dir;

    protected:
        // Gaussian Blur
        bool InitGB(int size = 5);   
        
        bool DeinitGB();

        bool GaussianBlue(gsl_matrix* src, gsl_matrix* dst, int kernel_size);

        // Sobel
        bool InitSobel();

        int GetRoundedDirection(int gx, int gy);

        bool NonMaximumSuppression(gsl_matrix* dst,
                                   gsl_matrix_char* dir,
                                   gsl_matrix_ushort* src);

        bool Sobel(gsl_matrix_ushort* grad, 
                gsl_matrix_char* dir, 
                gsl_matrix* src,
                int direction, 
                int crop_r = 0, 
                int crop_c = 0, 
                int crop_w = 0, 
                int crop_h = 0);
    public:
        CImgProc();

        ~CImgProc();

        bool Init();

        bool EdgeDetectForLDWS(gsl_matrix* src, 
                                gsl_matrix* dst,
                                int linesize,
                                int threshold,
                                double* dir,
                                int double_edge);



        bool CopyMatrix(uint8_t* src, gsl_matrix* dst, int w, int h, int linesize);

        bool CopyBackMarix(gsl_matrix* src, uint8_t* dst, int w, int h, int linesize);

};
#endif