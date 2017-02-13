#ifndef NES6502_DEBUG_H
#define	NES6502_DEBUG_H

#include <stdint.h>

#define DBG_INSTR_HISTORY_SIZE  20

typedef enum {
    DHT_PCBREAK,
    DHT_MEMWATCH,
    DHT_REGWATCH
} DBG_HOOK_TYPE;

typedef enum{
    DREG_A,
    DREG_Y,
    DREG_X,
    DREG_S,
    DREG_P,
    DREG_PC
} DREG_TYPE;

typedef enum{
    DA_READ     = 0x01,
    DA_WRITE    = 0x02
} DACC_ACTION;

typedef struct {
    DBG_HOOK_TYPE HookType;
    uint32_t HookID;
    union{
        struct PCBreak{
            uint16_t BreakAddr;
        } PCBreak;
        struct MemWatchHook{
            uint16_t Addr;
            DACC_ACTION ActionFlags; //Action determines when to break.
            DACC_ACTION BrokeOn;
            uint8_t BreakValue;
        } MemWatchHook;
        struct RegWatchHook{
            DREG_TYPE Register;
            DACC_ACTION ActionFlags; //Action determines when to break. 
        } RegWatchHook;        
    } Data;
    
    struct DebugHook *Next;
} DebugHook;

#define DBGOP_STEP      0x01
#define DBGOP_STEPOVER  0x02

typedef struct {
    DebugHook *DebugHookList;
    DebugHook *DebugHookListEnd;
    uint32_t OperationFlags;
    uint32_t NextHookID;
} __Debug_Context;

//This structure is used to record the most recent
//instructions executed

typedef struct History{
    uint16_t Address;
    uint8_t Opcode;
    uint8_t Operands[3];
} __InstructionHistoryData;

typedef struct {
    __InstructionHistoryData History[DBG_INSTR_HISTORY_SIZE];
    
    uint32_t NextPos;
    uint32_t StartPos;
    uint32_t CurrentInstr;
} __InstructionRecord;

void debugPrompt(DebugHook *SourceHook);
void debugCycle(void);
void initDebug();

void DBGRecordInstruction(uint8_t Opcode, uint8_t* Operands, uint16_t Address);

#endif	/* NES6502_DEBUG_H */

