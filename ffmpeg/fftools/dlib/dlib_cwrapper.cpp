#include <stdio.h>
#include <stdlib.h>
#include "dlib_cwrapper.h"
#include "dlib.h"

static CDlib* dlib_obj = NULL;
  
std::vector<dlib::rectangle> rect;

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
