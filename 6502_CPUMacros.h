#ifndef _6502_CPUMACROS_H
#define	_6502_CPUMACROS_H

#include "NESMemory.h"


/* Combine flags into flag register */
#define  COMBINE_FLAGS()( \
    (FlagN & STATUS_M_SIGN) | \
    (FlagV ? STATUS_M_OVERFLOW : 0) | \
    0x20 | \
    (FlagB ? STATUS_M_SOFT_IRQ : 0) | \
    (FlagD ? STATUS_M_BCD : 0) | \
    (FlagI ? STATUS_M_INTERRUPT : 0) | \
    (FlagZ ? 0 : STATUS_M_ZERO) | \
    (FlagC ? STATUS_M_CARRY : 0) \
)

/* Scatter flags to separate variables */
#define  SCATTER_FLAGS(Value){ \
    FlagN = (Value) & STATUS_M_SIGN; \
    FlagV = (Value) & STATUS_M_OVERFLOW; \
    FlagB = (Value) & STATUS_M_SOFT_IRQ; \
    FlagD = (Value) & STATUS_M_BCD; \
    FlagI = (Value) & STATUS_M_INTERRUPT; \
    FlagZ = !((Value) & STATUS_M_ZERO); \
    FlagC = (Value) & STATUS_M_CARRY; \
}

#define  GET_GLOBAL_REGS(){ \
    PC = CPU.PC; \
    A = CPU.A; \
    X = CPU.X; \
    Y = CPU.Y; \
    SCATTER_FLAGS(CPU.P); \
    S = CPU.S; \
}

#define  STORE_LOCAL_REGS(){ \
    CPU.PC = PC; \
    CPU.A = A; \
    CPU.X = X; \
    CPU.Y = Y; \
    CPU.P = COMBINE_FLAGS(); \
    CPU.S = S; \
}

//Fetches a byte from memory and stores
//It only readas memory that is not related to I/O Registers. For those, 
//We use the Good Ol' ReadMemory mechanism
#define READ_MEMORY(Address)( \
{ \
    uint16_t TempAddr = Address; \
    uint8_t ReadTemp; \
    if(TempAddr < 0x800) \
        ReadTemp = (NESRAM.RAM[TempAddr]); \
    else if(TempAddr < 0x2000) \
        ReadTemp = (NESRAM.RAM[TempAddr & 0x7FF]); \
    else if(TempAddr >= 0x8000) \
        ReadTemp = (TempAddr < 0xA000 ? NESRAM.PRGROMLL[TempAddr & 0x1FFF] : TempAddr < 0xC000 ? NESRAM.PRGROMLH[TempAddr & 0x1FFF] : TempAddr < 0xE000 ? NESRAM.PRGROMHL[TempAddr & 0x1FFF] : NESRAM.PRGROMHH[TempAddr & 0x1FFF]);\
    else \
        ReadTemp = ReadMemory(TempAddr); \
    ReadTemp; \
} \
)

#define READ_MEMORY_TOVAL(Address) \
    Value = READ_MEMORY(Address);

#define READ_WORD(Address) \
    ( (READ_MEMORY(Address)) | (READ_MEMORY(Address + 1) << 8))

#define READ_WORD_WA(Address) \
    ((READ_MEMORY(Address)) | (READ_MEMORY((Address + 1) & 0xFF) << 8))

#define WRITE_MEMORY(Address, Value){ \
    uint16_t TempAddr_W = Address; \
    uint8_t TempVal_W = Value; \
    if(TempAddr_W < 0x800) \
        (NESRAM.RAM[TempAddr_W] = TempVal_W); \
    else if(TempAddr_W < 0x2000) \
        (NESRAM.RAM[TempAddr_W & 0x7FF] = TempVal_W); \
    else \
        WriteMemory(TempAddr_W, TempVal_W); \
}

#define CHECK_PAGE_BOUNDARY_CROSS(Address, Register) \
    if(Register > (Address & 0xFF)) \
        CPU.ClockTicks++;

/*Addressing Modes*/
    
/*Immediate Addressing Mode*/
#define AM_IMMD \
    Address = PC; \
    READ_MEMORY_TOVAL(Address); \
    PC++;

/*Absolute Addressing Modes*/
/*Absolute*/
#define AM_ABS_ADDR \
    Address = READ_WORD(PC); \
    PC+=2;
    
#define AM_ABS \
    AM_ABS_ADDR \
    READ_MEMORY_TOVAL(Address);

/*Absolute $nnnn,X*/
#define AM_ABS_INDX_ADDR \
    AM_ABS_ADDR \
    Address = (Address + X) & 0xFFFF; \
    CHECK_PAGE_BOUNDARY_CROSS(Address, X);
    
