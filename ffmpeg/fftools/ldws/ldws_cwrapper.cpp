#include "ldws_cwrapper.h"
#include "ldws.hpp"

static CLDWS* ldws_obj = NULL;
static lane *left = NULL, *right = NULL, *center = NULL;

void LDW_Init()
{
    if (ldws_obj == NULL) {
        ldws_obj = new CLDWS();

        if (ldws_obj)
            ldws_obj->Init();
    }
}
 
void LDW_DoDetection(uint8_t* src, int linesize, int w, int h)
{
    if (!ldws_obj) {
        dbg();
        return;
    }

    ldws_obj->DoDetection(src,
                         linesize,
                         w, 
                         h);
}

void LDW_DeInit(void)
{
    if (ldws_obj) {
        delete ldws_obj;
        ldws_obj = NULL;
    }
}

bool LDW_GetEdgeImg(uint8_t* dst, int w, int h, int linesize)
{
    if (ldws_obj) 
        return ldws_obj->GetEdgedImg(dst, w, h, linesize);

    return false;
}

bool LDW_DrawLanes(SDL_Renderer* render, uint32_t width)
{
    uint32_t idx;

    if (!ldws_obj || !render) {
        ldwsdbg();
        return false;
    }

    ldws_obj->GetLane(&left, &right, &center);

    if (left && left->pix && left->pix_count) {
        dbg("left %p: %d", left->pix, left->pix_count);
        for (idx = 0 ; idx < left->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                left->pix[idx].c,
                                left->pix[idx].r,
                                left->pix[idx].c + width,
                                left->pix[idx].r);
    }

    if (right && right->pix && right->pix_count) {
        dbg("right %p: %d", right->pix, right->pix_count);
        for (idx = 0 ; idx < right->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                right->pix[idx].c,
                                right->pix[idx].r,
                                right->pix[idx].c + width,
                                right->pix[idx].r);
    }

    if (center && center->pix && center->pix_count) {
        dbg("center %p: %d", center->pix, center->pix_count);
        for (idx = 0 ; idx < center->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                center->pix[idx].c,
                                center->pix[idx].r,
                                center->pix[idx].c + width,
                                center->pix[idx].r);
    }

    ldws_obj->DestroyLane(&left, &right, &center);

    return true;
}
