#ifndef STATISTICS_H
#define	STATISTICS_H

#include "List.h"

#include <stdint.h>
#include <pthread.h>

typedef struct STATCycles{
    double Time;
    uint64_t CyclesElapsed;
} STATCycles;

typedef struct EpochInfo{
    uint64_t Epoch;
    uint64_t HybridTransitions;
    uint64_t BBTransitions;
    uint64_t CFGTransitions;
    uint64_t TotalInstructions;
    uint64_t CompiledInstructions;
    uint64_t UTFuses;
    uint64_t CFGFuses;
    uint64_t CompiledCalls;
    uint64_t Fragments;
    __LinkedList* BasicBlocks;
    
    uint64_t AnalysisTime;
    uint64_t DispatchingTime;
} EpochInfo;

typedef struct Statistics{
    uint32_t DBT_TotalBBs;
    uint32_t DBT_TotalBBSplits;
    uint64_t DBT_CompiledInstructionsExecuted;
    uint64_t DBT_TotalExecutionCompiledCalls;
    uint64_t DBT_CompilerInvocations;
    uint64_t DBT_TransitionCount;
    uint64_t DBT_TotalCycles;
    uint64_t DBT_TotalEpochs;
    uint64_t DBT_TotalFragments;
    
    //Execution Timing:
    uint64_t DBT_TotalAnalysisTime;
    uint64_t TotalInstructionEmulationTime;
    uint64_t TotalInterpretedInstructionTime;
    uint64_t TotalCompiledInstructionTime;
    uint64_t DBT_TotalInstructionProfilingTime;
    uint64_t DBT_TotalTUDispatchingTime;
    uint64_t DBT_TotalTransitionUpdateTime;
    
    uint64_t TotalInstructionsExecuted;
        
    EpochInfo* CurrentEpochInfo;
    
    __LinkedList     *EpochInformation;
    
    __LinkedList     *CyclesElapsed;            //This list contains the amount of cycles
                                        //elapsed since emulation start
    pthread_t CycleTimerTID;
    uint32_t  CycleTimerDone;
    uint64_t  CycleTimerT0;
    
    pthread_mutex_t CycleTimerMutex;
    uint32_t CycleTimerCPUBusy;
} Statistics;

void InitStatistics(void);
void PrintStatistics(void);
STATCycles* STATCyclesNew(double Time, uint64_t CyclesElapsed);
void STATCyclesAdd(STATCycles* Cycles);
void InitCycleTimer();
void ShutdownCycleTimer();
void CycleTimerSetCPUBusy();
void CycleTimerClearCPUBusy();

EpochInfo* EpochInfoNew(uint64_t Epoch);
void SaveEpochInfo(const char* Filename);

#endif	/* STATISTICS_H */

