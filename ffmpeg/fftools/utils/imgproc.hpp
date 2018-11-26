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
        
        // Gradient
        bool            m_gradient_init;
        gsl_matrix_view m_dx;
        gsl_matrix_view m_dy;
        gsl_vector_view m_dx1d;
        gsl_vector_view m_dy1d;

    protected:
        // Gaussian Blur
        bool InitGB(int size = 5);   
        
        bool DeinitGB();

        // Gradient
        bool InitGradient();

    public:
        CImgProc();

        ~CImgProc();

        bool Init();

        // Gaussian Blur
        bool GaussianBlue(gsl_matrix* src, gsl_matrix* dst);

        //bool EdgeDetect(gsl_matrix* src, gsl_matrix
};
#endif
