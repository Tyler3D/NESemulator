#ifndef _CPU_H
#define _CPU_H
#include <stdbool.h>
#include <stdint.h>

struct CPU {
    uint8_t x;
    uint8_t y;
    uint8_t a;
    uint16_t pc;
    uint8_t status;
    uint8_t sp;
    uint8_t *memory;
    uint8_t ppu_regs[8];
    uint8_t apu_io_regs[0x18];
    //uint32_t cycles;
    void (*fail)();
} cpu;

bool cpu_read(uint16_t addr, uint8_t *data);
bool cpu_write(uint16_t addr, uint8_t *data);
void cpu_clock();
void cpu_reset();

#endif