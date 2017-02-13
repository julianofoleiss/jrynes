#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "PPUMemory.h"
#include "ASMUtilities.h"

//This is the global PPU memory
__PPUMemory PPUMemory;

/* Array of memory Handlers. One for each byte of the PPU address space 
 These handlers are used to treat PPU memory mirroring (which is a PITA)
 PPU RAM address space is 64KB, but only 16KB is present. */
uint8_t (*PPUMemoryReadHandlers[0x10000])(uint16_t Addr);
void    (*PPUMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

inline uint8_t ppuMemoryRead(uint16_t Addr){
    return PPUMemoryReadHandlers[Addr](Addr);
}

inline void ppuMemoryWrite(uint16_t Addr, uint8_t Val){
    PPUMemoryWriteHandlers[Addr](Addr, Val);
}

DEFREADPPUMEM_H(PatternTable0){
    return PPUMemory.PT[0][Addr & 0x0FFF];
}

DEFWRITEPPUMEM_H(PatternTable0){
    PPUMemory.PT[0][Addr & 0x0FFF] = Val;
}

DEFREADPPUMEM_H(PatternTable1){
    return PPUMemory.PT[1][Addr & 0x0FFF];
}

DEFWRITEPPUMEM_H(PatternTable1){
    PPUMemory.PT[1][Addr & 0x0FFF] = Val;
}

//return PPUMemory.NT[N][Addr & 0x03C0];
#define DEFNAMETABLE_READ(N) \
DEFREADPPUMEM_H(NameTable##N){\
    return PPUMemory.NT[N][Addr & 0x03FF];\
}

DEFNAMETABLE_READ(0)
DEFNAMETABLE_READ(1)
DEFNAMETABLE_READ(2)
DEFNAMETABLE_READ(3)

//PPUMemory.NT[N][Addr & 0x03C0] = Val;
#define DEFNAMETABLE_WRITE(N) \
DEFWRITEPPUMEM_H(NameTable##N){\
    PPUMemory.NT[N][Addr & 0x03FF] = Val;\
}

DEFNAMETABLE_WRITE(0)
DEFNAMETABLE_WRITE(1)
DEFNAMETABLE_WRITE(2)
DEFNAMETABLE_WRITE(3)

//return PPUMemory.AT[N][Addr & 0x40];
#define DEFATTRTABLE_READ(N) \
DEFREADPPUMEM_H(AttrTable##N){\
    return PPUMemory.AT[N][Addr & 0x3F];\
}

DEFATTRTABLE_READ(0)
DEFATTRTABLE_READ(1)
DEFATTRTABLE_READ(2)
DEFATTRTABLE_READ(3)

//PPUMemory.AT[N][Addr & 0x40] = Val;
#define DEFATTRTABLE_WRITE(N) \
DEFWRITEPPUMEM_H(AttrTable##N){\
    PPUMemory.AT[N][Addr & 0x3F] = Val;\
}

DEFATTRTABLE_WRITE(0)
DEFATTRTABLE_WRITE(1)
DEFATTRTABLE_WRITE(2)
DEFATTRTABLE_WRITE(3)

#ifndef ASMUTILITIES
#define PPUHANDLER_SETUP(INI, END, READFunc, WRITEFunc)\
for(i = (INI); i < (END); i++){\
    PPUMemoryReadHandlers[i] = READFunc;\
    PPUMemoryWriteHandlers[i] = WRITEFunc;\
}
#else
#define PPUHANDLER_SETUP(INI, END, READFunc, WRITEFunc)\
    memset64( (void*) (((uint64_t)PPUMemoryReadHandlers) + (INI * 8)), (uint64_t) READFunc, END - INI);\
    memset64( (void*) (((uint64_t)PPUMemoryWriteHandlers) + (INI * 8)), (uint64_t) WRITEFunc, END - INI);
#endif

DEFREADPPUMEM_H(Palette){
    return PPUMemory.Palette[Addr & 0x1F];
}

DEFWRITEPPUMEM_H(Palette){
    PPUMemory.Palette[Addr & 0x1F] = Val;
}

DEFREADPPUMEM_H(PaletteTransp){
    return PPUMemory.Palette[0];
}

DEFWRITEPPUMEM_H(PaletteTransp){
    PPUMemory.Palette[Addr & 0x1F] = Val;
}


void initPPUMemoryHandlers(){
    int i, k;
    
    #ifdef DEBUG
        fprintf(stderr, "Initializing PPU Memory Handlers...\n");
    #endif    
    
    //initialize all Memory Handlers to zero. This simplifies debugging
    memset(PPUMemoryReadHandlers, 0, sizeof(uint8_t(*)(uint16_t)) * 0x10000);
    memset(PPUMemoryWriteHandlers, 0, sizeof(uint8_t(*)(uint16_t)) * 0x10000);
                
    //Pattern table 0
    PPUHANDLER_SETUP(0, 0x1000, PatternTable0PPURead, PatternTable0PPUWrite)
    
    //Pattern table 1
    PPUHANDLER_SETUP(0x1000, 0x2000, PatternTable1PPURead, PatternTable1PPUWrite)  

    //Nametable 0
    PPUHANDLER_SETUP(0x2000, 0x23C0, NameTable0PPURead, NameTable0PPUWrite)
            
    //Attribute table 0
    PPUHANDLER_SETUP(0x23C0, 0x2400, AttrTable0PPURead, AttrTable0PPUWrite)
    
    //Nametable 1
    PPUHANDLER_SETUP(0x2400, 0x27C0, NameTable1PPURead, NameTable1PPUWrite)

    //Attribute table 1
    PPUHANDLER_SETUP(0x27C0, 0x2800, AttrTable1PPURead, AttrTable1PPUWrite)    

    //Nametable 2
    PPUHANDLER_SETUP(0x2800, 0x2BC0, NameTable2PPURead, NameTable2PPUWrite)

    //Attribute table 2
    PPUHANDLER_SETUP(0x2BC0, 0x2C00, AttrTable2PPURead, AttrTable2PPUWrite) 
    
    //Nametable 3
    PPUHANDLER_SETUP(0x2C00, 0x2FC0, NameTable3PPURead, NameTable3PPUWrite)

    //Attribute table 3
    PPUHANDLER_SETUP(0x2FC0, 0x3000, AttrTable3PPURead, AttrTable3PPUWrite)   
    
    //Palette Handling
    PPUHANDLER_SETUP(0x3F00, 0x4000, PalettePPURead, PalettePPUWrite)
           
    for(i = 0x3000; i < 0x3F00; i++){
        PPUMemoryReadHandlers[i] = PPUMemoryReadHandlers[i - 0x1000];
        PPUMemoryWriteHandlers[i] = PPUMemoryWriteHandlers[i - 0x1000];
    }
            
    //Create mirrors for transparency so that every fourth entry is a 
    //mirror to logical 0x3F00
    for(i = 0x3F04; i < 0x4000; i+=4){
        PPUMemoryReadHandlers[i] = PaletteTranspPPURead;
        PPUMemoryWriteHandlers[i] = PaletteTranspPPUWrite;
    }
    
}

//Initializes all PPU memory data structures
//Also handles PPU NT mirroring
//Palette mirroring is done at the PPU RAM memory handler level, setup
//in initPPUMemoryHandlers
void initPPUMemory(RomImage* ROM){
    //The following lines are ignored for now
    //Let's assume the first two VROM banks are loaded into PT[0] and PT[1]
    //Later, when mapper support comes in, this will have to be changed
    //That's why VROMBanks is limited to 2
    
    int VROMBanks, i;
    void* Buffer1, *Buffer2;
    
    #ifdef DEBUG
        fprintf(stderr, "Initializing PPU Memory...\n");
    #endif
    
    //VROMBanks = ROM->Header->VROMBanks > 2 ? 2 : ROM->Header->VROMBanks;
    
    //PPUMemory.PT = malloc(sizeof(uint8_t*) * ROM->Header->VROMBanks);
    
    //Point to the VROM banks loaded from the ROM File
    //for(i = 0; i < VROMBanks; i++)
        //PPUMemory.PT[i] = ROM->VROM[i];
    
    //Hack! hhahaha
    PPUMemory.PT = malloc(sizeof(uint8_t*) * 2);
    PPUMemory.PT[0] = ROM->VROM[0];
    PPUMemory.PT[1] = ROM->VROM[0] + 0x1000;
    
    //There are 4 Nametables
    PPUMemory.NT = malloc(sizeof(uint8_t*) * 4);    
    
    //Although there are only 2 physical nametables. Therefore, 
    //We should allocate the appropriate ones and get the mirroring going
    
    PPUMemory.NametableBuffers = malloc(sizeof(uint8_t) * 2);
    
    Buffer1 = calloc(0x03C0, sizeof(uint8_t));
    Buffer2 = calloc(0x03C0, sizeof(uint8_t));
    
    PPUMemory.NametableBuffers[0] = Buffer1;
    PPUMemory.NametableBuffers[1] = Buffer2;
    
    if(GetROMMirroring(ROM) == M_HORIZONTAL){
        PPUMemory.NT[0] = Buffer1;
        PPUMemory.NT[1] = Buffer1;
        PPUMemory.NT[2] = Buffer2;
        PPUMemory.NT[3] = Buffer2;
    }
    else{
        PPUMemory.NT[0] = Buffer1;
        PPUMemory.NT[1] = Buffer2;
        PPUMemory.NT[2] = Buffer1;
        PPUMemory.NT[3] = Buffer2;        
    }
    
    //There are 4 Attribute tables
    PPUMemory.AT = malloc(sizeof(uint8_t*) * 4);
    
    //There's enough physical RAM for all attribute tables
    PPUMemory.AT[0] = calloc(0x40, sizeof(uint8_t));
    PPUMemory.AT[1] = calloc(0x40, sizeof(uint8_t));
    PPUMemory.AT[2] = calloc(0x40, sizeof(uint8_t));
    PPUMemory.AT[3] = calloc(0x40, sizeof(uint8_t));
    
    //Zero-out SPR-RAM
    memset(PPUMemory.SPRRAM, 0, 0x100);
    
    initPPUMemoryHandlers();
}

