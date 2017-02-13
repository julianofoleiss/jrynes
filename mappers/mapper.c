#include <string.h>
#include <stdlib.h>
#include "mapper.h"
#include "mapper000.h"
#include "mapper002.h"
#include "mapper007.h"

#include "../romLoader.h"

Mapper* MAP_GetMapper(RomImage *ROM){    
    Mapper *M;
    
    M = malloc(sizeof(Mapper));
    
    M->ROM = ROM;
    
    switch(GetROMMapper(ROM)){
        case 0:
            M->Reset = M000_Reset;
            M->SetupCPUMemoryHandlers = M000_SetupCPUMemoryHandlers;
            M->SetupPPUMemoryHandlers = M000_SetupPPUMemoryHandlers;
        break;
        
        case 2:
            M->Reset = M002_Reset;
            M->SetupCPUMemoryHandlers = M002_SetupCPUMemoryHandlers;
            M->SetupPPUMemoryHandlers = M002_SetupPPUMemoryHandlers;
        break;        
        
        case 7:
            M->Reset = M007_Reset;
            M->SetupCPUMemoryHandlers = M007_SetupCPUMemoryHandlers;
            M->SetupPPUMemoryHandlers = M007_SetupPPUMemoryHandlers;
        break;          
        
        default:
            free(M);
            M = NULL;
    }
    
    return M;
}
