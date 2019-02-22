#include "imgproc.hpp"

enum {
    DIRECTION_45UP = 0,
    DIRECTION_45DOWN,
    DIRECTION_HORIZONTAL,
    DIRECTION_VERTICAL,
};

static double gk_3by3[] = {
    1,  2,  1,
    2,  4,  2,
    1,  2,  1,
};

static double gk_5by5[] = {
    2,  4,  5,  4, 2,
    4,  9, 12,  9, 4,
    5, 12, 15, 12, 5,
    4,  9, 12,  9, 4,
    2,  4,  5,  4, 2,
};

static double dx_ary[] = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

static double dx1d_ary[] = {
    -1, 0, 1
};

static double dy_ary[] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

//--------------------------------------------------------------------------
// Gaussian Blur
BOOL CImgProc::InitGB(int size)
{
    uint32_t r, c;

    if (m_gb_init == FALSE) {

        m_gk_weight = 0;

        if (size == 3)
            m_gk = gsl_matrix_view_array(gk_3by3, 3, 3);
        else if (size == 5)
            m_gk = gsl_matrix_view_array(gk_5by5, 5, 5);
        else {
            dbg("GB: size %d is not implemented yet.", size);
            return FALSE;
        }

        for (r=0 ; r<m_gk.matrix.size1 ; ++r) {
            for (c=0 ; c<m_gk.matrix.size2 ; ++c) {
                m_gk_weight += gsl_matrix_get(&m_gk.matrix, r, c);
            }
        }

        m_gb_init = TRUE;
    }

    return TRUE;
}

BOOL CImgProc::DeinitGB()
{
    FreeMatrix(&m_gb_src);
    FreeMatrix(&m_gb_dst);

    m_gb_init = FALSE;

    return TRUE;
}

BOOL CImgProc::GaussianBlue(const gsl_matrix* src, 
                            gsl_matrix* dst, 
                            int kernel_size)
{
    uint32_t r, c, i, j;
    uint32_t size;
    uint32_t row, col;
    double sum = 0.0;
    gsl_matrix_view submatrix_src;

    if (!src || !dst || (src->size1 != dst->size1 || src->size2 != dst->size2)) {
        dbg();
        return FALSE;
    }

    if (InitGB(kernel_size) == FALSE || m_gk.matrix.size1 != m_gk.matrix.size2 || m_gk_weight == 0) {
        dbg();
        return FALSE;
    }

    row = src->size1;
    col = src->size2;
    size = m_gk.matrix.size1;

    gsl_matrix_memcpy(dst, src);

    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            submatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, r - (size/2), c - (size/2), size, size); 

            sum = 0;
            for (i=0 ; i<submatrix_src.matrix.size1 ; i++)
                for (j=0 ; j<submatrix_src.matrix.size2 ; j++)
                    sum += (gsl_matrix_get((const gsl_matrix*)&submatrix_src.matrix, i, j) * gsl_matrix_get((const gsl_matrix*)&m_gk.matrix, i, j));
            
            sum /= m_gk_weight;
            gsl_matrix_set(dst, r, c, sum);
        }
    }

    return TRUE;
}

// Sobel
BOOL CImgProc::InitSobel()
{
    if (m_sobel_init == FALSE) {
        m_dx    = gsl_matrix_view_array(dx_ary, 3, 3);
        m_dy    = gsl_matrix_view_array(dy_ary, 3, 3);
        m_dx1d  = gsl_vector_view_array(dx1d_ary, 3);
        m_dy1d  = gsl_vector_view_array(dx1d_ary, 3);

        m_gradient = NULL;
        m_dir   = NULL;

        m_sobel_init = TRUE;
    }

    return TRUE;
}

int CImgProc::GetRoundedDirection(int gx, int gy)
{
    /* reference angles:
     *   tan( pi/8) = sqrt(2)-1
     *   tan(3pi/8) = sqrt(2)+1
     * Gy/Gx is the tangent of the angle (theta), so Gy/Gx is compared against
     * <ref-angle>, or more simply Gy against <ref-angle>*Gx
     *
     * Gx and Gy bounds = [-1020;1020], using 16-bit arithmetic:
     *   round((sqrt(2)-1) * (1<<16)) =  27146
     *   round((sqrt(2)+1) * (1<<16)) = 158218
     */
    if (gx) {
        int tanpi8gx, tan3pi8gx;

        if (gx < 0)
            gx = -gx, gy = -gy;
        gy <<= 16;
        tanpi8gx  =  27146 * gx;
        tan3pi8gx = 158218 * gx;
        if (gy > -tan3pi8gx && gy < -tanpi8gx)  return DIRECTION_45UP;
        if (gy > -tanpi8gx  && gy <  tanpi8gx)  return DIRECTION_HORIZONTAL;
        if (gy >  tanpi8gx  && gy <  tan3pi8gx) return DIRECTION_45DOWN;
    }

    return DIRECTION_VERTICAL;
}

