#include "fcws.h"

CFCWS::CFCWS()
{
}

CFCWS::~CFCWS()
{
    if (m_imgy) {
        gsl_matrix_free(m_imgy);
        m_imgy = NULL;
    }

}

bool CFCWS::DoDetection(uint8_t* img, int w, int h, gsl_vector* vertical_hist, gsl_vector* hori_hist, gsl_vector* grayscale_hist)
{
    if (!img || !vertical_hist || !hori_hist || !grayscale_hist) {
        dbg();
        return false;
    }

    if (!m_imgy || m_imgy->size1 != h || m_imgy->size2 != w) {
        if (m_imgy) {
            gsl_matrix_free(m_imgy);
            m_imgy = NULL;
        }

        if ((m_imgy = gsl_matrix_alloc(h, w)) == NULL)
            return false;
    }

    gsl_matrix_set_zero(m_imgy);

    // Copy image array to image matrix
    for (int r=0 ; r<m_imgy->size1 ; r++)
        for (int c=0 ; c<m_imgy->size2 ; c++)
            gsl_matrix_set(m_imgy, r, c, img[r * m_imgy->size2 + c]); 


    CalVertialHist(m_imgy, vertical_hist);
    CalGrayscaleHist(m_imgy, grayscale_hist);




    // Copy image matrix to image array
    for (int r=0 ; r<m_imgy->size1 ; r++)
        for (int c=0 ; c<m_imgy->size2 ; c++)
            img[r * m_imgy->size2 + c] = (uint8_t)gsl_matrix_get(m_imgy, r, c); 



    return true;
}

//------------------------------------------------------------------------------------
bool CFCWS::EdgeDetect(const gsl_matrix* src, gsl_matrix* edged, int threshold, int grandient, double* dir, int double_edge)
{

}

bool CFCWS::CalVertialHist(const gsl_matrix* imgy, gsl_vector* vertical_hist)
{
    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }
















    return true;
}
bool CFCWS::CalGrayscaleHist(const gsl_matrix* imgy, gsl_vector* grayscale_hist)
{
    if (!imgy || !grayscale_hist) {
        dbg();
        return false;
    }

    gsl_vector_set_zero(grayscale_hist);

    for (int r=0 ; r<imgy->size1 ; r++)
    {
        for (int c=0 ; c<imgy->size2 ; c++)
        {
            uint8_t pixel_val = (uint8_t)gsl_matrix_get(imgy, r, c);
            double val = gsl_vector_get(grayscale_hist, pixel_val);
            //dbg("(%d,%d) = %d, val %d", r, c, pixel_val, (int)val);
            gsl_vector_set(grayscale_hist, pixel_val, ++val);
        }
    }

    return true;
}
