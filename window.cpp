#include <sys/stat.h>
#include <dirent.h>
#include "window.h"
#include "utility.h"

CMainWindow::CMainWindow()
{

}

CMainWindow::CMainWindow(std::string titlename, std::string yuv_folder, int width, int height): 
m_terminate(false),
m_quit(false),
m_yuv_folder(yuv_folder),
m_titlename(titlename),
m_width(width),
m_height(height),
m_nextfile(false),
m_window(NULL), 
m_renderer(NULL),
m_surface(NULL), 
m_texture(NULL), 
m_event(NULL),
m_yuv_buf(NULL),
m_Font(NULL),
m_btn_n(false),
m_btn_s(false)
{
    m_vcs.clear();

    
}

CMainWindow::~CMainWindow()
{
//    for (int i=0 ; i<SELECTOR_TOTAL ; i++)
//    {
//        if (m_selector[i])
//        {
//            delete m_selector[i];
//            m_selector[i] = NULL;
//        }
//    }

    m_filelist.clear();
    if (m_yuv_buf)
    {
        free(m_yuv_buf);
        m_yuv_buf = NULL;
    }

    SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    m_texture = NULL;
    m_renderer = NULL;
    m_window = NULL;

    TTF_Quit();
    SDL_Quit();

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
    
    m_vcs.clear();

    m_terminate = true;
}

bool CMainWindow::Init()
{
    bool ret = true;
    int y_size, uv_size, yuv_size;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize. SDL Error:%s\n", SDL_GetError());
        ret = false;
    }
    else
    {
        //Set texture filtering to linear
        if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
        {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        m_window = SDL_CreateWindow((const char*)m_titlename.c_str(), 
                                    200, 
                                    SDL_WINDOWPOS_UNDEFINED,
                                    m_width,
                                    m_height,
                                    SDL_WINDOW_SHOWN);

        if (m_window == NULL)
        {
            printf("Window could not be created.SDL Error:%s\n", SDL_GetError());
            ret = false;
        }
        else
        {
            m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

            if (m_renderer == NULL)
            {
                printf("Renderer could not be created. SDL Error:%s\n", SDL_GetError());
                ret = false;
            }
            else
            {
                SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);

                m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
                
                if (m_texture == NULL)
                {
                    printf("Texture could not be created. SDL Error:%s\n", SDL_GetError());
                }
                else
                {
                    m_event = new SDL_Event();

                    if (m_event == NULL)
                    {
                       printf("Event queue could not be created.\n");
                       ret = false;
                    }
                    else
                    {
                        ret = InitFont();
                    }
                }
            }
        }
    }   

    if (ret == true)
    {
        LoadYUVFolder();
//
//        m_selector[SELECTOR_CENTER] = new CSelector(m_width, m_height);
//        m_selector[SELECTOR_CENTER]->SetColor(0xFF, 0, 0);
//        m_selector[SELECTOR_CENTER]->SetGeometryInfo(85, 10, 75, 20);
//        m_selector[SELECTOR_CENTER]->SetTitle(m_renderer, "Center");
//
//        m_selector[SELECTOR_LEFT] = new CSelector(m_width, m_height);
//        m_selector[SELECTOR_LEFT]->SetColor(0, 0xFF, 0);
//        m_selector[SELECTOR_LEFT]->SetGeometryInfo(3, 115, 30, 50);
//        m_selector[SELECTOR_LEFT]->SetTitle(m_renderer, "Left");
//
//        m_selector[SELECTOR_RIGHT] = new CSelector(m_width, m_height);
//        m_selector[SELECTOR_RIGHT]->SetColor(0, 0, 0xFF);
//        m_selector[SELECTOR_RIGHT]->SetGeometryInfo(260, 115, 30, 50);
//        m_selector[SELECTOR_RIGHT]->SetTitle(m_renderer, "Right");

        // m_selector[SELECTOR_PLATE] = new CSelector(m_width, m_height);
        // m_selector[SELECTOR_PLATE]->SetColor(0xFF, 0xFF, 0);
        // m_selector[SELECTOR_PLATE]->SetGeometryInfo(120, 115, 75, 50);
        // m_selector[SELECTOR_PLATE]->SetTitle(m_renderer, "Plate");

//        m_current_selector = m_selector[SELECTOR_CENTER];

        // Initialize YUV buffer
        y_size  = m_width * m_height;
        uv_size = ((m_width >> 1) * (m_height >> 1)) << 1;
        yuv_size = y_size + uv_size;

        m_yuv_buf = (uint8_t*)malloc(sizeof(uint8_t) * yuv_size);
        memset(m_yuv_buf, 0x80, yuv_size);        

        pthread_cond_init(&m_cond, NULL);
        pthread_mutex_init(&m_mutex, NULL);
    }

    return ret;
}

