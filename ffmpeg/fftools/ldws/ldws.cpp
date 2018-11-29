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
    m_terminate     = false;

    m_ip            = NULL;
    m_imgy          = NULL;
    m_edged_imgy    = NULL;

    memset(m_param, 0x0, sizeof(param_t) * 2);
    memset(m_thread, 0x0, sizeof(pthread_t) * 2);
    memset(m_mutex, 0x0, sizeof(pthread_mutex_t) * 2);
    memset(m_cond, 0x0, sizeof(pthread_cond_t) * 2);
    memset(m_jobdone_mutex, 0x0, sizeof(pthread_mutex_t) * 2);
    memset(m_jobdone_cond, 0x0, sizeof(pthread_cond_t) * 2);
}

CLDWS::~CLDWS()
{
    m_terminate     = true;

    DeInit();
}

bool CLDWS::Init()
{
    bool ret = true;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    memset(&m_lane_stat, 0x0, sizeof(lane_stat_t));

    for (int i=0 ; i < 2; ++i) {
        pthread_mutex_init(&m_mutex[i], NULL);
        pthread_cond_init(&m_cond[i], NULL);
        pthread_mutex_init(&m_jobdone_mutex[i], NULL);
        pthread_cond_init(&m_jobdone_cond[i], NULL);

        m_param[i].ldws = this;
        m_param[i].id   = i;
        pthread_create(&m_thread[i], NULL, FindPartialLane, (void*)&m_param[i]);
    }

    return ret;
}

bool CLDWS::DeInit()
{
    int i;

    for (i = 0; i < 2 ; ++i) {
        pthread_mutex_lock(&m_mutex[i]);
        pthread_cond_signal(&m_cond[i]);
        pthread_mutex_unlock(&m_mutex[i]);
    }

    pthread_join(m_thread[0], NULL);
    pthread_join(m_thread[1], NULL);

    for (i = 0; i < 2 ; ++i) {
        pthread_mutex_lock(&m_jobdone_mutex[i]);
        pthread_cond_signal(&m_jobdone_cond[i]);
        pthread_mutex_unlock(&m_jobdone_mutex[i]);
    }

    for (i = 0 ; i < 2 ; ++i) {
        pthread_cond_destroy(&m_cond[i]);
        pthread_cond_destroy(&m_jobdone_cond[i]);
        pthread_mutex_destroy(&m_mutex[i]);
        pthread_mutex_destroy(&m_jobdone_mutex[i]);
    }

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
    m_ip->EdgeDetectForLDWS(m_imgy, m_edged_imgy, linesize, 80, NULL, 0);

    FindLane(m_edged_imgy, 0, 0, m_lane_stat.p, m_lane_stat.l);

    return true;
}

bool CLDWS::GetEdgedImg(uint8_t* dst, int w, int h, int linesize)
{
   size_t r, c;

   if (!dst || !m_edged_imgy || m_edged_imgy->size1 != h || m_edged_imgy->size2 != w) {
       dbg();
       return false;
   }

    if (m_ip)
        return m_ip->CopyBackMarix(m_edged_imgy, dst, w, h, linesize);
//    for (r=0 ; r < m_edged_imgy->size1 ; ++r)
//        for (c=0 ; c < m_edged_imgy->size2 ; ++c)
//            dst[r * linesize + c] = (uint8_t)gsl_matrix_get(m_edged_imgy, r, c);


    return false;
}














//------------------------------------------------------------------------------
double CLDWS::agent_cal_dist(lanepoint* p1, lanepoint* p2)
{
    if (!p1 || !p2)
        return 0;

    return sqrt(pow(p1->c - p2->c, 2) + pow(p1->r - p2->r, 2)); 
}

/* 
 * i and j point to px and py which the length of px and py is the longest.
 * length of vector ij should be longer enough.
 */
int CLDWS::agent_determine_ijk(lanepoint* p1, lanepoint* p2, lanepoint* p3, lanepoint** i, lanepoint** j, lanepoint** k)
{
    int     rval = LANE_DETECT_CHECK_AGENT_DISTANCE_ERROR;
    double  max_dist = 0.0, dist = 0.0;

    if (!p1 || !p2 || !p3 || !i || !j || !k) {
        dbg();
        return rval;
    }

    dist = agent_cal_dist(p1, p2);
    if (max_dist < dist){
        max_dist = dist;
        *i = p1;
        *j = p2;
        *k = p3;
    }

    dist = agent_cal_dist(p2, p3);
    if (max_dist < dist){
        max_dist = dist;
        *i = p2;
        *j = p3;
        *k = p1;
    }

    dist = agent_cal_dist(p1, p3);
    if (max_dist < dist){
        max_dist = dist;
        *i = p1;
        *j = p3;
        *k = p2;
    }

    if (max_dist > VALID_AGENT_DISTANCE)
        rval = LANE_DETECT_CHECK_OK;

    return rval;
}

