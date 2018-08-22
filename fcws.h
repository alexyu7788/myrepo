#ifndef __FCWS_H__
#define __FCWS_H__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include "common.h"

class CFCWS {
    protected:
        gsl_matrix* m_imgy;
        gsl_matrix* m_temp_imgy;

    public:
        CFCWS();

        ~CFCWS();

        bool DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist);









    protected:
        bool EdgeDetect(const gsl_matrix* src, gsl_matrix* edged, int threshold, int grandient, double* dir, int double_edge);


        bool CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist);

        bool CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);

        bool CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist);

};

#endif
