#include "Input.h"
#include "NESMemory.h"
#include "SDL/Keyboard.h"
#include "JRYNES.h"
#include "Movie.h"
#include "6502.h"
#include "NES.h"
#include <stdio.h>
#include <stdlib.h>

extern EmulationSettings Settings;
extern MovieMacro* Movie;
extern __Context CPU;

//This is needed to setup Input registers in Main memory
extern uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
extern void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

static uint8_t Pad1State;
static uint8_t Pad2State;
static uint8_t PadStrobe;

static uint8_t (*getPadInput1)(ButtonConfig*);
static uint8_t (*getPadInput2)(ButtonConfig*);
static ButtonConfig* Pad1BC;
static ButtonConfig* Pad2BC;


// JOY1 I/O (0x4016) - Read
DEFREADMEM_H(JOY1){
    uint8_t temp;
    temp = Pad1State & 0x01;
    Pad1State >>=1;
    return temp;
}

// JOY1 I/O (0x4016) - Write
DEFWRITEMEM_H(JOY1){
    if(Val & 0x01){
        PadStrobe = 1;
    }
    else{
        if(PadStrobe){
            //Get the actual input.
            PadStrobe = 0;
            
            if(Settings.ArgumentFlags & AF_PLAYING){
                if(MOVPeekNextStepCycles() > CPU.ClockTicks){
                    Pad1State = MOVGetPad1State();
                    Pad2State = MOVGetPad2State();
                }
                else{
                    MOVNextData();
                    Pad1State = MOVGetPad1State();
                    Pad2State = MOVGetPad2State();
                    if(MOVMovieEnded()){
                        Settings.ArgumentFlags &= ~((uint64_t)AF_PLAYING);
                        printf("Movie playback ended.\n");
                        
                        if(Settings.ArgumentFlags & AF_QUIT_MOVIE_END){
                            TerminateEmulation();
                            exit(1);
                        }
                    }
                }
            }
            else{
                Pad1State = getPadInput1(Pad1BC);
                Pad2State = getPadInput2(Pad2BC);
            }
            
            if(Settings.ArgumentFlags & AF_RECORDING){
                if((Pad1State != MOVGetPreviousPad1State()) || 
                        (Pad2State != MOVGetPreviousPad2State())){
                    MOVAddData(Movie, CPU.ClockTicks, Pad1State, Pad2State);
                }
            }
            
            //Pad2State = getPadInput2(Pad2BC);
        }
    }
        
}

// JOY2 I/O (0x4017) - Read
DEFREADMEM_H(JOY2){
    uint8_t temp;
    temp = Pad2State & 0x01;
    Pad2State >>=1;
    return temp;    
}

void mapInputRegisters(){
    NESMemoryReadHandlers[0x4016] = JOY1Read;
    NESMemoryWriteHandlers[0x4016] = JOY1Write;
    
    NESMemoryReadHandlers[0x4017] = JOY2Read;
    NESMemoryWriteHandlers[0x4017] = JOY1Write;    
}

void initInput(){
    Pad1State = 0;
    Pad2State = 0;
    PadStrobe = 0;
}

void Pad1Init(uint8_t(*Functor)(), ButtonConfig* Config){
    getPadInput1 = Functor;
    Pad1BC = Config;
}

void Pad2Init(uint8_t(*Functor)(), ButtonConfig* Config){
    getPadInput2 = Functor;
    Pad2BC = Config;
}