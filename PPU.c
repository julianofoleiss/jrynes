/* Author: Juliano Henrique Foleiss
 * 
 * 2012 - The Virtual Machine Builder NES Emulation Experiment
 * 
 * This file is part of the jrynes emulator.
 * 
 * The PPU memory mapping is handled by PPUMemory.c/h. It is implemented
 * using (and abusing) the concept of function pointers and conventional
 * buffer pointers. My experiments indicate that although a function call
 * is necessary for each PPU memory fetch, it is at least as costly
 * as doing all calculations involved in calculating the correct address
 * from a simple linear address, which must be mirrored, and sometimes,
 * functions as an I/O register.
 * 
 * The rendering code is somewhat based on the Nester emulator. Emulating
 * the PPU is quite intricate, although it is an interesting programming
 * activity. Thanks to Darren Ranalli and all the NES emulation scene.
*/

#include "romLoader.h"
#include "NESMemory.h"
#include "PPUMemory.h"
#include "PPU.h"
#include "NESMemory.h"
#include "SDL/Video.h"
#include "JRYNES.h"
#include "6502.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//This is the global PPU context 
__PPUContext PPU;

//This is needed.
extern __PPUMemory PPUMemory;

extern EmulationSettings Settings;

//This is needed to setup PPU registers in Main memory
extern uint8_t (*NESMemoryReadHandlers[0x10000])(uint16_t Addr);
extern void    (*NESMemoryWriteHandlers[0x10000])(uint16_t Addr, uint8_t Val);

//Macros for PPUREG1

//Get name table address, 
//0 = 0x2000(P), 1 = 0x2400(P), 2 = 0x2800(P), 3 = 0x2C00(P)
#define PPU_GET_NTADDR  (PPU.PPUREG1 & 0x03)
//Get address increment amount, 0 => 1, 1 => 32
#define PPU_GET_INCAMT  (PPU.PPUREG1 & 0x04)
//Get which pattern table stores sprites
#define PPU_GET_SPRTPT  (PPU.PPUREG1 & 0x08)
//Background pattern table
#define PPU_GET_BKGTPT  (PPU.PPUREG1 & 0x10)
//Get sprite size 0 => 8x8, 1 => 8x16
#define PPU_GET_SPRTSZ  (PPU.PPUREG1 & 0x20)
//Get PPU mode (slave, master). Not used in NES
#define PPU_GET_PPUMOD  (PPU.PPUREG1 & 0x40)
//Should an NMI execute on VBlank?
#define PPU_GET_NMION   (PPU.PPUREG1 & 0x80)

//Macros for PPUREG2

//Indicates whether the system is in: 0 => color, 1 => monochrome
#define PPU_GET_MONOCH  (PPU.PPUREG2 & 0x01)
//Background clipping (Whether to hide the 8 leftmost pixels on screen 0=> yes, 1 => no)
#define PPU_GET_BKGCLP  (PPU.PPUREG2 & 0x02)
//Sprite clipping (Whether to hide the 8 leftmost pixels on screen 0=> yes, 1 => no)
#define PPU_GET_SPRCLP  (PPU.PPUREG2 & 0x04)
//Background enable
#define PPU_GET_BKGON   (PPU.PPUREG2 & 0x08)
//Sprite enable
#define PPU_GET_SPRON   (PPU.PPUREG2 & 0x10)
//Monochrome Background Color / Color intensity
#define PPU_GET_MCCI    (PPU.PPUREG2 & 0xE0)

//Macros for PPUSTATUS

//Should writes to VRAM be ignored?
#define PPU_GET_VRAMIGN (PPU.STATUS & 0x10)
//Sprite count. If set, indicates more than 8 in the current scanline
#define PPU_GET_SPRCNT  (PPU.STATUS & 0x20)
//Sprite 0 hit flag, set when a non-transparent pixel of
//sprite 0 overlaps a non-transparent background pixel.
#define PPU_GET_SPR0HIT (PPU.STATUS & 0x40)
//Is VBLANK occurring?
#define PPU_GET_VBLANK  (PPU.STATUS & 0x80)

