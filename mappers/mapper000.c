#include "mapper.h"
#include "mapper000.h"
#include "../NESMemory.h"

extern NESMainMemory NESRAM;

extern uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
extern void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

extern Mapper *CurrentMapper;

void M000_Reset(void){
    
    RomImage *ROM;
    
    ROM = CurrentMapper->ROM;
    
    //setting up PRGBanks
    NESRAM.PRGROMLL = ROM->PRGROM[0];
    NESRAM.PRGROMLH = ROM->PRGROM[1];
    
    if(ROM->Header->PRGBanks > 1){
        NESRAM.PRGROMHL = ROM->PRGROM[2];
        NESRAM.PRGROMHH = ROM->PRGROM[3];
    }
    else{
        NESRAM.PRGROMHL = ROM->PRGROM[0];
        NESRAM.PRGROMHH = ROM->PRGROM[1];
    }
    
    //TODO: setup VRAMBanks
    
}

//Default PRGROM Handlers
DEFREADMEM_H(PRGROMLL){
    return NESRAM.PRGROMLL[Addr & 0x1FFF];
}

DEFWRITEMEM_H(PRGROMLL){
    NESRAM.PRGROMLL[Addr & 0x1FFF] = Val;
}

DEFREADMEM_H(PRGROMLH){
    return NESRAM.PRGROMLH[Addr & 0x1FFF];
}

DEFWRITEMEM_H(PRGROMLH){
    NESRAM.PRGROMLH[Addr & 0x1FFF] = Val;
}

DEFREADMEM_H(PRGROMHL){
    return NESRAM.PRGROMHL[Addr & 0x1FFF];
}

DEFWRITEMEM_H(PRGROMHL){
    NESRAM.PRGROMHL[Addr & 0x1FFF] = Val;
}

DEFREADMEM_H(PRGROMHH){
    return NESRAM.PRGROMHH[Addr & 0x1FFF];
}

DEFWRITEMEM_H(PRGROMHH){
    NESRAM.PRGROMHH[Addr & 0x1FFF] = Val;
}

void M000_SetupCPUMemoryHandlers(void){

    int i;
    
    for(i = 0x8000; i < 0xA000; i++){
        NESMemoryReadHandlers[i] =  PRGROMLLRead;
        NESMemoryWriteHandlers[i] = PRGROMLLWrite;
    }
    
    for(i = 0xA000; i < 0xC000; i++){
        NESMemoryReadHandlers[i] =  PRGROMLHRead;
        NESMemoryWriteHandlers[i] = PRGROMLHWrite;
    }    
 
    for(i = 0xC000; i < 0xE000; i++){
        NESMemoryReadHandlers[i] =  PRGROMHLRead;
        NESMemoryWriteHandlers[i] = PRGROMHLWrite;
    }       
    
    for(i = 0xE000; i < 0x10000; i++){
        NESMemoryReadHandlers[i] =  PRGROMHHRead;
        NESMemoryWriteHandlers[i] = PRGROMHHWrite;
    }      
    
}

void M000_SetupPPUMemoryHandlers(void){

}