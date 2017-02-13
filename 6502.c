#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "NESMemory.h"
#include "6502.h"
#include "6502_Debug.h"
#include "JRYNES.h"

extern EmulationSettings Settings;

#ifdef INSTRUCTIONIR
#include "6502_InstructionDeclarations.h"
#endif

#ifdef DEBUGCONSOLE
extern __Debug_Context Debug;
#endif

uint16_t MakeWord(uint8_t LSB, uint8_t HSB){
    uint16_t Word = 0;
    Word = LSB | (HSB << 8);
    return Word;
}

const char* __AddrModeText[] = {
    "AM_IMMEDIATE",
    "AM_RELATIVE",
    "AM_IMPLIED",  
    "AM_ACCUMULATOR",    
    "AM_ZERO_PAGE",
    "AM_ZERO_PAGE_X",
    "AM_ZERO_PAGE_Y",
    "AM_ABSOLUTE",
    "AM_ABSOLUTE_X",
    "AM_ABSOLUTE_Y",
    "AM_INDIRECT", 
    "AM_INDIRECT_X",
    "AM_INDIRECT_Y"
};

// Holds the 6502 context
#ifdef INSTRUCTIONIR
extern __Context CPU;
#else
__Context CPU;
#endif

//Define how to the read the memory. This
//may be changed afterwards, which is why
//I am not accessing memory directly
/*#define ReadMemory(Pos)\
        (CPU.Memory[Pos])*/

#define CalculateEffectiveAddress(Base, Reg)\
        ((Base) + CPU.Reg)

#define CalculateEffectiveAddressWA(Base, Reg)\
        (CalculateEffectiveAddress((Base), Reg) & 0xFF)

#define ReadMemoryIndexed(Pos, Reg)\
        ReadMemory(CalculateEffectiveAddress(Pos, Reg))

//ReadMemoryIndexed with wrap around addition
#define ReadMemoryIndexedWA(Pos, Reg)\
        ReadMemory((CalculateEffectiveAddress(Pos, Reg)) & 0xFF)

//Define how to write to memory
/*#define WriteMemory(Pos, Value)\
        (CPU.Memory[Pos] = Value)*/

//Stack access
//PUSH
#define StackPush(Value)\
        WriteMemory((CPU.S | 0x100), Value);\
        CPU.S--

//PULL (aka. pop)
#define StackPop\
        ReadMemory((++CPU.S) | 0x100)

//Used to calculate whether to add or subtract a certain offset from PC
#define CalculateRelativePCOffset(Offset)\
        (Offset & 0x80 ? -(Offset & 0x7F) : Offset)

/*
void PrintCPUStatus(){
    fprintf(stderr, "CPU Context:\n");
    fprintf(stderr, "\tA:0x%02X, PC:0x%02X, S:0x%02X\n", CPU.A, CPU.PC, CPU.S);
    fprintf(stderr, "\tX:0x%02X, Y:0x%02X\n", CPU.X, CPU.Y);
    
    fprintf(stderr, "\tP: (0x%02X) - [", CPU.P);
    fprintf(stderr, "%c ", STATUS_SIGN ? 'S' : '_');
    fprintf(stderr, "%c ", STATUS_OVERFLOW ? 'V' : '_');
    fprintf(stderr, "%c ", (CPU.P & 0x20) ? '1' : '0');
    fprintf(stderr, "%c ", STATUS_SOFT_IRQ ? 'B' : '_');
    fprintf(stderr, "%c ", STATUS_BCD ? 'D' : '_');
    fprintf(stderr, "%c ", STATUS_INTERRUPT ? 'I' : '_');
    fprintf(stderr, "%c ", STATUS_ZERO ? 'Z' : '_');
    fprintf(stderr, "%c", STATUS_CARRY ? 'C' : '_');
    fprintf(stderr, "]\n");
    
    fprintf(stderr, "\tLast Effective Address: 0x%04X\n", CPU.LastEffectiveAddress);
    fprintf(stderr, "\tElapsed clock ticks: %ld\n", CPU.ClockTicks);
    fprintf(stderr, "\tCurrent Opcode: 0x%02X\n", CPU.CurrentOpcode);
    
    fprintf(stderr, "\tPending interrupts: (0x%02X) ", CPU.PendingInterrupts);
    
    fprintf(stderr, "%s ", CPU.PendingInterrupts & INT_IRQ ? "IRQ" : "_");
    fprintf(stderr, "%s ", CPU.PendingInterrupts & INT_NMI ? "NMI" : "_");
    fprintf(stderr, "\n");

    fprintf(stderr, "\tAM Prevent Read: %d\n", CPU.AMPreventRead);
    fprintf(stderr, "\tAccumulator as operand: %d\n", CPU.ACC_OP);
    fprintf(stderr, "\n");
}
*/

