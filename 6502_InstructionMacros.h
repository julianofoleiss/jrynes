
static inline void branch(uint8_t Flag, uint8_t Offset)




__attribute((always_inline))

;

static inline void branch(uint8_t Flag, uint8_t Offset){
    uint16_t NewAddress;
    int16_t Disp;


        printf("%02X ", Offset);



            printf("\n");


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


//ADC
#define adc(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ADC - ");\
\
\
\
            printf("\n");\
\
\
    Temp32 = CPU.A + Value + (CPU.P & 0x01);\
\
\
    if(Temp32 > 0xFF)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
\
\
    if(!((CPU.A ^ Value) & 0x80) && ((CPU.A ^ Temp32) & 0x80))\
        (CPU.P |= 0x40);\
    else\
        (CPU.P &= 0xBF);\
\
    CPU.A = ((uint8_t) Temp32) & 0xFF;\
\
\
    if(CPU.A & 0xFF)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\


//AND
#define and(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("AND - ");\
\
\
\
            printf("\n");\
\
\
    Value &= CPU.A;\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.A = Value;\


//ASL
#define asl(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ASL - ");\
\
\
\
            printf("\n");\
\
\
    Temp8 = CPU.ACC_OP ? CPU.A : Value;\
\
\
\
    if(Temp8 & 0x80)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
    Temp8 <<= 1;\
\
\
\
\
    if(Temp8 & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
\
    if(Temp8)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    if(CPU.ACC_OP){\
        CPU.A = Temp8;\
        CPU.ACC_OP = 0;\
    }\
    else\
        WriteMemory(CPU.LastEffectiveAddress, Temp8);\


//BCC
#define bcc(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BCC - ");\
\
\
    CPU.PC += Instructions[0x90].Size;\
\
    branch(!(CPU.P & 0x01), Value);\


//BCS
#define bcs(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BCS - ");\
\
\
    CPU.PC += Instructions[0xB0].Size;\
\
    branch((CPU.P & 0x01), Value);\


//BEQ
#define beq(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BEQ - ");\
\
\
    CPU.PC += Instructions[0xF0].Size;\
\
    branch((CPU.P & 0x02), Value);\


//BIT
#define bit(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BIT - ");\
\
\
\
            printf("\n");\
\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
\
    if(Value & 0x40)\
        (CPU.P |= 0x40);\
    else\
        (CPU.P &= 0xBF);\
\
\
\
\
    if(CPU.A & Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
\


//BMI
#define bmi(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BMI - ");\
\
\
    CPU.PC += Instructions[0x30].Size;\
\
    branch((CPU.P & 0x80), Value);\


//BNE
#define bne(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BNE - ");\
\
\
    CPU.PC += Instructions[0xD0].Size;\
\
    branch(!(CPU.P & 0x02), Value);\


//BPL
#define bpl(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BPL - ");\
\
\
    CPU.PC += Instructions[0x10].Size;\
\
    branch(!(CPU.P & 0x80), Value);\


//BRK
#define brk(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BRK - ");\
\
\
\
            printf("\n");\
\
\
    CPU.PC+= Instructions[0x00].Size;\
\
    WriteMemory((CPU.S | 0x100), ((CPU.PC >> 8) & 0xFF)); CPU.S--;\
    WriteMemory((CPU.S | 0x100), (CPU.PC & 0xFF)); CPU.S--;\
\
\
    (CPU.P |= 0x10);\
\
    WriteMemory((CPU.S | 0x100), CPU.P); CPU.S--;\
\
\
    (CPU.P |= 0x04);\
\
    CPU.PC = (ReadMemory(0xFFFE) | (ReadMemory(0xFFFF) << 8));\


//BVC
#define bvc(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BVC - ");\
\
\
    CPU.PC += Instructions[0x50].Size;\
\
    branch(!(CPU.P & 0x40), Value);\


//BVS
#define bvs(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("BVS - ");\
\
\
    CPU.PC += Instructions[0x70].Size;\
\
    branch((CPU.P & 0x40), Value);\


//CLC
#define clc(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CLS - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P &= 0xFE);\


//CLD
#define cld(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CLD - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P &= 0xF7);\


//CLI
#define cli(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CLI - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P &= 0xFB);\


//CLV
#define clv(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CLV - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P &= 0xBF);\


//CMP
#define cmp(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CMP - ");\
\
\
\
            printf("\n");\
\
\
    Temp32 = CPU.A - Value;\
\
    if(Temp32 & 0x100)\
        (CPU.P &= 0xFE);\
    else\
        (CPU.P |= 0x01);\
\
    Value = (uint8_t)Temp32;\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value & 0xFF)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\


//CPX
#define cpx(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CPX - ");\
\
\
\
            printf("\n");\
\
\
    Temp32 = CPU.X - Value;\
\
    if(Temp32 & 0x100)\
        (CPU.P &= 0xFE);\
    else\
        (CPU.P |= 0x01);\
\
    Value = (uint8_t)Temp32;\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value & 0xFF)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//CPY
#define cpy(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("CPY - ");\
\
\
\
            printf("\n");\
\
\
    Temp32 = CPU.Y - Value;\
\
    if(Temp32 & 0x100)\
        (CPU.P &= 0xFE);\
    else\
        (CPU.P |= 0x01);\
\
    Value = (uint8_t)Temp32;\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value & 0xFF)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//DEC
