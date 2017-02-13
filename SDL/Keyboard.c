#include <stdint.h>
#include <SDL/SDL.h>
#include "Keyboard.h"

#include <stdio.h>

#define KEYBOARD_MAX_KEYS 322

static uint8_t Keys[KEYBOARD_MAX_KEYS];

uint8_t KEYGetKeyState(uint32_t Keycode){
    uint8_t temp;
    temp = Keys[Keycode] ? 1 : 0;
    //Keys[Keycode] = 0;
    return temp;
}

void KEYPoll(){
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                Keys[event.key.keysym.sym] = 1;
            break;
            case SDL_KEYUP:
                Keys[event.key.keysym.sym] = 0;
            break;
        }
    }
}

void KEYClear(){
    memset(Keys, 0 ,KEYBOARD_MAX_KEYS);
}