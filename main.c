#include "6502.h"
#include "romLoader.h"
#include "NESMemory.h"
#include "NES.h"
#include "SDL/SDL.h"
#include "JRYNES.h"
#include "6502_Debug.h"
#include "Statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>

const static char optString[] = "f:x:y:r:p:mwdlcnqio::e:t:?\0";

EmulationSettings Settings;

#ifdef DEBUGCONSOLE
extern __Debug_Context Debug;
#endif

static const struct option longOpts[] = {
    { "filename", required_argument, NULL, 'f' },
    { "no_delay", no_argument, NULL, 'n' },
    { "scale_x", required_argument, NULL, 'x' },
    { "scale_y", required_argument, NULL, 'y' },
    { "more_sprites", no_argument, NULL, 'm' },
    { "record_movie", required_argument, NULL, 'r'},
    { "play_movie", required_argument, NULL, 'p'},
    { "quit_on_movie_end", no_argument, NULL, 'q'},
    { "debug_console", no_argument, NULL, 'd' },
    { NULL, no_argument, NULL, 0 }
};

void sigInt(int Code){
#ifdef DEBUGCONSOLE
    Debug.OperationFlags |= DBGOP_STEP;
#else
    printf("Terminating emulation...\n\n");
    TerminateEmulation();
    exit(1);
#endif
}

void showHelp(){
    
    printf("\n");
    
    printf("This is jrynes, an experimental NES emulator\n\n");
    
    printf("usage: jrynes [OPTIONS] --filename ROM_Filename\n\n");
    
    printf("OPTIONS are:\n\n");
    
    printf("Video Output:\n");
    printf("\t[--scale_x|-x] x-scaling-factor\n");
    printf("\t[--scale_y|-y] y-scaling-factor\n\n");
    
    printf("Display more than 8 sprites per scanline:\n");
    printf("\t[--more_sprites|-m]\n\n");
    
    printf("Show debug console at the start of the execution:\n");
    printf("\t[--debug_console|-d]\n\n");
    
    printf("No delay between frames:\n");
    printf("\t[--no_delay|-n]\n\n");
    
    printf("Don't draw frames\n");
    printf("\t[--dont_draw|-w]\n\n");    
    
    printf("Movie recording/playback options:\n");
    printf("\t[--record_movie|-r] movie-filename\n");
    printf("\t[--play_movie|-p] movie-filename\n");
    printf("\t[--quit_on_movie_end|-q]\n\n");
    
    
    #ifdef ASMUTILITIES
        printf("Compiled with ASM Utilities\n\n");
    #else
        printf("Compiled without ASM Utilities\n\n");
    #endif
    
}

static int FindNextChar(char* String, int InitialPos, char Which){
    int i;
    
    for(i = InitialPos; ((String[i]) && (String[i] != Which)); i++);
    
    return i;
}

static void ParseElapedCyclesArguments(char* Arguments){
    char* Option;
    int PosI, PosF, i, j;
    char *Temp;
    
    Settings.ContextMask = 0;
    
    Temp = calloc(0x100, 1);
    
    if(Arguments){
        
        Option = strstr(Arguments, "period=");
        if(Option){
            j = 0;
            printf("%s\n", Option);
            PosI = FindNextChar(Option, 0, '=');
            PosF = FindNextChar(Option, PosI, ',');
            for(i = PosI + 1; i < PosF; i++, j++)
                Temp[j] = Option[i];
            Temp[j] = 0;
            Settings.CycleCounterPeriod = atoi(Temp);
        }     
        else
            Settings.CycleCounterPeriod = 10000;
        
        Option = strstr(Arguments, "filename=");
        if(Option){
            j = 0;
            printf("%s\n", Option);
            PosI = FindNextChar(Option, 0, '=');
            PosF = FindNextChar(Option, PosI, ',');
            for(i = PosI + 1; i < PosF; i++, j++)
                Temp[j] = Option[i];
            Temp[j] = 0;
            strcpy(Settings.CycleCounterFilename, Temp);
        }     
        else
            strcpy(Settings.CycleCounterFilename, "cyclegraph.csv");
        
    }
    else{
        Settings.CycleCounterPeriod = 10000;
        strcpy(Settings.CycleCounterFilename, "cyclegraph.csv");
    }
    
    free(Temp);    
}

int main(int argc, char *argv[]){
    char* RomFilename = NULL;
    int opt;
    int longIndex;
    
    memset(&Settings, 0, sizeof(EmulationSettings));
    
    //setup signal handlers
    struct sigaction* SigIntAction;
    SigIntAction = calloc(sizeof(struct sigaction), 1);
    SigIntAction->sa_handler = sigInt;
    sigemptyset(&(SigIntAction->sa_mask));
    sigaction(SIGINT, SigIntAction, NULL);
    
    free(SigIntAction);
    
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    
    while(opt != -1){
        switch(opt){
        
            //Movie Filename - Recording
            case 'r':
                Settings.RecordMovieFile = malloc(strlen(optarg) + 1);
                strcpy(Settings.RecordMovieFile, optarg);
                Settings.ArgumentFlags |= AF_RECORDING;
                printf("Recording movie to file %s\n", optarg);
            break;
            
            //Movie Filename - Playing
            case 'p':
                Settings.PlayMovieFile = malloc(strlen(optarg) + 1);
                strcpy(Settings.PlayMovieFile, optarg);
                Settings.ArgumentFlags |= AF_PLAYING;
                printf("Playing movie from file %s\n", optarg);
            break;
            
            //Rom Filename
            case 'f':
                RomFilename = malloc(strlen(optarg) + 1);
                Settings.RomFilename = RomFilename;
                strcpy(RomFilename, optarg);
            break;
            
            //X Scaling factor
            case 'x':
                Settings.xScaling = atoi(optarg);
            break;
            
            //Y Scaling factor
            case 'y':
                Settings.yScaling = atoi(optarg);
            break;
            
            //Show more than 8 sprites onscreen
            case 'm':
                Settings.ArgumentFlags |= AF_MORE_SPRITES;
            break;
            
            //Debug console on startup
            case 'd':
                Settings.ArgumentFlags |= AF_DEBUG_CONSOLE;
            break;
            
            //No delay. Useful for assessing speed.
            case 'n':
                Settings.ArgumentFlags |= AF_NO_DELAY;
            break;
            
            //Quit as soon as the movie ends. Also prints out CPU context.
            case 'q':
                Settings.ArgumentFlags |= AF_QUIT_MOVIE_END;
            break;            
            
            //Don't draw on screen
            case 'w':
                Settings.ArgumentFlags |= AF_DONT_DRAW;
            break;              
            
            //Help
            case '?':
            case ':':
                showHelp();
                exit(0);       
            break;
            
        }
        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }
    
    if(!RomFilename){
        showHelp();
        fprintf(stderr, "ROM Filename not supplied! Please do so passing the -f argument!\n\n");
        exit(1);
    }
    
    if(initSDL() < 0){
        fprintf(stderr, "Error! Could not initialize SDL!");
        exit(1);
    }
    
    if(!Settings.xScaling)
        Settings.xScaling = 1;
    
    if(!Settings.yScaling)
        Settings.yScaling = 1;
    
    if((Settings.ArgumentFlags & (AF_PLAYING | AF_RECORDING)) == (AF_PLAYING | AF_RECORDING)){
        printf("Recording and playing a movie at the same time is not allowed!\n");
        exit(1);
    }
   
    InitStatistics();
    
    //Begin emulation
    NESrun(RomFilename);
    
    exit(0);
}

