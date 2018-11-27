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
        bool            m_gradient_init;
        gsl_matrix_view m_dx;
        gsl_matrix_view m_dy;
        gsl_vector_view m_dx1d;
        gsl_vector_view m_dy1d;

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

        bool Sobel(gsl_matrix_ushort* dst, 
                gsl_matrix_char* dir, 
                gsl_matrix* src,
                int direction, 
                int crop_r, 
                int crop_c, 
                int crop_w, 
                int crop_h);
    public:
        CImgProc();

        ~CImgProc();

        bool Init();

        //bool EdgeDetect(gsl_matrix* src, gsl_matrix



        bool CopyMatrix(uint8_t* src, gsl_matrix* dst, int w, int h, int linesize);

        bool CopyBackMarix(gsl_matrix* src, uint8_t* dst, int w, int h, int linesize);

};
#endif