#define dec(CONTEXT)\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("DEC - ");\
\
\
\
            printf("\n");\
\
\
    Value = (Value - 1) & 0xFF;\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    WriteMemory(CPU.LastEffectiveAddress, Value);\


//DEX
#define dex(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("DEX - ");\
\
\
\
            printf("\n");\
\
\
    CPU.X = (CPU.X - 1) & 0xFF;\
\
    if(CPU.X & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.X)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//DEY
#define dey(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("DEY - ");\
\
\
\
            printf("\n");\
\
\
    CPU.Y = (CPU.Y - 1) & 0xFF;\
\
    if(CPU.Y & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.Y)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//EOR
#define eor(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("EOR - ");\
\
\
\
            printf("\n");\
\
\
    CPU.A ^= Value;\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.A)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//INC
#define inc(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("INC - ");\
\
\
\
            printf("\n");\
\
\
    Value = (Value + 1) & 0xFF;\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    WriteMemory(CPU.LastEffectiveAddress, Value);\


//INX
#define inx(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("INX - ");\
\
\
\
            printf("\n");\
\
\
    CPU.X = (CPU.X + 1) & 0xFF;\
\
    if(CPU.X & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.X)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//INY
#define iny(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("INY - ");\
\
\
\
            printf("\n");\
\
\
    CPU.Y = (CPU.Y + 1) & 0xFF;\
\
    if(CPU.Y & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.Y)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\


//JMP
#define jmp(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("JMP - ");\
\
\
\
            printf("\n");\
\
\
    CPU.PC = CPU.LastEffectiveAddress;\


//JSR
#define jsr(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("JSR - ");\
\
\
\
            printf("\n");\
\
\
    CPU.PC += 2;\
\
    WriteMemory((CPU.S | 0x100), ((CPU.PC >> 8) & 0xFF)); CPU.S--;\
    WriteMemory((CPU.S | 0x100), (CPU.PC & 0xFF)); CPU.S--;\
\
    CPU.PC = CPU.LastEffectiveAddress;\


//LDA
#define lda(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("LDA - ");\
\
\
\
            printf("\n");\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.A = Value;\


//LDX
#define ldx(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("LDX - ");\
\
\
\
            printf("\n");\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.X = Value;\


//LDY
#define ldy(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("LDY - ");\
\
\
\
            printf("\n");\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.Y = Value;\


//LSR
#define lsr(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("LSR - ");\
\
\
\
            printf("\n");\
\
\
    Temp8 = CPU.ACC_OP ? CPU.A : Value;\
\
\
\
    if(Temp8 & 0x01)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
    Temp8 >>= 1;\
\
\
    Temp8 &= 0x7F;\
\
\
    (CPU.P &= 0x7F);\
\
\
    if(Temp8)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    if(CPU.ACC_OP){\
        CPU.A = Temp8;\
        CPU.ACC_OP = 0;\
    }\
    else\
        WriteMemory(CPU.LastEffectiveAddress, Temp8);\


//NOP
#define nop(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("NOP - ");\
\
\
\
            printf("\n");\
\


//ORA
#define ora(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ORA - ");\
\
\
\
            printf("\n");\
\
\
    Value |= CPU.A;\
\
\
    if(Value & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
\
    if(Value)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.A = Value;\


//PHA
#define pha(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("PHA - ");\
\
\
\
            printf("\n");\
\
\
    WriteMemory((CPU.S | 0x100), CPU.A); CPU.S--;\


//PHP
#define php(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("PHP - ");\
\
\
\
            printf("\n");\
\
\
\
    WriteMemory((CPU.S | 0x100), (CPU.P | 0x10)); CPU.S--;\


//PLA
#define pla(CONTEXT)\
\
        printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("PLA - ");\
\
\
\
        printf("\n");\
\
\
    CPU.A = ReadMemory((++CPU.S) | 0x100);\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.A)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\


//PLP
#define plp(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("PLP - ");\
\
\
\
            printf("\n");\
\
\
    CPU.P = ReadMemory((++CPU.S) | 0x100);\


//ROL
#define rol(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ROL - ");\
\
\
\
            printf("\n");\
\
\
    Temp16 = CPU.ACC_OP ? CPU.A : Value;\
\
    Temp16 <<= 1;\
\
    if((CPU.P & 0x01))\
        Temp16 |= 1;\
    else\
        Temp16 &= 0xFE;\
\
    if(Temp16 & 0x100)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
    Temp16 &= 0xFF;\
