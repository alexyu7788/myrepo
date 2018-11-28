#include "ldws_cwrapper.h"
#include "ldws.hpp"

CLDWS* ldws_obj = NULL;

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