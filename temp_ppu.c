#include "ppu.c"
#include <types.h>

// Pseudocode for grabbing from CHROM
u8 get_CHROM_byte(u8 addr){
	u8 byte;
	return byte;
}

u8 get_CHROM_bit(u32 addr){
	u8 bit;
	return bit;
}

// Function for grabbing 8x16 tile addr
// Note all addresses use u32
u32 get_large_tile_addr(u8 tile){
	u32 addr;
	return addr;
}

// Implement later; for now just send b/w
pixel convert_rgb(u8 hue, u8 value, u8 prio){
	pixel rgb;
	rgb.r = value;
	rgb.g = value;
	rgb.b = value;
	rgb.a = prio
	return rgb;
}

void pixel_to_buffer(u8 pixel, u8 x, u8 y){
	if (pixel.a > buffer[y * SCREEN_WIDTH + x]){
		buffer[y * SCREEN_WIDTH + X] = pixel;
	}
}

// Hardcoded 8x8 tile get that puts it into buffer
// Palette not implemented
// Rotation not yet implemented
void tile_to_buffer(u32 addr, u8 palette, u8 prio, u8 x, u8 y, u8 rotation){
	for (int i = addr; i < addr + 8 * 8; i++){
		u8 x_offset;
		u8 y_offset;
		u8 value;
		if (get_CHROM_BIT(i) & get_CHROM_BIT(i + 64)){
			value = 3;
		}
		else if (get_CHROM_BIT(i + 64)){
			value = 2;
		}
		else if (get_CHROM_BIT(i)){
			value = 1;
		}
		else value = 0;
		// Note palette is set to 0 for temporary purposes
		pixel rgb = convert_rgb(0, value, prio);
		pixel_to_buffer(rgb, x + x_offset, y + y_offset)
		x_offset++;
		if (x_offset >= 8){
			x_offset = 0;
			y_offset++;
		}
	}
}

// Specific to sprites; background code has something else
// Also for 16x8 mode we use size; the selection of tile/rotation differs
void sprite_to_buffer(u8 y, u32 addr, u8 sprite, u8 x, u8 size)
	u8 rotation = sprite >> 6;
	u8 priority;
	if (sprite && (1 << 5)){
		priority = 0;
	}
	else priority = 2;

	if (y > 0){
		y--
	}
	else return;
	if (size){
		switch(rotation){
			// No swap at all
			case 0:
				tile_to_buffer(get_large_tile_addr(addr), sprite && 0x3, priority, x, y, rotation);
				tile_to_buffer(get_large_tile_addr(addr) + 64, sprite && 0x3, priority, x + 8, y, rotation);
				break;
			// Horizontal rotation
			// Will need to swap tile locations when rotation is implemented
			case 1:
				tile_to_buffer(get_large_tile_addr(addr), sprite && 0x3, priority, x, y, rotation);
				tile_to_buffer(get_large_tile_addr(addr) + 64, sprite && 0x3, priority, x + 8, y, rotation);
				break;
			// Vertical rotation
			case 2:
				tile_to_buffer(get_large_tile_addr(addr), sprite && 0x3, priority, x, y, rotation);
				tile_to_buffer(get_large_tile_addr(addr) + 64, sprite && 0x3, priority, x + 8, y, rotation);
				break;
			// Horizontal and vertical rotation
			// Will need to swap tile locations
			case 3:
				tile_to_buffer(get_large_tile_addr(addr), sprite && 0x3, priority, x, y, rotation);
				tile_to_buffer(get_large_tile_addr(addr) + 64, sprite && 0x3, priority, x + 8, y, rotation);
				break;
		}
	}
	else {
		switch(rotation){
			// No swap at all
			case 0:
				tile_to_buffer(addr, sprite && 0x3, priority, x, y, rotation);
				break;
			// Horizontal swap
			// Will need to swap tile locations when rotation is implemented
			case 1:
				tile_to_buffer(addr, sprite && 0x3, priority, x, y, rotation);
				break;
			// Vertical rotation
			case 2:
				tile_to_buffer(addr, sprite && 0x3, priority, x, y, rotation);
				break;
			// Horizontal and vertical rotation
			// Will need to swap tile locations
			case 3:
				tile_to_buffer(addr, sprite && 0x3, priority, x, y, rotation);
				break;
		}
	}

void oam_to_buffer(){

}



