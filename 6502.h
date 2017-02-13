#ifndef __6502_H
#define	__6502_H

#include <stdint.h>

typedef struct {    
    uint8_t  A;         //Accumulator
    uint8_t  X;         //X index Register
    uint8_t  Y;         //Y index Register
    uint8_t  P;         //Program status register
    uint16_t PC;        //Program counter
    uint8_t S;         //Stack Pointer - grows down (towards lower addresses)

    //Is the current operation using the accumulator addr mode?
    //Needs to be reset manually after the corresponding instructions are
    //executed
    volatile uint8_t  ACC_OP; 
    
    //Stores the last operand effective address
    uint16_t LastEffectiveAddress;   
   
    uint8_t InstructionClockTicks;
    
    uint8_t CurrentOpcode;
    
    //Stores the numbers of clock ticks elapsed since power-on
    uint64_t ClockTicks;
    
    uint8_t PendingInterrupts;
    
    //Prevents reading from I/O registers when in ST(A,Y,X) operations.
    volatile uint8_t AMPreventRead;
    
    //Used for DMA cycle burning
    uint16_t DMACycles;
    
    //uint8_t Memory[0xFFFF];
    
    uint32_t Jammed;
    
} __Context;

//Don't change this. The order MATTERS.
typedef enum {
    AM_IMMEDIATE = 0,
    AM_RELATIVE,
    AM_IMPLIED,      
    AM_ACCUMULATOR,            
    AM_ZERO_PAGE,
    AM_ZERO_PAGE_X,
    AM_ZERO_PAGE_Y,
    AM_ABSOLUTE,
    AM_ABSOLUTE_X,
    AM_ABSOLUTE_Y,
    AM_INDIRECT, 
    AM_INDIRECT_X,
    AM_INDIRECT_Y,     
} __AddrModes;

typedef struct{
    void (*InstructionPtr)();
    __AddrModes AddressingMode;
    uint8_t Size;
    uint8_t OpCycles;
    uint8_t ScanData;
    uint8_t LastEffectiveAddressInstruction;
} InstructionData;

typedef struct{
    const char* (*InstructionPtr)();
    __AddrModes AddressingMode;
    uint8_t Size;
    uint8_t OpCycles;
    uint8_t ScanData;
    uint8_t LastEffectiveAddressInstruction;
} InstructionDebugData;

//Interrupt definitions
#define INT_NMI 0x01
#define INT_IRQ 0x02

//Status Flags
#define STATUS_CARRY            (CPU.P & 0x01)
#define STATUS_ZERO             (CPU.P & 0x02)
#define STATUS_INTERRUPT        (CPU.P & 0x04)
#define STATUS_BCD              (CPU.P & 0x08)
#define STATUS_SOFT_IRQ         (CPU.P & 0x10)
#define STATUS_OVERFLOW         (CPU.P & 0x40)
#define STATUS_SIGN             (CPU.P & 0x80)

#define STATUS_M_CARRY          (0x01)
#define STATUS_M_ZERO           (0x02)
#define STATUS_M_INTERRUPT      (0x04)
#define STATUS_M_BCD            (0x08)
#define STATUS_M_SOFT_IRQ       (0x10)
#define STATUS_M_OVERFLOW       (0x40)
#define STATUS_M_SIGN           (0x80)

#define STATUS_SET_CARRY            (CPU.P |= 0x01)
#define STATUS_SET_ZERO             (CPU.P |= 0x02)
#define STATUS_SET_INTERRUPT        (CPU.P |= 0x04)
#define STATUS_SET_BCD              (CPU.P |= 0x08)
#define STATUS_SET_SOFT_IRQ         (CPU.P |= 0x10)
#define STATUS_SET_OVERFLOW         (CPU.P |= 0x40)
#define STATUS_SET_SIGN             (CPU.P |= 0x80)

#define STATUS_CLR_CARRY            (CPU.P &= 0xFE)
#define STATUS_CLR_ZERO             (CPU.P &= 0xFD)
#define STATUS_CLR_INTERRUPT        (CPU.P &= 0xFB)
#define STATUS_CLR_BCD              (CPU.P &= 0xF7)
#define STATUS_CLR_SOFT_IRQ         (CPU.P &= 0xEF)
#define STATUS_CLR_OVERFLOW         (CPU.P &= 0xBF)
#define STATUS_CLR_SIGN             (CPU.P &= 0x7F)

void init6502();

int32_t INT_Main(int32_t Cycles);
int32_t INT_runCycles(int32_t Cycles);
int32_t HandleInterrupts(int32_t Cycles, uint32_t* Interrupt);

void CPUsetNMI();
void CPUsetIRQ();

void CPUaddDMACycles(uint16_t Cycles);

void TerminateCPU();

uint8_t CPUGetInstructionSize(uint8_t Opcode);

#endif	/* __6502_H */
