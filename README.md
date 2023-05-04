# NESemulator

Basic rendering frame for CPU
This also assumes CPU and PPU are on separate threads (i gues we do some shared memory shit?)
May also require a third thread to handle sending to FPGA

Lines 1-240 (repeat)
Each line is 341 PPU cycles
CPU gives signal to PPU to begin rendering; assuming this will be sync
0 CPU signal
1 PPU begins rendering (first ppu cycle)
2 Second ppu cycle
3 Third ppu cycle
4 Second CPU cycle
4 Fourth ppu cycle
..
340 Last PPU cycle
0 CPU signal once more

Line 241
PPU calls the set_tile() and set_pixel() functions to create a buffer
This buffer is then sent to verilog async
V_BLANK is set for PPU
As soon as PPU render function is finished, begin sending pixel data to Verilog as fast as possible (use 100mhz loop if possible)
Send pixel by pixel (r, g, b)

Timing for Verilog pipeline
0 Send some flag (e.g. draw)
1 Send (r, g, b)
..
240 * width Send (r, g, b)
Send VBLANK to display or equivalent


Line 260
Last line; then CPU/PPU resets again to render another frame