BOOL CImgProc::NonMaximumSuppression(gsl_matrix* dst,
                                   gsl_matrix_char* dir,
                                   gsl_matrix* gradient)
{
    uint32_t r, c;
    uint32_t row, col;
    uint32_t size;

    if (!gradient || !dst || !dir) {
        dbg();
        return FALSE;
    }

#define COPY_MAXIMA(ay, ax, by, bx) do {                                                    \
    if (gsl_matrix_get(gradient, r, c) > gsl_matrix_get(gradient, r+(ay), c+ax) &&      \
        gsl_matrix_get(gradient, r, c) > gsl_matrix_get(gradient, r+(by), c+bx))        \
        gsl_matrix_set(dst, r, c, (uint8_t)gsl_matrix_get(gradient, r, c));                \
} while (0)

    row = gradient->size1;
    col = gradient->size2;

    for (r=1 ; r < row - 1; r++) {
        for (c=1 ; c < col - 1; c++) {
            switch (gsl_matrix_char_get(dir, r, c)) {
            case DIRECTION_45UP:
                COPY_MAXIMA(1, -1, -1, 1);
                break;
            case DIRECTION_45DOWN:
                COPY_MAXIMA(-1, -1, 1, 1);
                break;
            case DIRECTION_HORIZONTAL:
                COPY_MAXIMA( 0, -1, 0, 1);
                break;
            case DIRECTION_VERTICAL:
                COPY_MAXIMA(-1,  0, 1, 0);
                break;
            }
        }
    }

    return TRUE;
}

BOOL CImgProc::Sobel(gsl_matrix* grad, 
        gsl_matrix_char* dir, 
        const gsl_matrix* src,
        int direction, 
        BOOL double_edge,
        int crop_r, 
        int crop_c, 
        int crop_w, 
        int crop_h)
{
    int cr, cc, cw, ch;
    uint32_t i, j;
    uint32_t r, c, row, col;
    uint32_t size;
    gsl_matrix*     pmatrix;
    gsl_matrix_view submatrix_src;
    gsl_matrix_view cropmatrix_src;

    if (!src || !grad || !dir) {
        dbg();
        return FALSE;
    }

    if (InitSobel() == FALSE) {
        dbg();
        return FALSE;
    }

    if (crop_r && crop_c && crop_w && crop_h) {
        //Check boundary
        if (crop_r < 0)
            cr = 0;
        else if (crop_r > src->size1)
            cr = src->size1 - 1;
        else
            cr = crop_r;

        if (crop_c < 0)
            cc = 0;
        else if (crop_c > src->size2)
            cc = src->size2 - 1;
        else
            cc = crop_c;

        if (crop_r + crop_h >= src->size1)
            ch = src->size1 - crop_r;
        else 
            ch = crop_h;

        if (crop_c + crop_w >= src->size2)
            cw = src->size2 - crop_c;
        else
            cw = crop_w;

        CheckOrReallocMatrix(&grad, ch, cw, TRUE);
        CheckOrReallocMatrixChar(&dir, ch, cw, TRUE);

        cropmatrix_src = gsl_matrix_submatrix((gsl_matrix*)src, cr, cc, ch, cw);
        pmatrix = &cropmatrix_src.matrix;
    } else {
        pmatrix = (gsl_matrix*)src;
    }

    row = pmatrix->size1;
    col = pmatrix->size2;
    size = m_dx.matrix.size1;

    // Convolution operation
    for (r=(size/2) ; r<(row - (size/2)) ; r++) {
        for (c=(size/2) ; c<(col - (size/2)) ; c++) {
            int gx=0, gy=0;

            submatrix_src = gsl_matrix_submatrix(pmatrix, r - (size/2), c - (size/2), size, size); 

            // direction : 
            // 0 : vertical & horizontal edge
            // 1 : vertical edge
            // 2 : horizontal edge
            if (direction != 2) {
                for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                    for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                        gx += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dx.matrix, i, j));
                    }
                }
            }

            if (direction != 1) {
                for (i=0 ; i<submatrix_src.matrix.size1 ; i++) {
                    for (j=0 ; j<submatrix_src.matrix.size2 ; j++) {
                        gy += (int)(gsl_matrix_get(&submatrix_src.matrix, i, j) * gsl_matrix_get(&m_dy.matrix, i, j));
                    }
                }
            }

            //gsl_matrix_ushort_set(grad, r, c, gsl_hypot(gx, gy));
            if (double_edge)
                gsl_matrix_set(grad, r, c, abs(gx) + abs(gy));
            else
                gsl_matrix_set(grad, r, c, gx + gy);

            gsl_matrix_char_set(dir, r, c, GetRoundedDirection(gx, gy));
        }
    }

    return TRUE;
}


//---------------- dlib----------------------------

