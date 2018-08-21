#include "mainwindow.h"

CMainWindow::CMainWindow(string titlename, string yuv_folder, int w, int h):CBaseWindow(titlename, yuv_folder, w, h)
{
    m_process_image = 
    m_update_img    =
    false;

    m_fcws = NULL;

    m_vertical_hist = NULL;
    m_horizontal_hist = NULL;
    m_grayscale_hist = NULL;





}

CMainWindow::~CMainWindow()
{
    if (m_vertical_hist) {
        gsl_vector_free(m_vertical_hist);
        m_vertical_hist = NULL;
    }

    if (m_horizontal_hist) {
        gsl_vector_free(m_horizontal_hist);
        m_horizontal_hist = NULL;
    }
    
    if (m_grayscale_hist) {
        gsl_vector_free(m_grayscale_hist);
        m_grayscale_hist = NULL;
    }

    if (m_fcws) {
        delete m_fcws;
        m_fcws = NULL;
    }

}

bool CMainWindow::Init()
{
    m_fcws = new CFCWS();

    m_vertical_hist = gsl_vector_alloc(m_height);
    m_horizontal_hist = gsl_vector_alloc(m_width);
    m_grayscale_hist = gsl_vector_alloc(256);

    return CBaseWindow::Init();
}

void CMainWindow::ProcessEvent()
{
    while (!m_terminate)
    {
        while (SDL_PollEvent(m_event) != 0)
        {
            if (m_event->type == SDL_KEYDOWN)
                ProcessKeyEvent();

            if (m_event->type == SDL_MOUSEBUTTONDOWN || 
                m_event->type == SDL_MOUSEBUTTONUP || 
                m_event->type == SDL_MOUSEMOTION || 
                m_event->type == SDL_MOUSEWHEEL)
                CBaseWindow::ProcessMouseEvent();

            if (m_event->type == SDL_QUIT)
                m_terminate = true;
        }

        if (ProcessImage())
            UpdateTexture();

        Draw();
    }
}

//-------------------------------------------------------------------
void CMainWindow::Draw()
{
    if (m_renderer && m_texture)
    {
        pthread_mutex_lock(&m_mutex);

        // Clear renderer
        SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(m_renderer);

        // Load and draw yuv image
        DrawYUVImage();
        SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);

        //Draw selector
//        DrawSelector();

        UpdateTexture();

        //DrawShiftWindow();
        // Show
        SDL_RenderPresent(m_renderer);

        pthread_mutex_unlock(&m_mutex);
    }
}

void CMainWindow::ProcessKeyEvent()
{
    if (m_event == NULL)
        return;

    switch(m_event->key.keysym.sym)
    {
        case SDLK_q:
            m_terminate = true;
            break;
         case SDLK_n:
            m_nextfile = true;
            break;
        case SDLK_s:
            m_process_image = true;
            break;
        default:
        break;
    }
} 

bool CMainWindow::ProcessImage()
{
    bool ret = true;

    if (m_process_image) {
        m_process_image = false;

        if (m_fcws) {
            m_fcws->DoDetection(m_yuv_buf, m_width, m_height, m_vertical_hist, m_horizontal_hist, m_grayscale_hist);
        }

        return true;
    }

    return false;
}

bool CMainWindow::UpdateTexture()
{
    SDL_Rect rect;
    SDL_Rect Viewport;

    rect.x = 0;
    rect.y = 0;
    rect.w = m_width;
    rect.h = m_height;

    SDL_UpdateTexture(m_texture, &rect, m_yuv_buf, m_width);

//    Viewport.x = 0;
//    Viewport.y = 0;
//    Viewport.w = m_width;
//    Viewport.h = m_height;
//    SDL_RenderSetViewport(m_renderer, &Viewport);

    SDL_SetRenderDrawColor(m_renderer, 0x00, 0x00, 0xFF, 0x80);
    for (int i=0 ; i<m_grayscale_hist->size - 1 ; i++)
    {
        SDL_RenderDrawLine(m_renderer,
                         240 - i,
                        gsl_vector_get(m_grayscale_hist, i) * 1/3,
                        240 - i-1,
                        gsl_vector_get(m_grayscale_hist, i+1) * 1/3);
    }

    return true;
}