void initPPU(){
    #ifdef DEBUG
        fprintf(stderr, "Initializing NES PPU...\n");
    #endif
                
    PPU.PPUREG1 = 0;
    PPU.PPUREG2 = 0;
    PPU.CurrentScanline = 0;
    
    //NOTE: Memory is initialized in initPPUMemory
}

void PPU_SetMirroring(uint8_t nt1, uint8_t nt2, uint8_t nt3, uint8_t nt4){
    PPUMemory.NT[0] = PPUMemory.NametableBuffers[nt1];
    PPUMemory.NT[1] = PPUMemory.NametableBuffers[nt2];
    PPUMemory.NT[2] = PPUMemory.NametableBuffers[nt3];
    PPUMemory.NT[3] = PPUMemory.NametableBuffers[nt4];
}

//////PPUREG1 (0x2000)//////

//PPUREG1 (0x2000) - Read
DEFREADMEM_H(PPUREG1){
    //Actually, 0x2000 is Write-only
    return 0;
}

//PPUREG1 (0x2000) - Write
DEFWRITEMEM_H(PPUREG1){
    //Write Val into PPUREG1
    PPU.PPUREG1 = Val;
    
    PPU.bg_pattern_table_addr  =  PPU_GET_BKGTPT ? 0x1000 : 0x0000;
    PPU.spr_pattern_table_addr = PPU_GET_SPRTPT ? 0x1000 : 0x0000;
    PPU.addr_increment = PPU_GET_INCAMT ? 32 : 1;

    // t:0000110000000000=d:00000011
    PPU.loopy_t = (PPU.loopy_t & 0xF3FF) | (((uint16_t)(Val & 0x03)) << 10);    
}

//////PPUREG2 (0x2001)//////

//PPUREG2 (0x2001) - Read
DEFREADMEM_H(PPUREG2){
    //Actually, 0x2001 is Write-only
    return 0;    
}

//PPUREG2 (0x2001) - Write
DEFWRITEMEM_H(PPUREG2){
    PPU.PPUREG2 = Val;   
}

//////PPUSTATUS (0x2002)//////

//PPUSTATUS (0x2002) - Read
DEFREADMEM_H(PPUSTATUS){
    uint8_t temp;

    // clear toggle
    PPU.toggle_2005_2006 = 0;

    temp = PPU.STATUS;

    // clear v-blank flag
    PPU.STATUS &= 0x7F;

    return temp;    
}

//This handler might be wrong.. Not using it.
//PPUSTATUS - Mirrors of 0x2002
DEFREADMEM_H(PPUSTATUSMIRROR){
    //VBLANK flag and 2005/05 toggle
    //are not reset on reads of 0x2002 mirrors.
    return PPU.STATUS;    
}

//PPUSTATUS (0x2002) - Write
DEFWRITEMEM_H(PPUSTATUS){
    //Actually, 0x2001 is Read-only   
}

//////SPRRAMADDR (0x2003)//////

//SPRRAMADDR (0x2003) - Read
DEFREADMEM_H(SPRRAMADDR){
    //Actually, 0x2003 is Write-only
    return 0;    
}

//SPRRAMADDR (0x2003) - Write
DEFWRITEMEM_H(SPRRAMADDR){
    PPU.spr_ram_rw_ptr = Val;
}

//////SPRRAMDATA (0x2004)//////

//SPRRAMDATA (0x2004) - Read
DEFREADMEM_H(SPRRAMDATA){
    //Actually, 0x2004 is Write-only
    return 0;    
}

//SPRRAMADDR (0x2004) - Write
DEFWRITEMEM_H(SPRRAMDATA){
    PPUMemory.SPRRAM[PPU.spr_ram_rw_ptr++] = Val;
}

//////SCROLLBG (0x2005)//////

//SCROLLBG (0x2005) - Read
DEFREADMEM_H(SCROLLBG){
    //Actually, 0x2005 is Write-only
    return 0;    
}

