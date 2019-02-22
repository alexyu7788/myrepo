#ifdef __cplusplus
extern "C" {
#include "libavutil/avutil.h"
}
#endif
#include <unistd.h>
#include "ldws.hpp"
#include "../utils/imgproc.hpp"

//============================================================================

CLDWS::CLDWS()
{
    m_terminate     = FALSE;

    m_left_coloffset    =
    m_right_coloffset   = 0;
 
    m_ip            = NULL;
    m_imgy          = NULL;
    m_edged_imgy    = NULL;

    memset(&m_vp, 0x0, sizeof(m_vp));

    m_slope_left    = 
    m_delta_left    = 
    m_slope_right   = 
    m_delta_right   = 0.0;

    memset(m_param, 0x0, sizeof(param_t) * 2);
    memset(m_thread, 0x0, sizeof(pthread_t) * 2);
    memset(m_mutex, 0x0, sizeof(pthread_mutex_t) * 2);
    memset(m_cond, 0x0, sizeof(pthread_cond_t) * 2);
    memset(&m_jobdone_mutex, 0x0, sizeof(pthread_mutex_t));
    memset(&m_jobdone_cond, 0x0, sizeof(pthread_cond_t));
}

CLDWS::~CLDWS()
{
    m_terminate     = TRUE;

    DeInit();
}

BOOL CLDWS::Init()
{
    BOOL ret = TRUE;
    uint32_t i;
    pthread_attr_t attr;

    m_terminate = FALSE;

    m_rows              =
    m_cols              =
    m_rowoffset         = 
    m_left_coloffset    =
    m_right_coloffset   = 0;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    memset(&m_lane_stat, 0x0, sizeof(lane_stat_t));
    for (i = 0 ; i < LANE_NUM ; ++i)
        m_lane_stat.l[i] = LaneInit();

    // pthread relevant
    pthread_mutex_init(&m_jobdone_mutex, NULL);
    pthread_cond_init(&m_jobdone_cond, NULL);

    pthread_attr_init(&attr);
 
    for (int i=0 ; i < LANE_THREADS; ++i) {
        pthread_mutex_init(&m_mutex[i], NULL);
        pthread_cond_init(&m_cond[i], NULL);

        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

        m_param[i].obj  = this;
        m_param[i].id   = i;
        
        pthread_create(&m_thread[i], &attr, FindPartialLane, (void*)&m_param[i]);
    }

    pthread_attr_destroy(&attr);

    return ret;
}

BOOL CLDWS::DeInit()
{
    uint32_t i;

    for (i = 0; i < LANE_THREADS ; ++i) {
        pthread_mutex_lock(&m_mutex[i]);
        pthread_cond_signal(&m_cond[i]);
        pthread_mutex_unlock(&m_mutex[i]);
    }

    for (i = 0 ; i < LANE_THREADS ; ++i)
        pthread_join(m_thread[i], NULL);

    pthread_cond_destroy(&m_jobdone_cond);
    pthread_mutex_destroy(&m_jobdone_mutex);

    for (i = 0 ; i < LANE_THREADS ; ++i) {
        pthread_cond_destroy(&m_cond[i]);
        pthread_mutex_destroy(&m_mutex[i]);
    }

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_edged_imgy);

    if (m_ip) {
        delete m_ip;
        m_ip = NULL;
    }

    for (i = 0 ; i < LANE_NUM ; ++i)
        LaneDeinit(&m_lane_stat.l[i]);

    return TRUE;
}


