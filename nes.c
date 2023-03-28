#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "rom.h"

int main(int argc, char **argv) {
    FILE *fp;
    if (argc != 2) {
        printf("Usage: %s <game.nes>\n", argv[0]);
        return 0;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
        printf("Could not open file\n");

    if (fread(rom.header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
        printf("Could not read header\n");
        goto fileEnd;
    }

    if (strncmp((char *) rom.header, "NES", 4) != 0)
        printf("Missing NES at Start of Header\n");

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

    if (!(rom.PRG_ROM_data = malloc(16384 * rom.PRG_ROM_SIZE))) {
        printf("Malloc failed\n");
        goto fileEnd;
    }

    if (fread(rom.PRG_ROM_data, 16384, rom.PRG_ROM_SIZE, fp) != 16384 * rom.PRG_ROM_SIZE) {
        printf("Could not read PRG_ROM\n");
        goto fail;
    }

    if (rom.CHR_ROM_SIZE > 0) {
        if (!(rom.CHR_ROM_data = malloc(8192 * rom.CHR_ROM_SIZE))) {
            printf("Malloc failed\n");
            free(rom.PRG_ROM_data);
            goto fileEnd;
        }
        
        if (fread(rom.CHR_ROM_data, 8192, rom.CHR_ROM_SIZE, fp) != 8192 * rom.CHR_ROM_SIZE) {
            printf("Could not read CHR_ROM\n");
            goto fail;
        }
    }

    if(!setMapper())
        goto fail;
    
    if (!(cpu.memory = malloc(0x07FF))) { // 2K internal Ram
        printf("Malloc failed\n");
        goto fail;
    }


    //ppuFail:
    free(cpu.memory);
    fail:
    if (rom.CHR_ROM_SIZE > 0)
        free(rom.CHR_ROM_data);
    free(rom.PRG_ROM_data);

    fileEnd:
    fclose(fp);
    return 0;
}