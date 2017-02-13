/* 
 * File:   mapper.h
 * Author: juliano
 *
 * Created on April 8, 2015, 10:56 AM
 */

#ifndef MAPPER_H
#define MAPPER_H

#include "../romLoader.h"

typedef struct{
    RomImage *ROM;
    void(*Reset)();
    void(*SetupCPUMemoryHandlers)();
    void(*SetupPPUMemoryHandlers)();
    
    union{
        struct{
            uint8_t bits;
        }M001;
    }Data;
    
} Mapper;

Mapper* MAP_GetMapper(RomImage *ROM);

#endif	/* MAPPER_H */