//SCROLLBG (0x2005) - Write
DEFWRITEMEM_H(SCROLLBG){
      PPU.toggle_2005_2006 = !(PPU.toggle_2005_2006);

      if(PPU.toggle_2005_2006){
        // first write
        
        // t:0000000000011111=d:11111000
        PPU.loopy_t = (PPU.loopy_t & 0xFFE0) | (((uint16_t)(Val & 0xF8)) >> 3);

        // x=d:00000111
        PPU.loopy_x = Val & 0x07;
      }
      else{
        // second write

        // t:0000001111100000=d:11111000
        PPU.loopy_t = (PPU.loopy_t & 0xFC1F) | (((uint16_t)(Val & 0xF8)) << 2);
	      
        // t:0111000000000000=d:00000111
        PPU.loopy_t = (PPU.loopy_t & 0x8FFF) | (((uint16_t)(Val & 0x07)) << 12);
      }
}

//////PPUMEMADDR (0x2006)//////

//PPUMEMADDR (0x2006) - Read
DEFREADMEM_H(PPUMEMADDR){
    //Actually, 0x2006 is Write-only
    return 0;    
}

//PPUMEMADDR (0x2006) - Write
DEFWRITEMEM_H(PPUMEMADDR){
    PPU.toggle_2005_2006 = !(PPU.toggle_2005_2006);

    if(PPU.toggle_2005_2006){
        // first write

        // t:0011111100000000=d:00111111
        // t:1100000000000000=0
        PPU.loopy_t = (PPU.loopy_t & 0x00FF) | (((uint16_t)(Val & 0x3F)) << 8);
    }
    else{
        // second write

        // t:0000000011111111=d:11111111
        PPU.loopy_t = (PPU.loopy_t & 0xFF00) | ((uint16_t)Val);

        // v=t
        PPU.loopy_v = PPU.loopy_t;
    }
}

//////PPUMEMDATA (0x2007)//////

//PPU Data Register (0x2007) - Read
DEFREADMEM_H(PPUMEMDATA){
  uint16_t addr;
  uint8_t temp;

  addr = PPU.loopy_v;
  PPU.loopy_v += PPU.addr_increment;
  
  addr &= 0x3FFF;
     
  temp = PPU.read_2007_buffer;
  PPU.read_2007_buffer = ppuMemoryRead(addr);   
  
  return temp;
}

//PPU Data Register (0x2007) - Write
DEFWRITEMEM_H(PPUMEMDATA){
  uint16_t addr;

  addr = PPU.loopy_v;
  PPU.loopy_v += PPU.addr_increment;

  addr &= 0x3FFF; 
  
  ppuMemoryWrite(addr, Val);
}

//////SPRRAMDMA (0x4014)//////

//SPRRAMDMA (0x4014) - Read
DEFREADMEM_H(SPRRAMDMA){
    return PPU.HighReg0x4014;
}

//SPRRAMDMA (0x4014) - Write
DEFWRITEMEM_H(SPRRAMDMA){
    uint32_t addr;
    uint32_t i;
    
    PPU.HighReg0x4014 = Val;

    addr = ((uint32_t)Val) << 8;

    // Transfer data
    for(i = 0; i < 256; i++){
        PPUMemory.SPRRAM[i] = ReadMemory(addr++);
    }
    
    CPUaddDMACycles(514);
}

