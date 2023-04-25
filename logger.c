#include "logger.h"
#include "cpu.h"
#include <time.h>
#include <string.h>

bool log_init() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char buf[512];
    sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d.txt", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    logfp = fopen(buf, "w+");
    if (!logfp)
        return false;
    return true;
}

/*
Simple logger to keep track of emulator state
Usage: 

Will produce output like this
C000  4C F5 C5  JMP $C5F5                             A:FF X:0  Y:0  Status:24 SP:FD CYC: 7
C000  A2 00     LDX #$00                              A:FF X:0  Y:0  Status:24 SP:FD CYC: 7
C000  EA        NOP                                   A:FF X:0  Y:0  Status:24 SP:FD CYC: 7
*/

void log_state() {
    char buf[128];
    switch (cpu.asm_argc) {
        case 3:
        sprintf(buf, "%-5X %-2X %-2X %-2X  %-3s %-33s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %X\n", cpu.pc, \
        cpu.opcode, cpu.low, cpu.high, cpu.instruction, cpu.asm_args, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.cycles);
        break;
        case 2:
        sprintf(buf, "%-5X %-2X %-6X %-3s %-33s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %X\n", cpu.pc, \
        cpu.opcode, cpu.low, cpu.instruction, cpu.asm_args, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.cycles);
        break;
        case 1:
        sprintf(buf, "%-5X %-9X %-37s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %X\n", cpu.pc, \
        cpu.opcode, cpu.instruction, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.cycles);
        break;
        default:
        printf("Bro what... log fail\n");
        cpu.fail();
        break;
    }
    fwrite(buf, sizeof(char), strnlen(buf, 128), logfp);
    fwrite(buf, sizeof(char), strnlen(buf, 128), stdout);
}

void log_fail() {

}