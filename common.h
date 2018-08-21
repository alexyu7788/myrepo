#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <pthread.h>
#include <SDL.h>

#define DEBUG

#ifdef DEBUG
#define dbg(format, args...) printf("[%s][%s][%d] " format "\n", __FILE__, __func__, __LINE__, ##args)
#else
#define dbg(format, args...)
#endif

#endif
