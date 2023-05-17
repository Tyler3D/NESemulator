#include "logger.h"
#include "cpu.h"
#include "ppu.h"

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
    return;
    char buf[128];
    switch (cpu.asm_argc) {
        case 3:
        sprintf(buf, "%-5X %-2X %-2X %-2X  %-3s %-33s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %d\n", cpu.logPC, \
        cpu.opcode, cpu.low, cpu.high, cpu.instruction, cpu.asm_args, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.logCycles);
        break;
        case 2:
        sprintf(buf, "%-5X %-2X %-6X %-3s %-33s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %d\n", cpu.logPC, \
        cpu.opcode, cpu.low, cpu.instruction, cpu.asm_args, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.logCycles);
        break;
        case 1:
        sprintf(buf, "%-5X %-9X %-37s A:%-2X X:%-2X Y:%-2X Status:%-2X SP:%-2X CYC: %d\n", cpu.logPC, \
        cpu.opcode, cpu.instruction, cpu.a, cpu.x, cpu.y, cpu.status, cpu.sp, cpu.logCycles);
        break;
        default:
        printf("Bro what... log fail\n");
        cpu.fail();
        break;
    }
    fwrite(buf, sizeof(char), strnlen(buf, 128), logfp);
    //fwrite(buf, sizeof(char), strnlen(buf, 128), stdout);
}

void log_fail() {

}

void log_page(uint16_t pageNum) {
    char buf[0x100];
    for (uint16_t i = (pageNum << 8); i < ((pageNum + 1) << 8); i++) {
        if ((i % 0x10) == 0) {
            sprintf(buf, "\n0x%-4X ", i);
            fwrite(buf, sizeof(char), strnlen(buf, 128), logfp);
            fwrite(buf, sizeof(char), strnlen(buf, 128), stdout);
        }
        uint8_t byte;
        cpu_read(i, &byte);
        sprintf(buf, "%2X ", byte);
        fwrite(buf, sizeof(char), strnlen(buf, 128), logfp);
        fwrite(buf, sizeof(char), strnlen(buf, 128), stdout);
    }
    sprintf(buf, "\n");
    fwrite(buf, sizeof(char), strnlen(buf, 128), logfp);
    fwrite(buf, sizeof(char), strnlen(buf, 128), stdout);
}

void log_namespace() {
    return;
    char buf[4096] = {0};
    for (uint8_t y = 0; y < 30; y++) {
        for (uint8_t x = 0; x < 32; x++) {
            sprintf(buf + strlen(buf), "%-2x ", ppu.vram[(y * 32) + x]);
        }
        sprintf(buf + strlen(buf), "\n");
    }
    fwrite("Printing namespace\n", sizeof(char), strlen("Printing namespace\n"), logfp);
    fwrite(buf, sizeof(char), strlen(buf), logfp);
}

void log_second_namespace() {
    return;
    char buf[4096] = {0};
    for (uint8_t y = 0; y < 30; y++) {
        for (uint8_t x = 0; x < 32; x++) {
            sprintf(buf + strlen(buf), "%-2x ", ppu.vram[(y * 32) + x + 1024]);
        }
        sprintf(buf + strlen(buf), "\n");
    }
    fwrite("Printing namespace\n", sizeof(char), strlen("Printing namespace\n"), logfp);
    fwrite(buf, sizeof(char), strlen(buf), logfp);
}


void log_oam() {
    return;
    char buf[4096] = {0};
    uint8_t *oam = (uint8_t *) ppu.OAM;
    for (uint8_t y = 0; y < 16; y++) {
        for (uint8_t x = 0; x < 16; x++) {
            sprintf(buf + strlen(buf), "%-2x ", oam[(y * 32) + x]);
        }
        sprintf(buf + strlen(buf), "\n");
    }
    fwrite("Printing OAM\n", sizeof(char), strlen("Printing OAM\n"), logfp);
    fwrite(buf, sizeof(char), strlen(buf), logfp);
}

/*
void log_OAM() {
    char buf[4096] = {0};
    uint8_t *oam = (uint8_t *) ppu.OAM;
    for (uint8_t y = 0; y < 16; y++) {
        for (uint8_t x = 0; x < 16; x++) {
            sprintf(buf + strlen(buf), "%-2x ", oam[(y * 32) + x]);
        }
        sprintf(buf + strlen(buf), "\n");
    }
    fwrite("Printing OAM\n", sizeof(char), strlen("Printing OAM\n"), logfp);
    fwrite(buf, sizeof(char), strlen(buf), logfp);
}
*/


void log_pixels() {
    return;
    char buf[4096000] = {0};
    for (int y = 0; y < SCREEN_HEIGHT; y ++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x ++)
        {
        // 81, 245, 125
        //sprintf(buf + strlen(buf), "(%-2x %-2x %-2x)", buffer[y * SCREEN_WIDTH + x].r, buffer[y * SCREEN_WIDTH + x].b, buffer[y * SCREEN_WIDTH + x].g);
        sprintf(buf + strlen(buf), "%-2x", buffer[y * SCREEN_WIDTH + x].r);
        //setPixel(x, y, 81, 245, 125, 0);
        //setPixel(x, y, buffer[y * SCREEN_WIDTH + x].r, buffer[y * SCREEN_WIDTH + x].b, buffer[y * SCREEN_WIDTH + x].g, 0);
        // setPixel(x, y, screen[y][x].r, screen[y][x].b, screen[y][x].g, 0);
        }
        sprintf(buf + strlen(buf), "\n");
    }
    fwrite("Printing pixels\n", sizeof(char), strlen("Printing pixels\n"), logfp);
    fwrite(buf, sizeof(char), strlen(buf), logfp);
}

void log_byte(char *buf, uint16_t byte) {
    return;
    fflush(logfp);
    char buffer[4096];
    sprintf(buffer, "%s %x\n", buf, byte);
    fwrite(buffer, sizeof(char), strnlen(buffer, 4096), logfp);
}