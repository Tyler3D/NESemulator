#ifndef _CPU_H
#define _CPU_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // Debugging

#define SET_ZERO_FLAG(X) {\
    if (!(X))\
        cpu.status |= zero;\
    else\
        cpu.status &= ~zero;}

#define SET_NEG_FLAG(X) {\
    if (((X) & 0x80) > 0)\
        cpu.status |= negative;\
    else\
        cpu.status &= ~negative;}

// https://forums.nesdev.org/viewtopic.php?t=6331
#define SET_OVERFLOW_FLAG(X, BYTE, CARRY) {\
    if (!(((X) ^ (BYTE)) & 0x80) && (((X) ^ ((X) + (BYTE) + (CARRY))) & 0x80))\
        cpu.status |= overflow;\
    else\
        cpu.status &= ~overflow;}

#define SET_CARRY_FLAG(X) {\
    if ((X))\
        cpu.status |= carry;\
    else\
        cpu.status &= ~carry;}

#define GET_PAGE_NUM(X) (((X) % 0xFF00) >> 8)

#define READ_BYTE_FROM_ADDR(ADDR, X) {\
    if (!cpu_read((ADDR), &(X))) {\
        printf("Could not read byte at %X\n", ADDR);\
        cpu.fail();}}

#define READ_BYTE(X) READ_BYTE_FROM_ADDR(cpu.pc, X)

#define READ_BYTE_PC(X) {\
    READ_BYTE(X)\
    cpu.pc++;}

#define READ_WORD_PC() {\
    READ_BYTE_PC(cpu.low)\
    READ_BYTE_PC(cpu.high)}

#define SET_ADDR(ADDR, OFFSET) (ADDR) = ((((uint16_t) cpu.high) << 8) | (uint16_t) cpu.low) + OFFSET;

#define IS_FLAG_ON(flag) ((cpu.status & flag) > 0)
#define IS_FLAG_OFF(flag) ((cpu.status & flag) == 0)

struct CPU {
    uint8_t x;
    uint8_t y;
    uint8_t a;
    uint16_t pc;
    uint8_t status;
    uint8_t sp;
    uint8_t *memory;
    uint8_t apu_io_regs[0x18];
    uint32_t cycles;
    uint32_t logCycles;
    uint16_t logPC;
    uint8_t opcode;
    uint8_t low;
    uint8_t high;
    uint8_t asm_argc;
    char *instruction;
    char *asm_args;
    void (*fail)();
    bool nmi;
} cpu;

enum flags{
    carry = 1 << 0,
    zero = 1 << 1,
    irq = 1 << 2,
    decimal = 1 << 3,
    b_flag = 1 << 4,
    always_on_flag = 1 << 5, // https://www.nesdev.org/wiki/Status_flags#The_B_flag
    overflow = 1 << 6,
    negative = 1 << 7
};

bool cpu_read(uint16_t addr, uint8_t *data);
bool cpu_write(uint16_t addr, uint8_t *data);
uint8_t cpu_clock();
void cpu_reset();
void handleControl();
void handleALU();
void handleRMW();
void push(uint8_t *byte);
void pull(uint8_t *byte);

#endif