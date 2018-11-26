#ifndef __LDWS_CWRAPPER_H__
#define __LDWS_CWRAPPER_H__

#include "../utils/common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void LDW_Init();

void LDW_DoDetection(uint8_t* src, int linesize, int w, int h);

void LDW_DeInit(void);
#ifdef __cplusplus
}
#endif

#endif
