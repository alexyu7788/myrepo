#ifdef __cplusplus
extern "C" {
#include "libavutil/avutil.h"
}
#endif
#include "ldws.hpp"
#include "../utils/imgproc.hpp"

static gsl_matrix* m_imgy       = NULL;
static gsl_matrix* m_edged_imgy = NULL;

static void LDW_GaussianBlur(uint8_t* src_data, uint8_t* dst_data, int src_w, int src_h, int src_stride, int matrix_size)
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
                dst_data[i] = ((src_data[-src_stride + i-1] + src_data[-src_stride + i+1]) 
                            +  (src_data[ src_stride + i-1] + src_data[ src_stride + i+1]) 
                            +  (src_data[-src_stride + i  ] + src_data[ src_stride + i  ]) * 2
                            +  (src_data[ src_stride + i-1] + src_data[ src_stride + i+1]) * 2
                            +  (src_data[ src_stride + i  ]                              ) * 4
                            ) / 16;
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
                             + (src_data[-2*src_stride + i  ] + src_data[2*src_stride + i  ]) * 5
                             + (src_data[-2*src_stride + i+1] + src_data[2*src_stride + i+1]) * 4
                             + (src_data[-2*src_stride + i+2] + src_data[2*src_stride + i+2]) * 2

                             + (src_data[  -src_stride + i-2] + src_data[  src_stride + i-2]) *  4
                             + (src_data[  -src_stride + i-1] + src_data[  src_stride + i-1]) *  9
                             + (src_data[  -src_stride + i  ] + src_data[  src_stride + i  ]) * 12
                             + (src_data[  -src_stride + i+1] + src_data[  src_stride + i+1]) *  9
                             + (src_data[  -src_stride + i+2] + src_data[  src_stride + i+2]) *  4

                             + src_data[i-2] *  5
                             + src_data[i-1] * 12
                             + src_data[i  ] * 15
                             + src_data[i+1] * 12
                             + src_data[i+2] *  5) / 159;
            }
            dst_data[i    ] = src_data[i];
            dst_data[i + 1] = src_data[i + 1];

            dst_data += src_stride;
            src_data += src_stride;
        }

        memcpy(dst_data, src_data, src_w); dst_data += src_stride; src_data += src_stride;
        memcpy(dst_data, src_data, src_w);
    }
}
 
static void LDW_SobelAndThreshoding(uint8_t *src_data, uint8_t *dst_data, int src_w, int src_h, int src_stride, int threshold, int grandient, double *dir, int double_edge)
{
    int i, j;

    for (j = 1; j < src_h - 1; j++) 
    {
        dst_data += src_stride;
        src_data += src_stride;

        if (dir)
            dir += src_stride;
        
        for (i = 1; i < src_w - 1; i++) 
        {
            if (grandient == 0)// Gx
            {
                /*
                *   { -1 , 0 , 1}        
                *   { -2 , 0 , 2}    
                *   { -1 , 0 , 1}
                */            
                int gx =
                    (src_data[-src_stride + i+1] + 2 * src_data[i+1] + src_data[src_stride + i+1]) - \
                    (src_data[-src_stride + i-1] + 2 * src_data[i-1] + src_data[src_stride + i-1]);

                if (double_edge)
                    gx = FFABS(gx);

                //threshoding
                if (gx < threshold)
                    gx = 255;   //non-edge
                else
                    gx = 0;     //edge

                dst_data[i] = gx;
            }
            else if (grandient == 1)//Gy
            {
                /*
                *   { -1 , -2 , -1}        
                *   {  0 ,  0 ,  0}    
                *   {  1 ,  2 ,  1}
                */            
                int gy =
                    (src_data[ src_stride + i-1] + 2 * src_data[ src_stride + i] + src_data[ src_stride + i+1]) - \
                    (src_data[-src_stride + i-1] + 2 * src_data[-src_stride + i] + src_data[-src_stride + i+1]);

                if (double_edge)
                    gy = FFABS(gy);

                //threshoding
                if (gy < threshold)
                    gy = 255;   //non-edge
                else
                    gy = 0;     //edge

                dst_data[i] = gy;
            }
            else if (grandient == 2)//Gx+Gy
            {
                int gx =
                    (src_data[-src_stride + i+1] + 2 * src_data[i+1] + src_data[src_stride + i+1]) - \
                    (src_data[-src_stride + i-1] + 2 * src_data[i-1] + src_data[src_stride + i-1]);
                
                int gy =
                    (src_data[ src_stride + i-1] + 2 * src_data[ src_stride + i] + src_data[ src_stride + i+1]) - \
                    (src_data[-src_stride + i-1] + 2 * src_data[-src_stride + i] + src_data[-src_stride + i+1]);

                if (dir)
                {
                    //dir[i] = get_rounded_direction(gx, gy);
                    dir[i] = atan2((double)gy, (double)gx) * 180 / M_PI;
                }
                
                if (double_edge)
                {
                    gx = FFABS(gx);
                    gy = FFABS(gy);
                }

                dst_data[i] = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
                
                //threshoding
                if (dst_data[i] < threshold)
                    dst_data[i] = 255;  //non-edge
                else
                    dst_data[i] = 0;    //edge
            }
        }
    }
}

