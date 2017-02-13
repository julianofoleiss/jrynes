#include "KeyboardJoypad.h"
#include "SDL/Keyboard.h"
#include "Input.h"

uint8_t getKeybJoypadState(ButtonConfig* BC){
    uint8_t PadState = 0;
    
    if(KEYGetKeyState(BC->AButton))
        PadState |= BTN_A;
    
    if(KEYGetKeyState(BC->BButton))
        PadState |= BTN_B;
    
    if(KEYGetKeyState(BC->SELECTButton))
        PadState |= BTN_SELECT;
    
    if(KEYGetKeyState(BC->STARTButton))
        PadState |= BTN_START;
    
    if(KEYGetKeyState(BC->UpButton))
        PadState |= BTN_UP;
    
    if(KEYGetKeyState(BC->DownButton))
        PadState |= BTN_DOWN;
    
    if(KEYGetKeyState(BC->LeftButton))
        PadState |= BTN_LEFT;
    
    if(KEYGetKeyState(BC->RightButton))
        PadState |= BTN_RIGHT;
    
    return PadState;
}
