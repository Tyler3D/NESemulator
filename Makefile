CFLAGS = -Wall

OBJECTS = nes.o rom.o cpu.o opcodes.o logger.o ppu.o

nes : $(OBJECTS)
	cc -Wall -g $(CFLAGS) -o nes_emu $(OBJECTS)

nes.o : nes.c rom.h cpu.h
cpu.o : cpu.c cpu.h rom.h
rom.o : rom.c rom.h
opcodes.o : opcodes.c cpu.h rom.h
logger.o : logger.c logger.h
ppu.o : ppu.c cpu.h ppu.h rom.h

.PHONY : clean
clean :
	rm -rf *.o nes_emu *.txt