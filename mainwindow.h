#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "common.h"
#include "basewindow.h"
#include "fcws.h"

class CMainWindow : public CBaseWindow
{
protected:
    bool m_process_image;
    bool m_update_img;

    CFCWS*  m_fcws;

    gsl_vector*     m_vertical_hist;
    gsl_vector*     m_horizontal_hist;
    gsl_vector*     m_grayscale_hist;

public:
    CMainWindow(string titlename, string yuv_folder, int w, int h);

    ~CMainWindow();

    bool Init();

    void ProcessEvent();


protected:
    void Draw();

    void ProcessKeyEvent();

    bool ProcessImage();

    bool UpdateTexture();
};
#endif
