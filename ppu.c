#include "ppu.h"
#include "rom.h"
#include "cpu.h"

#define MAX_CROM
#define CROM_WIDTH
#define CROM_HEIGHT

/*

color crom[CROM_WIDTH * CROM_HEIGHT];
color palettes[32];
uint8_t vram[2048];
uint8_t oam[256];
uint8_t system_pallete[64] = [
	(0x80, 0x80, 0x80), (0x00, 0x3D, 0xA6), (0x00, 0x12, 0xB0), (0x44, 0x00, 0x96), (0xA1, 0x00, 0x5E),
   (0xC7, 0x00, 0x28), (0xBA, 0x06, 0x00), (0x8C, 0x17, 0x00), (0x5C, 0x2F, 0x00), (0x10, 0x45, 0x00),
   (0x05, 0x4A, 0x00), (0x00, 0x47, 0x2E), (0x00, 0x41, 0x66), (0x00, 0x00, 0x00), (0x05, 0x05, 0x05),
   (0x05, 0x05, 0x05), (0xC7, 0xC7, 0xC7), (0x00, 0x77, 0xFF), (0x21, 0x55, 0xFF), (0x82, 0x37, 0xFA),
   (0xEB, 0x2F, 0xB5), (0xFF, 0x29, 0x50), (0xFF, 0x22, 0x00), (0xD6, 0x32, 0x00), (0xC4, 0x62, 0x00),
   (0x35, 0x80, 0x00), (0x05, 0x8F, 0x00), (0x00, 0x8A, 0x55), (0x00, 0x99, 0xCC), (0x21, 0x21, 0x21),
   (0x09, 0x09, 0x09), (0x09, 0x09, 0x09), (0xFF, 0xFF, 0xFF), (0x0F, 0xD7, 0xFF), (0x69, 0xA2, 0xFF),
   (0xD4, 0x80, 0xFF), (0xFF, 0x45, 0xF3), (0xFF, 0x61, 0x8B), (0xFF, 0x88, 0x33), (0xFF, 0x9C, 0x12),
   (0xFA, 0xBC, 0x20), (0x9F, 0xE3, 0x0E), (0x2B, 0xF0, 0x35), (0x0C, 0xF0, 0xA4), (0x05, 0xFB, 0xFF),
   (0x5E, 0x5E, 0x5E), (0x0D, 0x0D, 0x0D), (0x0D, 0x0D, 0x0D), (0xFF, 0xFF, 0xFF), (0xA6, 0xFC, 0xFF),
   (0xB3, 0xEC, 0xFF), (0xDA, 0xAB, 0xEB), (0xFF, 0xA8, 0xF9), (0xFF, 0xAB, 0xB3), (0xFF, 0xD2, 0xB0),
   (0xFF, 0xEF, 0xA6), (0xFF, 0xF7, 0x9C), (0xD7, 0xE8, 0x95), (0xA6, 0xED, 0xAF), (0xA2, 0xF2, 0xDA),
   (0x99, 0xFF, 0xFC), (0xDD, 0xDD, 0xDD), (0x11, 0x11, 0x11), (0x11, 0x11, 0x11)
]
// Taken from bugzmanov.github.io

uint8_t screen_buf[SCREEN_HEIGHT * SCREEN_WIDTH * 3];

// Easily just grab crom, palletes, vram, oam_data using array[index]
// No need to do complicated registers

void set_pixel(uint8_t x, uint8_t y, color rgb){
	screen_buf[(y * SCREEN_WIDTH + x) * 3] = rgb.r;
	screen_buf[(y * SCREEN_WIDTH + x) * 3] = rgb.b;
	screen_buf[(y * SCREEN_WIDTH + x) * 3] = rgb.g;
}

void set_tile(uint8_t tile, uint8_t x, uint8_t y){
   uint8_t initial_index = ((tile / 8) * CROM_WIDTH + (tile % 8));
   uint8_t crom_index;
   for (int row = 0; row < 8; row++){
      for(int col = 0; col < 8; col++){
         crom_index = initial_index + row * CROM_WIDTH + col;
         set_pixel(x + col, y + row, crom[crom_index]);
      }
   }
}

*/

void ppu_reset() {
   for (int i = 0; i < 8; i++)
      ppu.ppu_regs[i] = 0;
   for (int i = 0; i < 32; i++)
      ppu.palettes[i] = 0;
   ppu.cycles = 0;
   ppu.scanline = 0; // Might be -1
}

/* 
   Read and write functions for ppu 
   Palettes info might work?
*/

