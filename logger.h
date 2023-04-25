#ifndef _LOGGER_H
#define _LOGGER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

FILE *logfp;

bool log_init();
void log_state();

#endif