#include "mapper.h"
#include "mapper000.h"
#include "../NESMemory.h"

extern NESMainMemory NESRAM;

extern uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
extern void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

extern Mapper *CurrentMapper;

void M001_Reset(void){
    
}

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
    
}

void M001_SetupCPUMemoryHandlers(void){
    
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

void M001_SetupPPUMemoryHandlers(void){
    
}