#include "fcws.h"

static double gk_5by5[] = {
    2,  4,  5,  4, 2,
    4,  9, 12,  9, 4,
    5, 12, 15, 12, 5,
    4,  9, 12,  9, 4,
    2,  4,  5,  4, 2,
};

CFCWS::CFCWS()
{
    m_imgy = m_edged_imgy = m_temp_imgy = NULL;

    m_gaussian_kernel = gsl_matrix_view_array(gk_5by5, 5, 5);    
    
}

CFCWS::~CFCWS()
{
    if (m_imgy) {
        gsl_matrix_free(m_imgy);
        m_imgy = NULL;
    }

    if (m_edged_imgy) {
        gsl_matrix_free(m_edged_imgy);
        m_edged_imgy = NULL;
    }

    if (m_temp_imgy) {
        gsl_matrix_free(m_temp_imgy);
        m_temp_imgy = NULL;
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

    if (!m_edged_imgy || m_edged_imgy->size1 != h || m_edged_imgy->size2 != w) {
        if (m_edged_imgy) {
            gsl_matrix_free(m_edged_imgy);
            m_edged_imgy = NULL;
        }

        if ((m_edged_imgy = gsl_matrix_alloc(h, w)) == NULL)
            return false;
    }

    if (!m_temp_imgy || m_temp_imgy->size1 != h || m_temp_imgy->size2 != w) {
        if (m_temp_imgy) {
            gsl_matrix_free(m_temp_imgy);
            m_temp_imgy = NULL;
        }

        if ((m_temp_imgy = gsl_matrix_alloc(h, w)) == NULL)
            return false;
    }

    gsl_matrix_set_zero(m_imgy);
    gsl_matrix_set_zero(m_temp_imgy);

    // Copy image array to image matrix
    for (int r=0 ; r<m_imgy->size1 ; r++)
        for (int c=0 ; c<m_imgy->size2 ; c++)
            gsl_matrix_set(m_imgy, r, c, img[r * m_imgy->size2 + c]); 

    gsl_matrix_memcpy(m_temp_imgy, m_imgy);

    GaussianBlur(m_imgy, m_edged_imgy);

    CalGrayscaleHist(m_imgy, m_temp_imgy, grayscale_hist);

    CalVerticalHist(m_temp_imgy, vertical_hist);

    CalHorizontalHist(m_temp_imgy, hori_hist);


    // Copy image matrix to image array
    for (int r=0 ; r<m_imgy->size1 ; r++)
        for (int c=0 ; c<m_imgy->size2 ; c++)
            img[r * m_imgy->size2 + c] = (uint8_t)gsl_matrix_get(m_temp_imgy, r, c); 



    return true;
}

//------------------------------------------------------------------------------------
bool CFCWS::GaussianBlur(const gsl_matrix* src, gsl_matrix* dst)
{
    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        dbg();
        return false;
    }

    if (m_gaussian_kernel.matrix.size1 != m_gaussian_kernel.matrix.size2) {
        dbg();
        return false;
    }

    uint32_t r, c, size;
    uint32_t row, col;
    gsl_matrix_view submatrix_src, submatrix_dst;
    gsl_vector_view row_src, row_dst;

    row = src->size1;
    col = src->size2;
    size = m_gaussian_kernel.matrix.size1;

    gsl_matrix_set_zero(dst);

    // copy first 2 rows 
    for (r=0 ; r<(size/2) ; r++) {
        row_src = gsl_matrix_row((gsl_matrix*)src, r);
        row_dst = gsl_matrix_row(dst, r);
        gsl_vector_memcpy(&row_dst.vector, &row_src.vector);
    }

    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            submatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, r - (size/2), c - (size/2), size, size); 
            submatrix_dst = gsl_matrix_submatrix((gsl_matrix*)dst, r - (size/2), c - (size/2), size, size); 
            // C= a*op(A)*op(B) + b*C
            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &submatrix_src.matrix, &m_gaussian_kernel.matrix, 0.0, &submatrix_dst.matrix); 
        }
    }

    // copy last 2 rows 
    for (r=(size/2) ; r>0 ; r--) {
        row_src = gsl_matrix_row((gsl_matrix*)src, src->size1 - r);
        row_dst = gsl_matrix_row(dst, dst->size1 - r);
        gsl_vector_memcpy(&row_dst.vector, &row_src.vector);
    }

    return true;
}

bool CFCWS::EdgeDetect(const gsl_matrix* src, gsl_matrix* edged, int threshold, int grandient, double* dir, int double_edge)
{
//    GaussianBlur(src, edged);



}

bool CFCWS::CalGrayscaleHist(const gsl_matrix* imgy, gsl_matrix* result_imgy, gsl_vector* grayscale_hist)
{
    if (!imgy || !result_imgy || !grayscale_hist) {
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

    int cutoff_pixel_value = 70;
    int pixel_value_peak = 0;
    double hist_peak = 0;

    for (int i=0 ; i<cutoff_pixel_value ; i++)
    {
        if (gsl_vector_get(grayscale_hist, i) > hist_peak)
        {
            hist_peak = gsl_vector_get(grayscale_hist, i);
            pixel_value_peak = i;
        }
    }
            
    //dbg("%d %lf\n", pixel_value_peak, hist_peak);

    for (int r=0 ; r<imgy->size1 ; r++)
    {
        for (int c=0 ; c<imgy->size2 ; c++)
        {
            if (gsl_matrix_get(result_imgy, r, c) > pixel_value_peak)
                gsl_matrix_set(result_imgy, r, c, 255.0);
        }
    }

    return true;
}

bool CFCWS::CalVerticalHist(const gsl_matrix* imgy, gsl_vector* vertical_hist)
{
    if (!imgy || !vertical_hist) {
        dbg();
        return false;
    }

    double val = 0;
    gsl_vector_view column_view;

    gsl_vector_set_zero(vertical_hist);

    for (int c=0 ; c<imgy->size2 ; c++)
    {
        column_view = gsl_matrix_column((gsl_matrix*)imgy, c);

        for (int r=0 ; r<column_view.vector.size ; r++)
        {
            if (gsl_vector_get(&column_view.vector, r) != 255.0)
            {
                val = gsl_vector_get(vertical_hist, c);
                gsl_vector_set(vertical_hist, c, ++val);
            }
        }
    }

    return true;
}

bool CFCWS::CalHorizontalHist(const gsl_matrix* imgy, gsl_vector* horizontal_hist)
{
    if (!imgy || !horizontal_hist) {
        dbg();
        return false;
    }

    double val = 0;
    gsl_vector_view row_view;

    gsl_vector_set_zero(horizontal_hist);

    for (int r=0 ; r<imgy->size1 ; r++)
    {
        row_view = gsl_matrix_row((gsl_matrix*)imgy, r);

        for (int c=0 ; c<row_view.vector.size ; c++)
        {
            if (gsl_vector_get(&row_view.vector, c) != 255)
            {
                val = gsl_vector_get(horizontal_hist, r);
                gsl_vector_set(horizontal_hist, r, ++val);
            }
        }
    }

    return true;
}
