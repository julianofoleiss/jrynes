#include <stdint.h>
#include <SDL/SDL.h>

uint32_t initSDL(){
    return SDL_Init(SDL_INIT_VIDEO);
}
