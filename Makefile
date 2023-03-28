CFLAGS = -Wall

OBJECTS = nes.o rom.o cpu.o

nes : $(OBJECTS)
	cc $(CFLAGS) -o nes_emu $(OBJECTS)

nes.o : nes.c rom.h cpu.h
cpu.o : cpu.c cpu.h rom.h
rom.o : rom.c rom.h

.PHONY : clean
clean :
	rm -rf *.o nes_emu