//Maps PPU I/O registers into main memory
void mapPPURegisters(){
    uint16_t i;
    
    for(i = 0x2000; i < 0x4000; i+=0x08){
        //PPUREG1
        NESMemoryReadHandlers[i] = PPUREG1Read;
        NESMemoryWriteHandlers[i] = PPUREG1Write;
        
        //PPUREG2
        NESMemoryReadHandlers[i+1] = PPUREG2Read;
        NESMemoryWriteHandlers[i+1] = PPUREG2Write;
        
        //PPUSTATUS
        NESMemoryReadHandlers[i+2] = PPUSTATUSRead;
        NESMemoryWriteHandlers[i+2] = PPUSTATUSWrite;      
        
        //SPRRAMADDR
        NESMemoryReadHandlers[i+3] = SPRRAMADDRRead;
        NESMemoryWriteHandlers[i+3] = SPRRAMADDRWrite;              

        //SPRRAMDATA
        NESMemoryReadHandlers[i+4] = SPRRAMDATARead;
        NESMemoryWriteHandlers[i+4] = SPRRAMDATAWrite;
        
        //SCROLLBG
        NESMemoryReadHandlers[i+5] = SCROLLBGRead;
        NESMemoryWriteHandlers[i+5] = SCROLLBGWrite;      
        
        //PPUMEMADDR
        NESMemoryReadHandlers[i+6] = PPUMEMADDRRead;
        NESMemoryWriteHandlers[i+6] = PPUMEMADDRWrite;         
        
        //PPUMEMDATA
        NESMemoryReadHandlers[i+7] = PPUMEMDATARead;
        NESMemoryWriteHandlers[i+7] = PPUMEMDATAWrite;
    }
    
    //The following comment might be inaccurate.
    //Read handler for 0x2002 is different than for its mirrors. See handler
    //for details.
    //NESMemoryReadHandlers[0x2002] = PPUSTATUSRead;

    //PPUMEMDATA (0x4014)
    NESMemoryReadHandlers[0x4014] = SPRRAMDMARead;
    NESMemoryWriteHandlers[0x4014] = SPRRAMDMAWrite;    
    
}

/* From hereon, only frame rendering code shall pass*/

void render_spr(uint8_t* buf);
void render_bg (uint8_t* buf);

/*
scanline start (if background or sprites are enabled):
	v:0000010000011111=t:0000010000011111
*/
#define LOOPY_SCANLINE_START(v,t) \
  { \
    v = (v & 0xFBE0) | (t & 0x041F); \
  }
        
/*
bits 12-14 are the tile Y offset.
you can think of bits 5,6,7,8,9 as the "y scroll"(*8).  this functions
slightly different from the X.  it wraps to 0 and bit 11 is switched when
it's incremented from _29_ instead of 31.  there are some odd side effects
from this.. if you manually set the value above 29 (from either 2005 or
2006), the wrapping from 29 obviously won't happen, and attrib data will be
used as name table data.  the "y scroll" still wraps to 0 from 31, but
without switching bit 11.  this explains why writing 240+ to 'Y' in 2005
appeared as a negative scroll value.
*/
#define LOOPY_NEXT_LINE(v) \
  { \
    if((v & 0x7000) == 0x7000){ /* is subtile y offset == 7? */ \
      v &= 0x8FFF; /* subtile y offset = 0 */ \
      if((v & 0x03E0) == 0x03A0){ /* name_tab line == 29? */ \
        v ^= 0x0800;  /* switch nametables (bit 11) */ \
        v &= 0xFC1F;  /* name_tab line = 0 */ \
      } \
      else{ \
        if((v & 0x03E0) == 0x03E0){ /* line == 31? */ \
          v &= 0xFC1F;  /* name_tab line = 0 */ \
        } \
        else{ \
          v += 0x0020; \
        } \
      } \
    } \
    else{ \
      v += 0x1000; /* next subtile y offset */ \
    } \
  }

void PPUStartFrame(){
    PPU.CurrentScanline = 0;

    if(PPU_GET_SPRON || PPU_GET_BKGON)
        PPU.loopy_v = PPU.loopy_t;
}

inline void PPUEndFrame(){}

static inline uint8_t getBGColor(){
    return ppuMemoryRead(0x3F00);
}

