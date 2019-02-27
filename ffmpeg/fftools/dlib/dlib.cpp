#include "dlib.h"
 
CDlib::CDlib()
{
    m_terminate = 0;
    m_busy = false;

    memset(&m_processthread, 0x0, sizeof(pthread_t));
    memset(&m_cond, 0x0, sizeof(pthread_cond_t));
    memset(&m_mutex, 0x0, sizeof(pthread_mutex_t));
}

CDlib::~CDlib()
{
    m_terminate = 1;

    if (m_processthread) {
        pthread_cond_signal(&m_cond);
        pthread_join(m_processthread, NULL);
    }

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

bool CDlib::Init(char* model)
{
    dlib::deserialize(model) >> m_net >> m_sp;

    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);

    pthread_create(&m_processthread, NULL, ProcessThread, this);
    return true;
}

void CDlib::DoDetection(uint8_t* img, int w, int h, int linesize)
{
    long r, c;
    dlib::matrix<unsigned char> temp;

    if (m_busy)
        return;

    temp.set_size(h, w);

    for (r=0 ; r<h ; ++r) {
        for (c=0 ; c<w ; ++c) {
            temp(r, c) = img[r * linesize + c];
        }
    }

    assign_image(m_img, temp);

    pthread_mutex_lock(&m_mutex);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void* CDlib::ProcessThread(void* args)
{
    CDlib* pThis = (CDlib*)args;

    pthread_mutex_lock(&pThis->m_mutex);

    while (!pThis->m_terminate) {
        pthread_cond_wait(&pThis->m_cond, &pThis->m_mutex);

        if (pThis->m_terminate) {
            break;
        }

        pThis->m_busy = true;

        for (auto&& d : pThis->m_net(pThis->m_img)) {
            auto fd = pThis->m_sp(pThis->m_img, d);
            dlib::rectangle rect;

            for (unsigned long j=0 ; j < fd.num_parts() ; ++j) {
                rect += fd.part(j);
            }

            if (d.label == "rear") {
                printf("Rear at (%d, %d) with (%d, %d)\n",
                        rect.left(),
                        rect.top(),
                        rect.right(),
                        rect.bottom());
            }
        }

        pThis->m_busy = false;
    }

    pthread_mutex_unlock(&pThis->m_mutex);

    return NULL;
}
