CFLAGS = -Wall -g -std=c11

OBJECTS = nes.o rom.o cpu.o opcodes.o logger.o ppu.o controller.o buffer.o fb.o

nes : $(OBJECTS)
	cc $(CFLAGS) -o nes_emu $(OBJECTS)

nes.o : nes.c rom.h cpu.h fb.h
cpu.o : cpu.c cpu.h rom.h
rom.o : rom.c rom.h
opcodes.o : opcodes.c cpu.h rom.h
logger.o : logger.c logger.h
ppu.o : ppu.c cpu.h ppu.h rom.h
controller.o: controller.c controller.h cpu.h
buffer.o : buffer.c ppu.h
fb.o: fb.c ppu.h fb.h

.PHONY : clean
clean :
	rm -rf *.o nes_emu *.txt