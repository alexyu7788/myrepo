/*
 * =====================================================================================
 *
 *       Filename:  hellow.sdl.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/21/2018 04:54:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <pthread.h>

#define SCREEN_W    640
#define SCREEN_H    480

int main(int argc, char * argv[])
{
    SDL_Window *window = NULL;
    SDL_Surface *surface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL initialization fails.(%s)\n", SDL_GetError());
    }
    else
    {
        window = SDL_CreateWindow("SDL tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

        if (!window)
        {
            printf("Can not create window.(%s)\n", SDL_GetError());
        }
        else
        {
            surface = SDL_GetWindowSurface(window);

            SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0xFF, 0xFF, 0xFF ) );

            SDL_UpdateWindowSurface(window);

            SDL_Delay(2000);
        }
    }
    
    if (window)
        SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
