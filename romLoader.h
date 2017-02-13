#include <stdint.h>

#ifndef ROMLOADER_H
#define	ROMLOADER_H

typedef struct {
    
    uint8_t NES[3];
    uint8_t Magic;
    uint8_t PRGBanks;
    uint8_t VROMBanks;
    uint8_t RCB1;       //ROM control byte 1
    uint8_t RCB2;       //ROM control byte 1
    uint8_t RAMBanks;
    uint8_t RESERVED[7];
    
} RomImageHeader;

typedef struct {
    
    RomImageHeader *Header;
    uint8_t     *Trainer;
    uint8_t    **PRGROM;
    uint8_t    **VROM;
    
} RomImage;

typedef enum {
    M_HORIZONTAL = 0,
    M_VERTICAL = 1
} ROMMirroring;

RomImage* LoadROMImage(const char* Filename);

ROMMirroring GetROMMirroring(RomImage* ROM);
uint8_t GetROMSRAMEnabled(RomImage* ROM);
uint8_t GetROMTrainerEnabled(RomImage* ROM);
uint8_t GetROM4ScreenMirroringEnabled(RomImage* ROM);
uint8_t GetROMMapper(RomImage* ROM);
void PrintROMImageInformation(RomImage* ROM);

#endif	/* ROMLOADER_H */

