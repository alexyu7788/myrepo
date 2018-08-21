#ifndef __FCWS_H__
#define __FCWS_H__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include "common.h"

class CFCWS {
    protected:
        gsl_matrix* m_imgy;

    public:
        CFCWS();

        ~CFCWS();

        bool DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist);









    protected:
        bool EdgeDetect(const gsl_matrix* src, gsl_matrix* edged, int threshold, int grandient, double* dir, int double_edge);


        bool CalVertialHist(const gsl_matrix* imgy, gsl_vector* vertical_hist);

        bool CalGrayscaleHist(const gsl_matrix* imgy, gsl_vector* grayscale_hist);
};

#endif
