/*
 * fbputchar: Framebuffer character generator
 *
 * Assumes 32bpp
 *
 * References:
 *
 * http://web.njit.edu/all_topics/Prog_Lang_Docs/html/qt/emb-framebuffer-howto.html
 * http://www.diskohq.com/docu/api-reference/fb_8h-source.html
 */

#include "fb.h"
#include "ppu.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/fb.h>
#include <linux/string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
size_t size;
uint8_t *fbp;
#include "ppu.h"

#define FBDEV "/dev/fb0"
#define BITS_PER_PIXEL 32
#define NES_PIXEL_BITS 8
#define SCREEN_SCALER 3

struct fb_fix_screeninfo fb_finfo;
struct fb_var_screeninfo fb_vinfo;
unsigned char *framebuffer;

/*
 * Open the framebuffer to prepare it to be written to.  Returns 0 on success
 * or one of the FBOPEN_... return codes if something went wrong.
 * (Safe) (do not touch)
 */
int fbopen()
{
  int fd = open(FBDEV, O_RDWR); /* Open the device */
  if (fd == -1)
    return FBOPEN_DEV;

  if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_finfo)) /* Get fixed info about fb */
    return FBOPEN_FSCREENINFO;

  if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_vinfo)) /* Get varying info about fb */
    return FBOPEN_VSCREENINFO;

  if (fb_vinfo.bits_per_pixel != 32)
    return FBOPEN_BPP; /* Unexpected */

  framebuffer = mmap(0, fb_finfo.smem_len, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
  if (framebuffer == (unsigned char *)-1)
    return FBOPEN_MMAP;

  return 0;
}

/*
 * sets pixel to the screen location repeats pixel into matrix for
 * upscaling is SCREEN_SCALER value > 1
 */
void setPixel(uint32_t x, uint32_t y, uint32_t r,
              uint32_t g, uint32_t b, uint32_t a)
{
  uint32_t pixel = (r << fb_vinfo.red.offset) |
                   (g << fb_vinfo.green.offset) |
                   (b << fb_vinfo.blue.offset) |
                   (a << fb_vinfo.transp.offset);

  uint32_t j, k;
  /*
   *upscale pixel and offset to render nes screen to center of view
   */
  for (j = 0; j < SCREEN_SCALER; j++)
  {
    for (k = 0; k < SCREEN_SCALER; k++)
    {
      uint32_t location = (((fb_vinfo.xres / 2 - (NES_SCRN_W * SCREEN_SCALER) / 2) + (x + k)) * fb_vinfo.bits_per_pixel / 8) +
                          (((fb_vinfo.yres / 2 - (NES_SCRN_H * SCREEN_SCALER) / 2) + (y + j)) * fb_finfo.line_length);
      *((uint32_t *)(framebuffer + location)) = pixel;
    }
  }
}

/*
 * take in martix of colors NES_SCRN_H (uint32_t)256 x NES_SCRN_W (uint32_t)240 see in header
 * and render to screen
 */
void nes_screen()
// void nes_screen(color **screen)
{
  printf("Framebuffer %d", fbopen());
  printf("Framebuffer %d", fb_vinfo.yres);
  uint32_t x, y;
  for (y = 0; y < NES_SCRN_H * SCREEN_SCALER; y += SCREEN_SCALER)
  {
    for (x = 0; x < NES_SCRN_W * SCREEN_SCALER; x += SCREEN_SCALER)
    {
      // 81, 245, 125

      setPixel(x, y, 81, 245, 125, 0);
      setPixel(x, y, buffer[y * SCREEN_WIDTH + x].r, buffer[y * SCREEN_WIDTH + x].b, buffer[y * SCREEN_WIDTH + x].g, 0);
      // setPixel(x, y, screen[y][x].r, screen[y][x].b, screen[y][x].g, 0);
    }
  }
}