BOOL CLDWS::DoDetection(uint8_t* src, 
                        int w, 
                        int h, 
                        int linesize, 
                        int rowoffset,
                        BOOL crop)
{
    if (!src) {
        ldwsdbg();
        return FALSE;
    }
  
    if (!m_ip) {
        ldwsdbg();
        return FALSE;
    }

    if (w <= 0 || h <= 0 || linesize <= 0 || h <= rowoffset) {
        dbg("Incorrect geometric parameters.");
        return FALSE;
    }

    m_rows      = (crop == TRUE ? (h - rowoffset) : h);
    m_cols      = w;
    m_rowoffset = rowoffset;

    CheckOrReallocMatrix(&m_imgy        , m_rows, m_cols, TRUE);
    CheckOrReallocMatrix(&m_edged_imgy  , m_rows, m_cols, TRUE);

    if (crop)
        m_ip->CropImage(src, m_imgy, w, h, linesize, m_rowoffset);
    else
        m_ip->CopyImage(src, m_imgy, w, h, linesize);

    m_ip->EdgeDetectForLDW(m_imgy, m_edged_imgy, 60, NULL, 0);
    m_ip->RemoveNoisyBlob(m_edged_imgy, 16, 10);

    FindLane(m_edged_imgy, 0, 0, m_lane_stat.p, m_lane_stat.l);

    return TRUE;
}

BOOL CLDWS::GetEdgedImg(uint8_t* dst, int w, int h, int linesize)
{
   if (!dst || !m_edged_imgy /*|| m_edged_imgy->size1 != h || m_edged_imgy->size2 != w*/) {
       ldwsdbg();
       return FALSE;
   }

    if (m_ip)
        return m_ip->CopyBackImage(m_edged_imgy, dst, w, h, linesize, m_rowoffset);

    return FALSE;
}

// For drawing
BOOL CLDWS::GetLane(lane** left, lane** right, lane** center)
{
    if (!left || !right || !center) {
        ldwsdbg();
        return FALSE;
    }

    *left   = LaneInit(m_lane_stat.l[LANE_LEFT]);
    *right  = LaneInit(m_lane_stat.l[LANE_RIGHT]);
    *center = LaneInit(m_lane_stat.l[LANE_CENTER]);

    return TRUE;
}

BOOL CLDWS::DestroyLane(lane** left, lane** right, lane** center)
{
    LaneDeinit(left);
    LaneDeinit(right);
    LaneDeinit(center);

    return TRUE;
}

BOOL CLDWS::GetLanePoints(point_t* ltop, 
                          point_t* lbottom, 
                          point_t* rtop, 
                          point_t* rbottom,
                          point_t* ctop, 
                          point_t* cbottom
                          )
{
    lane *left = NULL, *right = NULL, *center = NULL;

    if (!ltop || !lbottom || !rtop || !rbottom || !ctop || !cbottom) {
        ldwsdbg();
        return FALSE;
    }

    left    = m_lane_stat.l[LANE_LEFT];
    right   = m_lane_stat.l[LANE_RIGHT];
    center  = m_lane_stat.l[LANE_CENTER];

    if (!left || !right || !center) {
        ldwsdbg();
        return FALSE;
    }

    pthread_mutex_lock(&left->mutex);
    pthread_mutex_lock(&right->mutex);
    pthread_mutex_lock(&center->mutex);

    if (left->exist && left->pix) {
        if (m_vp.r && m_vp.c) {
            ltop->r = m_vp.r;
            ltop->c = m_vp.c;
        } else {
            ltop->r = left->pix[left->pix_count / 2].r;
            ltop->c = left->pix[left->pix_count / 2].c;
        }

        lbottom->r = left->pix[left->pix_count - 1].r;
        lbottom->c = left->pix[left->pix_count - 1].c;
    }

    if (right->exist && right->pix) {
        if (m_vp.r && m_vp.c) {
            rtop->r = m_vp.r;
            rtop->c = m_vp.c;
        } else {
            rtop->r = right->pix[right->pix_count / 2].r;
            rtop->c = right->pix[right->pix_count / 2].c;
        }

        rbottom->r = right->pix[right->pix_count - 1].r;
        rbottom->c = right->pix[right->pix_count - 1].c;
    }

    if (center->exist && center->pix) {
        if (m_vp.r && m_vp.c) {
            ctop->r = m_vp.r;
            ctop->c = m_vp.c;
        } else {
            ctop->r = center->pix[center->pix_count / 2].r;
            ctop->c = center->pix[center->pix_count / 2].c;
        }

        cbottom->r = center->pix[center->pix_count - 1].r;
        cbottom->c = center->pix[center->pix_count - 1].c;
    }

    pthread_mutex_unlock(&left->mutex);
    pthread_mutex_unlock(&right->mutex);
    pthread_mutex_unlock(&center->mutex);

    return TRUE;
}