void PPUDoScanlineAndDraw(uint8_t* buf){
    if(PPU_GET_BKGON){
        // set to background color
        memset(buf, getBGColor(), NES_BACKBUF_WIDTH);
    }

    if(PPU_GET_SPRON || PPU_GET_BKGON){
        LOOPY_SCANLINE_START(PPU.loopy_v, PPU.loopy_t);

        if(PPU_GET_BKGON){
            // draw background
            render_bg(buf);
        }
        else{
            // clear out solid buffer
            memset(PPU.solid_buf, 0x00, NES_BACKBUF_WIDTH);
        }

        if(PPU_GET_SPRON){
            // draw sprites
            render_spr(buf);
        }

        LOOPY_NEXT_LINE(PPU.loopy_v);
    }

    PPU.CurrentScanline++;
}

void PPUDoScanlineAndDontDraw(){
  /*
  // mmc2 / punchout -- we must simulate the ppu for every line
  if(parent_NES->ROM->get_mapper_num() == 9)
  {
    do_scanline_and_draw(dummy_buffer);
  }
  else**/
  
  // if sprite 0 flag not set and sprite 0 on current line
    if((!PPU_GET_SPR0HIT) && 
     (PPU.CurrentScanline >= ((uint32_t)(PPUMemory.SPRRAM[0]+1))) && 
     (PPU.CurrentScanline <  ((uint32_t)(PPUMemory.SPRRAM[0]+1+(PPU_GET_SPRTSZ? 16 : 8))))
    )
    {
        // render line to dummy buffer
        PPUDoScanlineAndDraw(PPU.dummy_buffer);
    }
    else{
        if(PPU_GET_SPRON || PPU_GET_BKGON){
            LOOPY_SCANLINE_START(PPU.loopy_v, PPU.loopy_t);
            LOOPY_NEXT_LINE(PPU.loopy_v);
        }
        PPU.CurrentScanline++;
    }
}

void PPUStartVBlank(){
  PPU.in_vblank = 1;

  // set vblank register flag
  PPU.STATUS |= 0x80;
}

void PPUEndVBlank(){
  PPU.in_vblank = 0;

  // reset vblank register flag and sprite0 hit flag1
  PPU.STATUS &= 0x3F;
}

uint8_t PPUGetNMIEnabled(){return PPU_GET_NMION;}

#define DRAW_BG_PIXEL() \
  col = attrib_bits; \
 \
  if(pattern_lo & pattern_mask) col |= 0x01; \
  if(pattern_hi & pattern_mask) col |= 0x02; \
 \
  if(col & 0x03){ \
    *p = ppuMemoryRead(col + 0x3F00);\
    /* set solid flag */ \
    *solid = BG_WRITTEN_FLAG; \
  } \
  else{ \
    *p = getBGColor(); \
    /* set solid flag */ \
    *solid = 0; \
  } \
  solid++; \
  p++; \
  