// Maps an addressing mode to functions that
// decode operands
#ifdef INSTRUCTIONIR
extern uint8_t (*AddressingModeDecoders[13])();
#else
uint8_t (*AddressingModeDecoders[13])();
#endif

#ifndef INSTRUCTIONIR
// Decodes the Immediate Addressing Mode
uint8_t ImmediateAddrModeDecoder(){
    uint8_t Operand;
    
    CPU.LastEffectiveAddress = CPU.PC + 1;
    
    if(!CPU.AMPreventRead)
        Operand = ReadMemory(CPU.LastEffectiveAddress);
    
    return Operand;
}

// Decodes the Absolute Addressing Mode
uint8_t AbsoluteAddrModeDecoder(){
    uint8_t Operand[2];
    uint8_t Value;
    Operand[0] = ReadMemory(CPU.PC+1);
    Operand[1] = ReadMemory(CPU.PC+2);
    
    CPU.LastEffectiveAddress = MakeWord(Operand[0], Operand[1]);
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    return Value;
}

uint8_t ZeroPageAddrModeDecoder(){
    uint8_t Operand;
    uint8_t Value;
    
    Operand = ReadMemory(CPU.PC+1);
    
    CPU.LastEffectiveAddress = Operand;
    
    Value = ReadMemory(CPU.LastEffectiveAddress);
    
    return Value;
}

uint8_t ZeroPageXAddrModeDecoder(){
    uint8_t Operand;
    uint8_t Value;
    
    Operand = ReadMemory(CPU.PC + 1);
    
    CPU.LastEffectiveAddress = (CalculateEffectiveAddress(Operand, X) & 0xFF);
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    return Value;
}

uint8_t ZeroPageYAddrModeDecoder(){
    uint8_t Operand;
    uint8_t Value;
    
    Operand = ReadMemory(CPU.PC + 1);
    
    CPU.LastEffectiveAddress = (CalculateEffectiveAddress(Operand, Y) & 0xFF);
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    return Value;
}

// Decodes the Absolute,X Addressing Mode
uint8_t AbsoluteXAddrModeDecoder(){
    uint8_t Operand[2];
    uint8_t Value;
    uint16_t AbsAddress;
    
    Operand[0] = ReadMemory(CPU.PC+1);
    Operand[1] = ReadMemory(CPU.PC+2);
    
    AbsAddress = MakeWord(Operand[0], Operand[1]);
    
    CPU.LastEffectiveAddress =  CalculateEffectiveAddress(AbsAddress, X);
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    switch(CPU.CurrentOpcode){
        case 0x7D:
        case 0x3D:
        case 0xDD:
        case 0x5D:
        case 0xBD:
        case 0xBC:
        case 0x10:
        case 0xFD:
            if((AbsAddress & 0xFF00) != (CPU.LastEffectiveAddress & 0xFF))
                CPU.InstructionClockTicks++;
    }
    
    return Value;
}

// Decodes the Absolute,Y Addressing Mode
uint8_t AbsoluteYAddrModeDecoder(){
    uint8_t Operand[2];
    uint8_t Value;
    uint16_t AbsAddress;
    
    Operand[0] = ReadMemory(CPU.PC+1);
    Operand[1] = ReadMemory(CPU.PC+2);
    
    AbsAddress = MakeWord(Operand[0], Operand[1]);
    
    CPU.LastEffectiveAddress =  CalculateEffectiveAddress(AbsAddress, Y);
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    switch(CPU.CurrentOpcode){
        case 0x39:
        case 0xD9:
        case 0x59:
        case 0xB9:
        case 0xBE:
        case 0x19:
        case 0xF9:    
            if((AbsAddress & 0xFF00) != (CPU.LastEffectiveAddress & 0xFF))
                CPU.InstructionClockTicks++;
    }
    
    return Value;
}

