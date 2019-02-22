#ifndef __IMGPROC_HPP__
#define __IMGPROC_HPP__

#include "common.hpp"
#include "mop.h"

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

        // Integral Image
        gsl_matrix*         m_intimg;
        gsl_matrix*         m_iitemp;

        //---------------- dlib----------------------------
		uint8_t*			m_dlib_temp;
		unsigned int		m_dlib_temp_size;
		smatrix 			m_dlib_temp2;

        // Sobel
        smatrix             m_dlib_gradient;
        cmatrix             m_dlib_dir;

        // Integral Image
        smatrix             m_dlib_intimg;
        smatrix             m_dlib_iitemp;

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

        //---------------- dlib----------------------------
        // Gaussian Blur
        BOOL GaussianBlur(const smatrix& src,
        				  smatrix& dst,
                          int kernel_size);

        void GaussianBlur(const uint8_t* src_data,
        		          uint8_t* dst_data,
						  int src_w,
						  int src_h,
						  int src_stride,
						  int matrix_size);

        // Sobel
//        BOOL NonMaximumSuppression(gsl_matrix* dst,
//                                   gsl_matrix_char* dir,
//                                   gsl_matrix* gradient);

        BOOL Sobel(smatrix& grad, 
                   cmatrix& dir, 
                   const smatrix& src,
                   int direction, 
                   BOOL double_edge,
                   int crop_r = 0, 
                   int crop_c = 0, 
                   int crop_w = 0, 
                   int crop_h = 0);

        BOOL SobelAndThreshoding(const uint8_t* src_data,
        						 smatrix& dst_data,
								 int src_w,
								 int src_h,
								 int src_stride,
								 int threshold,
								 int grandient,
								 int double_edge);
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

        BOOL GenIntegralImageOfEdgeImage(const gsl_matrix* src, gsl_matrix* dst);

        BOOL ThresholdingByIntegralImage(gsl_matrix* src, 
                                         gsl_matrix* intimg, 
                                         gsl_matrix* dst, 
                                         uint32_t s, 
                                         float p);

        BOOL CalHorizonProject(const gsl_matrix* const src,
                               gsl_vector* proj);
        
        BOOL RemoveNoisyBlob(gsl_matrix* src, 
                             uint32_t outer_window = 5,
                             uint32_t inner_window = 3);
        
        //---------------- dlib----------------------------
        BOOL EdgeDetectForLDW(const smatrix& src, 
                               smatrix& dst,
                               int threshold,
                               double* dir,
                               int double_edge);

        BOOL EdgeDetectForLDW(const uint8_t* src,
        					  smatrix& dst,
							  int src_w,
							  int src_h,
							  int src_stride,
							  int threshold,
							  int direction,
							  int double_edge);

//        BOOL EdgeDetectForFCW(const gsl_matrix* src,
//                              gsl_matrix* dst,
//                              gsl_matrix* gradient,
//                              gsl_matrix_char* dir,
//                              int direction);

        BOOL CropImage(uint8_t* src, 
                       smatrix& dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize,
                       uint32_t rowoffset);

        BOOL CopyImage(uint8_t* src, 
                       smatrix& dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize);

        BOOL CopyBackImage(smatrix& src, 
                           uint8_t* dst, 
                           uint32_t w, 
                           uint32_t h, 
                           uint32_t linesize, 
                           uint32_t rowoffset = 0);

        BOOL GenIntegralImage(smatrix& src, smatrix& dst);

        BOOL GenIntegralImageOfEdgeImage(const smatrix& src, smatrix& dst);

//        BOOL ThresholdingByIntegralImage(gsl_matrix* src,
//                                         gsl_matrix* intimg,
//                                         gsl_matrix* dst,
//                                         uint32_t s,
//                                         float p);
//
//        BOOL CalHorizonProject(const gsl_matrix* const src,
//                               gsl_vector* proj);
        
        BOOL RemoveNoisyBlob(smatrix& src, 
                             uint32_t outer_window = 5,
                             uint32_t inner_window = 3);
};
#endif
