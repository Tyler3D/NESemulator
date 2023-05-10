#ifndef _LOGGER_H
#define _LOGGER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

FILE *logfp;

bool log_init();
void log_state();
void log_page(uint16_t);

#endif