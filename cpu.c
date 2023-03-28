#include <stdio.h>
#include "rom.h"
#include "cpu.h"

/*
    NES CPU memory map
    https://www.nesdev.org/wiki/CPU_memory_map

    $0000–$07FF	$0800	2 KB internal RAM
    $0800–$0FFF	$0800	Mirrors of $0000–$07FF
    $1000–$17FF	$0800   Mirrors of $0000–$07FF
    $1800–$1FFF	$0800   Mirrors of $0000–$07FF
    $2000–$2007	$0008	NES PPU registers
    $2008–$3FFF	$1FF8	Mirrors of $2000–$2007 (repeats every 8 bytes)
    $4000–$4017	$0018	NES APU and I/O registers
    $4018–$401F	$0008	APU and I/O functionality that is normally disabled. See CPU Test Mode.
    $4020–$FFFF	$BFE0	Cartridge space: PRG ROM, PRG RAM, and mapper registers (see note)
*/

// Returns true if read
// False if out of bounds
bool cpu_read(uint16_t addr, uint8_t *data) {
    uint32_t mapped_addr;
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        *data = cpu.memory[addr & 0x07FF]; // Reading from RAM
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        *data = cpu.ppu_regs[(addr - 0x2000) % 8]; // NES PPU registers
    } else if (addr >= 0x4000 && addr <= 0x4017) {
        *data = cpu.apu_io_regs[addr - 0x4000];
    } else if (addr >= 0x4018 && addr <= 0x401F) {
        // Need to figure out what this
        return true;
    } else if (addr >= 0x4020 && addr <= 0xFFFF) {
        mapped_addr = rom.readCPUMapper(addr, &mapped_addr);
        *data = rom.PRG_ROM_data[mapped_addr];
    } else
        return false;
    return true;
}

// Return true if written
// Returns false if cannot write
bool cpu_write(uint16_t addr, uint8_t *data) {
    //uint32_t mapped_addr;
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cpu.memory[addr & 0x07FF] = *data; // Writing to RAM
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        cpu.ppu_regs[(addr - 0x2000) % 8] = *data; // NES PPU registers
    } else if (addr >= 0x4000 && addr <= 0x4017) {
        cpu.apu_io_regs[addr - 0x4000] = *data;
    } else if (addr >= 0x4018 && addr <= 0x401F) {
        // Need to figure out what this
        return true;
    } else if (addr >= 0x4020 && addr <= 0xFFFF) {
        // For other mappers
        // Might need different method than using rom.readCPUMapper
        printf("Writing to rom? Sus???\n");
        return false;
        //mapped_addr = rom.readCPUMapper(addr, &mapped_addr);
        //rom.PRG_ROM_data[mapped_addr] = *data;
    } else
        return false;
    return true;
}

void cpu_reset() {
    /*
    https://www.nesdev.org/wiki/CPU_memory_map
    The CPU expects interrupt vectors in a fixed place at the end of the cartridge space:
    $FFFA–$FFFB = NMI vector
    $FFFC–$FFFD = Reset vector
    $FFFE–$FFFF = IRQ/BRK vector

    Need to implement IRQ and NMI
*/
    // 6502 is little endian
	uint8_t lo, hi = 0;
    if (!cpu_read(0xFFFC, &lo) || !cpu_read(0xFFFD, &hi)) {
        printf("Could not read reset vector\n");
        cpu.fail();
    }
	cpu.pc = (hi << 8) | lo;

	// Reset registers
	cpu.a = 0;
	cpu.x = 0;
	cpu.y = 0;
	cpu.sp = 0; // Might need to be set to some value
	cpu.status = 0x00;
}

void cpu_clock() {
    uint8_t opcode;
    if (!cpu_read(cpu.pc, &opcode)) {
        printf("Could not read opcode\n");
        cpu.fail();
    }
}