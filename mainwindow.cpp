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

        //Draw selector
//        DrawSelector();

        UpdateTexture();

        // Draw Grayscale Histogram
        color.r = 0x80;
        color.g = 0x00;
        color.b = 0x80;
        color.a = 0xFF;
        DrawHistogram(m_grayscale_hist, 0, 200, &color, 256, 4);

        // Draw Vertical Histogram
        color.r = 0x00;
        color.g = 0xff;
        color.b = 0;
        color.a = 0xFF;
        DrawHistogram(m_vertical_hist, 0, 5, &color, m_width, 4);

        // Draw Vertical Histogram
        color.r = 0xff;
        color.g = 0x0;
        color.b = 0x00;
        color.a = 0xFF;
        DrawHistogram(m_horizontal_hist, 1, 5, &color, 2, m_height);

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
                SDL_RenderDrawLine(m_renderer,
                        (i * m_width) / scale_w,
                        m_height - offset - (gsl_vector_get(vect, i) * (m_height / scale_h)) / max_val,
                        (( i + 1) * m_width ) / scale_w,
                        m_height - offset - (gsl_vector_get(vect, i+1) * (m_height / scale_h)) / max_val);
            }
            break;
        case 1: // right
            max_val = gsl_vector_max(vect);

            for (uint32_t i=0 ; i<vect->size - 1 ; i++)
            {
                SDL_RenderDrawLine(m_renderer,
                                   m_width - offset - (gsl_vector_get(vect, i) * (m_width / scale_w)) / max_val,
                                   i,
                                   m_width - offset - (gsl_vector_get(vect, i+1) * (m_width / scale_w)) / max_val,
                                   i + 1);
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


    return true;
}

