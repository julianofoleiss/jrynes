/* 
 * File:   Keyboard.h
 * Author: juliano
 *
 * Created on April 12, 2012, 10:18 PM
 */

#ifndef KEYBOARD_H
#define	KEYBOARD_H

#include <stdint.h>

uint8_t KEYGetKeyState(uint32_t Keycode);
void KEYPoll();
void KEYClear();

#endif	/* KEYBOARD_H */