int CLDWS::check_agent_is_horizontal(lanepoint* i, lanepoint* j)
{
    int rval = LANE_DETECT_CHECK_HORIZONTAL_ERROR;

    if (!i || !j) {
        dbg();
        return rval;
    }
    
    if (abs(i->r - j->r) >= VALID_AGENT_HORIZONTAL)
        rval = LANE_DETECT_CHECK_OK;
         
    return rval;
}

int CLDWS::check_dist_of_k_to_vector_ij(lanepoint* i, lanepoint* j, lanepoint* k)
{
    int     rval = LANE_DETECT_CHECK_DIST_OF_K_TO_VECTOR_IJ_ERROR;
    double  dist = 0.0;

    if (!i || !j || !k) {
        dbg();
        return rval;
    }

    dist = fabs(((j->c - i->c)*k->r) + ((i->r - j->r)*k->c) + (i->c * j->r) - (j->c * i->r)) / sqrt(pow(j->c - i->c, 2) + pow(j->r - i->r, 2));

    if (dist <= VALID_DIST_OF_K_TO_VECTOR_IJ)
        rval = LANE_DETECT_CHECK_OK;

    return rval;
}

int CLDWS::check_dist_of_k_to_ij(lanepoint* i, lanepoint* j, lanepoint* k)
{
    int rval = LANE_DETECT_CHECK_DIST_OF_K_TO_IJ_ERROR;
    double d1,d2;

    if (!i || !j || !k) {
        dbg();
        return rval;
    }

    d1 = agent_cal_dist(i, k);
    d2 = agent_cal_dist(j, k);

    //fprintf(stdout, "d1=%lf,d2=%lf\n", d1, d2);
    if (d1 > VALID_DIST_OF_POINT_TO_POINT && d2 > VALID_DIST_OF_POINT_TO_POINT)
        rval = LANE_DETECT_CHECK_OK;

    return rval;
}

int CLDWS::check_agent_valid(lanepoint** p1, lanepoint** p2, lanepoint** p3)
{
    int rval = 0;
    lanepoint *i = NULL,*j = NULL,*k = NULL;

    if (!*p1 || !*p2 || !*p3) {
        dbg();
        return 0;
    }

    rval = agent_determine_ijk(*p1, *p2, *p3, &i, &j, &k);

    if (rval == LANE_DETECT_CHECK_OK)
        rval = check_agent_is_horizontal(i, j);

    if (rval == LANE_DETECT_CHECK_OK)
        rval = check_dist_of_k_to_vector_ij(i, j, k);
    
    if (rval == LANE_DETECT_CHECK_OK)
        rval = check_dist_of_k_to_ij(i, j, k);

    if (rval == LANE_DETECT_CHECK_OK){
        /* reorder */
        *p1 = i;*p2 = j;*p3 = k;
        //fprintf(stdout, "(%d,%d),(%d,%d),(%d,%d)\n" , i->c,i->r,j->c,j->r,k->c,k->r);
    }

    return rval;
}

void* CLDWS::FindPartialLane(void* args)
{
    param_t *param = (param_t*)args;
    CLDWS* pThis = param->ldws;
    int id = param->id;

    pthread_mutex_lock(&pThis->m_mutex[id]);

    while (!pThis->m_terminate) {
        dbg();
        pthread_cond_wait(&pThis->m_cond[id], &pThis->m_mutex[id]);

        if (!pThis->m_terminate) {
            pthread_mutex_lock(&pThis->m_mutex[id]);
            return NULL;
        }

        dbg("id %d is done", id);
        pThis->m_thread_done++;



        dbg();
        pthread_mutex_lock(&pThis->m_jobdone_mutex[id]);
        dbg();
        pthread_cond_signal(&pThis->m_jobdone_cond[id]);
        dbg();
        pthread_mutex_unlock(&pThis->m_jobdone_mutex[id]);
        dbg();
    }

    pthread_mutex_unlock(&pThis->m_mutex[id]);

    return NULL;
}

bool CLDWS::FindLane(gsl_matrix* src,
                    int start_row,
                    int start_col,
                    lanepoint* p,
                    lane *l[LANE_NUM])
{
    int i;
    size_t r, c;

    if (!src) {
        dbg();
        return false;
    }

    m_thread_done = 0;
    
    for (i = 0 ; i < 2; ++i) {
        dbg();
        pthread_mutex_lock(&m_mutex[i]);
        dbg();
        pthread_cond_signal(&m_cond[i]);
        dbg();
        pthread_mutex_unlock(&m_mutex[i]);
    }

        pthread_yield();

    for (i = 0 ; i < 2; ++i) {
        dbg();
        pthread_mutex_lock(&m_jobdone_mutex[i]);
        dbg();
        pthread_cond_wait(&m_jobdone_cond[i], &m_jobdone_mutex[i]);
        dbg("m_thread_done %d", m_thread_done);
        pthread_mutex_unlock(&m_jobdone_mutex[i]);
        dbg();
    }

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
