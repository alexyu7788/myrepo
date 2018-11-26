#include "ldws_cwrapper.h"
#include "ldws.hpp"

CLDWS* ldws_obj = NULL;

void LDW_Init()
{
    if (ldws_obj == NULL)
        ldws_obj = new CLDWS();
}

void LDW_DoDetection(uint8_t* src, int linesize, int w, int h)
{
    if (!ldws_obj) {
        dbg();
        return;
    }

    ldws_obj->DoDetection();
}

void LDW_DeInit(void)
{
    if (!ldws_obj) {
        dbg();
        return;
    }

    delete ldws_obj;
    ldws_obj = NULL;
}
