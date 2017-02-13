#include "6502_Debug.h"
#include "6502_Dis.h"
#include "NESMemory.h"
#include "PPUMemory.h"
#include "6502.h"
#include "NES.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

__Debug_Context Debug;

__InstructionRecord InstrRecord;
        
extern __Context CPU;

extern const InstructionData Instructions[256];

static void AddDebugHook(DebugHook* Hook){
    
    Hook->HookID = Debug.NextHookID;
    Debug.NextHookID++;
    
    //If there's already an element in the list, make the last one point
    //to the new one
    if(Debug.DebugHookListEnd)
        Debug.DebugHookListEnd->Next = (struct DebugHook*) Hook;
    
    Debug.DebugHookListEnd = Hook;
    
    //If debug hook list empty, this is the first element
    if(!Debug.DebugHookList)
        Debug.DebugHookList = Hook;      
}

static void AddDebugPCBreakHook(uint16_t BreakAddr){
    DebugHook* New;
    
    New = malloc(sizeof(DebugHook));
    New->HookType = DHT_PCBREAK;
    New->Data.PCBreak.BreakAddr = BreakAddr;
    New->Next = NULL;
    AddDebugHook(New);
}

static void AddDebugMemWatchHook(uint16_t MemAddr, DACC_ACTION Action){
    DebugHook* New;
    
    New = malloc(sizeof(DebugHook));
    New->HookType = DHT_MEMWATCH;
    New->Data.MemWatchHook.Addr = MemAddr;
    New->Data.MemWatchHook.ActionFlags = Action;
    New->Next = NULL;  
    AddDebugHook(New);
}

static void AddDebugRegWatchHook(DREG_TYPE Register, DACC_ACTION Action){
    DebugHook* New;
    
    New = malloc(sizeof(DebugHook));
    New->HookType = DHT_MEMWATCH;
    New->Data.RegWatchHook.Register = Register;
    New->Data.RegWatchHook.ActionFlags = Action;
    New->Next = NULL;  
    AddDebugHook(New);    
}

static uint32_t RemoveDebugHook(uint32_t HookID){
    DebugHook *p, *ant;
    uint32_t removed = 0;
    ant = NULL;
    
    for(p = (DebugHook*)Debug.DebugHookList; p != NULL; p = (DebugHook*)(p->Next)){
        if(p->HookID == HookID){
            if(p == Debug.DebugHookList)
                Debug.DebugHookList = (DebugHook*)p->Next;
            else
                ant->Next = p->Next;
            
            if(!p->Next)
                Debug.DebugHookListEnd = ant; 
            
            free(p);
            removed = 1;
            break;
        }
        ant = p;
    }
    
    return removed;
    
}

void DBGRecordInstruction(uint8_t Opcode, uint8_t* Operands, uint16_t Address){
    InstrRecord.History[InstrRecord.NextPos].Opcode = Opcode;
    memcpy(InstrRecord.History[InstrRecord.NextPos].Operands, Operands, 2);
    InstrRecord.History[InstrRecord.NextPos].Address = Address;
    InstrRecord.CurrentInstr = InstrRecord.NextPos;
    InstrRecord.NextPos++;
    
    if(InstrRecord.NextPos == DBG_INSTR_HISTORY_SIZE)
        InstrRecord.NextPos = 0;
    
    if(InstrRecord.NextPos == InstrRecord.StartPos)
        InstrRecord.StartPos++;
    
    if(InstrRecord.StartPos == DBG_INSTR_HISTORY_SIZE)
        InstrRecord.StartPos = 0;
}

static void initInstructionRecord(){
    InstrRecord.NextPos = 0;
    InstrRecord.StartPos = 0;
    InstrRecord.CurrentInstr = 0;
    memset(InstrRecord.History, 0, sizeof(__InstructionHistoryData) * DBG_INSTR_HISTORY_SIZE);
}

void initDebug(){
    Debug.DebugHookList = NULL;
    Debug.DebugHookListEnd = NULL;
    Debug.OperationFlags = 0;
    Debug.NextHookID = 1;
    initInstructionRecord();
} 

