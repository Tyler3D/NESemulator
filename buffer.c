// Implement later; for now just send b/w
pixel convert_rgb(uint8_t hue, uint8_t value, uint8_t prio){
	pixel rgb;
	rgb.r = value;
	rgb.g = value;
	rgb.b = value;
	rgb.prio = prio;
	return rgb;
}

void pixel_to_buffer(pixel pixel, uint8_t x, uint8_t y){
	if (pixel.a > buffer[y * SCREEN_WIDTH + x] && buffer[y * SCREEN_WIDTH.x].prio <= pixel.prio){
		buffer[y * SCREEN_WIDTH + x] = pixel;
	}
}

// Hardcoded 8x8 tile get that puts it into buffer
// Palette not implemented
// Rotation not yet implemented
void tile_to_buffer(uint32_t addr, uint8_t palette, uint8_t prio, uint8_t x, uint8_t y, uint8_t rotation){
	uint8_t x_offset;
	uint8_t y_offset;
	for (int i = addr; i < addr + 8; i++){
		uint8_t value;
		uint8_t left;
		uint8_t right;

		ppu_read(i, &left);
		ppu_read(i + 8, &left);
		for (int j = 7; j >= 0; j--){
			uint8_t bit_left = (left >> j) && 1;
			uint8_t bit_right = (right >> j) && 1;
			if (bit_left & bit_right){
				value = 3;
			}
			else if (bit_right){
				value = 2;
			}
			else if (bit_left){
				value = 1;
			}
			else value = 0;
			// Note palette is set to 0 for temporary purposes
			if (value > 0){
				pixel rgb = convert_rgb(0, value, prio);
				pixel_to_buffer(rgb, prio, x + x_offset, y + y_offset)
			}
		}
		// Note palette is set to 0 for temporary purposes
		// Implement rotation here
		x_offset++;
		if (x_offset >= 8){
			x_offset = 0;
			y_offset++;
		}
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
}

// Specific to sprites; background code has something else
// Also for 16x8 mode we use size; the selection of tile/rotation differs
void sprite_to_buffer(uint8_t y, uint8_t addr, uint8_t id, uint8_t x){
	uint32_t sprite_addr;
	if (sprite & 1){
		sprite_addr = 0x1000 + ((addr >> 1) * 16);
	}
	else sprite_addr = (addr >> 1) * 16; 

	uint8_t rotation = sprite >> 6;
	uint8_t priority;
	if ((sprite << 5) & 1){
		priority = 0;
	}
	else priority = 2;
	if (y > 0){
		y--
	}
	else return;
	tile_to_buffer(sprite_addr, sprite && 0x3, priority, x, y, rotation);
}

void oam_to_buffer(){
	uint8_t y;
	uint8_t id;
	uint8_t attributes;
	uint8_t x; 
	uint8_t *oam = (uint8_t *) ppu.OAM;
	for (int i = 0; i < 256; i = i + 4){
		y = oam[i];
		id = oam[i + 1];
		attributes = oam[i + 2];
		x = oam[i + 3];
		if (y && id && attributes && x) {
			sprite_to_buffer(y, id, attributes, x);
		}
	}
}

*/