#include <stdio.h>
#include <stdlib.h>
#include "dlib_cwrapper.h"
#include "dlib.h"

static CDlib* dlib_obj = NULL;
  
std::vector<dlib::rectangle> rect, hog_result;

int Dlib_Init(char* model)
{
    if (!dlib_obj) 
        dlib_obj = new CDlib();

    if (!dlib_obj)
        return -1;

    if (false == dlib_obj->Init(model)) {
        delete dlib_obj;
        dlib_obj = NULL;
        return -2;
    }

    return 0;
}

int Dlib_HogDetectorInit(char* model)
{
    if (!dlib_obj) 
        dlib_obj = new CDlib();

    if (!dlib_obj)
        return -1;

    if (false == dlib_obj->HogDectectorInit(model)) {
        delete dlib_obj;
        dlib_obj = NULL;
        return -2;
    }

    return 0;
}

void Dlib_DeInit()
{
    if (dlib_obj) {
        delete dlib_obj;
        dlib_obj = NULL;
    }
}
 
int Dlib_DoDetection(uint8_t* src,
                     int w,
                     int h,
                     int linesize)
{
    if (dlib_obj) {
        dlib_obj->DoDetection(src,
                              w,
                              h,
                              linesize);

        return 0;
    }

    return -1;
}

int Dlib_DoHogDetection(uint8_t* src,
                     int w,
                     int h,
                     int linesize)
{
    if (dlib_obj) {
        dlib_obj->DoHogDetection(src,
                              w,
                              h,
                              linesize);

        return 0;
    }

    return -1;
}

void Dlib_DrawResult(SDL_Renderer* const render)
{
    SDL_Rect rect;

    std::vector<dlib::rectangle>::iterator it;

    if (dlib_obj) {
        dlib_obj->GetHogDetectionResult(hog_result);

        if (hog_result.size())
        {
            SDL_SetRenderDrawColor(render, 0xff, 0x00, 0x00, 0x00);

            for (it = hog_result.begin() ; it != hog_result.end() ; ++it) {

                rect.x = it->left();
                rect.y = it->top();
                rect.w = it->right() - it->left() + 1;
                rect.h = it->bottom() - it->top() + 1;
        
                SDL_RenderDrawRect(render, &rect);
            }
        }
    }
}