#define AM_ABS_INDX \
    AM_ABS_INDX_ADDR \
    READ_MEMORY_TOVAL(Address);

/*Absolute $nnnn,Y*/
#define AM_ABS_INDY_ADDR \
    AM_ABS_ADDR \
    Address = (Address + Y) & 0xFFFF; \
    CHECK_PAGE_BOUNDARY_CROSS(Address, Y);
    
#define AM_ABS_INDY \
    AM_ABS_INDY_ADDR \
    READ_MEMORY_TOVAL(Address);
    
/*Zero-Page Addressing Modes*/    
/*Zero-Page*/    
#define AM_ZP_ADDR \
    AM_IMMD \
    Address = ((uint16_t) (Value & 0xFF));
    
#define AM_ZP \
    AM_ZP_ADDR \
    READ_MEMORY_TOVAL(Address)

/*Zero-Page $nn,X*/
#define AM_ZP_INDX_ADDR \
    AM_ZP_ADDR \
    Address = (Address + X) & 0xFF;

#define AM_ZP_INDX \
    AM_ZP_INDX_ADDR \
    READ_MEMORY_TOVAL(Address)

/*Zero-Page $nn,Y*/
#define AM_ZP_INDY_ADDR \
    AM_ZP_ADDR \
    Address = (Address + Y) & 0xFF;
    
#define AM_ZP_INDY \
    AM_ZP_INDY_ADDR \
    READ_MEMORY_TOVAL(Address)
    
/*Indexed Indirect*/

#define AM_INDIR_X_ADDR \
    AM_IMMD \
    Address = ((uint16_t) (Value & 0xFF)); \
    Address =  READ_WORD_WA(((Address + X) ));

#define AM_INDIR_X \
    AM_INDIR_X_ADDR \
    READ_MEMORY_TOVAL(Address)
    
/*Indirect Indexed*/
#define AM_INDIR_Y_ADDR \
    AM_IMMD \
    Address = ((uint16_t) (Value & 0xFF)); \
    Address = (READ_WORD_WA(Address) + Y);
 
#define AM_INDIR_Y \
    AM_INDIR_Y_ADDR \
    READ_MEMORY_TOVAL(Address); \
    CHECK_PAGE_BOUNDARY_CROSS(Address, Y);
    
//Stack access
//PUSH
#define StackPush(Value)\
    WRITE_MEMORY((S | 0x100), Value);\
    S--;

//PULL (aka. pop)
#define StackPop\
    READ_MEMORY(((++S) | 0x100))

#define SET_NZ_FLAGS(Value) \
    FlagN = FlagZ = (Value);

#define EXTRA_CYCLES(Value){ \
    CPU.ClockTicks += Value; \
    Cycles -= Value; \
}

/* For BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS */
#define RELATIVE_BRANCH(Condition){ \
    if (Condition){ \
        AM_IMMD \
        if (((int8_t) Value + (PC & 0x00FF)) & 0x100) \
            EXTRA_CYCLES(1); \
        PC += ((int8_t) Value); \
    } \
    else{ \
        EXTRA_CYCLES(2) \
        PC++; \
    } \
}

#define JUMP(Address){ \
   PC = READ_WORD((Address)); \
}

#define JMP_INDIRECT(){ \
   Temp16 = READ_WORD(PC); \
   /* bug in crossing page boundaries */ \
   if (0xFF == (Temp16 & 0xFF)) \
      PC = (READ_MEMORY(Temp16 & 0xFF00) << 8) | READ_MEMORY(Temp16); \
   else \
      JUMP(Temp16); \
}

#define JMP_ABSOLUTE(){ \
   JUMP(PC); \
}

/*
** Interrupt macros
*/
#define NMI_PROC(){ \
   StackPush(PC >> 8); \
   StackPush(PC & 0xFF); \
   FlagB = 0; \
   StackPush(COMBINE_FLAGS()); \
   FlagI = 1; \
   JUMP(0xFFFA); \
}

#define IRQ_PROC(){ \
   StackPush(PC >> 8); \
   StackPush(PC & 0xFF); \
   FlagB = 0; \
   StackPush(COMBINE_FLAGS()); \
   FlagI = 1; \
   JUMP(0xFFFE); \
}

#define NMI(){ \
   NMI_PROC(); \
   EXTRA_CYCLES(7); \
}

#define IRQ(){ \
   IRQ_PROC(); \
   EXTRA_CYCLES(7); \
}

/*Instruction Macros*/

