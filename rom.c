#include <stdio.h>
#include "rom.h"
/*
    Docs from https://www.nesdev.org/wiki/NROM
    Implement Mapper_000
    PRG ROM size: 16 KiB for NROM-128, 32 KiB for NROM-256 (DIP-28 standard pinout)
    PRG ROM bank size: Not bankswitched
    PRG RAM: 2 or 4 KiB, not bankswitched, only in Family Basic (but most emulators provide 8)
    CHR capacity: 8 KiB ROM (DIP-28 standard pinout) but most emulators support RAM
    CHR bank size: Not bankswitched, see CNROM
    CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch
    CPU $8000-$BFFF: First 16 KB of ROM.
    CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
    Nevertheless, tile animation can be done by swapping between pattern tables $0000 and $1000
*/

bool mapper000_cpuRead(uint16_t addr, uint32_t *mapped_addr) {
    // Don't care about Family basic PRG RAM
    // Just read ROM data
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		*mapped_addr = addr & (rom.PRG_ROM_SIZE == 1 ? 0x3FFF : 0x7FFF);
		return true;
	}

	return false;
}

bool mapper000_ppuRead(uint16_t addr, uint32_t *mapped_addr) {
    // Mapper 000 uses Pattern table 0 and 1
    // Address <= 0x1FFF
    // PPUCTRL bits 4-3 controls the swapping
    // But for now we can yolo read
	if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		*mapped_addr = addr;
		return true;
	}
	return false;
}

bool setMapper() {
    rom.mapperID = (((rom.flag_6 >> 4) << 4) | (rom.flag_7 >> 4));
    printf("Using mapper %u\n", rom.mapperID);
    switch(rom.mapperID) {
        case 0:
            rom.readCPUMapper = &mapper000_cpuRead;
            rom.readPPUMapper = &mapper000_ppuRead;
            return true;
        default:
            printf("Unknown mapper\n");
            return false;
    }
}