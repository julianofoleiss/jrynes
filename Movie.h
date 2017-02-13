#ifndef MOVIE_H
#define	MOVIE_H

#define MOV_INITIAL_SIZE        0x1000
#define MOV_SIZE_INCREASE       0x1000

typedef struct{
    uint64_t Cycle;
    uint8_t Pad1State;
    uint8_t Pad2State;
} MovieData;

typedef struct{
    uint64_t Length;
    uint64_t Position;
    MovieData* Data;
} MovieMacro;

void MOVSaveFile(MovieMacro* Movie, char* MovieFile);

MovieMacro* MOVLoadFile(char* MovieFile);

MovieMacro* MOVInit();

void MOVAddData(MovieMacro *Movie, uint64_t Cycles, uint8_t Pad1State, uint8_t Pad2State);

void MOVNextData();

uint32_t MOVMovieEnded();

uint8_t MOVGetPreviousPad1State();

uint8_t MOVGetPreviousPad2State();

uint8_t MOVGetPad1State();

uint8_t MOVGetPad2State();

uint64_t MOVPeekNextStepCycles();

#endif	/* MOVIE_H */

