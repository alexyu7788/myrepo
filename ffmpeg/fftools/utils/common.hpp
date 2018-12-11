#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <SDL.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_linalg.h>

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"


#define DEBUG
//#define DEBUG_LDWS
#define DEBUG_FCWS

#ifdef DEBUG
//#define dbg(format, args...) printf("[%s][%s][%d] " format "\n", __FILE__, __func__, __LINE__, ##args)
#define dbg(format, args...) printf("[%d] " format "\n", __LINE__, ##args)
#define dbg(format, args...) printf("[%d] " format "\n", __LINE__, ##args)
#else
#define dbg(format, args...)
#endif

#ifdef DEBUG_LDWS
#define ldwsdbg(format, args...) printf("[LDWS][%d] " format "\n", __LINE__, ##args)
#else
#define ldwsdbg(format, args...)
#endif

#ifdef DEBUG_FCWS
#define fcwsdbg(format, args...) printf("[FCWS][%d] " format "\n", __LINE__, ##args)
#else
#define fcwsdbg(format, args...)
#endif

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef TRUE 
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif


enum adas_color{
    COLOR_RED = 0,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_GOLD,
    COLOR_MAGENTA,
    COLOR_BROWN,
    COLOR_DARKGREEN,
    COLOR_TOTAL
};

static SDL_Color COLOR[COLOR_TOTAL] = {
   /*[COLOR_RED]          =*/ {0xff,    0,    0, 0x50},
   /*[COLOR_GREEN]        =*/ {   0, 0xff,    0, 0x50},
   /*[COLOR_BLUE]         =*/ {   0,    0, 0xff, 0x50},
   /*[COLOR_YELLOW]       =*/ {0xff, 0xff,    0, 0x50},
   /*[COLOR_CYAN]         =*/ {   0, 0xff, 0xff, 0x50},
   /*[COLOR_GOLD]         =*/ {0xff, 0xd7,    0, 0x50},
   /*[COLOR_MAGENTA]      =*/ {0xff,    0, 0xff, 0x50},
   /*[COLOR_DARKGREEN]    =*/ {   0, 0x64,    0, 0x50},
   /*[COLOR_BROWN]        =*/ {0x8b, 0x45, 0x13, 0x50},
};

typedef struct rect_s {
    int r;
    int c;
    int w;
    int h;
}rect;

typedef struct point_s {
    int r;
    int c;
}point_t;

#endif