BOOL CLDWS::ApplyDynamicROI(gsl_matrix* src)
{
    BOOL ret = FALSE;
    uint32_t r, c;
    lane *left = NULL, *right = NULL, *center = NULL;

    left    = m_lane_stat.l[LANE_LEFT];
    right   = m_lane_stat.l[LANE_RIGHT];
    center  = m_lane_stat.l[LANE_CENTER];

    if (!src || !left || !right || !center) {
        ldwsdbg();
        return ret;
    }

    pthread_mutex_lock(&left->mutex);
    pthread_mutex_lock(&right->mutex);

    if (left->exist && right->exist && m_slope_left != -FLT_MAX && m_slope_right != -FLT_MAX) {
        for (r = 0 ; r < src->size1 ; ++r) {
            for (c = 0 ; c < src->size2 ; ++c) {
                if ((r * m_slope_left + m_delta_left > c) || (r * m_slope_right + m_delta_right < c))
                    gsl_matrix_set(src, r, c, 255);
            }
        }

        ret = TRUE;
    } else
        ret = FALSE;

    pthread_mutex_unlock(&left->mutex);
    pthread_mutex_unlock(&right->mutex);

    return ret;
}







 
//------------------------------------------------------------------------------
lane* CLDWS::LaneInit(void)
{
    lane* l = NULL;
    pthread_mutexattr_t attr;

    l = new lane;
    memset(l, 0x0, sizeof(lane));
    l->pix = NULL;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&l->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return l;
}

lane* CLDWS::LaneInit(const lane* const ref)
{
    uint32_t cnt = 0;

    lane* l = LaneInit();

    memcpy(l, ref, sizeof(lane));

    l->pix = new lanepoint[l->pix_count];

    for (cnt = 0 ; cnt < l->pix_count ; ++cnt) {
        l->pix[cnt].r = ref->pix[cnt].r;
        l->pix[cnt].c = ref->pix[cnt].c;
    }

    return l;
}

void CLDWS::LaneDeinit(lane** l)
{
    if (*l) {
        if ((*l)->pix) {
            delete [] (*l)->pix;
            (*l)->pix = NULL;
        }

        pthread_mutex_destroy(&((*l)->mutex));

        delete (*l);
        (*l) = NULL; 
    }
}

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
        ldwsdbg();
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
        ldwsdbg();
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
        ldwsdbg();
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
        ldwsdbg();
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
        ldwsdbg();
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

// ----------------kluge polynomial relevant----------------
// Examples of https://www.gnu.org/software/gsl/doc/html/linalg.html 
void CLDWS::solve_equation(gsl_matrix* a, gsl_vector* x, gsl_vector* b)
{
    int s;
    gsl_permutation* p = NULL;

    p = gsl_permutation_alloc(a->size1);
    gsl_linalg_LU_decomp(a, p, &s);
    gsl_linalg_LU_solve(a, p, b, x);

    gsl_permutation_free(p);
}

/* Kluge poly for describe lane model. 
 *
 * c = k/r + br + v,
 * where r,c are in the rc-plane.
 *
 * | 1/r0 r0 1| |k|   |c0|
 * | 1/r1 r1 1|*|b| = |c1|
 * | 1/r2 r2 1| |v|   |c2|
 *
 * A*X = B,
 * where A and B are known, to slove X
 */

