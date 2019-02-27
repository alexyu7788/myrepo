#ifndef __DLIB_CWRAPPER_H__
#define __DLIB_CWRAPPER_H__

<<<<<<< HEAD
#include "dlib.h"

=======
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int Dlib_Init(char* model);

void Dlib_DeInit(void);

int Dlib_DoDetection(uint8_t* src,
                     int w,
                     int h,
                     int linesize);
#ifdef __cplusplus
}
#endif
>>>>>>> 1. USe dlib instead of gsl.(not ready)
#endif

