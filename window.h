#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <SDL.h>
#include <SDL_ttf.h>
#include <pthread.h>

//#include "selector.h"

enum {
    SELECTOR_CENTER = 0,
    SELECTOR_LEFT,
    SELECTOR_RIGHT,
    SELECTOR_PLATE,
    SELECTOR_TOTAL,
};

class CMainWindow {
    protected:
        bool            m_terminate;
        bool            m_quit;

        std::string     m_yuv_folder;
        std::string     m_output_yuv_folder;
        std::string     m_titlename;
        int             m_width;
        int             m_height;

        bool            m_nextfile;
        std::vector<std::string> m_filelist;
        std::vector<std::string>::iterator m_it;

//        CSelector*      m_current_selector;
//        CSelector*      m_selector[SELECTOR_TOTAL];

        pthread_cond_t  m_cond;
        pthread_mutex_t m_mutex;

        SDL_Window*     m_window;
        SDL_Renderer*   m_renderer;
        SDL_Surface*    m_surface;
        SDL_Texture*    m_texture;
        SDL_Event*      m_event;

        std::string     m_cur_fpath;
        uint8_t*        m_yuv_buf;

        TTF_Font*       m_Font;

        bool            m_btn_n;
        bool            m_btn_s;

    public:
        CMainWindow();

        CMainWindow(std::string titlename, std::string yuv_folder, int width, int height);

        ~CMainWindow();

        bool Init();

        void ProcessEvent();

        bool IsQuit();

        void Terminate();

        bool BtnPressed_n();

        bool BtnPressed_s();


        void GotoNextFile();

        void GotoNextStep();
    protected:
        bool InitFont();

        void Draw();


        void ProcessKeyEvent();

        void ProcessMouseEvent();

        void ProcessWindowEvent();

        bool LoadYUVFolder();

        bool DrawYUVImage();

        void DrawSelector();

        void CreateOutputFolder();

        void SaveFeatureYUV();
};

#endif
