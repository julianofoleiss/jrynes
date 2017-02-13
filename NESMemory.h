#ifndef NESMEMORY_H
#define	NESMEMORY_H

#include "romLoader.h"
#include "mappers/mapper.h"

//NES CPU Memory Layout
typedef struct {
    uint8_t RAM[0x2000];            /*0x0000 - 0x1FFF*/
    uint8_t IO [0x2020];            /*0x2000 - 0x401F*/
    uint8_t Expansion[0x1FE0];      /*0x4020 - 0x5FFF*/
    uint8_t SRAM[0x2000];           /*0x6000 - 0x7FFF*/
    uint8_t *PRGROMLL;              /*0x8000 - 0x9FFF*/
    uint8_t *PRGROMLH;              /*0xA000 - 0xBFFF*/
    uint8_t *PRGROMHL;              /*0xC000 - 0xDFFF*/
    uint8_t *PRGROMHH;              /*0xE000 - 0xFFFF*/
} NESMainMemory;

//Helper macros
#define DEFREADMEM_H(X)         static uint8_t X ## Read(uint16_t Addr)
#define DEFWRITEMEM_H(X)        static void X ## Write(uint16_t Addr, uint8_t Val)

/* Wrapper funcions for reading and writing Memory */
uint8_t ReadMemory(uint16_t Addr);
void    WriteMemory(uint16_t Addr, uint8_t Val);

//Initialize the NES Main Memory
//PPU Memory is initialized in the PPU module

void initNESMemory(RomImage *ROM, Mapper *M);

#endif	/* NESMEMORY_H */

