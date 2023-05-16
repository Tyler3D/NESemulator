#include "ppu.h"
#include "rom.h"
#include "cpu.h"
#include "logger.h"

/*
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
*/

void ppu_reset() {
   for (int i = 0; i < 8; i++)
      ppu.ppu_regs[i] = 0;
   for (int i = 0; i < 32; i++)
      ppu.palettes[i] = 0;
   ppu.cycles = 0;
   ppu.scanline = 0; // Might be -1
   ppu.framecount = 0;
   ppu.data_buffer = 0;
   ppu.dma_page = 0;
   ppu.dma_addr = 0;
   ppu.dma_buffer = 0;
   ppu.dma = false;
   ppu.dma_starting = false;
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
         addr = (addr - 0x2000) % 0xF00;
         bool mirroring = rom.flag_6 & 0x01;
         if (mirroring) { // Vertical (horizontal arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
				   *data = ppu.vram[addr];
            if (addr >= 0x0400 && addr <= 0x07FF)
               *data = ppu.vram[addr + 1024];
            if (addr >= 0x0800 && addr <= 0x0BFF)
               *data = ppu.vram[addr];
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               *data = ppu.vram[addr + 1024];
         } else { // horizontal (vertical arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
               *data = ppu.vram[addr];
            if (addr >= 0x0400 && addr <= 0x07FF)
               *data = ppu.vram[addr];
            if (addr >= 0x0800 && addr <= 0x0BFF)
               *data = ppu.vram[addr + 1024];
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               *data = ppu.vram[addr + 1024];
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
         //ppu.vram[(addr - 0x2000) % 2048] = *data;
         // VRAM
         addr = (addr - 0x2000) % 0xF00;
         bool mirroring = rom.flag_6 & 0x01;
         if (mirroring) { // Vertical (horizontal arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
				   ppu.vram[addr] = *data;
            if (addr >= 0x0400 && addr <= 0x07FF)
               ppu.vram[addr + 1024] = *data;
            if (addr >= 0x0800 && addr <= 0x0BFF)
               ppu.vram[addr] = *data;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               ppu.vram[addr + 1024] = *data;
         } else { // horizontal (vertical arrangement)
            if (addr >= 0x0000 && addr <= 0x03FF)
               ppu.vram[addr] = *data;
            if (addr >= 0x0400 && addr <= 0x07FF)
               ppu.vram[addr] = *data;
            if (addr >= 0x0800 && addr <= 0x0BFF)
               ppu.vram[addr + 1024] = *data;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
               ppu.vram[addr + 1024] = *data;
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
      
      /*
      Reflects the state of PPU
      CPU often reads this register when trying to draw to screen.
      There is a bit that tells the CPU it is safe to draw
      From https://www.nesdev.org/wiki/PPU_registers
      7  bit  0
      ---- ----
      VSO. ....
      |||| ||||
      |||+-++++- PPU open bus. Returns stale PPU bus contents.
      ||+------- Sprite overflow. The intent was for this flag to be set
      ||         whenever more than eight sprites appear on a scanline, but a
      ||         hardware bug causes the actual behavior to be more complicated
      ||         and generate false positives as well as false negatives; see
      ||         PPU sprite evaluation. This flag is set during sprite
      ||         evaluation and cleared at dot 1 (the second dot) of the
      ||         pre-render line.
      |+-------- Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 overlaps
      |          a nonzero background pixel; cleared at dot 1 of the pre-render
      |          line.  Used for raster timing.
      +--------- Vertical blank has started (0: not in vblank; 1: in vblank).
               Set at dot 1 of line 241 (the line *after* the post-render
               line); cleared after reading $2002 and at dot 1 of the
               pre-render line.
      */
      case PPUSTATUS: // $2002 PPUSTATUS
      /* TODO: address latch needs to be cleared. Which is used by PPUSCROLL and PPUADDR */
      *data = ppu.ppu_regs[PPUSTATUS];
      //ppu.vram_addr = 0;
      CLEAR_VERTICAL_BLANK()
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
      if (ppu.vram_addr >= 0x3F00)
         ppu_read(ppu.vram_addr, data);
      else {
         *data = ppu.data_buffer;
         ppu_read(ppu.vram_addr, &ppu.data_buffer); // Check
      }
      log_byte("Address Read", ppu.vram_addr);
      log_byte("Data", (uint16_t) (*data));
      log_state();
      ppu.vram_addr += (VRAM_INCREMENT ? 32 : 1);
      break;

      default:
      return false;
   }
   return true;
}

bool cpu_ppu_write(uint16_t addr, uint8_t *data) {
   uint8_t *oam = (uint8_t *) ppu.OAM;
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
      ppu.oam_addr = *data;
      break;

      case OAMDATA: // $2004 OAMDATA
      oam[ppu.oam_addr] = *data;
      ppu.oam_addr++;
      break;

      case PPUSCROLL: // $2005 PPUSCROLL

      break;

      case PPUADDR: // $2006 PPUADDR
      // Check
      log_byte("Vram update Address", ppu.vram_addr);
      log_byte("Data", (uint16_t) (*data));
      ppu.vram_addr = (ppu.vram_addr << 8) | *data;
      ppu.vram_addr &= 0x3FFF;
      break;

      case PPUDATA: // $2007 PPUDATA
      log_byte("Address", ppu.vram_addr);
      log_byte("Data", (uint16_t) (*data));
      ppu_write(ppu.vram_addr, data); // Check
      ppu.vram_addr += (VRAM_INCREMENT ? 32 : 1);
      log_byte("Address", ppu.vram_addr);
      log_byte("Data", (uint16_t) (*data));
      log_namespace();
      log_second_namespace();
      break;

      default:
      return false;
   }
   return true;
}

void ppu_dma(bool odd) {
   if (odd) {
      uint16_t addr = ((uint16_t) ppu.dma_page) << 8 | ppu.dma_addr;
      cpu_read(addr, &ppu.dma_buffer);
   } else {
      uint8_t *oam = (uint8_t *) ppu.OAM;
      oam[ppu.dma_addr] = ppu.dma_buffer;
      ppu.dma_addr++;
      if (ppu.dma_addr == 0x00) { // When we wrap around 0xFF bytes have been copied
         ppu.dma = false;
         ppu.dma_starting = false;
      }
   }
}

void ppu_clock() {
   if (ppu.scanline == -1) {
      CLEAR_VERTICAL_BLANK()
   }
   ppu.cycles++;

   if (ppu.cycles >= DOTS) {
      ppu.scanline++;
      ppu.cycles = 0;
   }
   if (ppu.scanline == SCREEN_HEIGHT && ppu.cycles == 0) {
      /*
      PPU has a flag (which can be set by CPU) when it should not perform
      a non maskable interrupt (NMI)
      */
      if (IS_NMI_ENABLED())
         cpu_nmi();
      nametable_to_buffer();
      oam_to_buffer();
      nes_screen();
      SET_VERTICAL_BLANK()
      ppu.framecount++;
   } else if (ppu.scanline >= SCANLINES) {
      ppu.scanline = -1;
      log_namespace();
   }
}