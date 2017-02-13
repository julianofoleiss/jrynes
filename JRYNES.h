#ifndef JRYNES_H
#define	JRYNES_H

#include <stdint.h>

#define AF_MORE_SPRITES                         0x01
#define AF_DEBUG_CONSOLE                        0x02
#define AF_RECORDING                            0x04
#define AF_PLAYING                              0x08
#define AF_DBT                                  0x10
#define AF_DBT_PARALLEL_COMPILATION             0x20
#define AF_NO_DELAY                             0x40
#define AF_QUIT_MOVIE_END                       0x80
#define AF_OPT_IMMEDIATE                        0x100
#define AF_CTXT_INFO                            0x200
#define AF_OPT_BLOCK_COALESCING                 0x400
#define AF_DONT_DRAW                            0x800
#define AF_GENERATE_ELAPSED_CYCLES_GRAPH        0x1000
#define AF_INTERPRETATIVE_PROFILING             0x2000
#define AF_COUNT_EPOCHS                         0x4000

typedef struct{
    uint32_t xScaling;
    uint32_t yScaling;
    uint64_t ArgumentFlags;
    uint32_t CompilationThreshold;      //for -t option
    uint8_t  ContextMask;
    uint64_t  ContextThreshold;
    uint32_t MaxContexts;
    uint64_t EpochLength;
    double TranslationThreshold;
    char* RecordMovieFile;
    char* PlayMovieFile;
    char* RomFilename;
    
    uint64_t CycleCounterPeriod;        //in 1000000ths of seconds
    char CycleCounterFilename[0x100];
} EmulationSettings;

#endif	/* JRYNES_H */