void CLDWS::kp_solve(lanepoint* pp[3], lane* l)
{
    double r0, r1, r2;
    gsl_matrix* m = NULL;
    gsl_vector* x = NULL;
    gsl_vector* b = NULL;

    CheckOrReallocMatrix(&m, 3, 3, TRUE);
    CheckOrReallocVector(&x, 3, TRUE);
    CheckOrReallocVector(&b, 3, TRUE);

    gsl_vector_set(b, 0, (double)pp[0]->c);
    gsl_vector_set(b, 1, (double)pp[1]->c);
    gsl_vector_set(b, 2, (double)pp[2]->c);
    r0 = (double)pp[0]->r;
    r1 = (double)pp[1]->r;
    r2 = (double)pp[2]->r;
    gsl_matrix_set(m, 0, 0, 1/r0);
    gsl_matrix_set(m, 0, 1, r0);
    gsl_matrix_set(m, 0, 2, 1);
    gsl_matrix_set(m, 1, 0, 1/r1);
    gsl_matrix_set(m, 1, 1, r1);
    gsl_matrix_set(m, 1, 2, 1);
    gsl_matrix_set(m, 2, 0, 1/r2);
    gsl_matrix_set(m, 2, 1, r2);
    gsl_matrix_set(m, 2, 2, 1);

    solve_equation(m, x, b);

    l->kluge_poly.k = gsl_vector_get(x, 0);
    l->kluge_poly.b = gsl_vector_get(x, 1);
    l->kluge_poly.v = gsl_vector_get(x, 2);
//    fprintf(stdout, "k=%lf,b=%lf,v=%lf\n", l_temp->kluge_poly.k, l_temp->kluge_poly.b, l_temp->kluge_poly.v);

    FreeMatrix(&m);
    FreeVector(&x);
    FreeVector(&b);
}

double CLDWS::kp_dist_of_points_to_kp(lanepoint* p, lane* l, uint32_t search_range_r)
{
    double k,b,v;
    double mini_dist = 1.0E+10,dist;
    int r,c;
    int rr;
	
    k = l->kluge_poly.k;
    b = l->kluge_poly.b;
    v = l->kluge_poly.v;

    //fprintf(stdout, "src=(%d,%d)\n", p->c, p->r);
    r = p->r;
    for ((rr=r-search_range_r) ; rr<=(r+search_range_r) ; rr++) {
        if (r>0) {
            c = (int)(k/rr+b*rr+v);  
            dist = sqrt(pow(c-p->c, 2)+pow(rr - p->r, 2));
            //fprintf(stdout, "mini_dist = %lf, dist=%lf,(%d,%d) to (%d,%d)\n", mini_dist, dist, p->c, p->r, c, rr);
            if (dist < mini_dist)
                mini_dist = dist;
        }
    }
    //fprintf(stdout, "mini_dist=%lf\n", mini_dist);
    return mini_dist;
}

double CLDWS::kp_evidence_check(int count, lanepoint* p, lane* l)
{
    double rval=0;
    int i;
    int evidence_count=0;

    for (i=0 ; i<count ; i++) {
        if (!p[i].pickup) {
            if (kp_dist_of_points_to_kp(&p[i], l, 10) < VALID_DIST_OF_K_TO_KLUGE_POLY) {
                p[i].pol = 1;
                evidence_count++;
            }
        }
    }

    //fprintf(stdout, "ec=%d, c=%d\n", evidence_count, count);
    rval = evidence_count/(double)count;

    return rval;
}

void CLDWS::kp_gen_point(uint32_t rows, uint32_t cols, lane* l, uint32_t rowoffset, uint32_t coloffset)
{
    uint32_t pix_cnt=0;
    uint32_t col_cnt=0;
    double k,b,v;
    double r, c;

    if (!l) {
        ldwsdbg();
        return;
    }

    pthread_mutex_lock(&l->mutex);

    kp_rel_point(l);
    l->pix = new lanepoint[rows];
    memset(l->pix, 0x0, sizeof(lanepoint) * rows);

    k = l->kluge_poly.k;
    b = l->kluge_poly.b;
    v = l->kluge_poly.v;

//    fprintf(stdout, "k=%lf, b=%lf, v=%lf\n", k, b, v);
    for (r=1, pix_cnt=0 ; r<rows ; r++) {
        c = (int)((k/r) + (b*r) + v);
        if( c>=0 && c<= cols) {
            l->pix[pix_cnt].r = (int)(r + rowoffset);
            l->pix[pix_cnt].c = (int)(c + coloffset);
            col_cnt += c;
  //          fprintf(stdout, "cnt=%d, (%d,%d)\n", cnt, (*l)->pix[cnt].c, (*l)->pix[cnt].r);
            pix_cnt++;
        }
    }

    l->pix_count        = pix_cnt;
    l->pix_col_center   = (int)(col_cnt/pix_cnt);

//    ldwsdbg("l_temp: count %d, column center %d", 
//            l_temp->pix_count, l_temp->pix_col_center);
    pthread_mutex_unlock(&l->mutex);
}