void render_bg(uint8_t* buf){
    uint8_t *p;
    uint32_t i;

    uint32_t *solid;

    uint8_t col;

    uint32_t tile_x; // pixel coords within nametable
    uint32_t tile_y;
    uint32_t name_addr;

    uint32_t pattern_addr;
    uint8_t  pattern_lo;
    uint8_t  pattern_hi;
    uint8_t  pattern_mask;

    uint32_t attrib_addr;
    uint8_t  attrib_bits;

    tile_x = (PPU.loopy_v & 0x001F);
    tile_y = (PPU.loopy_v & 0x03E0) >> 5;

    name_addr = 0x2000 + (PPU.loopy_v & 0x0FFF);

    attrib_addr = 0x2000 + (PPU.loopy_v & 0x0C00) + 0x03C0 + ((tile_y & 0xFFFC)<<1) + (tile_x>>2);
    
    if(0x0000 == (tile_y & 0x0002)){
        if(0x0000 == (tile_x & 0x0002)){
            //attrib_bits = (VRAM(attrib_addr) & 0x03) << 2;
            attrib_bits = (ppuMemoryRead(attrib_addr) & 0x03) << 2;
        }
        else{
            //attrib_bits = (VRAM(attrib_addr) & 0x0C);
            attrib_bits = (ppuMemoryRead(attrib_addr) &0x0C);
        }
    }
    else{
        if(0x0000 == (tile_x & 0x0002)){
            //attrib_bits = (VRAM(attrib_addr) & 0x30) >> 2;
            attrib_bits = (ppuMemoryRead(attrib_addr) & 0x30) >> 2;
        }
        else{
            //attrib_bits = (VRAM(attrib_addr) & 0xC0) >> 4;
            attrib_bits = (ppuMemoryRead(attrib_addr) & 0xC0) >> 4;
        }
    }

    p     = buf           + (SIDE_MARGIN - PPU.loopy_x);
    solid = PPU.solid_buf + (SIDE_MARGIN - PPU.loopy_x); // set "solid" buffer ptr

  // draw 33 tiles
    for(i = 33; i; i--){
        pattern_addr = PPU.bg_pattern_table_addr + ((int32_t) ppuMemoryRead(name_addr) << 4) + ((PPU.loopy_v & 0x7000) >> 12);

        pattern_lo   = ppuMemoryRead(pattern_addr);
        pattern_hi   = ppuMemoryRead(pattern_addr+8);
        pattern_mask = 0x80;

    //CHECK_MMC2(pattern_addr);

        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();
        pattern_mask >>= 1;
        DRAW_BG_PIXEL();

        tile_x++;
        name_addr++;

        // are we crossing a dual-tile boundary?
        if(0x0000 == (tile_x & 0x0001)){
            // are we crossing a quad-tile boundary?
            if(0x0000 == (tile_x & 0x0003)){
                // are we crossing a name table boundary?
                if(0x0000 == (tile_x & 0x001F)){
                    name_addr ^= 0x0400; // switch name tables
                    attrib_addr ^= 0x0400;
                    name_addr -= 0x0020;
                    attrib_addr -= 0x0008;
                    tile_x -= 0x0020;
                }
                attrib_addr++;
            }

            if(0x0000 == (tile_y & 0x0002)){
                if(0x0000 == (tile_x & 0x0002))
                    attrib_bits = (ppuMemoryRead(attrib_addr) & 0x03) << 2;
                else
                    attrib_bits = (ppuMemoryRead(attrib_addr) & 0x0C);
            }
            else{
                if(0x0000 == (tile_x & 0x0002))
                    attrib_bits = (ppuMemoryRead(attrib_addr) & 0x30) >> 2;
                else
                    attrib_bits = (ppuMemoryRead(attrib_addr) & 0xC0) >> 4;
            }
        }
    }

    if(PPU_GET_BKGCLP){
        // clip left 8 pixels
        memset(buf, getBGColor(), 8);
        memset(solid, 0, sizeof(solid[0])*8);
    }
}

