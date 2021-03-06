#include "mapper.h"
#include "mapper000.h"
#include "../NESMemory.h"
#include "../PPU.h"

extern NESMainMemory NESRAM;

extern uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
extern void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

extern Mapper *CurrentMapper;

void M007_Reset(){
    
    RomImage *ROM;
    
    ROM = CurrentMapper->ROM;
    
    //setting up PRGBanks
    NESRAM.PRGROMLL = ROM->PRGROM[0];
    NESRAM.PRGROMLH = ROM->PRGROM[1];
    NESRAM.PRGROMHL = ROM->PRGROM[2];
    NESRAM.PRGROMHH = ROM->PRGROM[3];
    
    //TODO: setup VRAMBanks
    
}

//Default PRGROM Handlers
DEFREADMEM_H(PRGROMLL){
    return NESRAM.PRGROMLL[Addr & 0x1FFF];
}

DEFREADMEM_H(PRGROMLH){
    return NESRAM.PRGROMLH[Addr & 0x1FFF];
}

DEFREADMEM_H(PRGROMHL){
    return NESRAM.PRGROMHL[Addr & 0x1FFF];
}

DEFREADMEM_H(PRGROMHH){
    return NESRAM.PRGROMHH[Addr & 0x1FFF];
}

DEFWRITEMEM_H(PRGROM){
    
    int _32kbank;
    
    _32kbank = Val & 0x7;
    
    NESRAM.PRGROMLL = CurrentMapper->ROM->PRGROM[(_32kbank*4)];
    NESRAM.PRGROMLH = CurrentMapper->ROM->PRGROM[(_32kbank*4)+1];
    NESRAM.PRGROMHL = CurrentMapper->ROM->PRGROM[(_32kbank*4)+2];
    NESRAM.PRGROMHH = CurrentMapper->ROM->PRGROM[(_32kbank*4)+3];
    
    if(Val & 0x10){
        PPU_SetMirroring(1, 1, 1, 1);
    }
    else{
        PPU_SetMirroring(0, 0, 0, 0);
    }
    
}


void M007_SetupCPUMemoryHandlers(void){

    int i;
    
    for(i = 0x8000; i < 0xA000; i++){
        NESMemoryReadHandlers[i] =  PRGROMLLRead;
        NESMemoryWriteHandlers[i] = PRGROMWrite;
    }
    
    for(i = 0xA000; i < 0xC000; i++){
        NESMemoryReadHandlers[i] =  PRGROMLHRead;
        NESMemoryWriteHandlers[i] = PRGROMWrite;
    }    
 
    for(i = 0xC000; i < 0xE000; i++){
        NESMemoryReadHandlers[i] =  PRGROMHLRead;
        NESMemoryWriteHandlers[i] = PRGROMWrite;
    }       
    
    for(i = 0xE000; i < 0x10000; i++){
        NESMemoryReadHandlers[i] =  PRGROMHHRead;
        NESMemoryWriteHandlers[i] = PRGROMWrite;
    }      
    
}

void M007_SetupPPUMemoryHandlers(void){

}