// Gaussian Blur
BOOL CImgProc::GaussianBlur(const smatrix& src,
							smatrix& dst,
                            int kernel_size)
{
    long r, c;
    long src_nr, src_nc, dst_nr, dst_nc;
    short ret;

    src_nr = src.nr();
    src_nc = src.nc();
    dst_nr = dst.nr();
    dst_nc = dst.nc();

    if (!src_nr || !src_nc || !dst_nr || !dst_nc) {
        dbg();
        return FALSE;
    }

    if (src_nr != dst_nr || src_nc != dst_nc) {
        dbg();
        return FALSE;
    }

    dst = src;

	if (kernel_size == 3) {
		for (r = 1 ; r < src_nr - 1 ; ++r) {
			for (c = 1 ; c < src_nc - 1 ; ++c) {

				ret = ((src.operator()(r-1, c-1) + src.operator()(r+1, c-1))
						   + (src.operator()(r-1,   c) + src.operator()(r+1,   c)) * 2
						   + (src.operator()(r-1, c+1) + src.operator()(r+1, c+1))

						   + src.operator()(  r, c-1) 	* 2
						   + src.operator()(  r,   c) 	* 4
						   + src.operator()(  r, c+1) 	* 2) / 16;

				// < 6%
				 dst.operator()(r, c) = ret;
				// < 9%
			}
		}
	} else if (kernel_size == 5) {

	}

    return TRUE;
}

void CImgProc::GaussianBlur(const uint8_t* src_data,
		                    uint8_t* dst_data,
							int src_w,
							int src_h,
							int src_stride,
							int matrix_size)
{
	int i,j;

	if (matrix_size != 3 && matrix_size != 5)
		matrix_size = 3;

	if (matrix_size == 3)
	{
		memcpy(dst_data, src_data, src_w); dst_data += src_stride; src_data += src_stride;

		for (j = 1; j < src_h - 1; j++)
		{
			dst_data[0] = src_data[0];
			for (i = 1; i < src_w - 1; i++)
			{
				/* Gaussian mask of size 3x3*/
				dst_data[i] = (    (src_data[-src_stride + i - 1] + src_data[-src_stride + i + 1])
								+  (src_data[-src_stride + i    ] + src_data[ src_stride + i    ]) * 2
								+  (src_data[ src_stride + i - 1] + src_data[ src_stride + i + 1])
								+  (src_data[              i - 1] + src_data[              i + 1]) * 2
								+  (src_data[              i    ]) * 4
								)/ 16;
			}
			dst_data[i] = src_data[i];

			dst_data += src_stride;
			src_data += src_stride;
		}

		memcpy(dst_data, src_data, src_w);
	}
	else if (matrix_size == 5)
	{
		memcpy(dst_data, src_data, src_w); dst_data += src_stride; src_data += src_stride;
		memcpy(dst_data, src_data, src_w); dst_data += src_stride; src_data += src_stride;

		for (j = 2; j < src_h - 2; j++)
		{
			dst_data[0] = src_data[0];
			dst_data[1] = src_data[1];
			for (i = 2; i < src_w - 2; i++)
			{
				/* Gaussian mask of size 5x5*/
				dst_data[i] = ((src_data[-2*src_stride + i-2] + src_data[2*src_stride + i-2]) * 2
							 + (src_data[-2*src_stride + i-1] + src_data[2*src_stride + i-1]) * 4
							 + (src_data[-2*src_stride + i	] + src_data[2*src_stride + i  ]) * 5
							 + (src_data[-2*src_stride + i+1] + src_data[2*src_stride + i+1]) * 4
							 + (src_data[-2*src_stride + i+2] + src_data[2*src_stride + i+2]) * 2

							 + (src_data[  -src_stride + i-2] + src_data[  src_stride + i-2]) *  4
							 + (src_data[  -src_stride + i-1] + src_data[  src_stride + i-1]) *  9
							 + (src_data[  -src_stride + i	] + src_data[  src_stride + i  ]) * 12
							 + (src_data[  -src_stride + i+1] + src_data[  src_stride + i+1]) *  9
							 + (src_data[  -src_stride + i+2] + src_data[  src_stride + i+2]) *  4

							 + src_data[i-2] *	5
							 + src_data[i-1] * 12
							 + src_data[i  ] * 15
							 + src_data[i+1] * 12
							 + src_data[i+2] *	5) / 159;
			}
			dst_data[i	  ] = src_data[i	];
			dst_data[i + 1] = src_data[i + 1];

			dst_data += src_stride;
			src_data += src_stride;
		}

		memcpy(dst_data, src_data, src_w); dst_data += src_stride; src_data += src_stride;
		memcpy(dst_data, src_data, src_w);
	}
}