void render_spr(uint8_t* buf){
    int32_t s;              // sprite #
    int32_t  spr_x;         // sprite coordinates
    uint32_t spr_y;
    uint8_t* spr;           // pointer to sprite RAM entry
    uint8_t* p;             // draw pointer

    uint32_t *solid;
    uint32_t priority;

    int32_t inc_x;           // drawing vars
    int32_t start_x, end_x;
    int32_t x,y;             // in-sprite coords

    uint32_t num_sprites = 0;

    uint32_t spr_height;
    spr_height = PPU_GET_SPRTSZ ? 16 : 8;

    //Draw every sprite
    for(s = 0; s < 64; s++){
      
        spr = &(PPUMemory.SPRRAM[s<<2]);

        // get y coord
        spr_y = spr[0] + 1;

        // on current scanline?
        if((spr_y > PPU.CurrentScanline) || ((spr_y+(spr_height)) <= PPU.CurrentScanline))
            continue;

        num_sprites++;
        
        //The real PPU had a limit of 8 sprites per scanline.
        if(num_sprites > 8){
            if(!(Settings.ArgumentFlags & AF_MORE_SPRITES)) 
                break;
        }

        // get x coord
        spr_x = spr[3];

        start_x = 0;
        end_x = 8;

        // clip right
        if((spr_x + 7) > 255){
            end_x -= ((spr_x + 7) - 255);
        }

        // clip left
        if((spr_x < 8) && (PPU_GET_SPRCLP)){
            if(0 == spr_x) 
                continue;
            start_x += (8 - spr_x);
        }

        y = PPU.CurrentScanline - spr_y;

        //CHECK_MMC2(spr[1] << 4);

        // calc offsets into buffers
        p = &buf[SIDE_MARGIN + spr_x + start_x];
        solid = &(PPU.solid_buf[SIDE_MARGIN + spr_x + start_x]);

        // flip horizontally?
        if(spr[2] & 0x40){
          //YUP
          inc_x = -1;
          start_x = (8-1) - start_x;
          end_x = (8-1) - end_x;
        }
        else{
          inc_x = 1;
        }

        // flip vertically?
        if(spr[2] & 0x80){
          //YUP
          y = (spr_height-1) - y;
        }

        // get priority bit
        priority = spr[2] & 0x20;

        for(x = start_x; x != end_x; x += inc_x){
            uint8_t col = 0x00;
            uint32_t tile_addr;
            uint8_t tile_mask;

            // if a sprite has drawn on this pixel, don't draw anything
            if(!((*solid) & SPR_WRITTEN_FLAG)){
                if(PPU_GET_SPRTSZ){
                    tile_addr = spr[1] << 4;
                    if(spr[1] & 0x01){
                        tile_addr += 0x1000;
                        if(y < 8) 
                            tile_addr -= 16;
                    }
                    else{
                        if(y >= 8) 
                            tile_addr += 16;
                    }
                    tile_addr += y & 0x07;
                    tile_mask = (0x80 >> (x & 0x07));
                }
                else{
                    tile_addr = spr[1] << 4;
                    tile_addr += y & 0x07;
                    tile_addr += PPU.spr_pattern_table_addr;
                    tile_mask = (0x80 >> (x & 0x07));
                }

                if(ppuMemoryRead(tile_addr) & tile_mask) 
                    col |= 0x01;

                tile_addr += 8;

                if(ppuMemoryRead(tile_addr) & tile_mask) 
                    col |= 0x02;

                if(spr[2] & 0x02) 
                    col |= 0x08;

                if(spr[2] & 0x01) 
                    col |= 0x04;

                if(col & 0x03){
                    // set sprite 0 hit flag
                    if(!s){
                        if((*solid) & BG_WRITTEN_FLAG){
                            PPU.STATUS |= 0x40;
                        }
                    }

                    if(priority){
                        (*solid) |= SPR_WRITTEN_FLAG;
                        if(!((*solid) & BG_WRITTEN_FLAG)){
                            *p = ppuMemoryRead(0x3F10 + col);
                        }
                    }
                    else{
                        if(!((*solid) & SPR_WRITTEN_FLAG)){
                            *p = ppuMemoryRead(0x3F10 + col);
                            (*solid) |= SPR_WRITTEN_FLAG;
                        }
                    }
                }
            }

            p++;
            solid++;
        }
    }
    
    if(num_sprites > 8)
        PPU.STATUS |= 0x20;
    else
        PPU.STATUS &= 0xDF;
    
}

void draw_patterns(uint8_t *buf){
    
    int i = 0, j = 0, k = 0, l = 0;
    uint8_t mask, linha;
    uint8_t *p, *q;
    
    p = buf;
    
    for(i = 0x1000; i < 0x2000; i+=16){
        
        if((i % 256) == 0)
            p = p + (VIDGetPitch() * 4);
        else
            p = p + 8;
        
        linha = 0;
        
        for(j = i; j < i + 8; j++){
            
            k = j + 8;
            mask = 0x80;
            q = p + (VIDGetPitch() * (linha++));
            for(l = 0; l < 8; l++){
                *(q+l) |= ppuMemoryRead(i) & mask ? 0x02 : 1;
                *(q+l) |= ppuMemoryRead(k) & mask ? 0x04 : 0;
                mask >>= 1;
            }
        }
    }
    
}