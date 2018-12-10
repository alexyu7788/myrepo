#ifndef _FCWS_H_
#define _FCWS_H_
#ifdef __cplusplus
extern "C" {
#include "../utils/ma.h"
}
#endif

#include "../utils/common.hpp"
#include "../utils/imgproc.hpp"
#include "../ldws/ldws.hpp"

class CFCWS;

class CFCWS {
    protected:
        bool            m_terminate;

        // Image Process object
        CImgProc*       m_ip; 

        pthread_mutex_t m_mutex;
        pthread_cond_t  m_cond; 

        gsl_matrix*     m_imgy;
        gsl_matrix*     m_imgu;
        gsl_matrix*     m_imgv;
        gsl_matrix*     m_vedgeimg;
        gsl_matrix*     m_heatmap;
        gsl_matrix*     m_tempimg;
        gsl_matrix*     m_intimg;       // integral image
        gsl_matrix*     m_shadowimg;


        gsl_matrix_ushort*  m_gradient;
        gsl_matrix_char*    m_direction;
        gsl_matrix_char*    m_heatmapid;

    protected:

    public:
        CFCWS();

        ~CFCWS();

        bool Init();

        bool Deinit();

        bool DoDetection(CLDWS* ldws_obj,
                         uint8_t* imgy,
                         int w,
                         int h,
                         int linesize);
};
#endif
