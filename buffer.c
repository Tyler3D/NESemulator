#include "ppu.h"
#include "logger.h"

// Implement later; for now just send b/w
pixel convert_rgb(uint8_t hue, uint8_t value, uint8_t prio) {
	pixel rgb;
	rgb.r = value * 255 / 3;
	rgb.g = value * 255 / 3;
	rgb.b = value * 255 / 3;
	rgb.prio = prio;
	return rgb;
}

void pixel_to_buffer(pixel pixel, uint16_t x, uint16_t y) {
	//if (buffer[y * SCREEN_WIDTH + x].prio <= pixel.prio){
		buffer[y * SCREEN_WIDTH + x] = pixel;
	//}
}

// Hardcoded 8x8 tile get that puts it into buffer
// Palette not implemented
// Rotation not yet implemented
void tile_to_buffer(uint16_t addr, uint8_t palette, uint8_t prio, uint8_t x, uint8_t y, uint8_t rotation) {
	uint8_t x_offset = 0;
	uint8_t y_offset = 0;
	for (int i = addr; i < addr + 8; i++) {
		uint8_t value;
		uint8_t left;
		uint8_t right;

		ppu_read(i, &left);
		ppu_read(i + 8, &right);
		for (int j = 7; j >= 0; j--) {
			uint8_t bit_left = (left >> j) & 1;
			uint8_t bit_right = (right >> j) & 1;
			if (bit_left & bit_right) {
				value = 3;
			} else if (bit_right) {
				value = 2;
			} else if (bit_left) {
				value = 1;
			} else 
				value = 0;
			// Note palette is set to 0 for temporary purposes
			//if (value > 0) {
			pixel rgb = convert_rgb(0, value, prio);
			pixel_to_buffer(rgb, x + x_offset, y + y_offset);
			//}
			x_offset++;
		}
		// Note palette is set to 0 for temporary purposes
		// Implement rotation here
		x_offset = 0;
		y_offset++;
	}
}

	// Implementing rotation
	// 		// No swap at all
	// 	case 0:
	// 		tile_to_buffer(sprite_addr, sprite && 0x3, priority, x, y, rotation);
	// 		break;

	// 	// Horizontal rotation
	// 	case 1:
	// 		tile_to_buffer(sprite_addr, sprite && 0x3, priority, x, y, rotation);
	// 		break;

	// 	// Vertical rotation
	// 	case 2:
	// 		tile_to_buffer(sprite_addr, sprite && 0x3, priority, x, y, rotation);
	// 		break;

	// 	// Horizontal and vertical rotation
	// 	case 3:
	// 		tile_to_buffer(sprite_addr, sprite && 0x3, priority, x, y, rotation);
	// 		break;
	// }

// Specific to sprites; background code has something else
// Also for 16x8 mode we use size; the selection of tile/rotation differs
void sprite_to_buffer(uint8_t y, uint8_t id, uint8_t attributes, uint8_t x) {
	uint16_t sprite_addr = (id << 4) + (SPRITE_PATTERN_ADDR ? 0x1000 : 0x0000);
	printf("Sprite addr %X\n", sprite_addr);
	uint8_t rotation = attributes >> 6;
	uint8_t priority = (attributes >> 5) & 0x1;
	printf("Sprite addr %X\n", sprite_addr);
	tile_to_buffer(sprite_addr, attributes & 0x3, (priority == 0) ? -1 : 1, x, y, rotation);
}

void oam_to_buffer() {
	log_oam();
	fflush(logfp);
	for (int i = 0; i < 64; i++) {
		if (ppu.OAM[i].id == 0xa2) {
			printf("Printing 0xA2\n");
			sprite_to_buffer(ppu.OAM[i].y, ppu.OAM[i].id, ppu.OAM[i].attributes, ppu.OAM[i].x);
		}
	}
}

void nametable_to_buffer() {
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t byte;
	// Note: addr calculation is nametable_base + background_base + nametable_byte;
	uint16_t addr;
	for (int i = 0; i < 960; i++) { // Need to change for scroll
		ppu_read(0x2000 + i, &byte);
		// Calculate addr later!
		addr = byte << 4;
		// Addr, palette, prio, x, y, rotation
		// Palette needs to be implemented; prio and rotation are fixed
		//tile_to_buffer((0x19 << 4) + (BACKGROUND_PATTERN_ADDR ? 0x1000 : 0x0000), 0, 1, x, y, 0);
		tile_to_buffer(addr + (BACKGROUND_PATTERN_ADDR ? 0x1000 : 0x0000), 0, 1, x * 8, y * 8, 0);
		x++;
		if (x >= 32) {
			x = 0;
			y++;
		}
	}
}