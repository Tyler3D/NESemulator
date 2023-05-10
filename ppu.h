#ifndef _PPU_H
#define _PPU_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define SCREEN_WIDTH
#define SCREEN_HEIGHT 
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

struct PPU {
    uint8_t *memory;
    uint8_t *vram;
    uint8_t palettes[32]; // Might need to zero out somehow
    uint16_t cycles;
    uint16_t scanline;
    uint8_t ppu_regs[8];
    uint16_t vram_addr;
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

typedef struct color {
	uint8_t r;
	uint8_t g;
	uint8_t b; 
} color;

void ppu_clock();
bool cpu_ppu_read(uint16_t addr, uint8_t *data);
bool cpu_ppu_write(uint16_t addr, uint8_t *data);
void ppu_reset();

#endif