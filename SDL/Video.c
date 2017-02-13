#include "../PPU.h"
#include "../JRYNES.h"
#include <SDL/SDL.h>
#include <stdio.h>

extern EmulationSettings Settings;

SDL_Surface *Screen;
SDL_Surface *PaletteDebug;

SDL_Color Palette[0x100] = {
    #include "NES_pal.h"
};

void InitializeVideo(){
    Screen = SDL_SetVideoMode(NES_BACKBUF_WIDTH * Settings.xScaling, NES_SCREEN_HEIGHT * Settings.yScaling, 8, SDL_HWPALETTE);
    
    //Setup video mode
    if(!Screen){
        fprintf(stderr, "Error setting video mode: %s\n", SDL_GetError());
        exit(2);
    }
    
    //Setup palette
    SDL_SetPalette(Screen, SDL_LOGPAL|SDL_PHYSPAL,Palette, 0, 0x40);
}

void VIDSetCaption(char* Caption){
    SDL_WM_SetCaption(Caption, NULL);
}

uint8_t VIDLockScreen(){
    if (SDL_MUSTLOCK(Screen)) {
        if (SDL_LockSurface(Screen) < 0 )
            return 0;
    }    
    return 1;
}

void VIDUnlockScreen(){
    if (SDL_MUSTLOCK(Screen))
        SDL_UnlockSurface(Screen);

}

void VIDUpdateScreen(){
    SDL_UpdateRect(Screen,0,0,0,0);
}

void* VIDGetPixels(){
    return Screen->pixels;
}

uint32_t VIDGetPitch(){
    return Screen->pitch;
}

