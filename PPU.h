#ifndef PPU_H
#define	PPU_H
#include <stdint.h>

#define NES_SCREEN_WIDTH  256
#define NES_SCREEN_HEIGHT 240
#define SIDE_MARGIN       8
#define NES_SCREEN_WIDTH_VIEWABLE NES_SCREEN_WIDTH
#define NES_BACKBUF_WIDTH (NES_SCREEN_WIDTH + (2*SIDE_MARGIN))
#define BG_WRITTEN_FLAG  0x01
#define SPR_WRITTEN_FLAG 0x02

typedef struct {
    uint8_t PPUREG1;    //Mapped on 0x2000 - W only
    uint8_t PPUREG2;    //Mapped on 0x2001 - W only
    uint8_t STATUS;     //Mapped on 0x2002 - R only
    
    //The PPU Memory instantiated as a separate global variable
    //for performance
    
    uint16_t CurrentScanline;
    
    uint16_t loopy_v;
    uint16_t loopy_t;
    uint16_t loopy_x;
    
    // bit flags for pixels of current line
    uint32_t solid_buf[NES_BACKBUF_WIDTH]; 
    // used to do sprite 0 hit detection when we aren't supposed to draw
    uint8_t  dummy_buffer[NES_BACKBUF_WIDTH]; 
    
    uint8_t in_vblank;
    
    uint16_t addr_increment;
    
    uint16_t toggle_2005_2006;
    
    uint8_t read_2007_buffer;
    
    uint16_t bg_pattern_table_addr;
    
    uint16_t spr_pattern_table_addr;
    
    uint8_t spr_ram_rw_ptr;
    
    uint8_t HighReg0x4014;
    
} __PPUContext;
//Maps PPU I/O registers into Main Memory
void mapPPURegisters();

//Renders the current scanline into the external video buffer
void renderScanline();

//Initializes PPU internals
void initPPU();

//Sets Nametable mirroring
void PPU_SetMirroring(uint8_t nt1, uint8_t nt2, uint8_t nt3, uint8_t nt4);

//Initializes frame rendering
void PPUStartFrame();

//Finalizes frame rendering
void PPUEndFrame();

//Draws the current scanline
void PPUDoScanlineAndDraw(uint8_t* buf);

//Works on current scanline without generating any video output
void PPUDoScanlineAndDontDraw();

//Starts VBlank. It does not generate the NMI. Rather, it just sets
//the PPUSTATUS register
void PPUStartVBlank();

//Ends VBlank. Just unsets the PPUSTATUS register.
void PPUEndVBlank();

uint8_t PPUGetNMIEnabled();

void draw_patterns(uint8_t* buf);

#endif	/* PPU_H */