// Decodes the (Indirect,X) Addressing Mode
uint8_t IndirectXAddrModeDecoder(){
    uint8_t Operand;
    uint8_t Value;
    
    Operand = ReadMemory(CPU.PC+1);
    
    CPU.LastEffectiveAddress =                 
                MakeWord(   
                    ReadMemoryIndexedWA(Operand, X), 
                    ReadMemoryIndexedWA(Operand, X + 1)
                );
    
    if(!CPU.AMPreventRead)
        Value =  ReadMemory(CPU.LastEffectiveAddress);    
    
    return Value;
}

// Decodes the (Indirect), Y Addressing Mode
uint8_t IndirectYAddrModeDecoder(){
    uint8_t Operand;
    uint8_t Value;
    uint16_t Address;
    
    Operand = ReadMemory(CPU.PC+1);
    
    Address = MakeWord(   
                    ReadMemory(Operand), 
                    ReadMemory(
                        ((Operand + 1) & 0xFF)
                    ) 
                );
    
    CPU.LastEffectiveAddress = 
            CalculateEffectiveAddress(Address, Y);
    
    switch(CPU.CurrentOpcode){
        case 0xD1:
        case 0x51:    
        case 0xB1:
            if((Address & 0xFF00) != (CPU.LastEffectiveAddress & 0xFF))
                CPU.InstructionClockTicks++;
    }
    
    if(!CPU.AMPreventRead)
        Value = ReadMemory(CPU.LastEffectiveAddress);
    
    return Value;
}

// Decodes the Accumulator Addressing Mode
uint8_t AccumulatorAddrModeDecoder(){
    CPU.ACC_OP = 1;
    return 0;
}

// Decodes the Relative Addressing Mode
uint8_t RelativeAddrModeDecoder(){
    uint8_t Operand;
    
    CPU.LastEffectiveAddress = CPU.PC + 1;
    
    Operand = ReadMemory(CPU.LastEffectiveAddress);
    
    return Operand;
}

//There's no decoding in Implied addressing
uint8_t ImpliedAddrModeDecoder(){
    return 0;
}

// Decodes the Indirect Addressing Mode
uint8_t IndirectAddrModeDecoder(){
    
    //Since this is only used by the JMP instruction, which requires
    //a 16-bit value, we only set CPU.LastEffectiveAddress
    
    uint8_t Operand[2];
    uint16_t Address = 0, Address2 = 0;
    
    Operand[0] = ReadMemory(CPU.PC+1);
    Operand[1] = ReadMemory(CPU.PC+2);    
    
    Address = MakeWord(Operand[0], Operand[1]);
    
    //Bug on real 6502
    if(Operand[0] == 0xFF)
        Address2 = MakeWord(0, Operand[1]);
    else
        Address2 = Address + 1;
    
    CPU.LastEffectiveAddress = MakeWord(ReadMemory(Address), ReadMemory(Address2));
    
    return 0;
}

void setupAddressingModeDecoders(){
    
    //Initialize all addr. mode function pointers to NULL
    memset(AddressingModeDecoders, 0, sizeof(void*) * 11);
 
    //Registering all Addressing Mode Decoders
    AddressingModeDecoders[AM_ACCUMULATOR] = AccumulatorAddrModeDecoder;
    AddressingModeDecoders[AM_IMMEDIATE] = ImmediateAddrModeDecoder;
    AddressingModeDecoders[AM_ABSOLUTE] = AbsoluteAddrModeDecoder;
    AddressingModeDecoders[AM_ZERO_PAGE] = ZeroPageAddrModeDecoder;
    AddressingModeDecoders[AM_ZERO_PAGE_X] = ZeroPageXAddrModeDecoder;
    AddressingModeDecoders[AM_ZERO_PAGE_Y] = ZeroPageYAddrModeDecoder;
    AddressingModeDecoders[AM_ABSOLUTE_X] = AbsoluteXAddrModeDecoder;
    AddressingModeDecoders[AM_ABSOLUTE_Y] = AbsoluteYAddrModeDecoder;
    AddressingModeDecoders[AM_INDIRECT_X] = IndirectXAddrModeDecoder;
    AddressingModeDecoders[AM_INDIRECT_Y] = IndirectYAddrModeDecoder;
    AddressingModeDecoders[AM_RELATIVE] = RelativeAddrModeDecoder;
    AddressingModeDecoders[AM_IMPLIED] = ImpliedAddrModeDecoder;
    AddressingModeDecoders[AM_INDIRECT] = IndirectAddrModeDecoder;
}
#endif // NOT INSTRUCTIONIR

