#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "NESMemory.h"
#include "romLoader.h"
#include "6502.h"
#include "PPU.h"
#include "Input.h"
#include "6502_Debug.h"
#include "mappers/mapper.h"

#ifdef DEBUGCONSOLE
extern __Debug_Context Debug;
#endif

/* Array of memory Handlers. One for each byte of the address space 
 These handlers are used to treat things like mirroring and memory mapped
 IO registers*/

uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

//Okay. This global variable is excusable.
extern __Context CPU;

NESMainMemory NESRAM;

//Zero-page handlers
DEFREADMEM_H(ZeroPage) {
    //Mirrored across RAM. Only the first set is considered as actual RAM
    return NESRAM.RAM[Addr & 0x00FF];
}

DEFWRITEMEM_H(ZeroPage){
    //Mirrored across RAM.
    NESRAM.RAM[Addr & 0x00FF] = Val;
}

//Stack handlers
DEFREADMEM_H(Stack) {
    return NESRAM.RAM[(Addr & 0x00FF) | 0x100];
}

DEFWRITEMEM_H(Stack){
    NESRAM.RAM[(Addr & 0x00FF) | 0x100] = Val;
}

//Scrap RAM handlers
DEFREADMEM_H(ScrapRAM) {
    return NESRAM.RAM[Addr & 0x07FF];
}

DEFWRITEMEM_H(ScrapRAM){
    NESRAM.RAM[Addr & 0x07FF] = Val;
}

uint8_t ReadMemory(uint16_t Addr){
    #ifdef DEBUG
    if(!(NESMemoryReadHandlers[Addr])){
        fflush(stdout);
        fprintf(stderr, "\nERROR: Memory READ handler for address 0x%04X not set.\n\tIn: %s: %d\n", Addr, __FILE__, __LINE__);
        exit(1);
    }
    #endif

    #ifdef DEBUGCONSOLE
    {
        DebugHook *p;
        for(p = (DebugHook*) Debug.DebugHookList; p != NULL; p = (DebugHook*) (p->Next)){
            if(p->HookType == DHT_MEMWATCH){
                if ((Addr == p->Data.MemWatchHook.Addr) && (p->Data.MemWatchHook.ActionFlags & DA_READ) ){
                    p->Data.MemWatchHook.BrokeOn = DA_READ;
                    debugPrompt(p);
                    break;
                }
            }
        }
    }     
    #endif    
    
    return NESMemoryReadHandlers[Addr](Addr);
}

void    WriteMemory(uint16_t Addr, uint8_t Val){
    #ifdef DEBUG
    if(!(NESMemoryWriteHandlers[Addr])){
        fflush(stdout);
        fprintf(stderr, "\nERROR: Memory WRITE handler for address 0x%04X not set.\n\tIn: %s: %d\n", Addr, __FILE__, __LINE__);
        exit(1);
    }
    #endif    

    #ifdef DEBUGCONSOLE
    {
        DebugHook *p;
        for(p = (DebugHook*) Debug.DebugHookList; p != NULL; p = (DebugHook*) (p->Next)){
            if(p->HookType == DHT_MEMWATCH){
                if ((Addr == p->Data.MemWatchHook.Addr) && (p->Data.MemWatchHook.ActionFlags & DA_WRITE) ){
                    p->Data.MemWatchHook.BrokeOn = DA_WRITE;
                    p->Data.MemWatchHook.BreakValue = Val;
                    debugPrompt(p);
                    break;
                }
            }
        }
    }     
    #endif      
    
    NESMemoryWriteHandlers[Addr](Addr, Val);
}

//DUMMY MEMORY ACCESSORS
DEFREADMEM_H(Dummy){
    return 0;
}

DEFWRITEMEM_H(Dummy){}  

//EXPROM
DEFREADMEM_H(EXPROM){
    return NESRAM.Expansion[Addr & 0x1FDF];
}

DEFWRITEMEM_H(EXPROM){
    NESRAM.Expansion[Addr & 0x1FDF] = Val;
}

