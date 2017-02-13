#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "Movie.h"

MovieMacro* Movie;

void MOVSaveFile(MovieMacro* Movie, char* MovieFile){
    FILE* Out;
    
    Out = fopen(MovieFile, "w");
    
    fwrite(&(Movie->Position), sizeof(uint64_t), 1, Out);
    fwrite(Movie->Data, sizeof(MovieData), Movie->Position, Out);
    
    fclose(Out);
}

MovieMacro* MOVLoadFile(char* MovieFile){
    MovieMacro* Movie;
    FILE* In;
    uint32_t BytesRead;
    
    In = fopen(MovieFile, "r");
    
    if(!In){
        return NULL;
    }
    
    Movie = (MovieMacro*) malloc(sizeof(MovieMacro));
    
    BytesRead = fread(&(Movie->Length), sizeof(uint64_t), 1, In);
    
    Movie->Data = (MovieData*) malloc(sizeof(MovieData) * (Movie->Length + 1));
    
    BytesRead = fread(Movie->Data, sizeof(MovieData), Movie->Length, In);
    
    Movie->Position = 0;
    
    return Movie;
}

MovieMacro* MOVInit(){
    Movie = (MovieMacro*) malloc(sizeof(MovieMacro));
    
    Movie->Length = MOV_INITIAL_SIZE;
    Movie->Position = 0;
    Movie->Data = (MovieData*) malloc(sizeof(MovieData) * MOV_INITIAL_SIZE);
    
    return Movie;
}

void MOVAddData(MovieMacro *Movie, uint64_t Cycles, uint8_t Pad1State, uint8_t Pad2State){
    
    Movie->Data[Movie->Position].Cycle = Cycles;
    Movie->Data[Movie->Position].Pad1State = Pad1State;
    Movie->Data[Movie->Position].Pad2State = Pad2State;    
    
    Movie->Position++;
    
    if(Movie->Position == Movie->Length){
        Movie->Length += MOV_SIZE_INCREASE;
        Movie->Data = (MovieData*) realloc(Movie->Data, Movie->Length);
    }
    
}

void MOVNextData(){
    Movie->Position++;
}

uint32_t MOVMovieEnded(){
    return (Movie->Position >= Movie->Length) ? 1 : 0;
}

uint8_t MOVGetPreviousPad1State(){
    return Movie->Data[Movie->Position-1].Pad1State;
}

uint8_t MOVGetPreviousPad2State(){
    return Movie->Data[Movie->Position-1].Pad2State;
}

uint8_t MOVGetPad1State(){
    return Movie->Data[Movie->Position].Pad1State;
}

uint8_t MOVGetPad2State(){
    return Movie->Data[Movie->Position].Pad2State;
}

uint64_t MOVPeekNextStepCycles(){
    return Movie->Data[Movie->Position+1].Cycle;
}