#ifdef INSTRUCTIONIR
extern const InstructionData Instructions[256];
#else
//Forward declaration needed
const InstructionData Instructions[256];
#endif

/**********************************/
/*        INSTRUCTIONS            */
/**********************************/

/* ADC - Add memory to accumulator with carry */
void adc(void) {
    uint32_t Temp32;
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();     
      
    Temp32 = CPU.A + Value + STATUS_CARRY;
    
    // Set carry flag
    if(Temp32 > 0xFF)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    // Set the overflow flag
    //if((~(CPU.A ^ Value)) & (CPU.A ^ Temp) & 0x80)
    if(!((CPU.A ^ Value) & 0x80) && ((CPU.A ^ Temp32) & 0x80))
        STATUS_SET_OVERFLOW;
    else
        STATUS_CLR_OVERFLOW;
    
    CPU.A = ((uint8_t) Temp32) & 0xFF;
    
    // Set zero flag
    if(CPU.A & 0xFF)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    // Check if number is negative or positive
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN; 
    
}

/* AND - "AND" memory with accumulator */
void and(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
    
    Value &= CPU.A;
    
    // Set sign flag
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    // Set zero flag
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.A = Value;
}

/* ASL - Shift Left one Bit (Memory or Accumulator)*/
void asl(void){
    uint8_t Value;
    uint8_t Temp8;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 

    
    Temp8 = CPU.ACC_OP ? CPU.A : Value;
    
    // Setting the carry flag. There will be carry if the most significant
    // bit is set
    if(Temp8 & 0x80)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    Temp8 <<= 1;
    
    Temp8 &= 0xFE; //Make sure the last bit is clear
    
    // Set the sign flag
    if(Temp8 & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    // Set the zero flag
    if(Temp8)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    if(CPU.ACC_OP){
        CPU.A = Temp8;
        CPU.ACC_OP = 0;
    }
    else
        WriteMemory(CPU.LastEffectiveAddress, Temp8);
}

static inline void branch(uint8_t Flag, uint8_t Offset){
    uint16_t NewAddress;
    int16_t Disp;   
    
    if(Flag){
        Disp = (int8_t) Offset;
        
        NewAddress = CPU.PC + Disp;
        
        CPU.InstructionClockTicks += 
            ((CPU.PC & 0xFF00) != (NewAddress & 0xFF00)) 
            ? 2
            : 1;
        
        CPU.PC = NewAddress;
    }
}

void bcc(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();   

    CPU.PC += Instructions[0x90].Size;
    
    branch(!STATUS_CARRY, Value);
}

/* BCS - Branch on Carry Set */
void bcs(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
     
    CPU.PC += Instructions[0xB0].Size;
    
    branch(STATUS_CARRY, Value);
}

/* BEQ - Branch on equal (zero) */
void beq(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
            
    CPU.PC += Instructions[0xF0].Size;
    
    branch(STATUS_ZERO, Value);
}

/* BIT - Test bits in memory with accumulator */
void bit(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();       
        
    //Copy sign bit (N)
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    //Copy bit 6 to overflow flag (V)
    if(Value & 0x40)
        STATUS_SET_OVERFLOW;
    else
        STATUS_CLR_OVERFLOW;
        
    //This is correct. That's because if it was the other way around
    //Excitebike fails miserably.
        
    if(CPU.A & Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

void bmi(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();   
                 
    CPU.PC += Instructions[0x30].Size;
    
    branch(STATUS_SIGN, Value);
}

/* BNE - Branch on not equal (zero flag clear) */
void bne(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 

    CPU.PC += Instructions[0xD0].Size;
    
    branch(!STATUS_ZERO, Value);
}

/* BPL - Branch on positive (sign flag clear) */
void bpl(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
    
    CPU.PC += Instructions[0x10].Size;
    
    branch(!STATUS_SIGN, Value);
}

/* BRK - Force Break */
void brk(void){          
    CPU.PC+= Instructions[0x00].Size;
    
    StackPush(((CPU.PC >> 8) & 0xFF));
    StackPush((CPU.PC & 0xFF));
    
    //Set soft irq flag
    STATUS_SET_SOFT_IRQ;        
    
    StackPush(CPU.P);
    
    //Block interrupts
    STATUS_SET_INTERRUPT;       
    
    CPU.PC = (ReadMemory(0xFFFE) | (ReadMemory(0xFFFF) << 8));
}

/* BVC - Branch on Overflow Clear */
void bvc(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 

    CPU.PC += Instructions[0x50].Size;
    
    branch(!STATUS_OVERFLOW, Value);
}

/* BVS - Branch on Overflow Set */
//%%% INSTRUCTION DEFINITION
void bvs(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 

    
    CPU.PC += Instructions[0x70].Size;
    
    branch(STATUS_OVERFLOW, Value);
}

/* CLC - Clear Carry Flag */
void clc(void){   
    STATUS_CLR_CARRY;
}

/* CLD - Clear Decimal Flag */
void cld(void){
    STATUS_CLR_BCD;
}

/* CLI - Clear Interrupt (Disable) Flag */
void cli(void){
    STATUS_CLR_INTERRUPT;
}

/* CLV - Clear Overflow Flag */
//%%% INSTRUCTION DEFINITION
void clv(void){
    STATUS_CLR_OVERFLOW;
}

/* CMP - Compare Memory and Accumulator */
//%%% INSTRUCTION DEFINITION
void cmp(void){
    uint8_t Value;
    uint32_t Temp32;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Temp32 = CPU.A - Value;
    
    if(Temp32 & 0x100)
        STATUS_CLR_CARRY;
    else
        STATUS_SET_CARRY;
    
    Value = (uint8_t)Temp32;
    
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value & 0xFF)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
}

/* CPX - Compare Memory and X */
//%%% INSTRUCTION DEFINITION
void cpx(void){
    uint8_t Value;
    uint32_t Temp32;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Temp32 = CPU.X - Value;
    
    if(Temp32 & 0x100)
        STATUS_CLR_CARRY;
    else
        STATUS_SET_CARRY;
    
    Value = (uint8_t)Temp32;
    
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value & 0xFF)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* CPY - Compare Memory and Y */
void cpy(void){
    uint8_t Value;
    uint32_t Temp32;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
    
    Temp32 = CPU.Y - Value;
    
    if(Temp32 & 0x100)
        STATUS_CLR_CARRY;
    else
        STATUS_SET_CARRY;
    
    Value = (uint8_t)Temp32;
    
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value & 0xFF)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* DEC - Decrement Memory by One*/
void dec(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Value = (Value - 1) & 0xFF;
    
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    WriteMemory(CPU.LastEffectiveAddress, Value);
}

/* DEX - Decrement index X by one */
void dex(void){
    CPU.X = (CPU.X - 1) & 0xFF;
    
    if(CPU.X & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.X)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* DEY - Decrement index Y by one */
//%%% INSTRUCTION DEFINITION
void dey(void){
    CPU.Y = (CPU.Y - 1) & 0xFF;
    
    if(CPU.Y & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.Y)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* EOR - Exclusive Or memory with accumulator*/
void eor(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 

    CPU.A ^= Value;
    
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.A)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* INC - Increment Memory by One*/
void inc(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Value = (Value + 1) & 0xFF;
    
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    WriteMemory(CPU.LastEffectiveAddress, Value);
}

/* INX - Increment index X by one */
void inx(void){
    CPU.X = (CPU.X + 1) & 0xFF;
    
    if(CPU.X & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.X)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* INY - Increment index Y by one */
void iny(void){
    CPU.Y = (CPU.Y + 1) & 0xFF;
    
    if(CPU.Y & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.Y)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
}

/* JMP - Jump to new Location*/
void jmp(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    CPU.PC = CPU.LastEffectiveAddress;
}

/* JSR - Jump to new Location saving return address*/
void jsr(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
    CPU.PC += 2;
    
    StackPush(((CPU.PC >> 8) & 0xFF));
    StackPush((CPU.PC & 0xFF));
    
    CPU.PC = CPU.LastEffectiveAddress;
}

/* LDA - Load memory to accumulator */
void lda(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.A = Value;
}

/* LDX - Load memory to X index register */
void ldx(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.X = Value;
}

/* LDY - Load memory to Y index register */
void ldy(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.Y = Value;
}

/* LSR - Shift Right one Bit (Memory or Accumulator)*/
void lsr(void){
    uint8_t Value;
    uint8_t Temp8;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Temp8 = CPU.ACC_OP ? CPU.A : Value;
    
    // Setting the carry flag. There will be carry if the least significant
    // bit is set
    if(Temp8 & 0x01)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    Temp8 >>= 1;
    
    //Make sure the highest-order bit is clear
    Temp8 &= 0x7F; 
    
    // Clear the sign flag
    STATUS_CLR_SIGN;
    
    // Set the zero flag
    if(Temp8)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    if(CPU.ACC_OP){
        CPU.A = Temp8;
        CPU.ACC_OP = 0;
    }
    else
        WriteMemory(CPU.LastEffectiveAddress, Temp8);
}

/* NOP - Does nothing at all. )*/
void nop(void){
}

/* ORA - "OR" memory with accumulator */
void ora(void){
    uint8_t Value;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Value |= CPU.A;
    
    // Set sign flag
    if(Value & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    // Set zero flag
    if(Value)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.A = Value;
}

/* PHA Push accumulator on stack */
void pha(void){
    StackPush(CPU.A);
}

/* PHP Push processor status register on stack */
void php(void){
    //Set SOFT_IRQ flag
    StackPush((CPU.P | 0x10));  
}

/* PLA Pull accumulator from stack */
void pla(void){
    CPU.A = StackPop;
    
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.A)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
}

/* PLP Pull processor status register from stack */
void plp(void){
    CPU.P = StackPop;
}

/* ROL Rotate one bit left */
void rol(void){
    uint8_t Value;
    uint16_t Temp16;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Temp16 = CPU.ACC_OP ? CPU.A : Value;
    
    Temp16 <<= 1;
    
    if(STATUS_CARRY)
        Temp16 |= 1;
    else
        Temp16 &= 0xFE;
    
    if(Temp16 & 0x100)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    Temp16 &= 0xFF;
    
    if(Temp16 & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Temp16)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    if(CPU.ACC_OP){
        CPU.A = (uint8_t) Temp16;
        CPU.ACC_OP = 0;
    }
    else
        WriteMemory(CPU.LastEffectiveAddress, ((uint8_t)Temp16));    
}

/* ROR Rotate one bit right */
void ror(void){
    uint8_t Value;
    uint16_t Temp16;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Temp16 = CPU.ACC_OP ? CPU.A : Value;
    
    if(STATUS_CARRY)
        Temp16 |= 0x100;
    
    if(Temp16 & 0x01)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    Temp16 >>= 1;
    
    Temp16 &= 0xFF;
    
    if(Temp16 & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(Temp16)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;    
    
    if(CPU.ACC_OP){
        CPU.A = (uint8_t)Temp16;
        CPU.ACC_OP = 0;
    }
    else
        WriteMemory(CPU.LastEffectiveAddress, ((uint8_t)Temp16));    
}

/* RTI Return from interrupt */
void rti(void){
    uint8_t H, L;
    
    CPU.P = StackPop;
    L = StackPop;
    H = StackPop;
    
    CPU.PC = MakeWord(L, H);
}

/* RTS Return from subroutine */
void rts(void){
    uint8_t H, L;
    
    L = StackPop;
    H = StackPop;
    
    CPU.PC = MakeWord(L, H);
    CPU.PC++;
}

/* SBC Subtract from accumulator with carry */
void sbc(void){
    uint8_t Value;
    uint32_t Temp32;
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode](); 
        
    Value ^=0xFF;
      
    Temp32 = CPU.A + Value + STATUS_CARRY;
    
    // Set carry flag
    if(Temp32 > 0xFF)
        STATUS_SET_CARRY;
    else
        STATUS_CLR_CARRY;
    
    // Set the overflow flag
    if(!((CPU.A ^ Value) & 0x80) && ((CPU.A ^ Temp32) & 0x80))
        STATUS_SET_OVERFLOW;
    else
        STATUS_CLR_OVERFLOW;
    
    CPU.A = ((uint8_t) Temp32) & 0xFF;
    
    // Set zero flag
    if(CPU.A & 0xFF)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    // Check if number is negative or positive
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN; 
    
}

/* SEC Set Carry flag*/
void sec(void){
    STATUS_SET_CARRY;
}

/* SED Set Decimal Mode*/
void sed(void){    
    STATUS_SET_BCD;
}

/* SEI Set Interrupt Disable*/
void sei(void){
    STATUS_SET_INTERRUPT;
}

/* STA Store Accumulator in Memory */
void sta(void){
    uint8_t Value;
    
    CPU.AMPreventRead = 1;
 
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();    
    
    CPU.AMPreventRead = 0;    
        
    WriteMemory(CPU.LastEffectiveAddress, CPU.A);
}

/* STX Store X in Memory */
void stx(void){
    uint8_t Value;

    CPU.AMPreventRead = 1;
    
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();    
    
    CPU.AMPreventRead = 0;
    
    WriteMemory(CPU.LastEffectiveAddress, CPU.X);
}

/* STY Store Y in Memory */
void sty(void){
    uint8_t Value;

    CPU.AMPreventRead = 1;
    
    Value = AddressingModeDecoders[Instructions[CPU.CurrentOpcode].AddressingMode]();    
    
    CPU.AMPreventRead = 0;
        
    WriteMemory(CPU.LastEffectiveAddress, CPU.Y);
}

/* TAX Transfer Accumulator to X */
void tax(void){
    
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.A)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.X = CPU.A;
}

/* TAY Transfer Accumulator to Y */
void tay(void){
    if(CPU.A & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.A)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.Y = CPU.A;
}

/* TSX Transfer Stack Pointer to X */
void tsx(void){
    if(CPU.S & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.S)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.X = CPU.S;
}

/* TXA Transfer X to Accumulator */
void txa(void){
    if(CPU.X & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.X)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.A = CPU.X;
}

/* TXS Transfer X to Stack Pointer */
void txs(void){
    CPU.S = CPU.X;
}

/* TYA Transfer Y to Accumulator */
void tya(void){
    if(CPU.Y & 0x80)
        STATUS_SET_SIGN;
    else
        STATUS_CLR_SIGN;
    
    if(CPU.Y)
        STATUS_CLR_ZERO;
    else
        STATUS_SET_ZERO;
    
    CPU.A = CPU.Y;
}

/* Used for undefined / unimplemented opcodes*/
//%%% INSTRUCTION DEFINITION
void jam(void){
    printf("\n\nInvalid Opcode!\n");
    PrintCPUStatus();
    debugPrompt(NULL);
}

const InstructionData Instructions[] = {
    #include"6502_OpcodeMap.h"
};

/*
void init6502(){
    
    #ifdef DEBUG
        fprintf(stderr, "Initializing NES CPU...\n");
    #endif
    
    //Initialize CPU Registers
    CPU.A = 0;
    CPU.P = 0x20;
    CPU.X = 0;
    CPU.Y = 0;
    CPU.PC = MakeWord(ReadMemory(0xFFFC), ReadMemory(0xFFFD));
    CPU.S = 0xFD;
    
    //Initialize helper variables
    CPU.ACC_OP = 0;
    CPU.LastEffectiveAddress = 0;
    CPU.InstructionClockTicks = 0;
    CPU.PendingInterrupts = 0;
    CPU.AMPreventRead = 0;
    
    //Initialize the clock
    CPU.ClockTicks = 0;
    CPU.DMACycles = 0;
    
    //It's necessary to setup the addressing mode decoders
    setupAddressingModeDecoders();
    
    //CPU.Memory[0x0008] = 0xF7;
    //CPU.Memory[0x0009] = 0xEF;
    //CPU.Memory[0x000A] = 0xDF;
    //CPU.Memory[0x000F] = 0xBF;
}
*/

/*
void TerminateCPU(){
    PrintCPUStatus();
}
*/

/*
void CPUsetNMI(){
    CPU.PendingInterrupts |= INT_NMI;
}

void CPUsetIRQ(){
    CPU.PendingInterrupts |= INT_IRQ;
}
*/

void NMI(){
    #ifdef INSTRUCTION_TRACING
        printf("NMI - ");
    #endif     
    
    StackPush(((CPU.PC >> 8) & 0xFF));
    StackPush((CPU.PC & 0xFF));
    
    STATUS_CLR_SOFT_IRQ;        //Clear soft irq flag
    
    StackPush(CPU.P);
    
    STATUS_SET_INTERRUPT;       //Block interrupts
    
    CPU.PendingInterrupts &= (~((uint8_t)INT_NMI));
    
    CPU.PC = (ReadMemory(0xFFFA) | (ReadMemory(0xFFFB) << 8));    
}

void IRQ(){
    #ifdef INSTRUCTION_TRACING
        printf("IRQ - ");
    #endif     
    
    StackPush(((CPU.PC >> 8) & 0xFF));
    StackPush((CPU.PC & 0xFF));
    
    STATUS_CLR_SOFT_IRQ;        //Clear soft irq flag
    
    StackPush(CPU.P);
    
    STATUS_SET_INTERRUPT;       //Block interrupts
    
    CPU.PendingInterrupts &= (~((uint8_t)INT_IRQ));
    
    CPU.PC = (ReadMemory(0xFFFE) | (ReadMemory(0xFFFF) << 8));     
}

/*
void CPUaddDMACycles(uint16_t Cycles){
    CPU.DMACycles += Cycles;
}

static int32_t INT_Main(int32_t Cycles){    
    int32_t RemainingCycles;
    
    if(Settings.ArgumentFlags & AF_GENERATE_ELAPSED_CYCLES_GRAPH)
        CycleTimerSetCPUBusy();
    
    RemainingCycles = INT_runCycles(Cycles);
    
    if(Settings.ArgumentFlags & AF_GENERATE_ELAPSED_CYCLES_GRAPH)
        CycleTimerClearCPUBusy();
    
    return RemainingCycles;    
}

// Run the desired number of clock cycles
static int32_t INT_runCycles(int32_t Cycles){
    __AddrModes AM;
    uint32_t ElapsedCycles;
    
    ElapsedCycles = 0;
    
    while(ElapsedCycles < Cycles){
        
        if(CPU.DMACycles){
                if((Cycles - ElapsedCycles) <= CPU.DMACycles){
                    //burn remaining cycles and end cycle execution
                    CPU.DMACycles -= (Cycles - ElapsedCycles);
                    CPU.ClockTicks += (Cycles - ElapsedCycles);
                    break;
                }
                else{
                    ElapsedCycles += CPU.DMACycles;
                    CPU.ClockTicks += CPU.DMACycles;
                    CPU.DMACycles = 0;
                }                
        }
        
        if(CPU.PendingInterrupts){
            if(CPU.PendingInterrupts & INT_NMI){
                NMI();
            }

            if(CPU.PendingInterrupts & INT_IRQ){
                if(!STATUS_INTERRUPT){
                    IRQ();
                }
            }
            ElapsedCycles += 7;
        }         
        
        CPU.CurrentOpcode = ReadMemory(CPU.PC);        
        
        AM = Instructions[CPU.CurrentOpcode].AddressingMode;
        
        #ifdef INSTRUCTION_TRACING
            printf("PC: 0x%04X -", CPU.PC);
        #endif

        CPU.InstructionClockTicks = Instructions[CPU.CurrentOpcode].OpCycles;

        #ifdef INSTRUCTION_TRACING
            printf(" [%15s] ", __AddrModeText[AM]);
            printf(" (%02X) ", CPU.CurrentOpcode);
        #endif

        #ifdef DEBUGCONSOLE
            debugCycle(); 
        #endif            
            
        Instructions[CPU.CurrentOpcode].InstructionPtr();

        #ifdef INSTRUCTION_TRACING
        {
            int i;

            if(Instructions[CPU.CurrentOpcode].Size > 1 && AM != AM_RELATIVE)
                for(i = 1; i < Instructions[CPU.CurrentOpcode].Size; i++)
                    printf("%02X ", ReadMemory(CPU.PC + i));
            printf("\n");
        }
        #endif      

        CPU.ClockTicks += CPU.InstructionClockTicks;

        //Relative addressing mode instructions are all branch on the 6502
        //Therefore, PC should be adjusted inside the instruction
        if(AM != AM_RELATIVE)
            CPU.PC += Instructions[CPU.CurrentOpcode].Size;

        ElapsedCycles+= CPU.InstructionClockTicks;
    
    }
    return Cycles - ElapsedCycles;
}
*/

uint8_t CPUGetInstructionSize(uint8_t Opcode){
    
    uint8_t Size;
    
    Size = Instructions[Opcode].Size;
    
    if(!Size){
        if(Instructions[Opcode].AddressingMode == AM_ABSOLUTE)
            Size = 3;
        else if(Instructions[Opcode].AddressingMode == AM_IMPLIED)
            Size = 1;
        else if(Instructions[Opcode].AddressingMode == AM_INDIRECT)
            Size = 3;
        else{
            fprintf(stderr, "%d: Error generating code: Instruction cannot have size 0.\n\n", __LINE__);
            exit(1);
        }
    }    
    
    return Size;
}