#define ADC(ReadFunc){ \
    ReadFunc \
    Temp16 = A + Value + (FlagC ? 1 : 0); \
    FlagC = (Temp16 > 0xFF); \
    FlagV = ((~(A ^ Value)) & (A ^ Temp16) & 0x80); \
    A = (Temp16 & 0xFF); \
    SET_NZ_FLAGS(A) \
}

#define AND(ReadFunc){ \
    ReadFunc \
    A &= Value; \
    SET_NZ_FLAGS(A); \
}

#define ASL(ReadFunc, WriteFunc){ \
    ReadFunc \
    FlagC = (Value & 0x80); \
    Value <<= 1; \
    WriteFunc(Address, Value); \
    SET_NZ_FLAGS(Value); \
}

#define ASL_A(){ \
    FlagC = (A & 0x80); \
    A <<= 1; \
    SET_NZ_FLAGS(A); \
}

#define BCC(){ \
    RELATIVE_BRANCH(0 == FlagC); \
}

#define BCS(){ \
    RELATIVE_BRANCH(0 != FlagC); \
}

#define BEQ(){ \
    RELATIVE_BRANCH(0 == FlagZ); \
}

#define BIT(ReadFunc){ \
    ReadFunc \
    FlagZ = (Value & A); \
    /* move bit 7/6 of data into N/V flags */ \
    FlagN = Value; \
    FlagV = (Value & STATUS_M_OVERFLOW); \
}

#define BMI(){ \
    RELATIVE_BRANCH(FlagN & STATUS_M_SIGN); \
}

#define BNE(){ \
    RELATIVE_BRANCH(0 != FlagZ); \
}

#define BPL(){ \
    RELATIVE_BRANCH(0 == (FlagN & STATUS_M_SIGN)); \
}

#define BRK(){ \
    PC++; \
    StackPush((PC >> 8)); \
    StackPush((PC & 0xFF)); \
    FlagB = 1; \
    StackPush(COMBINE_FLAGS()); \
    FlagI = 1; \
    JUMP(0xFFFE); \
}

#define BVC(){ \
    RELATIVE_BRANCH(0 == FlagV); \
}

#define BVS(){ \
    RELATIVE_BRANCH(0 != FlagV); \
}

#define CLC(){ \
    FlagC = 0; \
}

#define CLD(){ \
    FlagD = 0; \
}

#define CLI(){ \
    FlagI = 0; \
    if ((CPU.PendingInterrupts & INT_IRQ) && (Cycles > 0)){ \
        IRQ(); \
        CPU.PendingInterrupts &= (~((uint8_t)INT_IRQ)); \
    } \
}

#define CLV(){ \
    FlagV = 0; \
}

#define _COMPARE(Reg, Value){ \
    Temp16 = (Reg) - (Value); \
    /* C is clear when data > A */ \
    FlagC = !(Temp16 & 0x100); \
    SET_NZ_FLAGS(Temp16 & 0xFF); \
}

#define CMP(ReadFunc){ \
    ReadFunc \
    _COMPARE(A, Value); \
}

#define CPX(ReadFunc){ \
    ReadFunc \
    _COMPARE(X, Value); \
}

#define CPY(ReadFunc){ \
    ReadFunc \
    _COMPARE(Y, Value); \
}

#define DEC(ReadFunc, WriteFunc){ \
    ReadFunc \
    Value = (Value - 1) & 0xFF; \
    WriteFunc(Address, Value); \
    SET_NZ_FLAGS(Value); \
}

#define DEX(){ \
    X = (X - 1) & 0xFF; \
    SET_NZ_FLAGS(X); \
}

#define DEY(){ \
    Y = (Y - 1) & 0xFF; \
    SET_NZ_FLAGS(Y); \
}

#define EOR(ReadFunc){ \
    ReadFunc \
    A ^= Value; \
    SET_NZ_FLAGS(A); \
}

#define INC(ReadFunc, WriteFunc){ \
    ReadFunc \
    Value = (Value+1) & 0xFF; \
    WriteFunc(Address, Value) \
    SET_NZ_FLAGS(Value); \
}

#define INX(){ \
    X = (X + 1) & 0xFF; \
    SET_NZ_FLAGS(X); \
}

#define INY(){ \
    Y = (Y + 1) & 0xFF; \
    SET_NZ_FLAGS(Y); \
}

#define JAM(){ \
    STORE_LOCAL_REGS() \
    CPU.Jammed = 1; \
}

#define JSR(){ \
    PC++; \
    StackPush((PC >> 8)); \
    StackPush((PC & 0xFF)); \
    JUMP(PC - 1); \
}

#define LDA(ReadFunc){ \
    ReadFunc \
    A = Value; \
    SET_NZ_FLAGS(A); \
}