void initNESMemoryHandlers(Mapper *M){
    uint32_t i, k;
    
    #ifdef DEBUG
        fprintf(stderr, "Initializing NES Main Memory Handlers...\n");
    #endif
    
    //Reset all these function pointers to zero.
    memset(NESMemoryReadHandlers, 0, sizeof(uint8_t(*)(uint16_t)) * 0x10000);
    memset(NESMemoryWriteHandlers, 0, sizeof(uint8_t(*)(uint16_t)) * 0x10000);
    
    //setup RAM handlers
    for(i = 0; i < 0x2000; i+= 0x0800){
        //setup zero-page handlers
        for(k = 0; k < 0x100; k++){
            NESMemoryReadHandlers[i + k] = ZeroPageRead;
            NESMemoryWriteHandlers[i + k] = ZeroPageWrite;
        }
        //setup stack handlers
        for(k = 0; k < 0x100; k++){
            NESMemoryReadHandlers[0x100 + i + k] = StackRead;
            NESMemoryWriteHandlers[0x100 + i + k] = StackWrite;
        }
        //setup scrap RAM handlers
        for(k = 0; k < 0x600; k++){
            NESMemoryReadHandlers[0x200 + i + k] = ScrapRAMRead;
            NESMemoryWriteHandlers[0x200 + i + k] = ScrapRAMWrite;
        }        
    }  
    
    //setup IO register memory Handlers
    //mapped in PPU, APU, INPUT, etc.
    
    for(i = 0x4000; i <= 0x4020; i++){
        if(i == 0x4014 || i == 0x4016 || i == 0x4017)
            continue;
        
        NESMemoryReadHandlers[i] = DummyRead;
        NESMemoryWriteHandlers[i] = DummyWrite;
        
    }
    //setup expansion rom memory handlers
    //TODO
    for(i = 0x4020; i < 0x6000; i++){
        NESMemoryReadHandlers[i] = EXPROMRead;
        NESMemoryWriteHandlers[i] = EXPROMWrite;
    }
    
    //setup SRAM memory handlers
    //TODO
    for(i = 0x6000; i <= 0x8000; i++){
        
        NESMemoryReadHandlers[i] = DummyRead;
        NESMemoryWriteHandlers[i] = DummyWrite;
    }    
    
    //PRGROM Handlers are setup by the Mapper
    M->SetupCPUMemoryHandlers();
}

void initNESMemory(RomImage *ROM, Mapper* M){
    /*Let's focus on emulating Mapper-less ROMs for now.
    This should be changed later to support multiple mappers.
    Actually, a initNESRAMMapper function may be called from here to setup
    the initial RAM configuration for the Main Memory. Likewise,
    there should be a similar function for initializing PPU memory.*/
    
    // Set the CPU Memory to 0's (Just for debugging. This should be removed later)
    #ifdef DEBUG
        memset(&NESRAM, 0x00, sizeof(NESMainMemory));
        fprintf(stderr, "Initializing NES Main Memory...\n");
    #endif
    
    //load the program ROM into RAM
    //memcpy(NESRAM.PRGROML, ROM->PRGROM[0], 0x4000);
    
    //If there are two program banks, the second should be 
    //loaded into the upper memory bank
    //if(ROM->Header->PRGBanks > 1)
    //    memcpy(NESRAM.PRGROMH, ROM->PRGROM[1], 0x4000);
    //else
        //otherwise, we should load de upper memory bank
        //with the same code as the lower memory bank
    //    memcpy(NESRAM.PRGROMH, ROM->PRGROM[0], 0x4000);
    
    M->Reset();
        
        
    //Initialize all RAM Mirroring
    //I/O registers are mapped in module-specific mapXXXRegisters functions
    initNESMemoryHandlers(M);
    
    //Initializes I/O registers for the PPU
    mapPPURegisters();
    
    //Initializes I/O registers for the Input system
    mapInputRegisters();
}
