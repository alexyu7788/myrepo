#include "fcws.hpp"

// ------------------protected function---------

// ------------------public function---------
 
CFCWS::CFCWS()
{
    m_terminate = false;

    m_ip = NULL;

    memset(&m_mutex, 0x0, sizeof(pthread_mutex_t));
    memset(&m_cond, 0x0, sizeof(pthread_cond_t));

    m_imgy      = 
    m_imgu      = 
    m_imgv      = 
    m_vedgeimg  =
    m_heatmap   =
    m_tempimg   =
    m_intimg    =   
    m_shadowimg = NULL;

    m_gradient  = NULL;
    m_direction = NULL;
    m_heatmapid = NULL;
}

CFCWS::~CFCWS()
{
    Deinit();
}

bool CFCWS::Init()
{
    pthread_mutexattr_t attr;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&m_cond, NULL);

    return false;
}

bool CFCWS::Deinit()
{
    m_terminate = true;

    if (m_ip) {
        delete m_ip;
        m_ip = NULL;
    }

    pthread_mutex_lock(&m_mutex);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_imgu);
    FreeMatrix(&m_imgv);
    FreeMatrix(&m_vedgeimg);
    FreeMatrix(&m_heatmap);
    FreeMatrix(&m_tempimg);
    FreeMatrix(&m_intimg);
    FreeMatrix(&m_shadowimg);

    FreeMatrixUshort(&m_gradient);
    FreeMatrixChar(&m_direction);
    FreeMatrixChar(&m_heatmapid);

    return true;
}

bool CFCWS::DoDetection(CLDWS* ldws_obj,
                        uint8_t* imgy,
                        int w,
                        int h,
                        int linesize)
{
    if (!m_ip || !imgy || !w || !h || !linesize) {
        fcwsdbg();
        return false;
    }

    // Allocate/Clear matrixs.
    CheckOrReallocMatrix(&m_imgy, h, w, true);
    CheckOrReallocMatrix(&m_imgu, h/2, w/2, true);
    CheckOrReallocMatrix(&m_imgv, h/2, w/2, true);
    CheckOrReallocMatrix(&m_vedgeimg, h, w, true);
    CheckOrReallocMatrix(&m_tempimg, h, w, true);
    CheckOrReallocMatrix(&m_intimg, h, w, true);
    CheckOrReallocMatrix(&m_shadowimg, h, w, true);
    CheckOrReallocMatrixUshort(&m_gradient, h, w, true);
    CheckOrReallocMatrixChar(&m_direction, h, w, true);

    // Result will be used for next round.
    CheckOrReallocMatrix(&m_heatmap, h, w, false);
    CheckOrReallocMatrixChar(&m_heatmapid, h, w, false);

    m_ip->CopyMatrix(imgy, m_imgy, w, h, linesize);
    m_ip->CopyMatrix(imgy, m_tempimg, w, h, linesize);

    // Integral Image & Thresholding
    m_ip->GenIntegralImage(m_imgy, m_intimg);
    m_ip->ThresholdingByIntegralImage(m_imgy, m_intimg, m_shadowimg, 50, 0.7);

    // ROI
    if (ldws_obj && ldws_obj->ApplyDynamicROI(m_shadowimg) == false) {
        fcwsdbg();
    }














    return true;
}
