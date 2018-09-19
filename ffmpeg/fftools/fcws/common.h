#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
//#include <string>
//#include <vector>
//#include <list>
//#include <algorithm>
#include <pthread.h>
#include <SDL.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>

#define DEBUG

#ifdef DEBUG
//#define dbg(format, args...) printf("[%s][%s][%d] " format "\n", __FILE__, __func__, __LINE__, ##args)
#define dbg(format, args...) printf("[%d] " format "\n", __LINE__, ##args)
#else
#define dbg(format, args...)
#endif

#ifndef bool
#define bool unsigned char
#endif

#ifndef true
#define true (1)
#endif

#ifndef false
#define false (0)
#endif

enum {
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_DARKGREEN,
    COLOR_BLUE,
    COLOR_GOLD,
    COLOR_BROWN,
    COLOR_MAGENTA,
    COLOR_GREEN,
    COLOR_TOTAL
};

static SDL_Color COLOR[COLOR_TOTAL] = {
   [COLOR_RED]          = {0xff,    0,    0, 0x50},
   [COLOR_GREEN]        = {   0, 0xff,    0, 0x50},
   [COLOR_BLUE]         = {   0,    0, 0xff, 0x50},
   [COLOR_YELLOW]       = {0xff, 0xff,    0, 0x50},
   [COLOR_CYAN]         = {   0, 0xff, 0xff, 0x50},
   [COLOR_GOLD]         = {0xff, 0xd7,    0, 0x50},
   [COLOR_MAGENTA]      = {0xff,    0, 0xff, 0x50},
   [COLOR_DARKGREEN]    = {   0, 0x64,    0, 0x50},
   [COLOR_BROWN]        = {0x8b, 0x45, 0x13, 0x50},
};

void FreeVector(gsl_vector** v);

void FreeMatrix(gsl_matrix** m);

void FreeMatrixUshort(gsl_matrix_ushort** m);

void FreeMatrixChar(gsl_matrix_char** m);

bool CheckOrReallocVector(gsl_vector** v, int size, bool reset);

bool CheckOrReallocMatrix(gsl_matrix** m, int h, int w, bool reset);

bool CheckOrReallocMatrixUshort(gsl_matrix_ushort** m, int h, int w, bool reset);

bool CheckOrReallocMatrixChar(gsl_matrix_char** m, int h, int w, bool reset);

#endif
