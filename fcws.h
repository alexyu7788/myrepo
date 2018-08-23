#ifndef __FCWS_H__
#define __FCWS_H__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include "common.h"

class CFCWS {
    protected:
        gsl_matrix*         m_imgy;
        gsl_matrix*         m_edged_imgy;
        gsl_matrix*         m_temp_imgy;

        gsl_matrix_ushort*  m_gradient;
        gsl_matrix_char*    m_direction;

        gsl_matrix_view     m_gk;
        double              m_gk_weight;

        gsl_matrix_view     m_dx;
        gsl_matrix_view     m_dy;

    public:
        CFCWS();

        ~CFCWS();

        bool DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist);









    protected:
        int  GetRounded_Direction(int gx, int gy);

        bool NonMaximum_Suppression(gsl_matrix* dst, gsl_matrix_char* dir, gsl_matrix_ushort* src);

        bool DoubleThreshold(int low, int high, gsl_matrix* dst, const gsl_matrix* src);

        bool Sobel(gsl_matrix_ushort* dst, gsl_matrix_char* dir, const gsl_matrix* src);

        bool GaussianBlur(gsl_matrix* dst, const gsl_matrix* src);

        bool EdgeDetect(const gsl_matrix* src, gsl_matrix* temp_buf, gsl_matrix* edged, gsl_matrix_ushort* gradients, gsl_matrix_char* directions);


        bool CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);

        bool CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);

        bool CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);

};

#endif
