#ifndef __DLIB_H__
#define __DLIB_H__

#include <iostream>
#include <dlib/dnn.h>
#include <dlib/image_io.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <pthread.h>

//using namespace dlib;

// The front and rear view vehicle detector network
template <long num_filters, typename SUBNET> using con5d = dlib::con<num_filters,5,5,2,2,SUBNET>;
template <long num_filters, typename SUBNET> using con5  = dlib::con<num_filters,5,5,1,1,SUBNET>;
template <typename SUBNET> using downsampler  = dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<32, dlib::relu<dlib::affine<con5d<16,SUBNET>>>>>>>>>;
template <typename SUBNET> using rcon5  = dlib::relu<dlib::affine<con5<55,SUBNET>>>;
<<<<<<< HEAD
//using net_type = dlib::loss_mmod<dlib::con<1,9,9,1,1,rcon5<rcon5<rcon5<downsampler<dlib::input_rgb_image_pyramid<dlib::pyramid_down<6>>>>>>>>;
using net_type = dlib::loss_mmod<dlib::con<1,9,9,1,1,rcon5<rcon5<rcon5<downsampler<dlib::input<dlib::matrix<unsigned char>>>>>>>>;
=======
using net_type = dlib::loss_mmod<dlib::con<1,9,9,1,1,rcon5<rcon5<rcon5<downsampler<dlib::input_rgb_image_pyramid<dlib::pyramid_down<6>>>>>>>>;
//using net_type = dlib::loss_mmod<dlib::con<1,9,9,1,1,rcon5<rcon5<rcon5<downsampler<dlib::input<dlib::matrix<unsigned char>>>>>>>>;
>>>>>>> 1. USe dlib instead of gsl.(not ready)

class CDlib {
protected:

    bool                        m_terminate;
<<<<<<< HEAD
    net_type                    m_net;
    dlib::shape_predictor       m_sp;
    dlib::matrix<unsigned char> m_img;
=======
    bool                        m_busy;
    net_type                    m_net;
    dlib::shape_predictor       m_sp;
    dlib::matrix<dlib::rgb_pixel>     m_img;
>>>>>>> 1. USe dlib instead of gsl.(not ready)

    pthread_t                   m_processthread;
    pthread_cond_t              m_cond;
    pthread_mutex_t             m_mutex;

public:
    CDlib();

    ~CDlib();

    bool Init(char* model);

    void DoDetection(uint8_t* img, int w, int h, int linesize);

protected:
    static void* ProcessThread(void* args);
};
#endif