void CLDWS::kp_gen_center_point(uint32_t rows, uint32_t cols, lane* left, lane* right, lane* center)
{
    uint32_t r, cnt = 0, mean_c = 0;

    if (!left || !left->pix || !right || !right->pix || !center) {
        ldwsdbg();
        return;
    }

    pthread_mutex_lock(&left->mutex);
    pthread_mutex_lock(&right->mutex);
    pthread_mutex_lock(&center->mutex);

    if (left->pix && right->pix) {
        kp_rel_point(center);
        center->pix = new lanepoint[rows];
        memset(center->pix, 0x0, sizeof(lanepoint) * rows);

        for (r = 0 ; ; ++r) {
            if ((r > left->pix_count - 1) || (r > right->pix_count - 1))
                break;

            center->pix[r].r = (left->pix[r].r + right->pix[r].r) / 2;
            center->pix[r].c = (left->pix[r].c + right->pix[r].c) / 2;
            ++cnt;
            mean_c += center->pix[r].c;
        }

        center->pix_count = cnt;
        mean_c /= cnt;
        center->shift = mean_c - cols/2;
        ldwsdbg("center shift %d", center->shift);
    }

    pthread_mutex_unlock(&left->mutex);
    pthread_mutex_unlock(&right->mutex);
    pthread_mutex_unlock(&center->mutex);
}

void CLDWS::kp_rel_point(lane* l)
{
    if (l) {
        pthread_mutex_lock(&l->mutex);

        delete [] l->pix;
        l->pix = NULL;
        l->pix_col_center = l->pix_count = 0;

        pthread_mutex_unlock(&l->mutex);
    }
}










void CLDWS::WakeUpThread()
{
    m_thread_done = 0;
    
    for (int i = 0 ; i < LANE_THREADS; ++i) {
        pthread_mutex_lock(&m_mutex[i]);
        pthread_cond_signal(&m_cond[i]);
        pthread_mutex_unlock(&m_mutex[i]);
    }

    //pthread_yield();
}

void CLDWS::WaitThreadDone()
{
    pthread_mutex_lock(&m_jobdone_mutex);

    while (m_thread_done < LANE_THREADS) {
        pthread_cond_wait(&m_jobdone_cond, &m_jobdone_mutex);
        //ldwsdbg("m_thread_done %d", m_thread_done);
    }

    pthread_mutex_unlock(&m_jobdone_mutex);
}

BOOL CLDWS::UpdateLaneStatus(uint32_t rows, uint32_t cols, lane* left, lane* right, lane* center)
{
    if (!left || !right || !center) {
        ldwsdbg();
        return FALSE;
    }

    center->exist = (left->exist & right->exist ? 1 : 0);

    if (center->exist)
        kp_gen_center_point(rows, cols, left, right, center);
    else
        kp_rel_point(center);

    return TRUE;
}

/*
 * Tranform spline of kluge polynomial into simple straight line polynomial.
 */
