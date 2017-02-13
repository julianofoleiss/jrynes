#include <SDL/SDL.h>
#include <stdint.h>

#ifndef TIMER_H
#define	TIMER_H

#define TIMGetTicks()   SDL_GetTicks()
#define TIMDelay(ms)    SDL_Delay(ms)

#endif	/* TIMER_H */

