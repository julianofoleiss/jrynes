#include "6502.h"
#include "6502_Dis.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//Instruction mnemonic disassembly routines
//Since the opcodes also identifies which addressing mode is being
//used, the only thing these routines should do is return
//the mnemonic text associated.
static const char* adc(){const char *Text = "ADC"; return Text;}
static const char* and(){const char *Text = "AND"; return Text;}
static const char* asl(){const char *Text = "ASL"; return Text;}
static const char* bcc(){const char *Text = "BCC"; return Text;}
static const char* bcs(){const char *Text = "BCS"; return Text;}
static const char* beq(){const char *Text = "BEQ"; return Text;}
static const char* bit(){const char *Text = "BIT"; return Text;}
static const char* bmi(){const char *Text = "BMI"; return Text;}
static const char* bne(){const char *Text = "BNE"; return Text;}
static const char* bpl(){const char *Text = "BPL"; return Text;}
static const char* brk(){const char *Text = "BRK"; return Text;}
static const char* bvc(){const char *Text = "BVC"; return Text;}
static const char* bvs(){const char *Text = "BVS"; return Text;}
static const char* clc(){const char *Text = "CLC"; return Text;}
static const char* cld(){const char *Text = "CLD"; return Text;}
static const char* cli(){const char *Text = "CLI"; return Text;}
static const char* clv(){const char *Text = "CLV"; return Text;}
static const char* cmp(){const char *Text = "CMP"; return Text;}
static const char* cpx(){const char *Text = "CPX"; return Text;}
static const char* cpy(){const char *Text = "CPY"; return Text;}
static const char* dec(){const char *Text = "DEC"; return Text;}
static const char* dex(){const char *Text = "DEX"; return Text;}
static const char* dey(){const char *Text = "DEY"; return Text;}
static const char* eor(){const char *Text = "EOR"; return Text;}
static const char* inc(){const char *Text = "INC"; return Text;}
static const char* inx(){const char *Text = "INX"; return Text;}
static const char* iny(){const char *Text = "INY"; return Text;}
static const char* jmp(){const char *Text = "JMP"; return Text;}
static const char* jsr(){const char *Text = "JSR"; return Text;}
static const char* lda(){const char *Text = "LDA"; return Text;}
static const char* ldx(){const char *Text = "LDX"; return Text;}
static const char* ldy(){const char *Text = "LDY"; return Text;}
static const char* lsr(){const char *Text = "LSR"; return Text;}
static const char* nop(){const char *Text = "NOP"; return Text;}
static const char* ora(){const char *Text = "ORA"; return Text;}
static const char* pha(){const char *Text = "PHA"; return Text;}
static const char* php(){const char *Text = "PHP"; return Text;}
static const char* pla(){const char *Text = "PLA"; return Text;}
static const char* plp(){const char *Text = "PLP"; return Text;}
static const char* rol(){const char *Text = "ROL"; return Text;}
static const char* ror(){const char *Text = "ROR"; return Text;}
static const char* rti(){const char *Text = "RTI"; return Text;}
static const char* rts(){const char *Text = "RTS"; return Text;}
static const char* sbc(){const char *Text = "SBC"; return Text;}
static const char* sec(){const char *Text = "SEC"; return Text;}
static const char* sed(){const char *Text = "SED"; return Text;}
static const char* sei(){const char *Text = "SEI"; return Text;}
static const char* sta(){const char *Text = "STA"; return Text;}
static const char* stx(){const char *Text = "STX"; return Text;}
static const char* sty(){const char *Text = "STY"; return Text;}
static const char* tax(){const char *Text = "TAX"; return Text;}
static const char* tay(){const char *Text = "TAY"; return Text;}
static const char* tsx(){const char *Text = "TSX"; return Text;}
static const char* txa(){const char *Text = "TXA"; return Text;}
static const char* txs(){const char *Text = "TXS"; return Text;}
static const char* tya(){const char *Text = "TYA"; return Text;}
static const char* aso(){const char *Text = "ASO"; return Text;}
static const char* dop(){const char *Text = "DOP"; return Text;}
static const char* top(){const char *Text = "TOP"; return Text;}
static const char* jam(){const char *Text = "JAM"; return Text;}       

const DisassemblyData DisassemblyInfo[256] = {
    #include "6502_OpcodeMap.h"    
};

char* DisAddressingModeImmediate(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(5, sizeof(uint8_t));
    
    sprintf(FinalOperand, "#$%02X", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeAbsolute(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(6, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X%02X", Operands[1], Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeAbsoluteX(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X%02X,X", Operands[1], Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeAbsoluteY(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X%02X,Y", Operands[1], Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeZeroPage(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(4, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeZeroPageX(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(6, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X,X", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeZeroPageY(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(6, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X,Y", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeIndirect(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    sprintf(FinalOperand, "($%02X%02X)", Operands[1], Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeIndirectX(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    sprintf(FinalOperand, "($%02X,X)", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeIndirectY(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    sprintf(FinalOperand, "($%02X),Y", Operands[0]);
    
    return FinalOperand;
}

char* DisAddressingModeAccumulator(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(8, sizeof(uint8_t));
    
    FinalOperand[0] = 'A';
    
    return FinalOperand;
}

char* DisAddressingModeImplied(uint8_t* Operands){
    return NULL;
}

char* DisAddressingModeRelative(uint8_t* Operands){
    char* FinalOperand;
    
    FinalOperand = calloc(16, sizeof(uint8_t));
    
    sprintf(FinalOperand, "$%02X (%d)", Operands[0], (int8_t)Operands[0]);
    
    return FinalOperand;    
}      

char* (*DisAMHandlers[13])(uint8_t*) = {
    DisAddressingModeImmediate,
    DisAddressingModeRelative,
    DisAddressingModeImplied,
    DisAddressingModeAccumulator,    
    DisAddressingModeZeroPage,
    DisAddressingModeZeroPageX,
    DisAddressingModeZeroPageY,
    DisAddressingModeAbsolute,
    DisAddressingModeAbsoluteX,
    DisAddressingModeAbsoluteY,
    DisAddressingModeIndirect,    
    DisAddressingModeIndirectX,
    DisAddressingModeIndirectY,
}; 

char* Disassemble(uint8_t Opcode, uint8_t* Operands, uint16_t Address){
    char *Mnemonic = NULL, *Ops = NULL, *Instruction = NULL;
    char *Nothing;
    uint32_t size = 0;
    
    Nothing = calloc(1, sizeof(char));
    
    Mnemonic = (char*) DisassemblyInfo[Opcode].InstructionPtr();
    Ops = DisAMHandlers[DisassemblyInfo[Opcode].AddressingMode](Operands);
    
    if(Ops)
        size += strlen(Ops);
    
    size += strlen(Mnemonic);
    size+=16;
    
    Instruction = calloc(size, sizeof(char));
    
    sprintf(Instruction, "<0x%04X> %s %s", Address, Mnemonic, Ops ? Ops : Nothing);
   
    free(Ops);
    free(Nothing);
    
    return Instruction;
}
