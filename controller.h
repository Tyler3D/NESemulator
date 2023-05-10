#ifndef _CONTROLLER_H
#define _CONTROLLER_H
#include <stdbool.h>
#include <stdint.h>

struct controller {
    uint8_t buttons;
    uint8_t bitCounter;
    bool isPolling;
    bool strobeBit;
};

struct controller player1;

struct controller player2;

#define SET_A_BUTTON(X) (X) |= (1 << 0);
#define SET_B_BUTTON(X) (X) |= (1 << 1);
#define SET_SELECT_BUTTON(X) (X) |= (1 << 2);
#define SET_START_BUTTON(X) (X) |= (1 << 3);
#define SET_UP_BUTTON(X) (X) |= (1 << 4);
#define SET_DOWN_BUTTON(X) (X) |= (1 << 5);
#define SET_LEFT_BUTTON(X) (X) |= (1 << 6);
#define SET_RIGHT_BUTTON(X) (X) |= (1 << 7);

#define CLEAR_A_BUTTON(X) (X) &= ~(1 << 0);
#define CLEAR_B_BUTTON(X) (X) &= ~(1 << 1);
#define CLEAR_SELECT_BUTTON(X) (X) &= ~(1 << 2);
#define CLEAR_START_BUTTON(X) (X) &= ~(1 << 3);
#define CLEAR_UP_BUTTON(X) (X) &= ~(1 << 4);
#define CLEAR_DOWN_BUTTON(X) (X) &= ~(1 << 5);
#define CLEAR_LEFT_BUTTON(X) (X) &= ~(1 << 6);
#define CLEAR_RIGHT_BUTTON(X) (X) &= ~(1 << 7);
#define CLEAR_CONTROLLER(X) (X) &= 0;

void poll_controllers(uint8_t *data);
uint8_t readController(struct controller *player);

#endif