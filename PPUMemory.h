#ifndef PPUMEMORY_H
#define	PPUMEMORY_H

#include <stdint.h>
#include "romLoader.h"

typedef struct {
    uint8_t **NametableBuffers;
    
    //Pattern Tables
    uint8_t **PT;
    //Name Tables
    uint8_t **NT;
    //Attribute Tables
    uint8_t **AT;
    //Image Palette
    uint8_t Palette[0x100];
    //SPR-RAM - Sprite RAM!
    uint8_t SPRRAM[0x100];
} __PPUMemory;

uint8_t ppuMemoryRead(uint16_t Addr);
void ppuMemoryWrite(uint16_t Addr, uint8_t Val);

//Helper macros
#define DEFREADPPUMEM_H(X)         static uint8_t X ## PPURead(uint16_t Addr)
#define DEFWRITEPPUMEM_H(X)        static void X ## PPUWrite(uint16_t Addr, uint8_t Val)

//Initializes PPU RAM, primarily pattern tables and NT mirroring
void initPPUMemory(RomImage *ROM);

#endif	/* PPUMEMORY_H */

