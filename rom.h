#ifndef _ROM_H
#define _ROM_H
#include <stdint.h>
#include <stdbool.h>
#define HEADER_SIZE 16
#define TRAINER_SIZE 512

struct ROM {
    uint8_t header[16];
    uint8_t PRG_ROM_SIZE;
    uint8_t CHR_ROM_SIZE;
    uint8_t flag_6;
    uint8_t flag_7;
    //uint8_t trainer[TRAINER_SIZE];
    uint8_t *PRG_ROM_data;
    uint8_t *CHR_ROM_data;
    uint8_t mapperID;
    bool (*readCPUMapper)(uint16_t, uint16_t *);
    bool (*readPPUMapper)(uint16_t, uint16_t *);
} rom;

bool setMapper();
#endif