BOOL CImgProc::Sobel(smatrix& grad,
           cmatrix& dir,
           const smatrix& src,
           int direction,
           BOOL double_edge,
           int crop_r,
           int crop_c,
           int crop_w,
           int crop_h)
{
	long r, c;
	long src_nr, src_nc;
	long grad_nr, grad_nc;
	long dir_nr, dir_nc;
	int gx, gy;

	src_nr 	= src.nr();
	src_nc 	= src.nc();
	grad_nr = grad.nr();
	grad_nc = grad.nc();
	dir_nr 	= dir.nr();
	dir_nc 	= dir.nc();

	if (!src_nr || !src_nc || !grad_nr || !grad_nc || !dir_nr || !dir_nc) {
		dbg();
		return FALSE;
	}



    // Convolution operation
	for (r = 1 ; r < src_nr - 1 ; ++r) {
		for (c = 1 ; c < src_nc - 1 ; ++c) {
			gx=0; gy=0;

            // direction :
            // 0 : vertical & horizontal edge
            // 1 : vertical edge
            // 2 : horizontal edge
			if (direction != 2) {
				gx = -1*src.operator()(r-1, c-1) + 1*src.operator()(r-1, c+1)
					 -2*src.operator()(  r, c-1) + 2*src.operator()(  r, c+1)
					 -1*src.operator()(r+1, c-1) + 1*src.operator()(r+1, c+1);
			}

			if (direction != 1) {
				gy = -1*src.operator()(r-1, c-1) + 1*src.operator()(r+1, c-1)
					 -2*src.operator()(r-1,   c) + 2*src.operator()(r+1,   c)
					 -1*src.operator()(r-1, c+1) + 1*src.operator()(r+1, c+1);
			}

			if (double_edge)
				grad.operator()(r, c) = abs(gx) + abs(gy);
			else
				grad.operator()(r, c) = gx + gy;

//			dir(r, c) = GetRoundedDirection(gx, gy);
		}
	}

	return TRUE;
}

BOOL CImgProc::SobelAndThreshoding(
								const uint8_t* src_data,
								smatrix& dst_data,
								int src_w,
								int src_h,
								int src_stride,
								int threshold,
								int direction,
								int double_edge)
{
	size_t r, c;
	int gx, gy, sum;

	if (!src_data || !dst_data.nr() || !dst_data.nc()) {
		dbg();
		return FALSE;
	}

	for (r = 1 ; r < src_h - 1 ; ++r) {
		src_data+=src_stride;

		for (c = 1 ; c < src_w - 1 ; ++c) {
			gx=0; gy=0;

            // direction :
            // 0 : vertical & horizontal edge
            // 1 : vertical edge
            // 2 : horizontal edge
			if (direction != 2) {
				gx = (src_data[-src_stride + c+1] + 2 * src_data[c+1] + src_data[src_stride + c+1]) -
					 (src_data[-src_stride + c-1] + 2 * src_data[c-1] + src_data[src_stride + c-1]);
			}

			if (direction != 1) {
				gy = (src_data[ src_stride + c-1] + 2 * src_data[ src_stride + c] + src_data[ src_stride + c+1]) -
					 (src_data[-src_stride + c-1] + 2 * src_data[-src_stride + c] + src_data[-src_stride + c+1]);
			}

			if (double_edge)
				sum = abs(gx) + abs(gy);
			else
				sum = gx + gy;

			if (sum < threshold)
				dst_data(r, c) = 255;
			else
				dst_data(r, c) = 0;

//			dir(r, c) = GetRoundedDirection(gx, gy);
		}
	}

	return TRUE;
}
































//--------------------------------------------------------------------------
CImgProc::CImgProc() {
    // Gaussian Blur
    m_gb_init   = FALSE;
    m_gk_weight = 0;

    m_gb_src    =
    m_gb_dst    = NULL;
    
    // Sobel 
    m_sobel_init    = FALSE;
    m_gradient      = NULL;
    m_dir           = NULL;

    // Integral Image
    m_intimg        =
    m_iitemp        = NULL;
}

CImgProc::~CImgProc()
{
    FreeMatrix(&m_gb_src);
    FreeMatrix(&m_gb_dst);

    FreeMatrix(&m_gradient);
    FreeMatrixChar(&m_dir);

    FreeMatrix(&m_intimg);
    FreeMatrix(&m_iitemp);
}

BOOL CImgProc::Init() 
{

    return TRUE;
}

BOOL CImgProc::EdgeDetectForLDW(const gsl_matrix* src, 
                                gsl_matrix* dst,
                                int threshold,
                                double* dir,
                                int double_edge)
{
    size_t r, c;
    gsl_matrix* temp = NULL; 

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if (InitSobel() == FALSE) {
        dbg("Fail to init sobel");
        return FALSE;
    }

    CheckOrReallocMatrix(&temp, src->size1, src->size2, TRUE);
    CheckOrReallocMatrix(&m_gradient, src->size1, src->size2, TRUE);
    CheckOrReallocMatrixChar(&m_dir, src->size1, src->size2, TRUE);

    GaussianBlue(src, temp, 3);
    Sobel(m_gradient, m_dir, temp, 1, FALSE);

    for (r=0 ; r < m_gradient->size1 ; ++r) {
        for (c=0 ; c < m_gradient->size2 ; ++c) {
            if (gsl_matrix_get(m_gradient, r, c) < threshold)
                gsl_matrix_set(dst, r, c, 255);
            else 
                gsl_matrix_set(dst, r, c, 0);
        }
    }

    FreeMatrix(&temp);

    return TRUE;
}

BOOL CImgProc::EdgeDetectForFCW(const gsl_matrix* src,
                                gsl_matrix* dst,
                                gsl_matrix* gradient,
                                gsl_matrix_char* dir,
                                int direction)
{
    if (!src || !dst || !gradient || !dir) {
        dbg();
        return FALSE;
    }

    Sobel(gradient, dir, src, direction, TRUE);
    gsl_matrix_set_zero(dst);
    NonMaximumSuppression(dst, dir, gradient);

    return TRUE;
}