\
    if(Temp16 & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Temp16)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    if(CPU.ACC_OP){\
        CPU.A = (uint8_t) Temp16;\
        CPU.ACC_OP = 0;\
    }\
    else\
        WriteMemory(CPU.LastEffectiveAddress, ((uint8_t)Temp16));\


//ROR
#define ror(CONTEXT)\
\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ROR - ");\
\
\
\
            printf("\n");\
\
\
    Temp16 = CPU.ACC_OP ? CPU.A : Value;\
\
    if((CPU.P & 0x01))\
        Temp16 |= 0x100;\
\
    if(Temp16 & 0x01)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
    Temp16 >>= 1;\
\
    Temp16 &= 0xFF;\
\
    if(Temp16 & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(Temp16)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    if(CPU.ACC_OP){\
        CPU.A = (uint8_t)Temp16;\
        CPU.ACC_OP = 0;\
    }\
    else\
        WriteMemory(CPU.LastEffectiveAddress, ((uint8_t)Temp16));\


//RTI
#define rti(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("RTI - ");\
\
\
\
            printf("\n");\
\
\
\
\
\
\
    CPU.P = ReadMemory((++CPU.S) | 0x100);\
    L = ReadMemory((++CPU.S) | 0x100);\
    H = ReadMemory((++CPU.S) | 0x100);\
\
    CPU.PC = MakeWord(L, H);\


//RTS
#define rts(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("RTS - ");\
\
\
\
            printf("\n");\
\
\
\
\
\
\
    L = ReadMemory((++CPU.S) | 0x100);\
    H = ReadMemory((++CPU.S) | 0x100);\
\
    CPU.PC = MakeWord(L, H);\
    CPU.PC++;\


//SBC
#define sbc(CONTEXT)\
\
\
\
\
\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("ADC - ");\
\
\
\
            printf("\n");\
\
\
    Value ^=0xFF;\
\
    Temp32 = CPU.A + Value + (CPU.P & 0x01);\
\
\
    if(Temp32 > 0xFF)\
        (CPU.P |= 0x01);\
    else\
        (CPU.P &= 0xFE);\
\
\
    if(!((CPU.A ^ Value) & 0x80) && ((CPU.A ^ Temp32) & 0x80))\
        (CPU.P |= 0x40);\
    else\
        (CPU.P &= 0xBF);\
\
    CPU.A = ((uint8_t) Temp32) & 0xFF;\
\
\
    if(CPU.A & 0xFF)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\


//SEC
#define sec(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("SEC - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P |= 0x01);\


//SED
#define sed(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("SED - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P |= 0x08);\


//SEI
#define sei(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("SEI - ");\
\
\
\
            printf("\n");\
\
\
    (CPU.P |= 0x04);\


//STA
#define sta(CONTEXT)\
\
\
\
\
    CPU.AMPreventRead = 1;\
\
\
\
\
\
    CONTEXT\
\
    CPU.AMPreventRead = 0;\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("STA - ");\
\
\
\
            printf("\n");\
\
\
    WriteMemory(CPU.LastEffectiveAddress, CPU.A);\


//STX
#define stx(CONTEXT)\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("STX - ");\
\
\
\
            printf("\n");\
\
\
    WriteMemory(CPU.LastEffectiveAddress, CPU.X);\


//STY
#define sty(CONTEXT)\
\
    CONTEXT\
\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("STY - ");\
\
\
\
            printf("\n");\
\
\
    WriteMemory(CPU.LastEffectiveAddress, CPU.Y);\


//TAX
#define tax(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TAX - ");\
\
\
\
            printf("\n");\
\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.A)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.X = CPU.A;\


//TAY
#define tay(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TAY - ");\
\
\
\
            printf("\n");\
\
\
    if(CPU.A & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.A)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.Y = CPU.A;\


//TSX
#define tsx(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TSX - ");\
\
\
\
            printf("\n");\
\
\
    if(CPU.S & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.S)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.X = CPU.S;\


//TXA
#define txa(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TXA - ");\
\
\
\
            printf("\n");\
\
\
    if(CPU.X & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.X)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.A = CPU.X;\


//TXS
#define txs(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TXS - ");\
\
\
\
            printf("\n");\
\
\
    CPU.S = CPU.X;\


//TYA
#define tya(CONTEXT)\
\
            printf("0x%04X: ", CPU.PC);\
\
\
\
        printf("TYA - ");\
\
\
\
            printf("\n");\
\
\
    if(CPU.Y & 0x80)\
        (CPU.P |= 0x80);\
    else\
        (CPU.P &= 0x7F);\
\
    if(CPU.Y)\
        (CPU.P &= 0xFD);\
    else\
        (CPU.P |= 0x02);\
\
    CPU.A = CPU.Y;\


//JAM
#define jam(CONTEXT)\
    printf("\n\nInvalid Opcode!\n");\
    PrintCPUStatus();\
    debugPrompt(((void *)0));\