static void LDW_EdgeDetect(uint8_t* src_data, uint8_t* dst_data, int src_w, int src_h, int src_stride, int threshold, int gradient, double* dir, int double_edge)
{
    uint8_t *temp_data = (uint8_t*)av_mallocz(sizeof(uint8_t)*src_w*src_h);

    LDW_GaussianBlur(src_data, temp_data, src_w, src_h, src_stride, 3);
    //LDW_SobelAndThreshoding(temp_data, dst_data, src_w, src_h, src_stride, threshold, gradient, dir, double_edge);

    av_freep(&temp_data);
}

static void LDW_EdgeDetect2(gsl_matrix* src, gsl_matrix* dst, int linesize, int threshold, int gradient, double* dir, int double_edge)
{
    gsl_matrix* temp = NULL;
    CheckOrReallocMatrix(&temp, src->size1, src->size2, true);


    FreeMatrix(&temp);
}
 
//============================================================================

CLDWS::CLDWS()
{
    m_ip            = NULL;
    m_imgy          = NULL;
    m_edged_imgy    = NULL;
}

CLDWS::~CLDWS()
{
    DeInit();
}

bool CLDWS::Init()
{
    bool ret = true;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    return ret;
}

bool CLDWS::DeInit()
{
    FreeMatrix(&m_imgy);
    FreeMatrix(&m_edged_imgy);

    if (m_ip) {
        delete m_ip;
        m_ip = NULL;
    }

    return true;
}

bool CLDWS::DoDetection(uint8_t* src, int linesize, int w, int h)
{
    uint32_t r, c;

    if (!src) {
        dbg();
        return false;
    }

    if (!m_ip) {
        dbg();
        return false;
    }

    CheckOrReallocMatrix(&m_imgy, h, w, true);
    CheckOrReallocMatrix(&m_edged_imgy, h, w, true);

    m_ip->CopyMatrix(src, m_imgy, w, h, linesize);



    return true;
}
//#ifdef __cplusplus
//extern "C" {
//void LDW_DoDetection(uint8_t* src, int linesize, int w, int h)
//{
//    uint32_t r, c;
//
//    CheckOrReallocMatrix(&m_imgy, h, w, true);
//    CheckOrReallocMatrix(&m_edged_imgy, h, w, true);
//
//    for (r=0 ; r<m_imgy->size1 ; ++r) {
//        for (c=0 ; c<m_imgy->size2 ; ++c) {
//            gsl_matrix_set(m_imgy, r, c, src[r * linesize]);
//        }
//    }
//
//    LDW_EdgeDetect2(m_imgy, m_edged_imgy, linesize, 80, 0, NULL, 0);
//}
//
//void LDW_DeInit(void)
//{
//    FreeMatrix(&m_imgy);
//    FreeMatrix(&m_edged_imgy);
//}
//}
//#endif
