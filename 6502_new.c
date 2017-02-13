#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "6502.h"
#include "NESMemory.h"
#include "6502_Debug.h"

#include "6502_CPUMacros.h"

#include "JRYNES.h"
#include "Statistics.h"

__Context CPU;
extern NESMainMemory NESRAM;

extern EmulationSettings Settings;

extern Statistics* ExecutionStatistics;

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

void TerminateCPU(){
    PrintCPUStatus();
}

void CPUsetNMI(){
    CPU.PendingInterrupts |= INT_NMI;
}

void CPUsetIRQ(){
    CPU.PendingInterrupts |= INT_IRQ;
}

void CPUaddDMACycles(uint16_t Cycles){
    CPU.DMACycles += Cycles;
}

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
    
    /*CPU.Memory[0x0008] = 0xF7;
    CPU.Memory[0x0009] = 0xEF;
    CPU.Memory[0x000A] = 0xDF;
    CPU.Memory[0x000F] = 0xBF;*/
}

/*
uint8_t READ_MEMORY(uint16_t Address){
    uint16_t TempAddr = Address;
    uint8_t ReadTemp;
    if(TempAddr < 0x800)
        ReadTemp = (NESRAM.RAM[TempAddr]);
    else if(TempAddr < 0x2000)
        ReadTemp = (NESRAM.RAM[TempAddr & 0x7FF]);
    else if(TempAddr > 0x8000)
        ReadTemp = (TempAddr < 0xC000 ? NESRAM.PRGROML[TempAddr & 0x3FFF] : NESRAM.PRGROMH[TempAddr & 0x3FFF]);
    else
        ReadTemp = ReadMemory(TempAddr);
    return ReadTemp;
}*/

/*
uint16_t READ_WORD(Address){
    uint16_t val;
    
    val = ( (READ_MEMORY(Address)) | (READ_MEMORY(Address + 1) << 8));
    
    return val;
}
*/



static void HandleNMI(){
    /*Flags*/
    uint8_t FlagN, FlagV, FlagB;
    uint8_t FlagD, FlagI, FlagZ, FlagC;

    /*Local copies of Registers*/
    uint32_t PC;
    uint8_t A, X, Y, S;   

    GET_GLOBAL_REGS();
    NMI_PROC();
    STORE_LOCAL_REGS();
    
   CPU.PendingInterrupts &= (~((uint8_t)INT_NMI));    

}

static void HandleIRQ(){
    
    /*Flags*/
    uint8_t FlagN, FlagV, FlagB;
    uint8_t FlagD, FlagI, FlagZ, FlagC;

    /*Local copies of Registers*/
    uint32_t PC;
    uint8_t A, X, Y, S;   
    
    GET_GLOBAL_REGS();
    if (!FlagI){
        IRQ_PROC();
        CPU.DMACycles += 7;
    }
    else
        CPU.PendingInterrupts |= INT_IRQ;
    STORE_LOCAL_REGS();
    
}

int32_t HandleInterrupts(int32_t Cycles, uint32_t* Interrupt){
    
    *Interrupt = 0;
    
    //Handle Interrupts
    if(CPU.DMACycles){
        if(Cycles <= CPU.DMACycles){
            //burn remaining cycles and end cycle execution
            CPU.DMACycles -= Cycles;
            CPU.ClockTicks += Cycles;
            goto ReturnToNES;
        }
        else{
            Cycles -= CPU.DMACycles;
            CPU.ClockTicks += CPU.DMACycles;
            CPU.DMACycles = 0;
        }                
    }

    if(CPU.PendingInterrupts){
        if(CPU.PendingInterrupts & INT_NMI){
            HandleNMI();
            *Interrupt = 1;
        }

        if(CPU.PendingInterrupts & INT_IRQ){
            if(!STATUS_INTERRUPT){
                HandleIRQ();
                *Interrupt = 1;
            }
        }
        EXTRA_CYCLES(7);
    }     
    
ReturnToNES:
        
    return Cycles;    
}

