#ifndef __DLIB_CWRAPPER_H__
#define __DLIB_CWRAPPER_H__

#include <stdint.h>
#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

int Dlib_Init(char* model);

int Dlib_HogDetectorInit(char* model);

void Dlib_DeInit(void);

int Dlib_DoDetection(uint8_t* src,
                     int w,
                     int h,
                     int linesize);

int Dlib_DoHogDetection(uint8_t* src,
                     int w,
                     int h,
                     int linesize);

void Dlib_DrawResult(SDL_Renderer* const render);

#ifdef __cplusplus
}
#endif
#endif

