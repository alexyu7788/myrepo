#include "dlib.h"
 
CDlib::CDlib()
{
    m_terminate = 0;

    memset(&m_processthread, 0x0, sizeof(pthread_t));
    memset(&m_cond, 0x0, sizeof(pthread_cond_t));
    memset(&m_mutex, 0x0, sizeof(pthread_mutex_t));

    // HOG
    m_results.clear();
}
  
CDlib::~CDlib()
{
    m_terminate = 1;

    if (m_processthread) {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);

        pthread_join(m_processthread, NULL);
    }

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);


    // HOG
    m_results.clear();
}

bool CDlib::Init(char* model)
{
    dlib::deserialize(model) >> m_net >> m_sp;

    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);

    pthread_create(&m_processthread, NULL, ProcessThread, this);
    return true;
}

bool CDlib::HogDectectorInit(char* model)
{
    dlib::deserialize(model) >> m_hog_detector;

    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);

    pthread_create(&m_processthread, NULL, ProcessHogDetectorThread, this);
    return true;
}

void CDlib::DoDetection(uint8_t* img, int w, int h, int linesize)
{
    long r, c;
    dlib::matrix<unsigned char> temp;

    temp.set_size(h, w);

    for (r=0 ; r<h ; ++r) {
        for (c=0 ; c<w ; ++c) {
            temp(r, c) = img[r * linesize + c];
        }
    }

    assign_image(m_img, temp);

    if (pthread_mutex_trylock(&m_mutex) == 0) {
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }
}

void CDlib::DoHogDetection(uint8_t* img, int w, int h, int linesize)
{
    long r, c;
    dlib::matrix<unsigned char> temp;

    m_hog_img.set_size(h, w);

    for (r=0 ; r<h ; ++r) {
        for (c=0 ; c<w ; ++c) {
            m_hog_img[r][c] = img[r * linesize + c];
        }
    }

    if (pthread_mutex_trylock(&m_mutex) == 0) {
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }
}

void CDlib::GetHogDetectionResult(std::vector<dlib::rectangle>& result)
{
    std::vector<dlib::rectangle>::iterator it;

    pthread_mutex_lock(&m_mutex);

    result.clear();

    for (it = m_results.begin() ; it != m_results.end() ; ++it) {
        dlib::rectangle rect(it->left(), it->top(), it->right(), it->bottom());
        result.push_back(rect);
    }

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

        for (auto&& d : pThis->m_net(pThis->m_img)) {
            auto fd = pThis->m_sp(pThis->m_img, d);
            dlib::rectangle rect;

            for (unsigned long j=0 ; j < fd.num_parts() ; ++j) {
                rect += fd.part(j);
            }

            if (d.label == "rear") {
                printf("Rear at (%ld, %ld) with (%ld, %ld)\n",
                        rect.left(),
                        rect.top(),
                        rect.right(),
                        rect.bottom());
            }
        }
    }

    pthread_mutex_unlock(&pThis->m_mutex);

    return NULL;
}

void* CDlib::ProcessHogDetectorThread(void* args)
{
    int count;
    CDlib* pThis = (CDlib*)args;
    std::vector<dlib::rectangle>::iterator it;

    pthread_mutex_lock(&pThis->m_mutex);

    while (!pThis->m_terminate) {
        pthread_cond_wait(&pThis->m_cond, &pThis->m_mutex);

        if (pThis->m_terminate)
            break;

        if (pThis->m_results.size())
            printf("=================\n");

        count = 0;
        pThis->m_results.clear();

        pThis->m_results = pThis->m_hog_detector(pThis->m_hog_img);

        for (it = pThis->m_results.begin() ; it != pThis->m_results.end() ; ++it) {
            printf("vehicle[%d] is at (%ld, %ld) with (%ld, %ld)\n",
                        count++, 
                        it->left(),
                        it->top(),
                        it->right(),
                        it->bottom());
        }
    }

    pthread_mutex_unlock(&pThis->m_mutex);

    return NULL;
}
