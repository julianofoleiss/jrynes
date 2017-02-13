#include "NES.h"
#include "romLoader.h"
#include "PPU.h"
#include "6502.h"
#include <SDL/SDL_keyboard.h>
#include "SDL/Video.h"
#include "SDL/Timer.h"
#include "JRYNES.h"
#include "Input.h"
#include "KeyboardJoypad.h"
#include "SDL/Keyboard.h"
#include "6502_Debug.h"
#include "Movie.h"
#include "Statistics.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <SDL/SDL_timer.h>
#include "mappers/mapper.h"

extern void sigInt(int Code);
extern EmulationSettings Settings;

extern MovieMacro *Movie;

extern __Context CPU;

uint8_t* ScaleLine(uint8_t* VideoBuffer, uint8_t* ScanlineBuffer, uint32_t x_scale, uint32_t y_scale);

Mapper *CurrentMapper;

void NESrun(char* Filename){

    RomImage* ROM;
    uint32_t CurrentScanline;
    uint32_t DiffCycles;
    char* WindowCaption;
    uint8_t draw;
    uint8_t curScanline[NES_BACKBUF_WIDTH];
    uint8_t* VideoBuffer, *OffscreenBuffer;
    uint64_t FrameCounter;
    uint32_t StartTime, EndTime, DiffTime;
    ButtonConfig Pad1BC, Pad2BC;
    
    if(Settings.ArgumentFlags & AF_DONT_DRAW){
        OffscreenBuffer = malloc(NES_BACKBUF_WIDTH * NES_SCREEN_HEIGHT * Settings.xScaling * Settings.yScaling);
    }
    else{
        OffscreenBuffer = NULL;
        WindowCaption = malloc(255);
        WindowCaption[0] = 0;

        strcat(WindowCaption, "jrynes - ");
        strcat(WindowCaption, Filename);        
        
        InitializeVideo();
        VIDSetCaption(WindowCaption);        
    }
    
    //STATCycles *CycleInfo;
    
    //time_t LastTime, ThisTime, FirstTime;
    
    int32_t (*RunCycles)(int32_t) ;
    
    memset(curScanline, 0, NES_BACKBUF_WIDTH);
    
    FrameCounter = 0;
    
    ROM = LoadROMImage(Filename);
    
    DiffCycles = 0;
    
    if(!ROM){
        printf("\nERROR: Could not load ROM from file %s\n", Filename);
        exit(1);
    }
        
    PrintROMImageInformation(ROM);
    
    Pad1BC.AButton = SDLK_s;
    Pad1BC.BButton = SDLK_a;
    Pad1BC.DownButton = SDLK_DOWN;
    Pad1BC.UpButton = SDLK_UP;
    Pad1BC.LeftButton = SDLK_LEFT;
    Pad1BC.RightButton = SDLK_RIGHT;
    Pad1BC.SELECTButton = SDLK_n;
    Pad1BC.STARTButton = SDLK_m;
    
    Pad2BC.AButton = SDLK_u;
    Pad2BC.BButton = SDLK_i;
    Pad2BC.DownButton = SDLK_o;
    Pad2BC.UpButton = SDLK_p;
    Pad2BC.LeftButton = SDLK_h;
    Pad2BC.RightButton = SDLK_j;
    Pad2BC.SELECTButton = SDLK_k;
    Pad2BC.STARTButton = SDLK_l;    
    
    Mapper *M;
    
    M = MAP_GetMapper(ROM);
    
    CurrentMapper = M;
    
    if(!M){
        fprintf(stderr, "ERROR: Mapper %d not supported.\n", GetROMMapper(ROM));
        exit(1);
    }
    
    initNESMemory(ROM, M);
    initPPUMemory(ROM);
    
    init6502();
    initPPU();
    initInput();
    
    if(Settings.ArgumentFlags & AF_RECORDING){
        Movie = MOVInit();
        MOVAddData(Movie, 0, 0, 0);
    }
    
    if(Settings.ArgumentFlags & AF_PLAYING){
        Movie = MOVLoadFile(Settings.PlayMovieFile);
        if(!Movie){
            fprintf(stderr, "\n\nERROR: Movie file \"%s\" not found!\n\n", Settings.PlayMovieFile);
            exit(1);
        }
    }

    RunCycles = INT_Main;
    
    Pad1Init(getKeybJoypadState, &Pad1BC);
    Pad2Init(getKeybJoypadState, &Pad2BC);
    
    #ifdef DEBUG
        fprintf(stderr, "Initializing emulation...\n");
        initDebug();
    #endif    

    #ifdef DEBUGCONSOLE
        if(Settings.ArgumentFlags & AF_DEBUG_CONSOLE){
            debugPrompt(NULL);
        }
    #endif
    
    //SDL_EnableKeyRepeat(100, 30);
    
    //LastTime = FirstTime = time(NULL);
    //ThisTime = 0;
    
    if(Settings.ArgumentFlags & AF_GENERATE_ELAPSED_CYCLES_GRAPH)
        InitCycleTimer();
    
    while(1){
        
        StartTime = TIMGetTicks();
        
        KEYPoll();
        
        PPUStartFrame();
        
        draw = 0;
        
        if(Settings.ArgumentFlags & AF_DONT_DRAW){
            VideoBuffer = OffscreenBuffer;
            draw = 1;
        }
        else{
            if(VIDLockScreen()){
                VideoBuffer = VIDGetPixels();
                draw = 1;
            }
        }
        
        for(CurrentScanline = 0; CurrentScanline < NES_NUM_SCANLINES; CurrentScanline++){
        
            DiffCycles = RunCycles(NES_CYCLES_PER_SCANLINE);
            
            //Fetch pointer to beginning of next line in the frame buffer
            if(draw){
                PPUDoScanlineAndDraw(curScanline);
                if(!(Settings.ArgumentFlags & AF_DONT_DRAW))
                    VideoBuffer = ScaleLine(VideoBuffer, curScanline, Settings.xScaling, Settings.yScaling);
            }
            else
                PPUDoScanlineAndDontDraw();
            
        }
        
        if(draw && !(Settings.ArgumentFlags & AF_DONT_DRAW)){
            //draw_patterns(((uint8_t*)VIDGetPixels()));
            VIDUnlockScreen();
            VIDUpdateScreen();
        }
        
        PPUEndFrame();
        
        for(CurrentScanline = 240; CurrentScanline <= 261; CurrentScanline++){
            if(CurrentScanline == 240){
                PPUStartVBlank();
            }
            else if(CurrentScanline == 261){
                PPUEndVBlank();
            }
            
            if(CurrentScanline == 241){
                DiffCycles = RunCycles(1);
                if(PPUGetNMIEnabled()){
                    //execute CPU NMI
                    CPUsetNMI();
                    DiffCycles = RunCycles(NES_CYCLES_PER_SCANLINE - 1);
                    continue;
                }
            }
            DiffCycles = RunCycles(NES_CYCLES_PER_SCANLINE);
        }
        
        FrameCounter++;        
        
        //if(!(FrameCounter % 60)){
        /*ThisTime = time(NULL);
        if(ThisTime > LastTime){
            LastTime = ThisTime;
            CycleInfo = STATCyclesNew(LastTime - FirstTime, CPU.ClockTicks);
            STATCyclesAdd(CycleInfo);
        }*/
        
        if(!(Settings.ArgumentFlags & AF_NO_DELAY)){
            EndTime = TIMGetTicks();
            DiffTime = EndTime - StartTime;
            if(DiffTime < 16)
                TIMDelay(16-DiffTime);
        }
        
    }
}

void TerminateEmulation(){
    TerminateCPU();
    
    if(Settings.ArgumentFlags & AF_RECORDING){
        MOVAddData(Movie, CPU.ClockTicks, 0, 0);
        MOVSaveFile(Movie, Settings.RecordMovieFile);
    }
    
    PrintStatistics();
    
    exit(0);
}

uint8_t* ScaleLine(uint8_t* VideoBuffer, uint8_t* ScanlineBuffer, uint32_t x_scale, uint32_t y_scale){
    
    int i, x, y;
    
    for(y = 0; y < y_scale; y++){
        for(i = 0; i < NES_BACKBUF_WIDTH; i++){
            for(x = 0; x < x_scale; x++){
                *VideoBuffer++ = ScanlineBuffer[i];
            }
        }
    }
    
    return VideoBuffer;
}
