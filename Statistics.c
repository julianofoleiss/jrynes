#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include "Statistics.h"
#include "JRYNES.h"
#include "6502.h"

extern EmulationSettings Settings;
extern __Context CPU;

Statistics *ExecutionStatistics;

void InitStatistics(void){
    ExecutionStatistics = calloc(sizeof(Statistics), 1);
    ExecutionStatistics->DBT_TotalBBSplits = 0;
    ExecutionStatistics->DBT_TotalBBs = 0;
    ExecutionStatistics->DBT_TotalExecutionCompiledCalls = 0;
    ExecutionStatistics->DBT_CompiledInstructionsExecuted = 0;
    ExecutionStatistics->TotalInstructionsExecuted = 0;
    ExecutionStatistics->DBT_CompilerInvocations = 0;
    ExecutionStatistics->DBT_TransitionCount = 0;
    ExecutionStatistics->CyclesElapsed = LST_New(NULL);
    ExecutionStatistics->CycleTimerCPUBusy = 0;
    ExecutionStatistics->CurrentEpochInfo = NULL;
    ExecutionStatistics->EpochInformation = LST_New(NULL);
    ExecutionStatistics->DBT_TotalCycles = 0;
    ExecutionStatistics->DBT_TotalEpochs = 0;
    ExecutionStatistics->DBT_TotalAnalysisTime = 0;
    ExecutionStatistics->TotalInstructionEmulationTime = 0;     //Total time used in instruction emulation
    ExecutionStatistics->TotalCompiledInstructionTime = 0;
    ExecutionStatistics->TotalInterpretedInstructionTime = 0;
    ExecutionStatistics->DBT_TotalInstructionProfilingTime = 0;     //Basic Block creation and execution count
    ExecutionStatistics->DBT_TotalTUDispatchingTime = 0;        
    ExecutionStatistics->DBT_TotalTransitionUpdateTime = 0;
    ExecutionStatistics->DBT_TotalFragments = 0;
}

static uint64_t toddiff(struct timeval *tod1, struct timeval *tod2)
{
    long long t1, t2;
    t1 = tod1->tv_sec * 1000000 + tod1->tv_usec;
    t2 = tod2->tv_sec * 1000000 + tod2->tv_usec;
    return t1 - t2;
}

static void* CycleTimer(void* Args){
    struct timeval LastTime, ThisTime;
    STATCycles *CycleInfo;
    
    gettimeofday(&LastTime, NULL);
    
    while(!ExecutionStatistics->CycleTimerDone){
        
        do{
            gettimeofday(&ThisTime, NULL);
        }while(toddiff(&ThisTime, &LastTime) < Settings.CycleCounterPeriod);   //era 10000
        
        if(ExecutionStatistics->CycleTimerCPUBusy){
            CycleInfo = STATCyclesNew(ThisTime.tv_sec + (ThisTime.tv_usec/(double)1000000), CPU.ClockTicks);
            STATCyclesAdd(CycleInfo);
        }
        
        memcpy(&LastTime, &ThisTime, sizeof(struct timeval));
    }
}

void CycleTimerSetCPUBusy(){
    pthread_mutex_lock(&(ExecutionStatistics->CycleTimerMutex));
    
        ExecutionStatistics->CycleTimerCPUBusy = 1;
        
    pthread_mutex_unlock(&(ExecutionStatistics->CycleTimerMutex));
}

void CycleTimerClearCPUBusy(){
    pthread_mutex_lock(&(ExecutionStatistics->CycleTimerMutex));
    
        ExecutionStatistics->CycleTimerCPUBusy = 0;
        
    pthread_mutex_unlock(&(ExecutionStatistics->CycleTimerMutex));
}

void InitCycleTimer(){
    ExecutionStatistics->CycleTimerDone = 0;
    ExecutionStatistics->CycleTimerT0 = time(NULL);
    pthread_mutex_init(&(ExecutionStatistics->CycleTimerMutex), NULL);
    pthread_create(&(ExecutionStatistics->CycleTimerTID), NULL, CycleTimer, NULL);
}

void ShutdownCycleTimer(){
    ExecutionStatistics->CycleTimerDone = 1;
    pthread_join(ExecutionStatistics->CycleTimerTID, NULL);
}

void SaveCycleGraph(const char* Filename){
    FILE* Out;
    STATCycles* Cycles;

    Out = fopen(Filename, "w");
    
    Cycles = LST_Dequeue(ExecutionStatistics->CyclesElapsed);
    while(Cycles){
        fprintf(Out, "%lf,%ld\n", Cycles->Time - (double) ExecutionStatistics->CycleTimerT0, Cycles->CyclesElapsed);      
        Cycles = LST_Dequeue(ExecutionStatistics->CyclesElapsed);
    }
    
    fclose(Out);
    
    printf("Cycle graph saved to %s\n", Filename);
}

void PrintStatistics(void){
    
    printf("Execution Statistics:\n\n");
    
    printf("\t# of instructions executed: %ld\n\n", ExecutionStatistics->TotalInstructionsExecuted);    
    
    if(Settings.ArgumentFlags & AF_GENERATE_ELAPSED_CYCLES_GRAPH){
        ShutdownCycleTimer();
        SaveCycleGraph(Settings.CycleCounterFilename);
    }
    
    printf("\n\n");
}

STATCycles* STATCyclesNew(double Time, uint64_t CyclesElapsed){
    STATCycles* New;
    
    New = calloc(sizeof(STATCycles), 1);
    
    New->Time = Time;
    New->CyclesElapsed = CyclesElapsed;
    
    return New;
}

void STATCyclesAdd(STATCycles* Cycles){
    LST_Enqueue(ExecutionStatistics->CyclesElapsed, Cycles);
}
