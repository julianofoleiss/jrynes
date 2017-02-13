#ifndef VIDEO_H
#define	VIDEO_H

#include <stdint.h>

void InitializeVideo();
void VIDSetCaption(char* Caption);
uint8_t VIDLockScreen();
void VIDUnlockScreen();
void VIDUpdateScreen();
void* VIDGetPixels();
uint32_t VIDGetPitch();

void PDGInitialize();

#endif	/* VIDEO_H */

