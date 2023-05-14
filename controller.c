#include "controller.h"
#include "cpu.h"

struct controller player1 = {
    .buttons = 0,
    .isPolling = false,
    .bitCounter = 0,
};

struct controller player2 = {
    .buttons = 0,
    .isPolling = false,
    .bitCounter = 0,
};

void poll_controllers(uint8_t *data) {
    if (((*data) & 0x01) > 0) { // Only care about strobe bit
        player1.bitCounter = 0;
        player2.bitCounter = 0;
        player1.strobeBit = true;
        player2.strobeBit = true;
        printf("Strobe bit on\n");
    } else {
        player1.strobeBit = false;
        player2.strobeBit = false;
        printf("Strobe bit off\n");
    }
    if (player1.isPolling) {
        player1.isPolling = false;
        player2.isPolling = false;
        return;
    }
    // Somehow poll controllers through usb keyboard
    // controller_byte = whatever
    //if (player1.buttons > 0) {
    //    CLEAR_CONTROLLER(player1.buttons);
    //} else {
    //SET_START_BUTTON(player1.buttons);
        //SET_A_BUTTON(player1.buttons);
    //}
    //printf("Polling controllers %X\n", player1.buttons);
    player1.isPolling = true;
    player2.isPolling = true;
}

uint8_t readController(struct controller *player) {
    if (player->strobeBit)
        return player->buttons & 0x01;
    if (player->bitCounter >= 8)
        return 1;
    uint8_t returned = (((player->buttons & (1 << player->bitCounter))) > 0) ? 1 : 0;
    player->bitCounter++;
    return returned;
}