bool CMainWindow::IsQuit()
{
    return m_quit;
}

void CMainWindow::Terminate()
{
    m_terminate = true;
}

bool CMainWindow::BtnPressed_n()
{
    bool res;

    pthread_mutex_lock(&m_mutex);
    res = m_btn_n;
    m_btn_n = false;
    pthread_mutex_unlock(&m_mutex);

    return res;
}

bool CMainWindow::BtnPressed_s()
{
    bool res;

    pthread_mutex_lock(&m_mutex);
    res = m_btn_s;
    m_btn_s = false;
    pthread_mutex_unlock(&m_mutex);

    return res;

}

void CMainWindow::GotoNextFile()
{
    pthread_mutex_lock(&m_mutex);
    m_nextfile = true;
    pthread_mutex_unlock(&m_mutex);
}

void CMainWindow::UpdateDrawInfo(Candidates &vcs)
{
    int i;
    int r, c, w, h;
    CandidatesIt it;

    pthread_mutex_lock(&m_mutex);

    m_vcs.clear();

    for (it = vcs.begin(); it != vcs.end(); ++it)
        m_vcs.push_back(*it);

    pthread_mutex_unlock(&m_mutex);
}









void CMainWindow::ProcessEvent()
{
//    while (!m_terminate)
//    {
        while (SDL_PollEvent(m_event) != 0)
        {
            if (m_event->type == SDL_KEYDOWN)
                ProcessKeyEvent();

            if (m_event->type == SDL_MOUSEBUTTONDOWN || 
                m_event->type == SDL_MOUSEBUTTONUP || 
                m_event->type == SDL_MOUSEMOTION || 
                m_event->type == SDL_MOUSEWHEEL)
                ProcessMouseEvent();

            if (m_event->type == SDL_QUIT)
                m_quit = true;
                //pThis->m_terminate = true;
        }

        Draw();
//    }
}

// --------------------------------------------------------------------------------------
// Protected Member Function
// --------------------------------------------------------------------------------------
bool CMainWindow::InitFont()
{
    bool ret = true;

    if (m_Font == NULL)
    {
        if (TTF_Init() == -1)
        {
            printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
            ret = false;
        }
        else
        {
            m_Font = TTF_OpenFont("/usr/share/fonts/truetype/droid/DroidSans.ttf", 16);

            if (m_Font == NULL)
            {
                printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
                ret = false;
            }
        }
    }

    return ret;
}

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

        DrawShiftWindow();

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
            //m_quit = true;
            //m_terminate = true;
            break;
         case SDLK_n:
            //m_nextfile = true;
            m_btn_n = true;
            break;
        case SDLK_s:
            m_btn_s = true;
            //SaveFeatureYUV();
            //m_nextfile = true;
            break;
        default:
        break;
    }
} 

void CMainWindow::ProcessMouseEvent()
{
//    for (int i=0 ; i<SELECTOR_TOTAL ; i++)
//    {
//        if (m_selector[i])
//            m_selector[i]->ProcessMouseEvent(m_event);
//    }
} 

void CMainWindow::ProcessWindowEvent()
{

} 

bool CMainWindow::LoadYUVFolder()
{
    bool ret = true;
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fpath[256];

    if ((dir = opendir(m_yuv_folder.c_str())) == NULL)
        return false;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type & DT_REG)
        {
            snprintf(fpath, sizeof(fpath), "%s/%s", m_yuv_folder.c_str(), entry->d_name);
            m_filelist.push_back(fpath);
        }
    }

    sort(m_filelist.begin(), m_filelist.end());

    // for (std::vector<std::string>::iterator it = m_filelist.begin() ; it != m_filelist.end() ; it++)
    // {
    //     printf("%s\n", it->c_str());
    // }

    closedir(dir);
    dir = NULL;
    //m_nextfile = true;

    CreateOutputFolder();

    return ret;
}

bool CMainWindow::DrawYUVImage()
{
    bool ret = true;
    std::string filename;
    FILE *fd = NULL;
    int y_size, uv_size, yuv_size;
    SDL_Rect rect;

    if (m_nextfile == true)
    {
        m_nextfile = false;

        if (m_filelist.size() != 0 && m_yuv_buf)
        {
            filename = (std::string)m_filelist[0];
            m_filelist.erase(m_filelist.begin());

            if ((fd = fopen(filename.c_str(), "r")) != NULL)
            {
                m_cur_fpath.clear();
                m_cur_fpath = filename;
                //printf("Load %s\n", m_cur_fpath.c_str());

                y_size  = m_width * m_height;
                uv_size = ((m_width >> 1) * (m_height >> 1)) << 1;
                yuv_size = y_size + uv_size;

                memset(m_yuv_buf, 0x80, yuv_size);

                fread(m_yuv_buf, 1, yuv_size, fd);

                rect.x = 0;
                rect.y = 0;
                rect.w = m_width;
                rect.h = m_height;

                SDL_UpdateTexture(m_texture, &rect, m_yuv_buf, m_width);

                fclose(fd);
                fd = NULL;
            }
        }
        else
            m_terminate = true;

        SDL_SetWindowTitle(m_window, filename.c_str());
    }

    return ret;
}

