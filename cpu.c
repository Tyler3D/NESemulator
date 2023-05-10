#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "logger.h"
#include "controller.h"

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
    uint16_t mapped_addr;
    //printf("Attempting to read %X\n", addr);
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        *data = cpu.memory[addr & 0x07FF]; // Reading from RAM
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        return cpu_ppu_read((addr - 0x2000) % 0x08, data); // NES PPU registers
        //*data = cpu.ppu_regs[(addr - 0x2000) % 8];
    } else if (addr >= 0x4000 && addr <= 0x4017) {
        if (addr == 0x4016) {
            *data = readController(&player1);
            printf("READING CONTROLLER 1: %X byte %X for button %X\n", player1.buttons, *data, player1.bitCounter);
            fwrite("Reading controller\n", sizeof(char), strlen("Reading controller\n"), logfp);
        }
        else if (addr == 0x4017)
            *data = readController(&player2);
        else
            *data = cpu.apu_io_regs[addr - 0x4000];
    } else if (addr >= 0x4018 && addr <= 0x401F) {
        // Need to figure out what this
        return true;
    } else if (addr >= 0x4020 && addr <= 0xFFFF) {
        if (!rom.readCPUMapper(addr, &mapped_addr))
            return false;
        *data = rom.PRG_ROM_data[mapped_addr];
    } else
        return false;
    //printf("Read data %X\n", *data);
    return true;
}

// Return true if written
// Returns false if cannot write
bool cpu_write(uint16_t addr, uint8_t *data) {
    //uint32_t mapped_addr;
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        cpu.memory[addr & 0x07FF] = *data; // Writing to RAM
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        return cpu_ppu_write((addr - 0x2000) % 0x08, data); // NES PPU registers
        //cpu.ppu_regs[(addr - 0x2000) % 8] = *data;
    } else if (addr >= 0x4000 && addr <= 0x4017) {
        if (addr == 0x4016) {
            poll_controllers(data);
            printf("Polling controller with data %X\n", *data);
            fwrite("Polling controller\n", sizeof(char), strlen("Polling controller\n"), logfp);
        }
        else
            cpu.apu_io_regs[addr - 0x4000] = *data;
    } else if (addr >= 0x4018 && addr <= 0x401F) {
        // Need to figure out what this
        return true;
    } else if (addr >= 0x4020 && addr <= 0xFFFF) {
        // For other mappers
        // Might need different method than using rom.readCPUMapper
        printf("Writing to rom? Sus???\n");
        return false;
        //if (!rom.readCPUMapper(addr, &mapped_addr))
        //    return false;
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
    /* Load reset vector */
	uint8_t lo, hi = 0;
    if (!cpu_read(0xFFFC, &lo) || !cpu_read(0xFFFD, &hi)) {
        printf("Could not read reset vector\n");
        cpu.fail();
    }

	cpu.pc = (hi << 8) | lo;
    //cpu.pc = 0xC000;
    //cpu.cycles = 0;
    /*
    if (!cpu_read(0xFFFE, &lo) || !cpu_read(0xFFFF, &hi)) {
        printf("Could not read IRQ/BRK vector\n");
        cpu.fail();
    }

    cpu.irq = (hi << 8) | lo;
    */

	// Reset registers
    cpu.asm_args = (char *) malloc(128 * sizeof(char));
	cpu.a = 0;
	cpu.x = 0;
	cpu.y = 0;
	cpu.sp = (uint8_t) 0xFD; // Stack addresses 0x100-0x1ff in memory
	cpu.status = 0x00 |  always_on_flag | irq; // https://www.nesdev.org/wiki/Status_flags#The_B_flag
    cpu.cycles = 0x07;

    // PPU Start up
    ppu.ppu_regs[2] = 0x10;
    //cpu.ppu_regs[2] = 0x10;//(1 << 7) | (1 << 5);
}

void cpu_nmi() {
    char buf[256];
    sprintf(buf, "Frame: %d, Scanline %d, Cycles: %d\n", ppu.framecount, ppu.scanline, ppu.cycles);
    printf("Frame: %d, Scanline %d, Cycles: %d\n", ppu.framecount, ppu.scanline, ppu.cycles);
    fwrite(buf, sizeof(char), strnlen(buf, 256), logfp);
    uint16_t returnAddr = cpu.pc; // check if -1 or not
    cpu.low = returnAddr & 0xFF;
    cpu.high = (returnAddr >> 8) & 0xFF;
    push(&cpu.high);
    push(&cpu.low);
    push(&cpu.status);
    cpu.status &= ~irq;
    
    if (!cpu_read(0xFFFA, &cpu.low) || !cpu_read(0xFFFB, &cpu.high)) {
        printf("Could not read NMI vector\n");
        cpu.fail();
    }

    cpu.pc = (cpu.high << 8) | cpu.low;
}

uint8_t cpu_clock() {
    /*
        Using this reference: https://www.nesdev.org/wiki/CPU_unofficial_opcodes
        It looks like the last 2 bits of the opcode determine which "parts" of the CPU work
        Whether to use control instructions (00), the ALU (01), or read/write instructions (10)
        The next three bits appear to determine the addressing mode
        We split opcodes into groups so we can generalize instructions and avoid
        rewriting code
    */

    cpu.logPC = cpu.pc;
    cpu.logCycles = cpu.cycles;
    READ_BYTE_PC(cpu.opcode);
    cpu.high = 0;
    cpu.low = 0;

    if ((cpu.opcode % 4) == 0) {
        // Control instructions
        //printf("handle Control 0x%x\n", cpu.opcode);
        handleControl();
    } else if ((cpu.opcode % 4) == 1) {
        // ALU instructions
        //printf("handle ALU 0x%x\n", cpu.opcode);
        handleALU();
    } else if ((cpu.opcode % 4) == 2) {
        // RMW operations
        //printf("handle RMW 0x%x\n", cpu.opcode);
        handleRMW();
    } else {
        // Illegal instructions
        //printf("Illegal instruction\n");
        cpu.fail();
    }
    log_state();
    /*
    Currently PC: D959 gives the wrong result
    D959  B1 FF     LDA ($FF),Y = 0146 @ 0245 = 12  A:01 X:65 Y:FF P:E5 SP:FA PPU: 77,215 CYC:8824
    A is supposed to become 12 after this.
    But LDA $0146 is 0 in this implemention
    We are logging page 1 for each instruction to debug this
    */
    //log_page(0);
    //log_page(1);
    //log_page(2);
    //log_page(3);
    //printf("CPU.A: %X, CPU:PC %X\n", cpu.a, cpu.pc);
    return cpu.cycles - cpu.logCycles;
}