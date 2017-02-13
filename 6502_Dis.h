#ifndef NES6502_DIS_H
#define	NES6502_DIS_H

#include <stdint.h>
#include "6502.h"

typedef struct{
    const char* (*InstructionPtr)();
    __AddrModes AddressingMode;
    uint8_t Size;
    uint8_t OpCycles;    
    uint8_t ScanData;
    uint8_t LastEffectiveAddressInstruction;
} DisassemblyData;

//Returns a disassembled instruction
char* Disassemble(uint8_t Opcode, uint8_t* Operands, uint16_t Address); 

#endif	/* 6502_DIS_H */

