#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "rom.h"
#include "ppu.h"
#include "logger.h"
#include "fb.h"

FILE *fp;

void fail() {
    // Print nes test
    //printf("NES TEST RESULTS\n");
    //uint8_t byte;
    //READ_BYTE_FROM_ADDR(0x02, byte)
    //printf("Branch tests %X\n", byte);
    //READ_BYTE_FROM_ADDR(0x03, byte)
    //printf("Other tests %X\n", byte);

    free(ppu.vram);
    free(cpu.memory);
    if (rom.CHR_ROM_SIZE > 0)
        free(rom.CHR_ROM_data);
    free(rom.PRG_ROM_data);
    free(cpu.asm_args);
    fclose(logfp);
    fclose(fp);
    exit(1);
}

int main(int argc, char **argv) {
    long long clocks = 0;
    if (argc != 2) {
        printf("Usage: %s <game.nes>\n", argv[0]);
        return 0;
    }

    //if (!fbopen()) {
    //    printf("Frame buffer isn't open\n");
    //    return 0;
    //}

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("Could not open file\n");
        return 0;
    }

    if (!log_init()) {
        printf("Could not start logger\n");
        fclose(fp);
        return 0;
    }

    if (fread(rom.header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
        printf("Could not read header\n");
        goto fileEnd;
    }

    if (strncmp((char *) rom.header, "NES", 3) != 0) {
        printf("Missing NES at Start of Header\n");
        goto fileEnd;
    }

    rom.PRG_ROM_SIZE = rom.header[4];
    rom.CHR_ROM_SIZE = rom.header[5];
    rom.flag_6 = rom.header[6];
    rom.flag_7 = rom.header[7];

    printf("Size of PRG_ROM %d\n", rom.PRG_ROM_SIZE);
    printf("Size of CHR_ROM %d\n", rom.CHR_ROM_SIZE);
    /*
    if ((rom.flag_6 & (1 << 2)) > 0) { 
        // 512-byte trainer present
        printf("Trainer present\n");
        if (fread(rom.trainer, 1, HEADER_SIZE, fp) != TRAINER_SIZE) {
            printf("Could not read trainer\n");
            goto fileEnd;
        }
    } else
        printf("Trainer not present\n");
    */

   if ((rom.flag_6 & 0x04) > 0) {
    // Skip trainer
    fseek(fp, 512, SEEK_CUR);
   }

    if (!(rom.PRG_ROM_data = calloc(16384 * rom.PRG_ROM_SIZE, sizeof(uint8_t)))) {
        printf("Calloc failed\n");
        goto fileEnd;
    }
    //printf("read: %lu\n", fread(rom.PRG_ROM_data, 16384, rom.PRG_ROM_SIZE, fp));
    if (fread(rom.PRG_ROM_data, 16384, rom.PRG_ROM_SIZE, fp) != rom.PRG_ROM_SIZE) {
        printf("Could not read PRG_ROM\n");
        goto rom_fail;
    }

    if (rom.CHR_ROM_SIZE > 0) {
        if (!(rom.CHR_ROM_data = calloc(8192 * rom.CHR_ROM_SIZE, sizeof(uint8_t)) )) {
            printf("Calloc failed\n");
            free(rom.PRG_ROM_data);
            goto fileEnd;
        }
        
        if (fread(rom.CHR_ROM_data, 8192, rom.CHR_ROM_SIZE, fp) != rom.CHR_ROM_SIZE) {
            printf("Could not read CHR_ROM\n");
            goto rom_fail;
        }
    }

    if(!setMapper())
        goto rom_fail;
    
    if (!(cpu.memory = calloc(2048, sizeof(uint8_t)))) { // 2K internal Ram
        printf("Calloc failed\n");
        goto rom_fail;
    }

    if (!(ppu.vram = calloc(2048, sizeof(uint8_t)))) { // 2K internal Ram
        printf("Calloc failed\n");
        goto ppu_fail;
    }

    cpu.fail = &fail;
    cpu_reset();
    ppu_reset();

    uint8_t deltaCycles = 0;

    while (1) {
        // Need to add timing
        //printf("clock count %lld\n", clocks);
        if ((clocks % 3) == 0) {
            // Run CPU
            if (!ppu.dma)
                deltaCycles = cpu_clock();
            else {
                /*
                DMA is ongoing
                During this process the CPU is suspended
                The CPU reads an entire page (256 bytes)
                and fills up the OAM buffer on the PPU
                The process only starts on an even CPU clock cycle
                So we have dma_starting to tell us when we start
                ppu_dma will handle the dma per clock
                On an even CPU clock, we read byte
                On an odd CPU clock, we store the byte in the OAM buffer
                */
                //printf("DMA start %x\n", ppu.dma_starting);
                if ((ppu.dma_starting == 0) && ((clocks % 2) == 1)) {
                // Start dma on even clock cycle
                    //printf("DMA START\n");
                    ppu.dma_starting = true;
                    //log_page(2);
                } else if (ppu.dma_starting) {
                    ppu_dma((clocks % 2 == 1) ? 1 : 0);
                }
                deltaCycles = 1;
                cpu.cycles++;
            }
        }
        for (int i = 0; i < deltaCycles * 3; i++) {
            ppu_clock();
        }
        deltaCycles = 0;
        clocks++;
    }

    ppu_fail:
    free(cpu.memory);
    rom_fail:
    if (rom.CHR_ROM_SIZE > 0)
        free(rom.CHR_ROM_data);
    free(rom.PRG_ROM_data);

    fileEnd:
    fclose(fp);
    fclose(logfp);
    return 0;
}