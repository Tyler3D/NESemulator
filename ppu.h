#ifndef _PPU_H
#define _PPU_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240
#define SCANLINES 261
#define DOTS 341

#define CLEAR_VERTICAL_BLANK() ppu.ppu_regs[PPUSTATUS] &= ~(1 << 7);
#define SET_VERTICAL_BLANK() ppu.ppu_regs[PPUSTATUS] |= (1 << 7);

/*
VPHB SINN
|||| ||||
|||| ||++- Base nametable address
|||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
|||| |+--- VRAM address increment per CPU read/write of PPUDATA
|||| |     (0: add 1, going across; 1: add 32, going down)
|||| +---- Sprite pattern table address for 8x8 sprites
||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
|||+------ Background pattern table address (0: $0000; 1: $1000)
||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
|+-------- PPU master/slave select
|          (0: read backdrop from EXT pins; 1: output color on EXT pins)
+--------- Generate an NMI at the start of the
           vertical blanking interval (0: off; 1: on)
PPU CTRL reg
*/

#define NAMETABLE_1  ((ppu.ppu_regs[PPUCTRL] & (1 << 0)) > 0)
#define NAMETABLE_2  ((ppu.ppu_regs[PPUCTRL] & (1 << 1)) > 0)
#define VRAM_INCREMENT ((ppu.ppu_regs[PPUCTRL] & (1 << 2)) > 0)
#define SPRITE_PATTERN_ADDR  ((ppu.ppu_regs[PPUCTRL] & (1 << 3)) > 0)
#define BACKGROUND_PATTERN_ADDR  ((ppu.ppu_regs[PPUCTRL] & (1 << 4)) > 0)
#define SPRITE_SIZE  ((ppu.ppu_regs[PPUCTRL] & (1 << 5)) > 0)
#define IS_NMI_ENABLED() ((ppu.ppu_regs[PPUCTRL] & (1 << 7)) > 0) // Disable/Enable nmi during vertical blank

typedef struct pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b; 
    uint8_t prio;
} pixel;

/*
https://www.nesdev.org/wiki/PPU_OAM
The OAM (Object Attribute Memory) is internal memory inside the PPU
that contains a display list of up to 64 sprites, where each sprite's information occupies 4 bytes.
We can emulate this as a struct with the x,y position of the tile, attributes (with pallete info and orientation), and index number
When writing to OAM we only write a byte at a time (not a OAM_tile struct) so this is really a 256 byte array
*/

struct OAM_tile {
    uint8_t y;
    uint8_t id;
    uint8_t attributes;
    uint8_t x;
};

struct PPU {
    uint8_t *memory;
    uint8_t *vram;
    uint8_t palettes[32]; // Might need to zero out somehow
    uint8_t data_buffer;
    int cycles;
    int scanline;
    int framecount;
    uint8_t ppu_regs[8];
    uint16_t vram_addr;
    uint8_t oam_addr;
    pixel screen[SCREEN_WIDTH][SCREEN_HEIGHT];
    struct OAM_tile OAM[64];
    uint8_t dma_page;
    uint8_t dma_addr;
    uint8_t dma_buffer;
    bool dma;
    bool dma_starting;
} ppu;

enum regs {
    PPUCTRL = 0,
    PPUMASK = 1,
    PPUSTATUS = 2,
    OAMADDR = 3,
    OAMDATA = 4,
    PPUSCROLL = 5,
    PPUADDR = 6,
    PPUDATA = 7
};

pixel buffer[SCREEN_HEIGHT * SCREEN_WIDTH];

void ppu_clock();
bool cpu_ppu_read(uint16_t addr, uint8_t *data);
bool cpu_ppu_write(uint16_t addr, uint8_t *data);
void ppu_reset();
void ppu_dma(bool odd);
bool ppu_read(uint16_t addr, uint8_t *data);
bool ppu_write(uint16_t addr, uint8_t *data);

#endif