#define LDX(ReadFunc){ \
    ReadFunc \
    X = Value; \
    SET_NZ_FLAGS(X); \
}

#define LDY(ReadFunc){ \
    ReadFunc \
    Y = Value; \
    SET_NZ_FLAGS(Y); \
}

#define LSR(ReadFunc, WriteFunc){ \
    ReadFunc \
    FlagC = (Value & 0x01); \
    Value >>= 1; \
    WriteFunc(Address, Value); \
    SET_NZ_FLAGS(Value); \
}

#define LSR_A(){ \
    FlagC = (A & 0x01); \
    A >>= 1; \
    SET_NZ_FLAGS(A); \
}

#define NOP(){ \
}

#define ORA(ReadFunc){ \
   ReadFunc \
   A |= Value; \
   SET_NZ_FLAGS(A);\
}

#define PHA(){ \
   StackPush(A); \
}

#define PHP(){ \
   /* B flag is pushed on stack as well */ \
   StackPush((COMBINE_FLAGS() | STATUS_M_SOFT_IRQ)); \
}

#define PLA(){ \
   A = StackPop; \
   SET_NZ_FLAGS(A); \
}

#define PLP(){ \
   Temp8 = StackPop; \
   SCATTER_FLAGS(Temp8); \
}

#define ROL(ReadFunc, WriteFunc){ \
   ReadFunc \
   if(FlagC){ \
      FlagC = (Value & 0x80); \
      Value = ((Value << 1) | 1); \
   } \
   else{ \
      FlagC = (Value & 0x80); \
      Value <<= 1; \
   } \
   WriteFunc(Address, Value) \
   SET_NZ_FLAGS(Value); \
}

#define ROL_A(){ \
   if (FlagC){ \
      FlagC = (A & 0x80); \
      A = ((A << 1) | 1); \
   } \
   else{ \
      FlagC = (A & 0x80); \
      A <<= 1; \
   } \
   SET_NZ_FLAGS(A); \
}

#define ROR(ReadFunc, WriteFunc){ \
   ReadFunc \
   if (FlagC){ \
      FlagC = (Value & 1); \
      Value = ((Value >> 1) | 0x80); \
   } \
   else{ \
      FlagC = (Value & 1); \
      Value >>= 1; \
   } \
   WriteFunc(Address, Value); \
   SET_NZ_FLAGS(Value); \
}

#define ROR_A(){ \
   if (FlagC){ \
      FlagC = (A & 1); \
      A = ((A >> 1) | 0x80); \
   } \
   else{ \
      FlagC = (A & 1); \
      A >>= 1; \
   } \
   SET_NZ_FLAGS(A); \
}

#define RTI(){ \
   Temp8 = StackPop; \
   SCATTER_FLAGS(Temp8); \
   PC = StackPop; \
   PC |= (StackPop << 8); \
   if ((!FlagI) && (CPU.PendingInterrupts & INT_IRQ) && (Cycles > 0)){ \
      CPU.PendingInterrupts &= (~((uint8_t)INT_IRQ)); \
      IRQ(); \
   } \
}

#define RTS(){ \
   PC = StackPop; \
   PC = (PC | ((StackPop) << 8)) + 1; \
}

#define SBC(ReadFunc){ \
   ReadFunc \
   Temp16 = A - Value - (FlagC ? 0 : 1); \
   FlagV = ((A ^ Value) & (A ^ Temp16) & 0x80); \
   FlagC = (Temp16 < 0x100); \
   A = (uint8_t)(Temp16 & 0xFF); \
   SET_NZ_FLAGS(A); \
}

#define SEC(){ \
   FlagC = 1; \
}

#define SED(){ \
   FlagD = 1; \
}

#define SEI(){ \
   FlagI = 1; \
}

#define STA(ReadFunc, WriteFunc){ \
   ReadFunc \
   WriteFunc(Address, A) \
}

#define STX(ReadFunc, WriteFunc){ \
   ReadFunc \
   WriteFunc(Address, X) \
}

#define STY(ReadFunc, WriteFunc){ \
   ReadFunc \
   WriteFunc(Address, Y) \
}

#define TAX(){ \
   X = A; \
   SET_NZ_FLAGS(X);\
}

#define TAY(){ \
   Y = A; \
   SET_NZ_FLAGS(Y);\
}

#define TSX(){ \
   X = S; \
   SET_NZ_FLAGS(X);\
}

#define TXA(){ \
   A = X; \
   SET_NZ_FLAGS(A);\
}

#define TXS(){ \
   S = X; \
}

#define TYA(){ \
   A = Y; \
   SET_NZ_FLAGS(A); \
}

#endif	/* _6502_CPUMACROS_H */

