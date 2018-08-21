#ifndef _BASEWINDOW_H_
#define _BASEWINDOW_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <SDL.h>
#include <SDL_ttf.h>
#include <pthread.h>

enum {
    SELECTOR_CENTER = 0,
    SELECTOR_LEFT,
    SELECTOR_RIGHT,
    SELECTOR_PLATE,
    SELECTOR_TOTAL,
};

using namespace std;

class CBaseWindow {
    protected:
        bool            m_terminate;
        bool            m_quit;

        string          m_yuv_folder;
        string          m_output_yuv_folder;
        string          m_titlename;
        int             m_width;
        int             m_height;

        bool            m_nextfile;
        vector<string>  m_filelist;
        vector<string>::iterator m_it;

        pthread_cond_t  m_cond;
        pthread_mutex_t m_mutex;

        SDL_Window*     m_window;
        SDL_Renderer*   m_renderer;
        SDL_Surface*    m_surface;
        SDL_Texture*    m_texture;
        SDL_Event*      m_event;

        string          m_cur_fpath;
        uint8_t*        m_yuv_buf;

        TTF_Font*       m_Font;

        bool            m_btn_n;
        bool            m_btn_s;

    public:
        CBaseWindow();

        CBaseWindow(std::string titlename, std::string yuv_folder, int width, int height);

        virtual ~CBaseWindow();

        bool Init();

        virtual void ProcessEvent();

        bool IsQuit();

        void Terminate();

        bool BtnPressed_n();

        bool BtnPressed_s();

        void GotoNextFile();

        void GotoNextStep();

    protected:
        bool InitFont();

        virtual void Draw();

        virtual void ProcessKeyEvent();

        void ProcessMouseEvent();

        void ProcessWindowEvent();

        bool LoadYUVFolder();

        bool DrawYUVImage();

        void DrawSelector();
        
        //void DrawShiftWindow();

        void CreateOutputFolder();

        void SaveFeatureYUV();
};

#endif
