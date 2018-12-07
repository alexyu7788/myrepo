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
 
void LDW_DoDetection(uint8_t* src, int w, int h, int linesize, int rowoffset, bool crop)
{
    if (!ldws_obj) {
        dbg();
        return;
    }

    ldws_obj->DoDetection(src,
                         w, 
                         h,
                         linesize,
                         rowoffset,
                         crop);
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

bool LDW_ApplyDynamicROI(gsl_matrix* src)
{
    if (!ldws_obj || !src) {
        ldwsdbg();
        return false;
    }

    return ldws_obj->ApplyDynamicROI(src);
}

bool LDW_DrawSplines(SDL_Renderer* const render, SDL_Rect* const rect, uint32_t width, enum adas_color color)
{
    uint32_t idx;
    SDL_Color *Color;

    if (!ldws_obj || !render || !rect) {
        ldwsdbg();
        return false;
    }

    Color = &COLOR[color];
    SDL_SetRenderDrawColor(render, Color->r, Color->g, Color->b, Color->a);

    ldws_obj->GetLane(&left, &right, &center);

    if (left && left->pix && left->pix_count) {
//        dbg("left %p: %d", left->pix, left->pix_count);
        for (idx = 0 ; idx < left->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                rect->x + left->pix[idx].c,
                                rect->y + left->pix[idx].r,
                                rect->x + left->pix[idx].c + width,
                                rect->y + left->pix[idx].r);
    }

    if (right && right->pix && right->pix_count) {
//        dbg("right %p: %d", right->pix, right->pix_count);
        for (idx = 0 ; idx < right->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                rect->x + right->pix[idx].c,
                                rect->y + right->pix[idx].r,
                                rect->x + right->pix[idx].c + width,
                                rect->y + right->pix[idx].r);
    }

    if (center && center->pix && center->pix_count) {
//        dbg("center %p: %d", center->pix, center->pix_count);
        for (idx = 0 ; idx < center->pix_count ; ++idx)
            SDL_RenderDrawLine(render,
                                rect->x + center->pix[idx].c,
                                rect->y + center->pix[idx].r,
                                rect->x + center->pix[idx].c + width,
                                rect->y + center->pix[idx].r);
    }

    ldws_obj->DestroyLane(&left, &right, &center);

    return true;
}

bool LDW_DrawLanes(SDL_Renderer* const render, SDL_Rect* const rect, enum adas_color color)
{
    point lanepoint[6];
    SDL_Color *Color;

    if (!ldws_obj || !render || !rect) {
        ldwsdbg();
        return false;
    }

    memset(lanepoint, 0x0, sizeof(point) * 6);
    ldws_obj->GetLanePoints(&lanepoint[0], 
                            &lanepoint[1], 
                            &lanepoint[2], 
                            &lanepoint[3],
                            &lanepoint[4], 
                            &lanepoint[5]);

    Color = &COLOR[color];
    SDL_SetRenderDrawColor(render, Color->r, Color->g, Color->b, Color->a);

    // Left lane
    SDL_RenderDrawLine(render, 
                       rect->x + lanepoint[0].c, 
                       rect->y + lanepoint[0].r, 
                       rect->x + lanepoint[1].c, 
                       rect->y + lanepoint[1].r);
    // Right lane
    SDL_RenderDrawLine(render, 
                       rect->x + lanepoint[2].c, 
                       rect->y + lanepoint[2].r, 
                       rect->x + lanepoint[3].c, 
                       rect->y + lanepoint[3].r);
    // Center lane
    SDL_RenderDrawLine(render, 
                       rect->x + lanepoint[4].c, 
                       rect->y + lanepoint[4].r, 
                       rect->x + lanepoint[5].c, 
                       rect->y + lanepoint[5].r);

    return true;
}
