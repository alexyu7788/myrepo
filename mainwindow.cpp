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

    m_vcs.clear();



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

    m_vertical_hist     = gsl_vector_alloc(m_width);
    m_horizontal_hist   = gsl_vector_alloc(m_height);
    m_grayscale_hist    = gsl_vector_alloc(256);

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

            if (m_event->type == SDL_WINDOWEVENT)
                ProcessWindowEvent();
        }

        ProcessImage();

        Draw();
    }
}

//-------------------------------------------------------------------
void CMainWindow::Draw()
{
    SDL_Color color;

    if (m_renderer && m_texture)
    {
        pthread_mutex_lock(&m_mutex);

        // Clear renderer
        SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(m_renderer);

        // Load and draw yuv image
        DrawYUVImage();
        SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);

        UpdateTexture();

        // Draw Grayscale Histogram
        color.r = 0x80;
        color.g = 0x00;
        color.b = 0x80;
        color.a = 0xFF;
        DrawHistogram(m_grayscale_hist, 0, 5, &color, m_window_width, 2);

        // Draw Vertical Histogram
        color.r = 0x00;
        color.g = 0xff;
        color.b = 0;
        color.a = 0xFF;
        DrawHistogram(m_vertical_hist, 0, 5, &color, m_window_width, 2);

        // Draw Horizontal Histogram
        color.r = 0xff;
        color.g = 0x0;
        color.b = 0x00;
        color.a = 0xFF;
        DrawHistogram(m_horizontal_hist, 1, 5, &color, 2, m_window_height);

        // Draw Vehicle Candidates
        DrawVehicleCandidates();

        // Show
        SDL_RenderPresent(m_renderer);

        pthread_mutex_unlock(&m_mutex);
    }
}

void CMainWindow::DrawHistogram(gsl_vector* vect, int pos, int offset, SDL_Color* color, int scale_w, int scale_h)
{
    if (!vect) {
        dbg();
        return ;
    }

    double max_val = 0;

    SDL_SetRenderDrawColor(m_renderer, color->r, color->g, color->b, color->a);

    switch (pos) {
        case 0: //bottom
            max_val = gsl_vector_max(vect);

            for (uint32_t i=0 ; i<vect->size - 1 ; i++)
            {
                for (int j=0 ; j<3 ; j++) {
                SDL_RenderDrawLine(m_renderer,
                        (i * scale_w) / vect->size,
                        m_window_height - offset + j - (gsl_vector_get(vect, i) * (m_window_height / scale_h)) / max_val,
                        (( i + 1) * scale_w) / vect->size,
                        m_window_height - offset + j - (gsl_vector_get(vect, i+1) * (m_window_height / scale_h)) / max_val);
                }
            }
            break;
        case 1: // right
            max_val = gsl_vector_max(vect);

            for (uint32_t i=0 ; i<vect->size - 1 ; i++)
            {
                for (int j=0 ; j<3 ; j++) {
                SDL_RenderDrawLine(m_renderer,
                        m_window_width - offset + j - (gsl_vector_get(vect, i) * (vect->size / scale_w)) / max_val,
                        i * m_window_height / vect->size,
                        m_window_width - offset + j - (gsl_vector_get(vect, i+1) * (vect->size / scale_w)) / max_val,
                        (i + 1 ) * m_window_height / vect->size);
                }
            }
            break;
        case 2: // up

            break;
        case 3: // left

            break;
        default:
            break;
    }
}
void CMainWindow::DrawVehicleCandidates()
{
    if (m_vcs.size() == 0)
        return;

    uint32_t r, c, w, h;
    CandidatesIT it;
    SDL_Color color;
    SDL_Rect rect;

    for (it = m_vcs.begin() ; it != m_vcs.end(); it++) {
        (*it)->GetPos(r, c);
        (*it)->GetWH(w, h);

        color.r = 0x0;
        color.g = 0xFF;
        color.b = 0xFF;
        color.a = 0x50;
        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);

        rect.x = c;
        rect.y = r;
        rect.w = w;
        rect.h = h;
        SDL_RenderFillRect(m_renderer, &rect);
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

void CMainWindow::ProcessWindowEvent()
{
    if (m_event == NULL)
        return;

    switch (m_event->window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            //dbg("%d, %d", m_event->window.data1, m_event->window.data2);
            m_window_width  = m_event->window.data1;
            m_window_height = m_event->window.data2;
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
            m_fcws->DoDetection(m_yuv_buf, 
                                m_width, 
                                m_height, 
                                m_vertical_hist, 
                                m_horizontal_hist, 
                                m_grayscale_hist,
                                m_vcs);
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
    rect.w = m_window_width;
    rect.h = m_window_height;

    SDL_UpdateTexture(m_texture, &rect, m_yuv_buf, m_window_width);

//    Viewport.x = 0;
//    Viewport.y = 0;
//    Viewport.w = m_window_width;
//    Viewport.h = m_window_height;
//    SDL_RenderSetViewport(m_renderer, &Viewport);


    return true;
}