BOOL CLDWS::CalStraightLanesPoly(void)
{
    int32_t r1, r2, c1, c2;
    lane *left = NULL, *right = NULL;

    left    = m_lane_stat.l[LANE_LEFT];
    right   = m_lane_stat.l[LANE_RIGHT];

    if (!left || !right) {
        ldwsdbg();
        return FALSE;
    }

    pthread_mutex_lock(&left->mutex);
    pthread_mutex_lock(&right->mutex);

    if (left->exist && right->exist) {
        r1 = left->pix[left->pix_count / 3].r; 
        c1 = left->pix[left->pix_count / 3].c; 
        r2 = left->pix[left->pix_count * 2 / 3].r; 
        c2 = left->pix[left->pix_count * 2 / 3].c; 

        if (c1 != c2) {
            m_slope_left = (float)(c1 - c2) / (float)(r1 - r2);
            m_delta_left = c1 - m_slope_left * r1;
        } else
            m_slope_left = -FLT_MAX;

        r1 = right->pix[right->pix_count / 2].r; 
        c1 = right->pix[right->pix_count / 2].c; 
        r2 = right->pix[right->pix_count - 1].r; 
        c2 = right->pix[right->pix_count - 1].c; 

        if (c1 != c2) {
            m_slope_right = (float)(c1 - c2) / (float)(r1 - r2);
            m_delta_right = c1 - m_slope_right * r1;
        } else
            m_slope_right = -FLT_MAX;

    } else {
        m_slope_left    =
        m_slope_right   = -FLT_MAX;
    }

    pthread_mutex_unlock(&left->mutex);
    pthread_mutex_unlock(&right->mutex);

    return TRUE;
}
 
/*
 * Vanishing point is the intersection of left- & right straight lane.
 */
BOOL CLDWS::CalVanishingPoint(void)
{
    uint32_t r;
    int32_t c1, c2;
    lane *left = NULL, *right = NULL;

    left    = m_lane_stat.l[LANE_LEFT];
    right   = m_lane_stat.l[LANE_RIGHT];

    if (!left || !right) {
        ldwsdbg();
        return FALSE;
    }

    pthread_mutex_lock(&left->mutex);
    pthread_mutex_lock(&right->mutex);

    if (m_slope_left != -FLT_MAX && m_slope_right != -FLT_MAX) {
        for (r = 0 ; r < m_rows + m_rowoffset ; ++r) {
            c1 = (int32_t)(r * m_slope_left  + m_delta_left);
            c2 = (int32_t)(r * m_slope_right + m_delta_right);

            if (c1 > 0 && c2 > 0 && c1 <= c2) {
                m_vp.r = r;
                m_vp.c = c1;
                break;
            }
        }
    } else
        m_vp.r = m_vp.c = 0;

    pthread_mutex_unlock(&left->mutex);
    pthread_mutex_unlock(&right->mutex);

    return TRUE;
}

