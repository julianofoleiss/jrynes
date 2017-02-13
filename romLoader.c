#include <stdio.h>
#include <stdlib.h>

#include "romLoader.h"

RomImage* LoadROMImage(const char* Filename){
    RomImageHeader* Header;
    RomImage* ROM;
    FILE* ROMFile;
    uint8_t i;
    void* Buffer;
    uint32_t BytesRead;
    
    #ifdef DEBUG
    printf("Loading the ROM from disk... (%s)", Filename);
    #endif
    
    ROM = malloc(sizeof(RomImage));
    
    ROMFile = fopen(Filename, "r");
    
    if(!ROMFile)
        return NULL;
    
    Header = malloc(sizeof(RomImageHeader));
    
    BytesRead = fread(Header, sizeof(RomImageHeader), 1, ROMFile);
    
    ROM->Header = Header;

    //Checking for a trainer
    if(Header->RCB1 & 0x04){
        ROM->Trainer = malloc(512);
        BytesRead = fread(ROM->Trainer, 512, 1, ROMFile);
    }
    else
        ROM->Trainer = NULL;
    
    Buffer = malloc(0x4000 * Header->PRGBanks);
    
    //Loading PRG-ROM pages
    ROM->PRGROM = malloc (sizeof(uint8_t*) * Header->PRGBanks * 2);
    BytesRead = fread(Buffer, 0x4000, Header->PRGBanks, ROMFile);
    for(i = 0; i < Header->PRGBanks * 2; i++)
        ROM->PRGROM[i] = Buffer + (i * 0x2000);
    
    
    if(!Header->VROMBanks){
        Buffer = malloc(0x2000 * 2);
        ROM->VROM = malloc(sizeof(uint8_t*) * 2);
        ROM->VROM[0] = Buffer;
        ROM->VROM[1] = Buffer + 0x2000;
    }
    else{
        Buffer = malloc(0x2000 * Header->VROMBanks);

        //Loading CHR-ROM (VROM) pages
        ROM->VROM = malloc (sizeof(uint8_t*) * Header->VROMBanks);
        BytesRead = fread(Buffer, 0x2000, Header->VROMBanks, ROMFile);
        for(i = 0; i < Header->VROMBanks; i++)
            ROM->VROM[i] = Buffer + (i * 0x2000);
    }
    
    fclose(ROMFile);
    
    return ROM;
}

ROMMirroring GetROMMirroring(RomImage* ROM){
    if(ROM->Header->RCB1 & 0x01)
        return M_VERTICAL;
    else
        return M_HORIZONTAL;
}

uint8_t GetROMSRAMEnabled(RomImage* ROM){
    return (ROM->Header->RCB1 & 0x02);
}

uint8_t GetROMTrainerEnabled(RomImage* ROM){
    return (ROM->Header->RCB1 & 0x04);
}

uint8_t GetROM4ScreenMirroringEnabled(RomImage* ROM){
    return (ROM->Header->RCB1 & 0x08);
}

uint8_t GetROMMapper(RomImage* ROM){
    return (((ROM->Header->RCB1 & 0xF0) >> 4) | (ROM->Header->RCB2 & 0xF0));
}

void PrintROMImageInformation(RomImage* ROM){
    printf("\n");
    printf("\tPRG-ROM pages: %d\n", ROM->Header->PRGBanks);
    printf("\tCHR-ROM pages: %d\n", ROM->Header->VROMBanks);
    printf("\tMirroring: %s\n", GetROMMirroring(ROM) == M_HORIZONTAL ? "HORIZONTAL" : "VERTICAL");
    printf("\tSRAM: %s\n", GetROMSRAMEnabled(ROM) ? "YES" : "NO");
    printf("\t4-Screen mirroring: %s\n", GetROM4ScreenMirroringEnabled(ROM) ? "YES" : "NO");
    printf("\tMapper: %d\n", GetROMMapper(ROM));
    printf("\tTrainer: %s\n", GetROMTrainerEnabled(ROM) ? "YES" : "NO");
}
