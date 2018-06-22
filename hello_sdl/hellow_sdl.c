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
    int flag = 0;

    SDL_Window *window = NULL;
    SDL_Surface *surface = NULL;

    flag = (SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    
    if (SDL_Init(flag) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
     else {
        window =  SDL_CreateWindow("SDL: Hellow", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

        if (!window) {
            printf("Can not create window. SDL_Error: %s\n", SDL_GetError());
        }
        else {
            surface = SDL_GetWindowSurface(window);
            SDL_FillRect( surface, NULL, SDL_MapRGB( surface->format, 0x00, 0xFF, 0xFF ) );
            SDL_UpdateWindowSurface( window );
            SDL_Delay(2000);
        }
     }

    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return 0;
}