BOOL CImgProc::CropImage(uint8_t* src, 
                       gsl_matrix* dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize,
                       uint32_t rowoffset)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if ((dst->size1 + rowoffset) != h || dst->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < h - rowoffset ; ++r) {
        for (c=0 ; c < w ; ++c) {
            gsl_matrix_set(dst, r, c, src[(r + rowoffset) * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyImage(uint8_t* src, 
                          gsl_matrix* dst, 
                          uint32_t w, 
                          uint32_t h, 
                          uint32_t linesize)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if (dst->size1 != h || dst->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < h ; ++r) {
        for (c=0 ; c < w ; ++c) {
            gsl_matrix_set(dst, r, c, src[r * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyBackImage(gsl_matrix* src, 
                             uint8_t* dst, 
                             uint32_t w, 
                             uint32_t h, 
                             uint32_t linesize, 
                             uint32_t rowoffset)
{
    uint32_t r, c;

    if (!src || !dst) {
        dbg();
        return FALSE;
    }

    if ((src->size1 + rowoffset) != h || src->size2 != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < src->size1 ; ++r) {
        for (c=0 ; c < src->size2 ; ++c) {
            dst[(r + rowoffset) * linesize + c] = (uint8_t)gsl_matrix_get(src, r, c);
        }
    }

    return TRUE;
}

BOOL CImgProc::GenIntegralImage(gsl_matrix* src, gsl_matrix* dst)
{
    uint32_t r, c;
    double sum;

    if (!src || !dst || src->size1 != dst->size1 || src->size2 != dst->size2) {
        dbg();
        return FALSE;
    }

    // Generate integral image for retrive average value of a rectange quickly.
    for (c=0 ; c<src->size2 ; ++c) {
        sum = 0;
        for (r=0 ; r<src->size1 ; ++r) {
            sum += gsl_matrix_get(src, r, c);

            if (c == 0)
                gsl_matrix_set(dst, r, c, sum);
            else
                gsl_matrix_set(dst, r, c, gsl_matrix_get(dst, r, c-1) + sum);
        }
    }

    return TRUE;
}

BOOL CImgProc::GenIntegralImageOfEdgeImage(const gsl_matrix* src, gsl_matrix* dst)
{
    uint32_t r, c;

    if (!src || !dst || src->size1 != dst->size1 || src->size2 != dst->size2) {
        dbg();
        return FALSE;
    }

    CheckOrReallocMatrix(&m_iitemp, src->size1, src->size2, TRUE);

    // white->black, black->white
    for (r = 0 ; r < src->size1 ; ++r) {
        for (c = 0 ; c< src->size2 ; ++c) {
            if (gsl_matrix_get(src, r, c) == 255)
                gsl_matrix_set(m_iitemp, r, c, 0);
            else
                gsl_matrix_set(m_iitemp, r, c, 255);
        }
    }   

    GenIntegralImage(m_iitemp, dst);

    return TRUE;
}

BOOL CImgProc::ThresholdingByIntegralImage(gsl_matrix* src, 
                                           gsl_matrix* intimg, 
                                           gsl_matrix* dst, 
                                           uint32_t s, 
                                           float p)
{
    uint32_t r, c;
    uint32_t r1, c1, r2, c2;
    uint32_t count;
    double sum;

    if (!src || !intimg || !dst) {
        dbg();
        return FALSE;
    }

    // Move a sxs sliding window pixel by pixel. The center pixel of this sliding window is (r, c).
    // If center pixel is p percentage lower than average value of sliding window, set it as black. Otherwise, set as white.
    for (r=0 ; r<src->size1 ; ++r) {
        for (c=0 ; c<src->size2 ; ++c) {
            if (r >= (s / 2 + 1) && r < (src->size1 - s/2 - 1) && c >= (s / 2 + 1) && c < (src->size2 - s/2 -1)) { // boundary checking.
                r1 = r - s/2;
                c1 = c - s/2;
                r2 = r + s/2;
                c2 = c + s/2;

                count = (r2 - r1) * (c2 - c1);
                sum = (gsl_matrix_get(intimg,   r2,   c2) - 
                       gsl_matrix_get(intimg, r1-1,   c2) - 
                       gsl_matrix_get(intimg,   r2, c1-1) + 
                       gsl_matrix_get(intimg, r1-1, c1-1));

                if ((gsl_matrix_get(src, r, c) * count) <= (sum * p))
                    gsl_matrix_set(dst, r, c, 0);
                else
                    gsl_matrix_set(dst, r, c, 255);
            }
            else
                gsl_matrix_set(dst, r, c, 255);
        }
    }

    return TRUE;
}

BOOL CImgProc::CalHorizonProject(const gsl_matrix* const src,
                                 gsl_vector* proj)
{
    uint32_t r, c;
    double val = 0;
    gsl_vector_view row_view;

    if (!src || !proj || src->size1 != proj->size) {
        dbg();
        return FALSE;
    }

    gsl_vector_set_zero(proj);

    // skip border
    for (r=1 ; r<src->size1 - 1 ; ++r) {
        row_view = gsl_matrix_row((gsl_matrix*)src, r);

        for (c=1 ; c<row_view.vector.size - 1 ; c++) {
            if (gsl_vector_get(&row_view.vector, c) != 255)
            {
                val = gsl_vector_get(proj, r);
                gsl_vector_set(proj, r, ++val);
            }
        }
    }

    return TRUE;
}
        
BOOL CImgProc::RemoveNoisyBlob(gsl_matrix* src, 
                             uint32_t outer_window,
                             uint32_t inner_window)
{
    uint32_t r, c, rr, cc;
    uint32_t outer_r1, outer_c1, outer_r2, outer_c2;
    uint32_t inner_r1, inner_c1, inner_r2, inner_c2;
    double outer_sum, inner_sum;

    if (!src) {
        dbg();
        return FALSE;
    }

    if (inner_window >= outer_window) {
        dbg("Inner window should be samller than outer window.");
        return FALSE;
    }

    CheckOrReallocMatrix(&m_intimg, src->size1, src->size2, TRUE);

    GenIntegralImageOfEdgeImage(src, m_intimg);

    for (r = 0 ; r < m_intimg->size1 ; ++r) {
        for (c = 0 ; c < m_intimg->size2 ; ++c) {
            if (r >= (outer_window / 2 + 1) && r < (m_intimg->size1 - outer_window / 2 - 1) && 
                c >= (outer_window / 2 + 1) && c < (m_intimg->size2 - outer_window / 2 - 1)) { // boundary checking.
                outer_r1 = r - outer_window / 2;
                outer_c1 = c - outer_window / 2;
                outer_r2 = r + outer_window / 2;
                outer_c2 = c + outer_window / 2;

                inner_r1 = r - inner_window / 2;
                inner_c1 = c - inner_window / 2;
                inner_r2 = r + inner_window / 2;
                inner_c2 = c + inner_window / 2;

                outer_sum = (gsl_matrix_get(m_intimg,   outer_r2,   outer_c2) - 
                             gsl_matrix_get(m_intimg, outer_r1-1,   outer_c2) - 
                             gsl_matrix_get(m_intimg,   outer_r2, outer_c1-1) + 
                             gsl_matrix_get(m_intimg, outer_r1-1, outer_c1-1));

                inner_sum = (gsl_matrix_get(m_intimg,   inner_r2,   inner_c2) - 
                             gsl_matrix_get(m_intimg, inner_r1-1,   inner_c2) - 
                             gsl_matrix_get(m_intimg,   inner_r2, inner_c1-1) + 
                             gsl_matrix_get(m_intimg, inner_r1-1, inner_c1-1));

                if (inner_sum > 0 && inner_sum == outer_sum) {
                    for (rr = inner_r1 ; rr <= inner_r2 ; ++rr) {
                        for (cc = inner_c1 ; cc <= inner_c2 ; ++cc) {
                            gsl_matrix_set(src, rr, cc, 255);
                        }
                    }

                    // Update integral image
                    GenIntegralImageOfEdgeImage(src, m_intimg);

                    //dbg("Remove isolated blob(%d) around (%d,%d)", (int)(inner_sum / 255), r, c);
                }
            }
        }
    }

    return TRUE;
}

//---------------- dlib----------------------------
BOOL CImgProc::EdgeDetectForLDW(const smatrix& src, 
                                smatrix& dst,
                                int threshold,
                                double* dir,
                                int double_edge)
{
    long r, c;
    long src_nr = src.nr();
    long src_nc = src.nc();
    long dst_nr = dst.nr();
    long dst_nc = dst.nc();
//    smatrix temp;

    if (!src_nr || !src_nc || !dst_nr || !dst_nc) {
        dbg();
        return FALSE;
    }

    if (src_nr != dst_nr || src_nc != dst_nc) {
        dbg();
        return FALSE;
    }

    // < 5%
//    if (!m_temp2.nr() || !m_temp2.nc())
	m_dlib_temp2.set_size(src_nr, src_nc);
//    m_temp2 = 0;
//    set_all_elements(m_temp2, 0);

//    if (!m_gradient.nr() || !m_gradient.nc())
	m_dlib_gradient.set_size(src_nr, src_nc);
//    m_gradient = 0;
//    set_all_elements(m_gradient, 0);

//    if (!m_dir.nr() || !m_dir.nc())
	m_dlib_dir.set_size(src_nr, src_nc);
//    m_dir = 0;
//    set_all_elements(m_dir, 0);

	// 5%
    GaussianBlur(src, m_dlib_temp2, 3);
    // 8%
    Sobel(m_dlib_gradient, m_dlib_dir, m_dlib_temp2, 1, FALSE);
    // 10%
    for (r=0 ; r < src_nr ; ++r) {
        for (c=0 ; c < src_nc ; ++c) {
            if (m_dlib_gradient.operator()(r, c) < threshold)
                dst.operator()(r, c) =  255;
            else
                dst.operator()(r, c) =  0;
        }
    }
    // 15%
    return TRUE;
}

BOOL CImgProc::EdgeDetectForLDW(const uint8_t* src,
		              smatrix& dst,
					  int src_w,
					  int src_h,
					  int src_stride,
					  int threshold,
					  int grandient,
					  int double_edge)
{
	unsigned int size = src_w * src_h;

	if (!src) {
		dbg();
		return FALSE;
	}

	if (m_dlib_temp && m_dlib_temp_size != size) {
		free(m_dlib_temp);
		m_dlib_temp = NULL;
	}

	if (m_dlib_temp == NULL) {
		m_dlib_temp = (uint8_t *)malloc(sizeof(uint8_t)*size);
		m_dlib_temp_size = size;
	}

	if (m_dlib_temp == NULL) {
		dbg("MemBroker_GetMemory fail");
		return FALSE;
	}

	GaussianBlur(src, m_dlib_temp, src_w, src_h, src_stride, 3);
	// < 7%
	SobelAndThreshoding(m_dlib_temp, dst, src_w, src_h, src_stride, 60, 1, 0);

	return TRUE;
}

//BOOL CImgProc::EdgeDetectForFCW(const gsl_matrix* src,
//                                gsl_matrix* dst,
//                                gsl_matrix* gradient,
//                                gsl_matrix_char* dir,
//                                int direction)
//{
//    if (!src || !dst || !gradient || !dir) {
//        dbg();
//        return FALSE;
//    }
//
//    Sobel(gradient, dir, src, direction, TRUE);
//    gsl_matrix_set_zero(dst);
//    NonMaximumSuppression(dst, dir, gradient);
//
//    return TRUE;
//}

BOOL CImgProc::CropImage(uint8_t* src, 
                       smatrix& dst, 
                       uint32_t w, 
                       uint32_t h, 
                       uint32_t linesize,
                       uint32_t rowoffset)
{
    uint32_t r, c;

    if (!src) {
        dbg();
        return FALSE;
    }

    if ((dst.nr() + rowoffset) != h || dst.nc() != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < h - rowoffset ; ++r) {
        for (c=0 ; c < w ; ++c) {
            dst.operator()( r, c) = static_cast<short>(src[(r + rowoffset) * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyImage(uint8_t* src, 
                          smatrix& dst, 
                          uint32_t w, 
                          uint32_t h, 
                          uint32_t linesize)
{
    long r, c;

    if (!src || dst.nr() != h || dst.nc() != w) {
        dbg();
        return FALSE;
    }

    for (r=0 ; r < h ; ++r) {
        for (c=0 ; c < w ; ++c) {
            dst.operator()( r, c) = static_cast<short>(src[r * linesize + c]);
        }
    }

    return TRUE;
}

BOOL CImgProc::CopyBackImage(smatrix& src, 
                             uint8_t* dst, 
                             uint32_t w, 
                             uint32_t h, 
                             uint32_t linesize, 
                             uint32_t rowoffset)
{
    size_t r, c;

    if (!dst) {
        dbg();
        return FALSE;
    }

    if ((src.nr() + rowoffset) != h || src.nc() != w) {
        dbg("Incorrect size of matrix.");
        return FALSE;
    }

    for (r=0 ; r < src.nr() ; ++r) {
        for (c=0 ; c < src.nc() ; ++c) {
            dst[(r + rowoffset) * linesize + c] = (uint8_t)src(r, c);
        }
    }

    return TRUE;
}

BOOL CImgProc::GenIntegralImage(smatrix& src, smatrix& dst)
{
    size_t r, c;
    double sum;

    if (src.nr() != dst.nr() || src.nc() != dst.nc()) {
        dbg();
        return FALSE;
    }

    // Generate integral image for retrieving average value of a rectangle quickly.
    for (c=0 ; c<src.nc() ; ++c) {
        sum = 0;
        for (r=0 ; r<src.nr() ; ++r) {
            sum += src(r, c);

            if (c == 0)
                dst(r, c) = sum;
            else
                dst(r, c) = dst(r, c-1) + sum;
        }
    }

    return TRUE;
}

BOOL CImgProc::GenIntegralImageOfEdgeImage(const smatrix& src, smatrix& dst)
{
    uint32_t r, c;

    if (src.nr() != dst.nr() || src.nc() != dst.nc()) {
        dbg();
        return FALSE;
    }

    m_dlib_iitemp.set_size(src.nr(), src.nc());
    set_all_elements(m_dlib_iitemp, 0);

    // white->black, black->white
    for (r = 0 ; r < src.nr() ; ++r) {
        for (c = 0 ; c< src.nc() ; ++c) {
            if (src(r, c) == 255)
                m_dlib_iitemp(r, c) = 0;
            else
                m_dlib_iitemp(r, c) = 255;
        }
    }   

    GenIntegralImage(m_dlib_iitemp, dst);

    return TRUE;
}

//BOOL CImgProc::ThresholdingByIntegralImage(gsl_matrix* src,
//                                           gsl_matrix* intimg,
//                                           gsl_matrix* dst,
//                                           uint32_t s,
//                                           float p)
//{
//    uint32_t r, c;
//    uint32_t r1, c1, r2, c2;
//    uint32_t count;
//    double sum;
//
//    if (!src || !intimg || !dst) {
//        dbg();
//        return FALSE;
//    }
//
//    // Move a sxs sliding window pixel by pixel. The center pixel of this sliding window is (r, c).
//    // If center pixel is p percentage lower than average value of sliding window, set it as black. Otherwise, set as white.
//    for (r=0 ; r<src->size1 ; ++r) {
//        for (c=0 ; c<src->size2 ; ++c) {
//            if (r >= (s / 2 + 1) && r < (src->size1 - s/2 - 1) && c >= (s / 2 + 1) && c < (src->size2 - s/2 -1)) { // boundary checking.
//                r1 = r - s/2;
//                c1 = c - s/2;
//                r2 = r + s/2;
//                c2 = c + s/2;
//
//                count = (r2 - r1) * (c2 - c1);
//                sum = (gsl_matrix_get(intimg,   r2,   c2) -
//                       gsl_matrix_get(intimg, r1-1,   c2) -
//                       gsl_matrix_get(intimg,   r2, c1-1) +
//                       gsl_matrix_get(intimg, r1-1, c1-1));
//
//                if ((gsl_matrix_get(src, r, c) * count) <= (sum * p))
//                    gsl_matrix_set(dst, r, c, 0);
//                else
//                    gsl_matrix_set(dst, r, c, 255);
//            }
//            else
//                gsl_matrix_set(dst, r, c, 255);
//        }
//    }
//
//    return TRUE;
//}
//
//BOOL CImgProc::CalHorizonProject(const gsl_matrix* const src,
//                                 gsl_vector* proj)
//{
//    uint32_t r, c;
//    double val = 0;
//    gsl_vector_view row_view;
//
//    if (!src || !proj || src->size1 != proj->size) {
//        dbg();
//        return FALSE;
//    }
//
//    gsl_vector_set_zero(proj);
//
//    // skip border
//    for (r=1 ; r<src->size1 - 1 ; ++r) {
//        row_view = gsl_matrix_row((gsl_matrix*)src, r);
//
//        for (c=1 ; c<row_view.vector.size - 1 ; c++) {
//            if (gsl_vector_get(&row_view.vector, c) != 255)
//            {
//                val = gsl_vector_get(proj, r);
//                gsl_vector_set(proj, r, ++val);
//            }
//        }
//    }
//
//    return TRUE;
//}
         
BOOL CImgProc::RemoveNoisyBlob(smatrix& src, 
                             uint32_t outer_window,
                             uint32_t inner_window)
{
    uint32_t r, c, rr, cc;
    uint32_t outer_r1, outer_c1, outer_r2, outer_c2;
    uint32_t inner_r1, inner_c1, inner_r2, inner_c2;
    double outer_sum, inner_sum;

    if (!src) {
        dbg();
        return FALSE;
    }

    if (inner_window >= outer_window) {
        dbg("Inner window should be samller than outer window.");
        return FALSE;
    }

    m_dlib_intimg.set_size(src.nr(), src.nc());
    set_all_elements(m_dlib_intimg, 0);

    GenIntegralImageOfEdgeImage(src, m_dlib_intimg);

    for (r = 0 ; r < m_dlib_intimg.nr() ; ++r) {
        for (c = 0 ; c < m_dlib_intimg.nc() ; ++c) {
            if (r >= (outer_window / 2 + 1) && r < (m_dlib_intimg.nr() - outer_window / 2 - 1) && 
                c >= (outer_window / 2 + 1) && c < (m_dlib_intimg.nc() - outer_window / 2 - 1)) { // boundary checking.
                outer_r1 = r - outer_window / 2;
                outer_c1 = c - outer_window / 2;
                outer_r2 = r + outer_window / 2;
                outer_c2 = c + outer_window / 2;

                inner_r1 = r - inner_window / 2;
                inner_c1 = c - inner_window / 2;
                inner_r2 = r + inner_window / 2;
                inner_c2 = c + inner_window / 2;

                outer_sum = (m_dlib_intimg(outer_r2,   outer_c2) - 
                             m_dlib_intimg(outer_r1-1, outer_c2) - 
                             m_dlib_intimg(outer_r2,   outer_c1-1) + 
                             m_dlib_intimg(outer_r1-1, outer_c1-1));

                inner_sum = (m_dlib_intimg(inner_r2,   inner_c2) - 
                             m_dlib_intimg(inner_r1-1, inner_c2) - 
                             m_dlib_intimg(inner_r2,   inner_c1-1) + 
                             m_dlib_intimg(inner_r1-1, inner_c1-1));

                if (inner_sum > 0 && inner_sum == outer_sum) {
                    for (rr = inner_r1 ; rr <= inner_r2 ; ++rr) {
                        for (cc = inner_c1 ; cc <= inner_c2 ; ++cc) {
                            src(rr, cc) = 255;
                        }
                    }

                    // Update integral image
                    GenIntegralImageOfEdgeImage(src, m_dlib_intimg);

                    //dbg("Remove isolated blob(%d) around (%d,%d)", (int)(inner_sum / 255), r, c);
                }
            }
        }
    }

    return TRUE;
}