void* CLDWS::FindPartialLane(void* args)
{
    int id = -1;
    CLDWS* partial = NULL;
    param_t *param = (param_t*)args;

    int agent_valid;
    uint8_t  lane_find;
    uint32_t i, r, c;
    uint32_t rows, cols, area;
    uint32_t count;
    uint32_t find_agent_fail_count,
             evidence_count,
             lane_num,
             find_lane_fail_count,
             max_evidence_percentage_lane_no,
             max_grade_lane_no;
    double evidence_percentage,
           max_evidence_percentage,
           max_grade;

    lanepoint* points   = NULL;
    lanepoint* pp[3];
    lane *l = NULL;
    lane *l_temp[MAX_LANE_NUMBER] = {0};
    gsl_matrix_view* sm = NULL;

    if (!param)
        return NULL;

    partial = param->obj;
    id      = param->id;

    l = partial->m_lane_stat.l[id];

    for (i = 0 ; i < MAX_LANE_NUMBER ; ++i) {
        if ((l_temp[i] = LaneInit()) == NULL)
                goto fail1;

        //ldwsdbg("id[%d]: l_temp[%d]: %p", id, i, l_temp[i]);
    }

    pthread_mutex_lock(&partial->m_mutex[id]);

    while (!partial->m_terminate) {
        pthread_cond_wait(&partial->m_cond[id], &partial->m_mutex[id]);

        if (partial->m_terminate || ((sm = &partial->m_subimgy[id]) == NULL)) {
            pthread_mutex_unlock(&partial->m_mutex[id]);

            pthread_mutex_lock(&partial->m_jobdone_mutex);
            partial->m_thread_done = LANE_THREADS;
            pthread_cond_signal(&partial->m_jobdone_cond);
            pthread_mutex_unlock(&partial->m_jobdone_mutex);

            break;
        }

        rows = sm->matrix.size1;
        cols = sm->matrix.size2;
        area = rows * cols;
        //ldwsdbg("id[%d] : %dx%d", id, rows, cols);

        if (!points)
            points = new lanepoint[area];

        if (!points)
            goto fail2;

        memset(points, 0x0,sizeof(lanepoint) * area); 
        //ldwsdbg("id[%d], points %p", id, points);

        lane_find               = 0;

        find_agent_fail_count   = 
        evidence_count          = 
        lane_num                = 
        find_lane_fail_count    = 
        max_grade_lane_no       = 
        max_evidence_percentage_lane_no = 0;

        evidence_percentage     =
        max_evidence_percentage = 
        max_grade               = 0;

        count = 0;
        for (r = 0 ; r < rows ; ++r) {
            for (c = 0 ; c < cols ; ++c) {
                if (gsl_matrix_get(&sm->matrix, r, c) == 0) {
                    points[count].r = r;
                    points[count].c = c;
                    points[count].pickup = 0;
                    points[count].pol = 0;
                    count++;
                }
            }
        }

        //ldwsdbg("[%d]: count %d", id, count);
        for (;;) {
            if (count < VALID_POINT_COUNT) {
                break;
            }

            find_agent_fail_count = 0;

            /* Find the valid agent points.
             * It might not find valid agent points, so we need a loop to terminate this procedure.
             */ 
            for (;;) {
                /* pickup 3 points randomly */
                for (i = 0 ; i < 3 ; ++i) {
                    while (1) {
                        r = (rand()%(count-1));

                        if (!points[r].pickup) {
                            points[r].pickup = 1;
                            pp[i] = &points[r];
                            break;
                        }
                    }
                }
  
                /* examine validness of agent points. */ 
                agent_valid = check_agent_valid(&pp[0], &pp[1], &pp[2]);

                if (agent_valid != LANE_DETECT_CHECK_OK) {
                    //ldwsdbg("%s: Invalid reason %d", (id == 0 ? "Left" : "Right"), agent_valid);
                    pp[0]->pickup =
                    pp[1]->pickup = 
                    pp[2]->pickup = 0;

                    if ((++find_agent_fail_count) >= MAX_FIND_AGENT_FAIL_COUNT)
                        break;
                } else {
                    /* find valid agent points. */
                    break;
                }
            }

            if (agent_valid != LANE_DETECT_CHECK_OK) {
                lane_find = 0;
                //ldwsdbg("%s: Can not find valid agent points", (id == 0 ? "Left" : "Right"));
                break;
            } else {
                //ldwsdbg("Find valid agent points");
            }

            evidence_count = 0;
            kp_solve(pp, l_temp[lane_num]);

            /* evidence checking. */
            evidence_percentage = kp_evidence_check(count, points, l_temp[lane_num]);

            if (evidence_percentage > VALID_LINE_EVIDENCE_PERCENTAGE) {
                lane_find = 1;
                l_temp[lane_num++]->evidence_percentage = evidence_percentage;

                if (lane_num < MAX_LANE_NUMBER) {
                    for (i = 0 ; i < count ; ++i)
                        points[i].pickup = 0;
                } else 
                    break; // exist for loop
            } else {
                if (++find_lane_fail_count >= MAX_FIND_AGENT_FAIL_COUNT) {
                    ldwsdbg("MAX_FIND_AGENT_FAIL_COUNT reach");
                    break; // exist for loop
                }
                //ldwsdbg("%s: find_lane_fail_count %d", (id == 0 ? "Left" : "Right"), find_lane_fail_count);
            }
        }

        if (lane_find) {
            //ldwsdbg("%s: Find lanes", (id == 0 ? "Left" : "Right"));

            /* calculate pixel center of each lane. */
            for (i = 0 ; i < lane_num ; ++i)
                kp_gen_point(rows, cols, l_temp[i], partial->m_rowoffset, (id == 0 ? partial->m_left_coloffset : partial->m_right_coloffset));

            /* find max grade lane.*/
            for (i = 0 ; i < lane_num ; ++i) {
                l_temp[i]->grade = (l_temp[i]->evidence_percentage * 100 * 0.6 + l_temp[i]->pix_col_center * 0.4);
                if (max_grade < l_temp[i]->grade) {
                    max_grade = l_temp[i]->grade;
                    max_grade_lane_no = i;
                }
            }

            l->kluge_poly.k = l_temp[max_grade_lane_no]->kluge_poly.k;
            l->kluge_poly.b = l_temp[max_grade_lane_no]->kluge_poly.b;
            l->kluge_poly.v = l_temp[max_grade_lane_no]->kluge_poly.v;
 
            l->exist        = 1;
            kp_gen_point(rows, cols, l, partial->m_rowoffset, (id == 0 ? partial->m_left_coloffset : partial->m_right_coloffset));
        } else {
            //ldwsdbg("%s: Can not find lanes", (id == 0 ? "Left" : "Right"));
            /* evidence checking for previous lane poly. */
            if (l->exist) {
                evidence_percentage = kp_evidence_check(count, points, l);
                kp_gen_point(rows, cols, l, partial->m_rowoffset, (id == 0 ? partial->m_left_coloffset : partial->m_right_coloffset));

                if (evidence_percentage > VALID_LINE_EVIDENCE_PERCENTAGE) {
                    l->fail_count = 0;
                } else {
                    if (++l->fail_count > LANE_FAIL_COUNT) {
                        l->exist = 0;
                        kp_rel_point(l);
                    } else {
                        ldwsdbg("%s: Keep previous lane", (id == 0 ? "Left" : "Right"));
                    }
                }
            }
        }

        pthread_mutex_lock(&partial->m_jobdone_mutex);
        partial->m_thread_done++;
        pthread_cond_signal(&partial->m_jobdone_cond);
        pthread_mutex_unlock(&partial->m_jobdone_mutex);
    }

    if (points) {
        delete [] points;
        points = NULL;
    }

    for (i = 0 ; i < LANE_NUM ; ++i)
        LaneDeinit(&l_temp[i]);

    pthread_mutex_unlock(&partial->m_mutex[id]);

    return NULL;

fail2:
    partial->m_terminate = TRUE;
    pthread_mutex_unlock(&partial->m_mutex[id]);

    pthread_mutex_lock(&partial->m_jobdone_mutex);
    partial->m_thread_done = LANE_THREADS;
    pthread_cond_signal(&partial->m_jobdone_cond);
    pthread_mutex_unlock(&partial->m_jobdone_mutex);
fail1:
    for (i = 0 ; i < LANE_NUM ; ++i)
        LaneDeinit(&l_temp[i]);

    return NULL;
}