bool ppu_read(uint16_t addr, uint8_t *data) {
    uint16_t mapped_addr;
    addr &= 0x3FFF; // Only 14 bit address range
    //printf("Attempting to read %X\n", addr);
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Pattern table 0/1
         if (!rom.readCPUMapper(addr, &mapped_addr))
            return false;
         // Mapper 000 doesn't support CHR_RAM
         // We can implement CHR_RAM later after mapper 000 works
        *data = rom.CHR_ROM_data[(mapped_addr & 0x0FFF) * ((mapped_addr & 0x1000) >> 12)];
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
         // VRAM
         addr &= 0x0F00;
         bool mirroring = rom.flag_6 & 0x01;
         if (mirroring) { // Vertical (horizontal arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
				   *data = ppu.vram[addr & 0x03FF];
            if (addr >= 0x0400 && addr <= 0x07FF)
               *data = ppu.vram[(addr & 0x03FF) * 2];
            if (addr >= 0x0800 && addr <= 0x0BFF)
               *data = ppu.vram[addr & 0x03FF];
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               *data = ppu.vram[(addr & 0x03FF) * 2];
         } else { // horizontal (vertical arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
               *data = ppu.vram[addr & 0x03FF];
            if (addr >= 0x0400 && addr <= 0x07FF)
               *data = ppu.vram[addr & 0x03FF];
            if (addr >= 0x0800 && addr <= 0x0BFF)
               *data = ppu.vram[(addr & 0x03FF) * 2];
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               *data = ppu.vram[(addr & 0x03FF) * 2];
         }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
         *data = ppu.palettes[addr & 0x20];
    } else
        return false;
    return true;
}

// Return true if written
// Returns false if cannot write
bool ppu_write(uint16_t addr, uint8_t *data) {
    uint16_t mapped_addr;
    addr &= 0x3FFF; // Only 14 bit address range
    //printf("Attempting to read %X\n", addr);
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // Pattern table 0/1
         if (!rom.readCPUMapper(addr, &mapped_addr))
            return false;
         rom.CHR_ROM_data[(mapped_addr & 0x0FFF) * ((mapped_addr & 0x1000) >> 12)] = *data;
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
         // VRAM
         addr &= 0x0F00;
         bool mirroring = rom.flag_6 & 0x01;
         if (mirroring) { // Vertical (horizontal arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
				   ppu.vram[addr & 0x03FF] = *data;
            if (addr >= 0x0400 && addr <= 0x07FF)
               ppu.vram[(addr & 0x03FF) * 2] = *data;
            if (addr >= 0x0800 && addr <= 0x0BFF)
               ppu.vram[addr & 0x03FF] = *data;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               ppu.vram[(addr & 0x03FF) * 2] = *data;
         } else { // horizontal (vertical arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
               ppu.vram[addr & 0x03FF] = *data;
            if (addr >= 0x0400 && addr <= 0x07FF)
               ppu.vram[addr & 0x03FF] = *data;
            if (addr >= 0x0800 && addr <= 0x0BFF)
               ppu.vram[(addr & 0x03FF) * 2] = *data;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               ppu.vram[(addr & 0x03FF) * 2] = *data;
         }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
         ppu.palettes[addr & 0x20] = *data;
    } else
        return false;
    return true;
}

/* Functions that read/write to the PPU registers */

bool cpu_ppu_read(uint16_t addr, uint8_t *data) {
   switch (addr) {
      case PPUCTRL: // $2000 Control reg
      *data = 0;
      break;

      case PPUMASK: // $2001 PPUMASK
      *data = 0;
      break;

      case PPUSTATUS: // $2002 PPUSTATUS
      /* TODO: address latch needs to be cleared. Which is used by PPUSCROLL and PPUADDR */
      *data = ppu.ppu_regs[PPUSTATUS];
      break;

      case OAMADDR: // $2003 OAMADDR
      *data = 0;
      break;

      case OAMDATA: // $2004 OAMDATA
      
      break;

      case PPUSCROLL: // $2005 PPUSCROLL
      *data = 0;
      break;

      case PPUADDR: // $2006 PPUADDR
      *data = 0;
      break;

      case PPUDATA: // $2007 PPUDATA
      ppu_read(ppu.vram_addr, data); // Check
      ppu.vram_addr += VRAM_INCREMENT ? 32 : 1;
      break;

      default:
      return false;
   }
   return true;
}

bool cpu_ppu_write(uint16_t addr, uint8_t *data) {
   switch (addr) {
      case PPUCTRL: // $2000 Control reg
      ppu.ppu_regs[PPUCTRL] = *data;

      case PPUMASK: // $2001 PPUMASK
      ppu.ppu_regs[PPUMASK] = *data;
      break;

      case PPUSTATUS: // $2002 PPUSTATUS
      ppu.ppu_regs[PPUSTATUS] = *data;
      ppu.vram_addr = 0x0000;
      CLEAR_VERTICAL_BLANK() // Clear Vertical Blank flag
      break;

      case OAMADDR: // $2003 OAMADDR

      break;

      case OAMDATA: // $2004 OAMDATA

      break;

      case PPUSCROLL: // $2005 PPUSCROLL

      break;

      case PPUADDR: // $2006 PPUADDR
      // Check
      ppu.vram_addr = (ppu.vram_addr << 8) | *data;
      ppu.vram_addr &= 0x3FFF;
      break;

      case PPUDATA: // $2007 PPUDATA
      ppu_write(ppu.vram_addr, data); // Check
      ppu.vram_addr += VRAM_INCREMENT ? 32 : 1;
      break;

      default:
      return false;
   }
   return true;
}

void ppu_clock() {
   cpu.nmi = false;

   if (ppu.cycles >= DOTS) {
      ppu.scanline++;
      ppu.cycles = 0;
   }

   if (ppu.scanline >= SCANLINES) {
      ppu.scanline = -1;
   }
}