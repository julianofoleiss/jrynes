#ifndef INPUT_H
#define	INPUT_H

#include <stdint.h>

typedef struct{
    uint32_t AButton;
    uint32_t BButton;
    uint32_t SELECTButton;
    uint32_t STARTButton;
    uint32_t UpButton;
    uint32_t DownButton;
    uint32_t LeftButton;
    uint32_t RightButton;
} ButtonConfig;

#define BTN_A           0x01
#define BTN_B           0x02
#define BTN_SELECT      0x04
#define BTN_START       0x08
#define BTN_UP          0x10
#define BTN_DOWN        0x20
#define BTN_LEFT        0x40
#define BTN_RIGHT       0x80

void initInput();

void mapInputRegisters();

void Pad1Init(uint8_t(*Functor)(), ButtonConfig* Config);

void Pad2Init(uint8_t(*Functor)(), ButtonConfig* Config);

#endif	/* INPUT_H */

