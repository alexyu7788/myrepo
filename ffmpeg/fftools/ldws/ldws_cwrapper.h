#ifndef __LDWS_CWRAPPER_H__
#define __LDWS_CWRAPPER_H__

#include "../utils/common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void LDW_Init(void);

void LDW_DoDetection(uint8_t* src, int w, int h, int linesize, int rowoffset, bool crop);

void LDW_DeInit(void);

bool LDW_GetEdgeImg(uint8_t* dst, int w, int h, int linesize);

bool LDW_ApplyDynamicROI(gsl_matrix* src);

bool LDW_DrawSplines(SDL_Renderer* const render, SDL_Rect* const rect, uint32_t width, enum adas_color color);

bool LDW_DrawLanes(SDL_Renderer* const render, SDL_Rect* const rect, enum adas_color color);

#ifdef __cplusplus
}
#endif

#endif
