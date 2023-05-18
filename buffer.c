#include "ppu.h"
#include "logger.h"

color system_pallete[64] = {
   {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E},
   {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00},
   {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E}, {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},
   {0x05, 0x05, 0x05}, {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
   {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00}, {0xC4, 0x62, 0x00},
   {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55}, {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21},
   {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09}, {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF},
   {0xD4, 0x80, 0xFF}, {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
   {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4}, {0x05, 0xFB, 0xFF},
   {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D}, {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF},
   {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB}, {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0},
   {0xFF, 0xEF, 0xA6}, {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
   {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}
};


// Implement later; for now just send b/w
pixel convert_rgb_tile(uint8_t value, uint8_t pallete, uint8_t prio) {
	uint8_t index;
	//printf("Trying to read pallette %x from address %x\n", pallete, 0x3F00 + (pallete * 4) + value);
	ppu_read(0x3F00 + (pallete * 4) + value, &index);
	// if (value)
	//printf("At index pallette %x\n", index);
	color rgb = system_pallete[index];
	pixel pix;
	// pix.r = rgb.r;
	// pix.g = rgb.g;
	// pix.b = rgb.b;
	pix.r = value * 255 / 3;
	pix.g = value * 255 / 3;
	pix.b = value * 255 / 3;
	pix.prio = prio;
	return pix;
}

pixel convert_rgb_background(uint8_t value, uint8_t pallete, uint8_t prio) {
	uint8_t index;
	//printf("Trying to read pallette %x from address %x\n", pallete, 0x3F00 + (pallete * 4) + value);
	ppu_read(0x3F00 + (pallete * 4) + value, &index);
	//printf("At index pallette %x\n", index);
	color rgb = system_pallete[index];
	pixel pix;
	pix.r = rgb.r;
	pix.g = rgb.g;
	pix.b = rgb.b;
	// pix.r = value * 255 / 3;
	// pix.g = value * 255 / 3;
	// pix.b = value * 255 / 3;
	pix.prio = prio;
	return pix;
}

void pixel_to_buffer(pixel *pixel, uint16_t x, uint16_t y) {
	if (x > 255 || y > 240)
		return;
	//if ((ppu.framecount % 2) == 1) {
	//	if (buffer[y * SCREEN_WIDTH + x].prio <= pixel->prio) {
			buffer[y * SCREEN_WIDTH + x] = *pixel;
	//	}
	//} else {
	//	if (screen[y * SCREEN_WIDTH + x].prio <= pixel->prio) {
			//screen[y * SCREEN_WIDTH + x] = *pixel;
	//	}
	//}tile_row / 4 * 8 +  tile_column / 4;
}

/*
// Hardcoded 8x8 tile get that puts it into buffer
// Palette not implemented
// Rotation not yet implemented
void tile_to_buffer(uint16_t addr, uint8_t palette, uint8_t prio, uint16_t x, uint16_t y, uint8_t rotation) {
	uint8_t x_offset;
	uint8_t y_offset = 0;
	if (rotation) x_offset = 7;
	else x_offset = 1;
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
			pixel_to_buffer(&rgb, x + x_offset, y + y_offset);
			//}
			x_offset++;
		}
		// Note palette is set to 0 for temporary purposes
		// Implement rotation here
		x_offset = 0;
		y_offset++;
	}
}


void pixel_to_buffer(pixel *pixel, uint16_t x, uint16_t y) {
	if (x > 255 || y > 240)
		return;
	if ((ppu.framecount % 2) == 1) {
		if (buffer[y * SCREEN_WIDTH + x].prio <= pixel->prio) {
			buffer[y * SCREEN_WIDTH + x] = *pixel;
		}
	} else {
		if (screen[y * SCREEN_WIDTH + x].prio <= pixel->prio) {
			screen[y * SCREEN_WIDTH + x] = *pixel;
		}
	}
}
*/

// Hardcoded 8x8 tile get that puts it into buffer
// Palette not implemented
// Rotation not yet implemented
void tile_to_buffer(uint16_t addr, uint8_t palette, uint8_t prio, uint16_t x, uint16_t y, uint8_t rotation, bool sprite) {
	uint8_t x_offset;
	uint8_t y_offset = 0;
	if (rotation){
		x_offset = 7;
	}
	else {
		x_offset = 0;
	}
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
			//printf("Attempting to convert RGB\n");
			if (sprite) pixel rgb = convert_rgb_tile(value, palette + 4, prio);
			else pixel rgb = convert_rgb_background(value, palette, prio);
			//printf("convert RGB\n");
			pixel_to_buffer(&rgb, x + x_offset, y + y_offset);
			//}
			if (rotation) x_offset--;
			else x_offset++;
		}
		// Note palette is set to 0 for temporary purposes
		// Implement rotation here
		if (rotation) x_offset = 7;
		else x_offset = 0;
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
	uint8_t rotation = attributes >> 6;
	uint8_t priority = (attributes >> 5) & 0x1;
	tile_to_buffer(sprite_addr, (attributes & 0x3), (priority == 1) ? 0 : 2, x, y, rotation, true);
}

void oam_to_buffer() {
	log_oam();
	fflush(logfp);
	for (int i = 63; i >= 0; i--) {
		sprite_to_buffer(ppu.OAM[i].y, ppu.OAM[i].id, ppu.OAM[i].attributes, ppu.OAM[i].x);
	}
}

void nametable_to_buffer() {
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t byte;
	// Note: addr calculation is nametable_base + background_base + nametable_byte;
	uint16_t nt;
	uint8_t at;
	uint8_t pallete_to_use;
	for (int i = 0; i < 960; i++) { // Need to change for scroll
		uint8_t x_mod = x % 4 / 2;
		uint8_t y_mod = y % 4 / 2;
		ppu_read(0x2000 + i, &byte);
		// Calculate addr later!
		nt = byte << 4;
		//printf("Reading from address %x\n", 0x23C0 | (i & 0x0C00));
		//fflush(logfp);
		ppu_read(0x23C0 + (y / 4 * 8) + x, &at);
		// printf("Reading from at %x %x\n", byte, at);
		// Pallete_to_use contains the 2 bits 
		if (x_mod == 0 && y_mod == 0) // Use upper left of AT
			pallete_to_use = (at & (0b11));
		else if (x_mod == 1 && y_mod == 0) // Use upper right of AT
			pallete_to_use = (at >> 2 & (0b11));
		else if (x_mod == 0 && y_mod == 1) // Use lower left of AT
			pallete_to_use = (at >> 4 & (0b11 << 4));
		else if (x_mod == 1 && y_mod == 1) // Use lower left of AT
			pallete_to_use = (at >> 6 & (0b11 << 6));
		else
			exit(1);
		//log_byte_at("Pallete to use", pallete_to_use);
		printf("Pallete to use %x\n", pallete_to_use);


		// Addr, palette, prio, x, y, rotation
		// Palette needs to be implemented; prio and rotation are fixed
		//tile_to_buffer((0x19 << 4) + (BACKGROUND_PATTERN_ADDR ? 0x1000 : 0x0000), 0, 1, x, y, 0);
		tile_to_buffer(nt + (BACKGROUND_PATTERN_ADDR ? 0x1000 : 0x0000), pallete_to_use, 1, x * 8, y * 8, 0, false);
		x++;
		if (x >= 32) {
			x = 0;
			y++;
		}
	}
}