BOOL CLDWS::FindLane(gsl_matrix* src,
                    int start_row,
                    int start_col,
                    lanepoint* p,
                    lane *l[LANE_NUM])
{
    lane *left = NULL, *right = NULL, *center = NULL;

    left    = l[LANE_LEFT];
    right   = l[LANE_RIGHT];
    center  = l[LANE_CENTER];

    if (!src || !left || !right || !center) {
        ldwsdbg();
        return FALSE;
    }

    ldwsdbg(LIGHT_RED "=============New Frame=============" NONE);

    // prepare partial data for each thead.
    m_left_coloffset = 0;
    m_subimgy[LANE_LEFT] = gsl_matrix_submatrix(src, 
                                                start_row, 
                                                0, 
                                                src->size1 - start_row, 
                                                src->size2 / 2);

    m_right_coloffset = src->size2 / 2;
    m_subimgy[LANE_RIGHT] = gsl_matrix_submatrix(src, 
                                                start_row, 
                                                src->size2 / 2, 
                                                src->size1 - start_row, 
                                                src->size2 / 2);

    WakeUpThread();
    WaitThreadDone();

    if (m_terminate)
        return FALSE;

    // Update status of center lane
    UpdateLaneStatus(src->size1 - start_row,
                     src->size2 - start_col,
                     left,
                     right,
                     center);

    CalStraightLanesPoly();
    CalVanishingPoint();

    return TRUE;
}