int32_t INT_Main(int32_t Cycles){
    int32_t RemainingCycles;
    uint32_t Interrupt;

    Cycles = HandleInterrupts(Cycles, &Interrupt);
    
    /*while(Cycles >= 0){
        Cycles += INT_RunCycles(1)-1;
    }*/
    
    INT_RunCycles(Cycles);
    
    RemainingCycles = Cycles;
    
    if(CPU.Jammed){
        printf("\n\nInvalid Opcode!\n");
        PrintCPUStatus();        
        exit(0);
    }
    
    return RemainingCycles;
}

#ifdef INSTRUCTION_TRACING

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

    const InstructionDebugData _DebugData[256] = {
        #include "6502_OpcodeMap.h"
    };
    
#endif

int32_t INT_RunCycles(int32_t Cycles){
 
    uint8_t Temp8;
    uint16_t Temp16;
    uint16_t Address;
    uint8_t  Value;
            
    /*Flags*/
    uint8_t FlagN, FlagV, FlagB;
    uint8_t FlagD, FlagI, FlagZ, FlagC;

    /*Local copies of Registers*/
    uint32_t PC;
    uint8_t A, X, Y, S;    
    
    #define  OPCODE_BEGIN(xx)  op##xx: \
        if(!(Settings.ArgumentFlags & AF_DBT)) \
                ExecutionStatistics->TotalInstructionsExecuted++;

#ifndef INSTRUCTION_TRACING
    
    #define  OPCODE_END \
        if (Cycles <= 0) \
            goto ReturnToNES; \
        goto *opcode_table[READ_MEMORY(PC++)];
   
#else
 
#ifdef DEBUGCONSOLE
    
    #define  OPCODE_END \
        if (Cycles <= 0) \
            goto ReturnToNES; \
        /*printf("0x%04X: %s\n", PC, _DebugData[READ_MEMORY(PC)].InstructionPtr()); \*/\
        STORE_LOCAL_REGS(); \
        CPU.CurrentOpcode = READ_MEMORY(PC); \
        debugCycle(); \
        goto *opcode_table[READ_MEMORY(PC++)];    
    
#else
    
    #define  OPCODE_END \
        if (Cycles <= 0) \
            goto ReturnToNES; \
        /*printf("0x%04X: %s\n", PC, _DebugData[READ_MEMORY(PC)].InstructionPtr()); \*/\
        goto *opcode_table[READ_MEMORY(PC++)];
#endif

#endif       

    
   static void *opcode_table[256] =
   {
      &&op00, &&op01, &&op02, &&op03, &&op04, &&op05, &&op06, &&op07,
      &&op08, &&op09, &&op0A, &&op0B, &&op0C, &&op0D, &&op0E, &&op0F,
      &&op10, &&op11, &&op12, &&op13, &&op14, &&op15, &&op16, &&op17,
      &&op18, &&op19, &&op1A, &&op1B, &&op1C, &&op1D, &&op1E, &&op1F,
      &&op20, &&op21, &&op22, &&op23, &&op24, &&op25, &&op26, &&op27,
      &&op28, &&op29, &&op2A, &&op2B, &&op2C, &&op2D, &&op2E, &&op2F,
      &&op30, &&op31, &&op32, &&op33, &&op34, &&op35, &&op36, &&op37,
      &&op38, &&op39, &&op3A, &&op3B, &&op3C, &&op3D, &&op3E, &&op3F,
      &&op40, &&op41, &&op42, &&op43, &&op44, &&op45, &&op46, &&op47,
      &&op48, &&op49, &&op4A, &&op4B, &&op4C, &&op4D, &&op4E, &&op4F,
      &&op50, &&op51, &&op52, &&op53, &&op54, &&op55, &&op56, &&op57,
      &&op58, &&op59, &&op5A, &&op5B, &&op5C, &&op5D, &&op5E, &&op5F,
      &&op60, &&op61, &&op62, &&op63, &&op64, &&op65, &&op66, &&op67,
      &&op68, &&op69, &&op6A, &&op6B, &&op6C, &&op6D, &&op6E, &&op6F,
      &&op70, &&op71, &&op72, &&op73, &&op74, &&op75, &&op76, &&op77,
      &&op78, &&op79, &&op7A, &&op7B, &&op7C, &&op7D, &&op7E, &&op7F,
      &&op80, &&op81, &&op82, &&op83, &&op84, &&op85, &&op86, &&op87,
      &&op88, &&op89, &&op8A, &&op8B, &&op8C, &&op8D, &&op8E, &&op8F,
      &&op90, &&op91, &&op92, &&op93, &&op94, &&op95, &&op96, &&op97,
      &&op98, &&op99, &&op9A, &&op9B, &&op9C, &&op9D, &&op9E, &&op9F,
      &&opA0, &&opA1, &&opA2, &&opA3, &&opA4, &&opA5, &&opA6, &&opA7,
      &&opA8, &&opA9, &&opAA, &&opAB, &&opAC, &&opAD, &&opAE, &&opAF,
      &&opB0, &&opB1, &&opB2, &&opB3, &&opB4, &&opB5, &&opB6, &&opB7,
      &&opB8, &&opB9, &&opBA, &&opBB, &&opBC, &&opBD, &&opBE, &&opBF,
      &&opC0, &&opC1, &&opC2, &&opC3, &&opC4, &&opC5, &&opC6, &&opC7,
      &&opC8, &&opC9, &&opCA, &&opCB, &&opCC, &&opCD, &&opCE, &&opCF,
      &&opD0, &&opD1, &&opD2, &&opD3, &&opD4, &&opD5, &&opD6, &&opD7,
      &&opD8, &&opD9, &&opDA, &&opDB, &&opDC, &&opDD, &&opDE, &&opDF,
      &&opE0, &&opE1, &&opE2, &&opE3, &&opE4, &&opE5, &&opE6, &&opE7,
      &&opE8, &&opE9, &&opEA, &&opEB, &&opEC, &&opED, &&opEE, &&opEF,
      &&opF0, &&opF1, &&opF2, &&opF3, &&opF4, &&opF5, &&opF6, &&opF7,
      &&opF8, &&opF9, &&opFA, &&opFB, &&opFC, &&opFD, &&opFE, &&opFF
   };
   
    //Get all registers from the CPU struct
    GET_GLOBAL_REGS();      
   
    //Fetch First Instruction
    OPCODE_END
    
    /*----------- JUMP TABLE -------------*/
            
    OPCODE_BEGIN(00)  /* BRK */
        BRK();
        EXTRA_CYCLES(7)
    OPCODE_END

    OPCODE_BEGIN(01)  /* ORA ($nn,X) */
        ORA(AM_INDIR_X);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(02)  /* JAM */
    OPCODE_BEGIN(12)  /* JAM */
    OPCODE_BEGIN(22)  /* JAM */
    OPCODE_BEGIN(32)  /* JAM */
    OPCODE_BEGIN(42)  /* JAM */
    OPCODE_BEGIN(52)  /* JAM */
    OPCODE_BEGIN(62)  /* JAM */
    OPCODE_BEGIN(72)  /* JAM */
    OPCODE_BEGIN(92)  /* JAM */
    OPCODE_BEGIN(B2)  /* JAM */
    OPCODE_BEGIN(D2)  /* JAM */
    OPCODE_BEGIN(F2)  /* JAM */
        JAM();
        /* kill the CPU */
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(03)  /* SLO ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(04)  /* NOP $nn */
    OPCODE_BEGIN(44)  /* NOP $nn */
    OPCODE_BEGIN(64)  /* NOP $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(05)  /* ORA $nn */
        ORA(AM_ZP); 
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(06)  /* ASL $nn */
        ASL(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(07)  /* SLO $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(08)  /* PHP */
        PHP(); 
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(09)  /* ORA #$nn */
        ORA(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(0A)  /* ASL A */
        ASL_A();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(0B)  /* ANC #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(0C)  /* NOP $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(0D)  /* ORA $nnnn */
        ORA(AM_ABS);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(0E)  /* ASL $nnnn */
        ASL(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(0F)  /* SLO $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END            
    
    OPCODE_BEGIN(10)  /* BPL $nn */
        BPL();
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(11)  /* ORA ($nn),Y */
        ORA(AM_INDIR_Y);
        EXTRA_CYCLES(5)
    OPCODE_END
      
    OPCODE_BEGIN(13)  /* SLO ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    /*Na realidade, DOPs(4)*/
    OPCODE_BEGIN(14)  /* NOP $nn,X */
    OPCODE_BEGIN(34)  /* NOP */
    OPCODE_BEGIN(54)  /* NOP $nn,X */
    OPCODE_BEGIN(74)  /* NOP $nn,X */
    OPCODE_BEGIN(D4)  /* NOP $nn,X */
    OPCODE_BEGIN(F4)  /* NOP ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(15)  /* ORA $nn,X */
        ORA(AM_ZP_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(16)  /* ASL $nn,X */
        ASL(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(17)  /* SLO $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(18)  /* CLC */
        CLC();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(19)  /* ORA $nnnn,Y */
        ORA(AM_ABS_INDY)
        EXTRA_CYCLES(4)
    OPCODE_END
      
    OPCODE_BEGIN(1A)  /* NOP */
    OPCODE_BEGIN(3A)  /* NOP */
    OPCODE_BEGIN(5A)  /* NOP */
    OPCODE_BEGIN(7A)  /* NOP */
    OPCODE_BEGIN(DA)  /* NOP */
    OPCODE_BEGIN(FA)  /* NOP */
        NOP();
        EXTRA_CYCLES(2)
    OPCODE_END
                
    OPCODE_BEGIN(1B)  /* SLO $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    /* Actually TOP(3)*/
    OPCODE_BEGIN(1C)  /* NOP $nnnn,X */
    OPCODE_BEGIN(3C)  /* NOP $nnnn,X */
    OPCODE_BEGIN(5C)  /* NOP $nnnn,X */
    OPCODE_BEGIN(7C)  /* NOP $nnnn,X */
    OPCODE_BEGIN(DC)  /* NOP $nnnn,X */
    OPCODE_BEGIN(FC)  /* NOP $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(1D)  /* ORA $nnnn,X */
        ORA(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(1E)  /* ASL $nnnn,X */
        ASL(AM_ABS_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(7)
    OPCODE_END

    OPCODE_BEGIN(1F)  /* SLO $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END
      
    OPCODE_BEGIN(20)  /* JSR $nnnn */
        JSR();
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(21)  /* AND ($nn,X) */
        AND(AM_INDIR_X);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(23)  /* RLA ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(24)  /* BIT $nn */
        BIT(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(25)  /* AND $nn */
        AND(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(26)  /* ROL $nn */
        ROL(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(27)  /* RLA $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(28)  /* PLP */
        PLP();
        EXTRA_CYCLES(4);
    OPCODE_END                
         
    OPCODE_BEGIN(29)  /* AND #$nn */
        AND(AM_IMMD);
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(2A)  /* ROL A */
        ROL_A();
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(2B)  /* ANC #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(2C)  /* BIT $nnnn */
        BIT(AM_ABS);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(2D)  /* AND $nnnn */
        AND(AM_ABS);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(2E)  /* ROL $nnnn */
        ROL(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(2F)  /* RLA $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(30)  /* BMI $nn */
        BMI();
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(31)  /* AND ($nn),Y */
        AND(AM_INDIR_Y);
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(33)  /* RLA ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(35)  /* AND $nn,X */
        AND(AM_ZP_INDX);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(36)  /* ROL $nn,X */
        ROL(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(37)  /* RLA $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(38)  /* SEC */
        SEC();
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(39)  /* AND $nnnn,Y */
        AND(AM_ABS_INDY);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(3B)  /* RLA $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(3D)  /* AND $nnnn,X */
        AND(AM_ABS_INDX);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(3E)  /* ROL $nnnn,X */
        ROL(AM_ABS_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(7);
    OPCODE_END                
         
    OPCODE_BEGIN(3F)  /* RLA $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(40)  /* RTI */
        EXTRA_CYCLES(6);    
        RTI();
    OPCODE_END

    OPCODE_BEGIN(41)  /* EOR ($nn,X) */
        EOR(AM_INDIR_X);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(43)  /* SRE ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(45)  /* EOR $nn */
        EOR(AM_ZP);
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(46)  /* LSR $nn */
        LSR(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(47)  /* SRE $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(48)  /* PHA */
        PHA();
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(49)  /* EOR #$nn */
        EOR(AM_IMMD);
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(4A)  /* LSR A */
        LSR_A();
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(4B)  /* ASR #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(4C)  /* JMP $nnnn */
        JMP_ABSOLUTE();
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(4D)  /* EOR $nnnn */
        EOR(AM_ABS);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(4E)  /* LSR $nnnn */
        LSR(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(4F)  /* SRE $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(50)  /* BVC $nn */
        BVC();
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(51)  /* EOR ($nn),Y */
        EOR(AM_INDIR_Y);
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(53)  /* SRE ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(55)  /* EOR $nn,X */
        EOR(AM_ZP_INDX);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(56)  /* LSR $nn,X */
        LSR(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(57)  /* SRE $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(58)  /* CLI */
        EXTRA_CYCLES(2);
        CLI();
    OPCODE_END

    OPCODE_BEGIN(59)  /* EOR $nnnn,Y */
        EOR(AM_ABS_INDY);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(5B)  /* SRE $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(5D)  /* EOR $nnnn,X */
        EOR(AM_ABS_INDX);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(5E)  /* LSR $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(5F)  /* SRE $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(60)  /* RTS */
        RTS();
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(61)  /* ADC ($nn,X) */
        ADC(AM_INDIR_X);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(63)  /* RRA ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(65)  /* ADC $nn */
        ADC(AM_ZP);
        EXTRA_CYCLES(3);
    OPCODE_END                

    OPCODE_BEGIN(66)  /* ROR $nn */
        ROR(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(67)  /* RRA $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(68)  /* PLA */
        PLA();
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(69)  /* ADC #$nn */
        ADC(AM_IMMD);
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(6A)  /* ROR A */
        ROR_A();
        EXTRA_CYCLES(2);
    OPCODE_END

    OPCODE_BEGIN(6B)  /* ARR #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(6C)  /* JMP ($nnnn) */
        JMP_INDIRECT();
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(6D)  /* ADC $nnnn */
        ADC(AM_ABS);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(6E)  /* ROR $nnnn */
        ROR(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(6F)  /* RRA $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(70)  /* BVS $nn */
        BVS();
        EXTRA_CYCLES(3);
    OPCODE_END

    OPCODE_BEGIN(71)  /* ADC ($nn),Y */
        ADC(AM_INDIR_Y);
        EXTRA_CYCLES(5);
    OPCODE_END

    OPCODE_BEGIN(73)  /* RRA ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(75)  /* ADC $nn,X */
        ADC(AM_ZP_INDX);
        EXTRA_CYCLES(4);
    OPCODE_END

    OPCODE_BEGIN(76)  /* ROR $nn,X */
        ROR(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6);
    OPCODE_END

    OPCODE_BEGIN(77)  /* RRA $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(78)  /* SEI */
        SEI();
        EXTRA_CYCLES(2);
    OPCODE_END                
                
    OPCODE_BEGIN(79)  /* ADC $nnnn,Y */
        ADC(AM_ABS_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(7B)  /* RRA $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(7D)  /* ADC $nnnn,X */
        ADC(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(7E)  /* ROR $nnnn,X */
        ROR(AM_ABS_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(7)
    OPCODE_END

    OPCODE_BEGIN(7F)  /* RRA $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    /*Actually DOP(2)*/
    OPCODE_BEGIN(80)  /* NOP #$nn */
    OPCODE_BEGIN(82)  /* NOP #$nn */
    OPCODE_BEGIN(89)  /* NOP #$nn */
    OPCODE_BEGIN(C2)  /* NOP #$nn */
    OPCODE_BEGIN(E2)  /* NOP #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(81)  /* STA ($nn,X) */
        STA(AM_INDIR_X_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(83)  /* SAX ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(84)  /* STY $nn */
        STY(AM_ZP_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(85)  /* STA $nn */
        STA(AM_ZP_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(86)  /* STX $nn */
        STX(AM_ZP_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(87)  /* SAX $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(88)  /* DEY */
        DEY();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(8A)  /* TXA */
        TXA();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(8B)  /* ANE #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(8C)  /* STY $nnnn */
        STY(AM_ABS_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(8D)  /* STA $nnnn */
        STA(AM_ABS_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(8E)  /* STX $nnnn */
        STX(AM_ABS_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(8F)  /* SAX $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(90)  /* BCC $nn */
        BCC();
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(91)  /* STA ($nn),Y */
        STA(AM_INDIR_Y_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(93)  /* SHA ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(94)  /* STY $nn,X */
        STY(AM_ZP_INDX_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(95)  /* STA $nn,X */
        STA(AM_ZP_INDX_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(96)  /* STX $nn,Y */
        STX(AM_ZP_INDY_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(97)  /* SAX $nn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(98)  /* TYA */
        TYA();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(99)  /* STA $nnnn,Y */
        STA(AM_ABS_INDY_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END                
       
    OPCODE_BEGIN(9A)  /* TXS */
        TXS();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(9B)  /* SHS $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(9C)  /* SHY $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(9D)  /* STA $nnnn,X */
        STA(AM_ABS_INDX_ADDR, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(9E)  /* SHX $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(9F)  /* SHA $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(A0)  /* LDY #$nn */
        LDY(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(A1)  /* LDA ($nn,X) */
        LDA(AM_INDIR_X);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(A2)  /* LDX #$nn */
        LDX(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(A3)  /* LAX ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(A4)  /* LDY $nn */
        LDY(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(A5)  /* LDA $nn */
        LDA(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(A6)  /* LDX $nn */
        LDX(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(A7)  /* LAX $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(A8)  /* TAY */
        TAY();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(A9)  /* LDA #$nn */
        LDA(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(AA)  /* TAX */
        TAX();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(AB)  /* LXA #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(AC)  /* LDY $nnnn */
        LDY(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(AD)  /* LDA $nnnn */
        LDA(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(AE)  /* LDX $nnnn */
        LDX(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(AF)  /* LAX $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END                
                
    OPCODE_BEGIN(B0)  /* BCS $nn */
        BCS();
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(B1)  /* LDA ($nn),Y */
        LDA(AM_INDIR_Y);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(B3)  /* LAX ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(B4)  /* LDY $nn,X */
        LDY(AM_ZP_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(B5)  /* LDA $nn,X */
        LDA(AM_ZP_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(B6)  /* LDX $nn,Y */
        LDX(AM_ZP_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(B7)  /* LAX $nn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(B8)  /* CLV */
        CLV();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(B9)  /* LDA $nnnn,Y */
        LDA(AM_ABS_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(BA)  /* TSX */
        TSX();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(BB)  /* LAS $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(BC)  /* LDY $nnnn,X */
        LDY(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(BD)  /* LDA $nnnn,X */
        LDA(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(BE)  /* LDX $nnnn,Y */
        LDX(AM_ABS_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(BF)  /* LAX $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(C0)  /* CPY #$nn */
        CPY(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(C1)  /* CMP ($nn,X) */
        CMP(AM_INDIR_X);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(C3)  /* DCP ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(C4)  /* CPY $nn */
        CPY(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(C5)  /* CMP $nn */
        CMP(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(C6)  /* DEC $nn */
        DEC(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(C7)  /* DCP $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(C8)  /* INY */
        INY();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(C9)  /* CMP #$nn */
        CMP(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(CA)  /* DEX */
        DEX();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(CB)  /* SBX #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(CC)  /* CPY $nnnn */
        CPY(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(CD)  /* CMP $nnnn */
        CMP(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(CE)  /* DEC $nnnn */
        DEC(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(CF)  /* DCP $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(D0)  /* BNE $nn */
        BNE();
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(D1)  /* CMP ($nn),Y */
        CMP(AM_INDIR_Y);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(D3)  /* DCP ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(D5)  /* CMP $nn,X */
        CMP(AM_ZP_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(D6)  /* DEC $nn,X */
        DEC(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(D7)  /* DCP $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(D8)  /* CLD */
        CLD();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(D9)  /* CMP $nnnn,Y */
        CMP(AM_ABS_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(DB)  /* DCP $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END                  

    OPCODE_BEGIN(DD)  /* CMP $nnnn,X */
        CMP(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(DE)  /* DEC $nnnn,X */
        DEC(AM_ABS_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(7)
    OPCODE_END

    OPCODE_BEGIN(DF)  /* DCP $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(E0)  /* CPX #$nn */
        CPX(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(E1)  /* SBC ($nn,X) */
        SBC(AM_INDIR_X);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(E3)  /* ISB ($nn,X) */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(E4)  /* CPX $nn */
        CPX(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(E5)  /* SBC $nn */
        SBC(AM_ZP);
        EXTRA_CYCLES(3)
    OPCODE_END

    OPCODE_BEGIN(E6)  /* INC $nn */
        INC(AM_ZP, WRITE_MEMORY);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(E7)  /* ISB $nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(E8)  /* INX */
        INX();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(EB)  /* USBC #$nn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(E9)  /* SBC #$nn */
        SBC(AM_IMMD);
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(EA)  /* NOP */
        NOP();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(EC)  /* CPX $nnnn */
        CPX(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(ED)  /* SBC $nnnn */
        SBC(AM_ABS);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(EE)  /* INC $nnnn */
        INC(AM_ABS, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(EF)  /* ISB $nnnn */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(F0)  /* BEQ $nn */
        BEQ();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(F1)  /* SBC ($nn),Y */
        SBC(AM_INDIR_Y);
        EXTRA_CYCLES(5)
    OPCODE_END

    OPCODE_BEGIN(F3)  /* ISB ($nn),Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(F5)  /* SBC $nn,X */
        SBC(AM_ZP_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(F6)  /* INC $nn,X */
        INC(AM_ZP_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(6)
    OPCODE_END

    OPCODE_BEGIN(F7)  /* ISB $nn,X */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(F8)  /* SED */
        SED();
        EXTRA_CYCLES(2)
    OPCODE_END

    OPCODE_BEGIN(F9)  /* SBC $nnnn,Y */
        SBC(AM_ABS_INDY);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(FB)  /* ISB $nnnn,Y */
        JAM();
        Cycles = 0;
    OPCODE_END

    OPCODE_BEGIN(FD)  /* SBC $nnnn,X */
        SBC(AM_ABS_INDX);
        EXTRA_CYCLES(4)
    OPCODE_END

    OPCODE_BEGIN(FE)  /* INC $nnnn,X */
        INC(AM_ABS_INDX, WRITE_MEMORY);
        EXTRA_CYCLES(7)
    OPCODE_END

    OPCODE_BEGIN(FF)  /* ISB $nnnn,X */
        JAM();
        Cycles = 0;
    OPCODE_END         
                
ReturnToNES:
    
    //Store all registers back into the CPU struct
    STORE_LOCAL_REGS();
 
    return Cycles;
    
}