void CMainWindow::DrawSelector()
{
//    for (int i=0 ; i<SELECTOR_TOTAL ; i++)
//    {
//        if (m_selector[i])
//            m_selector[i]->Draw(m_renderer);
//    }
}

void CMainWindow::DrawShiftWindow()
{
    int sw_r, sw_c, sw_w, sw_h;
    int local_r, local_c, local_w, local_h;
    SDL_Rect rect;
    CandidatesIt it;

    for (it = m_vcs.begin(); it != m_vcs.end(); ++it)
    {
        for (int i=FCWS__LOCAL__TYPE__LEFT ; i<FCWS__LOCAL__TYPE__TOTAL ; i++)
        {        
            (*it)->GetLocalInfo((FCWS__Local__Type)i, local_r, local_c, local_w, local_h);

//            printf("%s: %d, %d, %d, %d\n",
//                    search_local_model_pattern[i],
//                    r, c, w, h);

            rect.x = local_c;
            rect.y = local_r;
            rect.w = local_w;
            rect.h = local_h;

			SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderDrawRect(m_renderer, &rect);

            (*it)->GetSWInfo((FCWS__Local__Type)i, sw_r, sw_c, sw_w, sw_h);
            rect.x = local_c + sw_c;
            rect.y = local_r + sw_r;
            rect.w = sw_w;
            rect.h = sw_h;

			SDL_SetRenderDrawColor(m_renderer, 0xFF, 0x00, 0x00, 0xFF);
			SDL_RenderDrawRect(m_renderer, &rect);
        }
    }

}

void CMainWindow::CreateOutputFolder()
{
    char cmd[256] = {'\0'};
    struct stat folder_stat;

    m_output_yuv_folder = m_yuv_folder;
    m_output_yuv_folder.append("/out");

    // printf("m_output_yuv_folder %s\n", m_output_yuv_folder.c_str());

    if (stat(m_output_yuv_folder.c_str(), &folder_stat) == 0)
    {
        snprintf(cmd, sizeof(cmd), "rm -f %s/*", m_output_yuv_folder.c_str());
        system(cmd);
    }
    else
    {
        mkdir(m_output_yuv_folder.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    }

    //sync();
}

void CMainWindow::SaveFeatureYUV()
{
    int need_sync = 0;

    uint32_t x, y, w = 0, h = 0;
    std::string title;
    char outfile[256] = {"\0"}, file[256] = {"\0"};
    char *p1 = NULL, *p2 = NULL;
    FILE *fd = NULL;
    uint8_t *yuv_buf = NULL;

    p1 = strchr((char*)m_cur_fpath.c_str(), '/');
    p2 = strrchr((char*)m_cur_fpath.c_str(), '.');

    if (p1 && p2)
    {
        strncpy(file, p1 + 1, p2 - (p1 + 1));
        
//        for (int i=0 ; i<SELECTOR_TOTAL ; i++)
//        {
//            if (m_selector[i])
//            {
//                m_selector[i]->GetGeometryInfo(&x, &y, &w, &h);
//                m_selector[i]->GetTitle(title);
//
//                if (w && h)
//                {
//                    yuv_buf = (uint8_t*)malloc(sizeof(uint8_t) * ((w * h * 3) >> 1));
//                    snprintf(outfile, sizeof(outfile), "%s/%s_%s_%d_%d.yuv", 
//                        m_output_yuv_folder.c_str(), 
//                        file, 
//                        title.c_str(), 
//                        w, 
//                        h);
//                    
//                    if ((fd = fopen(outfile, "w")) != NULL && yuv_buf && m_yuv_buf)
//                    {
//                        memset(yuv_buf, 0x80, ((w * h * 3) >> 1));
//
//                        for (uint32_t yy=0 ; yy<h ; yy++)
//                            memcpy(yuv_buf + yy * w, m_yuv_buf + (yy + y) * m_width + x, w);
//
//                        printf("outfile:%s\n", outfile);
//                        fwrite(yuv_buf, 1, ((w * h * 3) >> 1), fd);
//     
//                        fclose(fd);
//                        fd = NULL;
//
//                        need_sync++;
//                    }
//                    else
//                        printf("Can not open %s\n", outfile);
//
//                    if (yuv_buf)
//                    {
//                        free(yuv_buf);
//                        yuv_buf = NULL;
//                    }
//                }
//            }
//        }     

//        if (need_sync)
//            sync();             
    }


}