static void PrintPCBreakHookBreak(DebugHook *Hook){
    fprintf(stderr, "\n");
    fprintf(stderr, "\tBREAK: execution stopped on PC = 0x%04X", Hook->Data.PCBreak.BreakAddr);
}

static void PrintMemWatchHookBreak(DebugHook *Hook){
    const char* Action;
    fprintf(stderr, "\n");
    fprintf(stderr, "\nBREAK: execution stopped on ");
    if(Hook->Data.MemWatchHook.BrokeOn & DA_READ)
        Action = "READ of";
    else
        Action = "WRITE to";
    fprintf(stderr, "%s 0x%04X", Action, Hook->Data.MemWatchHook.Addr);
    
    if(Hook->Data.MemWatchHook.BrokeOn & DA_WRITE)
        fprintf(stderr, " (0x%02X)", Hook->Data.MemWatchHook.BreakValue);
    
    Hook->Data.MemWatchHook.BrokeOn = 0;
}

void debugPrompt(DebugHook* SourceHook){
    
    char Command[256];
    char* CommandPtr;
    uint8_t quit = 0;
    
    if(SourceHook){
        switch (SourceHook->HookType){
            case DHT_PCBREAK:
                PrintPCBreakHookBreak(SourceHook);
            break;
            
            case DHT_MEMWATCH:
                PrintMemWatchHookBreak(SourceHook);
            break;
        }
    }
    
    while(!quit){
        fprintf(stderr, "\n\n");
        fprintf(stderr, "\tEnter a debug command. (? for help)\n\n");
        fprintf(stderr, "\t> ");
        CommandPtr = fgets(Command, 256, stdin);

        switch(Command[0]){
            case '?':
                fprintf(stderr, "\n");
                fprintf(stderr, "\tAvailable debug commands:\n\n");
                fprintf(stderr, "\td                - disassemble\n");
                fprintf(stderr, "\ti                - dump CPU information\n");
                fprintf(stderr, "\ts                - step into\n");
                //fprintf(stderr, "\tn                - step over (next)\n");
                fprintf(stderr, "\tc                - continue\n");
                fprintf(stderr, "\tb addr           - Break on PC = addr\n");
                fprintf(stderr, "\twm addr [action] - Break on memory address access (1 => read, 2 => write)\n");
                fprintf(stderr, "\tr hook_id        - Remove a break hook\n");
                fprintf(stderr, "\tq                - Terminate emulation\n");
                fprintf(stderr, "\tp STR [END]      - Show CPU memory contents starting from STR to END\n");
                fprintf(stderr, "\tpp STR [END]     - Show PPU memory contents starting from STR to END\n");
                //fprintf(stderr, "\twr reg [action]  - Breaks on register access\n");
                fprintf(stderr, "\n");
                fprintf(stderr, "\tAll memory addresses should be in Hexadecimal format 0xADDR\n");
            break;

            case 'p':
            {
                //Show memory
                uint8_t (*MemoryRead)(uint16_t);
                uint16_t i, ini, end, cont = 0;
                char* token;
                const char* MemoryArea;
                
                //Check for PPU memory
                if(Command[1] == 'p'){
                    MemoryRead = ppuMemoryRead;
                    MemoryArea = "PPU";
                }
                else{
                    MemoryRead = ReadMemory;
                    MemoryArea = "CPU";
                }
                
                token = strtok(Command, " ");
                token = strtok(NULL, " ");
                
                if(token)
                    ini = (uint16_t) strtol(token, NULL, 16);                
                
                token = strtok(NULL, " ");
                if(token)
                    end = (uint16_t) strtol(token, NULL, 16);                  
                else
                    end = ini;
                
                fprintf(stderr, "\n\t%s Memory data in address range 0x%04X - 0x%04X\n\n", MemoryArea, ini, end);
                
                fprintf(stderr, "\t<0x%04X>: ", ini);
                for(i = ini; i <= end; i++){
                    fprintf(stderr, "%02X ", MemoryRead(i));
                    cont++;
                    if(!(cont % 16)){
                        if(cont+1 <= end){
                            fprintf(stderr, "\n");
                            fprintf(stderr, "\t<0x%04X>: ", i+1);
                        }
                    }
                }
                
            }
            break;
            
            case 'q':
                fprintf(stderr, "Terminating emulation...\n\n");
                TerminateEmulation();
            break;
            
            case 'r':
            {    
                char* token;
                uint16_t targetID = 0;
                
                token = strtok(Command, " ");
                token = strtok(NULL, " ");
                
                if(token)
                    targetID = (uint16_t) strtol(token, NULL, 10);
                
                fprintf(stderr, "\n");                
                
                if(!targetID){
                    fprintf(stderr, "\tSyntax error. Could not remove hook.");
                }
                else{
                    if(RemoveDebugHook(targetID)){
                        fprintf(stderr, "\tHook %d removed successfully!", targetID);
                    }
                    else{
                        fprintf(stderr, "\tHook %d not found!", targetID);
                    }
                }
            }
            break;
            
            case 'd':
            {
                //print disassembly
                const char* Instruction;
                
                if(InstrRecord.NextPos != InstrRecord.StartPos){
                    uint32_t i;
                
                    fprintf(stderr, "\tDisassembly of execution region...\n\n");
                    
                    //Print instruction history
                   
                    i = InstrRecord.StartPos;
                    
                    while(i != InstrRecord.CurrentInstr){

                        Instruction = Disassemble(
                                InstrRecord.History[i].Opcode,
                                InstrRecord.History[i].Operands,
                                InstrRecord.History[i].Address);

                        fprintf(stderr, "\t%s\n", Instruction);
                        
                        i++;
                        
                        if(i == DBG_INSTR_HISTORY_SIZE)
                            i = 0;
                    }
                    
                    //Print current instruction
                    Instruction = 
                            Disassemble(InstrRecord.History[InstrRecord.CurrentInstr].Opcode,
                            InstrRecord.History[InstrRecord.CurrentInstr].Operands,
                            InstrRecord.History[InstrRecord.CurrentInstr].Address);

                    fprintf(stderr, "\n     --> %s\n\n", Instruction);
                    
                    //Print the next 5 instructions
                    {
                        uint8_t Operands[3] = {0,0,0};
                        uint8_t j = 0;
                        uint8_t Opcode;
                        uint16_t Size;
                        
                        uint16_t Address;
                        
                        if(Instructions[CPU.CurrentOpcode].Size){
                            Size = Instructions[CPU.CurrentOpcode].Size;
                        }
                        else{
                            if(Instructions[CPU.CurrentOpcode].AddressingMode == AM_ABSOLUTE)
                                Size = 3;
                            else if(Instructions[CPU.CurrentOpcode].AddressingMode == AM_IMPLIED)
                                Size = 1;  
                            else if(Instructions[CPU.CurrentOpcode].AddressingMode == AM_INDIRECT)
                                Size = 3;
                        }
                        
                        Address = CPU.PC + Size;
                        
                        for(i = 0; i < 30; i++){
                            Opcode = ReadMemory(Address);
                            
                            if(Instructions[Opcode].Size){
                                Size = Instructions[Opcode].Size;
                            }
                            else{
                                if(Instructions[Opcode].AddressingMode == AM_ABSOLUTE)
                                    Size = 3;
                                else if(Instructions[Opcode].AddressingMode == AM_IMPLIED)
                                    Size = 1;                            
                            }                            
                            
                            for(j = 0; j < Size - 1; j++){
                                Operands[j] = ReadMemory(Address + j + 1);
                            }
                            
                            Instruction = Disassemble(Opcode, Operands, Address);

                            fprintf(stderr, "\t%s\n", Instruction);
                            
                            if(Instructions[Opcode].AddressingMode == AM_RELATIVE)
                                Address += 2;
                            else if(Instructions[Opcode].AddressingMode == AM_IMPLIED)
                                Address ++;
                            else
                                Address += Size;
                            
                            Operands[0] = Operands[1] = Operands[2] = 0;
                            
                        }  
                    }
                    
                }
            }
            break;
            
            case 'i':
                fprintf(stderr, "\n");
                PrintCPUStatus();
            break;
            
            case 's':
                quit = 1;
                Debug.OperationFlags |= DBGOP_STEP;
            break;    
            
            case 'n':
                quit = 1;
                Debug.OperationFlags |= DBGOP_STEPOVER;
            break;                
            
            case 'c':
                quit = 1;
                Debug.OperationFlags &= ~((uint32_t)DBGOP_STEP);
                Debug.OperationFlags &= ~((uint32_t)DBGOP_STEPOVER);
                fprintf(stderr, "\n");
                fprintf(stderr, "\tResuming execution...\n");
            break;
            
            case 'w':
            {
                if(Command[1] == 'm'){   
                    char* token;
                    uint16_t targetAddr = 0, access = 0;

                    token = strtok(Command, " ");
                    token = strtok(NULL, " ");
                    
                    if(token)
                        targetAddr = (uint16_t) strtol(token, NULL, 16);                        

                    token = strtok(NULL, " ");
                    if(token)
                        access = (uint16_t) strtol(token, NULL, 10);

                    fprintf(stderr, "\n");

                    if(!targetAddr){
                        fprintf(stderr, "\tSyntax error. Could not set watch point.");
                    }
                    else{
                        
                        if(!access)
                            access = DA_READ | DA_WRITE;
                        
                        AddDebugMemWatchHook(targetAddr, access);
                        fprintf(stderr, "\t(%d) Watch point set on address 0x%04X", Debug.DebugHookListEnd->HookID, targetAddr);
                        if(access){
                            fprintf(stderr, " for");
                            fprintf(stderr, " %s", (access & DA_READ) ? "READ" : "_");
                            fprintf(stderr, " %s", (access & DA_WRITE) ? "WRITE" : "_");
                        }
                        else
                            fprintf(stderr, ".");
                        
                    }                        
                }
            }
            break;
            
            case 'b':
            {
                char* token;
                uint16_t targetAddr = 0;
                
                token = strtok(Command, " ");
                
                if(token)
                        token = strtok(NULL, " ");
                
                targetAddr = (uint16_t) strtol(token, NULL, 16);
                
                fprintf(stderr, "\n");
                
                if(!targetAddr){
                    fprintf(stderr, "\tSyntax error. Could not set breakpoint.");
                }
                else{
                    AddDebugPCBreakHook(targetAddr);
                    fprintf(stderr, "\t(%d) Breakpoint set on address 0x%04X.", Debug.DebugHookListEnd->HookID, targetAddr);
                }
            }
            break;
            
            default:
                fprintf(stderr, "\tUnrecognized command.");
        }
    }
    
}

void debugCycle(void){
        uint8_t Operands[3] = {0,0,0};
        uint16_t i, j, size;
        
        if(Instructions[CPU.CurrentOpcode].Size){
            size = Instructions[CPU.CurrentOpcode].Size;
        }
        else
            size = CPUGetInstructionSize(CPU.CurrentOpcode);
        
        size--;
        
        for(j = 0; j <  size; j++){
            Operands[j] = ReadMemory(CPU.PC + j + 1);
        }        
        
        //Record next instruction as current instruction
        DBGRecordInstruction(CPU.CurrentOpcode, Operands ,CPU.PC);
               
        //Check if step flag is set
        if(Debug.OperationFlags & DBGOP_STEP){
            debugPrompt(NULL);
        }        
        
        //Check for hooks before executing the next instruction
        
        DebugHook *p;
        for(p = (DebugHook*) Debug.DebugHookList; p != NULL; p = (DebugHook*) (p->Next)){
            if(p->HookType == DHT_PCBREAK){
                if (CPU.PC == p->Data.PCBreak.BreakAddr){
                    debugPrompt(p);
                    break;
                }
            }
        }
    }    