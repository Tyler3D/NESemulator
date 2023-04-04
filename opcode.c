#include <stdio.h>
#include "rom.h"
#include "cpu.h"

void ORA(uint8_t byte, char *assembly, uint16_t addr) {
    printf("ORA %s\n", assembly);
    cpu.a = cpu.a | byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void AND(uint8_t byte, char *assembly, uint16_t addr) {
    printf("AND %s\n", assembly);
    cpu.a = cpu.a & byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void EOR(uint8_t byte, char *assembly, uint16_t addr) {
    printf("EOR %s\n", assembly);
    cpu.a = cpu.a ^ byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void ADC(uint8_t byte, char *assembly, uint16_t addr) {
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    printf("ADC %s\n", assembly);
    SET_CARRY_FLAG(cpu.a, byte, carryBit)
    SET_OVERFLOW_FLAG(cpu.a, byte, carryBit)
    cpu.a = cpu.a + byte + carryBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void LDA(uint8_t byte, char *assembly, uint16_t addr) {
    printf("LDA %s\n", assembly);
    cpu.a = byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void STA(uint8_t byte, char *assembly, uint16_t addr) {
    printf("STA %s\n", assembly);
    if (addr) // If not STA #i which is NOP
        cpu_write(addr, &cpu.a);
}

void CMP(uint8_t byte, char *assembly, uint16_t addr) {
    printf("CMP %s\n", assembly);
    uint8_t result = cpu.a - byte;
    SET_CARRY_FLAG(cpu.a, -1 * (uint16_t) byte, 0)
    SET_ZERO_FLAG(result)
    SET_NEG_FLAG(result)
}

void SBC(uint8_t byte, char *assembly, uint16_t addr) {
    printf("SBC %s\n", assembly);
    uint16_t borrowBit = ((cpu.status & carry) > 0) ? 0 : 1;
    SET_CARRY_FLAG(cpu.a, -1 * (uint16_t) byte, -1 * borrowBit)
    SET_OVERFLOW_FLAG(cpu.a, -1 * (uint16_t) byte, -1 * borrowBit) // Jank see if it works

    cpu.a = cpu.a - byte - borrowBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void handleControl(uint8_t opcode) {

}

/*
According to this source: https://www.masswerk.at/6502/6502_instruction_set.html
There are various addressing modes for the 6502
Here's an example for AND

addressing	assembler	    opc     bytes	cycles    What?
(indirect,X) AND (oper,X)   21	    2	    6       -> Reads from lookup address (using zero page method) low:(oper + X), high:(oper + X + 1)
zeropage	AND oper	    25	    2	    3       -> operand is zeropage address (hi-byte is zero, address = $00 oper)
immediate	AND #oper	    29	    2	    2       -> A AND oper -> A
absolute	AND oper	    2D	    3	    4       -> A AND memory -> A. Read memory low first high next
(indirect),Y AND (oper),Y   31	    2	    5*      -> Finds lookup address (zero page) $(oper + 1)(oper) and adds Y. Reads from there
zeropage,X	AND oper,X	    35	    2	    4       -> Like zeropage but add X
absolute,Y	AND oper,Y	    39	    3	    4*      -> Like abs but add Y
absolute,X	AND oper,X	    3D	    3	    4*      -> Like abs but add X

For the *: every 0x100 is a page boundary. If we add to the low byte, we have to fix it by incrementing high byte and setting low byte to 0
We don't care about the specifics but have to increment cycles by a different amount depending on the result.
*/

void handleALU(uint8_t opcode) {
    uint8_t high = 0;
    uint8_t low = 0;
    uint16_t addr;
    uint8_t byte;
    char *assembly;
    void (*func)(uint8_t, char *, uint16_t);

    if (opcode < 0x20) // ORA
        func = &ORA;
    else if (opcode < 0x40) // AND
        func = &AND;
    else if (opcode < 0x60) // EOR
        func = &EOR;
    else if (opcode < 0x80) // ADC
        func = &ADC;
    else if (opcode < 0xA0) // STA
        func = &STA;
    else if (opcode < 0xC0) // LDA
        func = &LDA;
    else if (opcode < 0xE0) // CMP
        func = &CMP;
    else
        func = &SBC;

    switch(opcode % 0x20) {
        case 0x01: // (indirect, X)
        cpu.pc++;
        if (!cpu_read((cpu.pc + cpu.x), &low)) {
            printf("Could not read addr\n");
            cpu.fail();
        }
        if (!cpu_read((cpu.pc + cpu.x + 1), &high)) {
            printf("Could not read addr\n");
            cpu.fail();
        }
        addr = (((uint16_t) high) << 8) | (uint16_t) low;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.pc++;
        assembly = "(d,x)";
        cpu.cycles += 6;
        func(byte, assembly, addr);
        break;

        case 0x05: // zeropage
        cpu.pc++;
        if (!cpu_read(cpu.pc, &low)) {
            printf("Could not read addr\n");
            cpu.fail();
        }

        addr = (((uint16_t) high) << 8) | (uint16_t) low;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.pc++;
        assembly = "d";
        cpu.cycles += 3;
        func(byte, assembly, addr);
        break;

        case 0x09: // immediate
        cpu.pc++;
        if (!cpu_read(cpu.pc, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.pc++;
        assembly = "#i";
        cpu.cycles += 2;
        func(byte, assembly, 0);
        break;

        case 0x0D: // absolute
        cpu.pc++;
        if (!cpu_read(cpu.pc, &low)) {
            printf("Could not read low addr\n");
            cpu.fail();
        }
        cpu.pc++;
        if (!cpu_read(cpu.pc, &high)) {
            printf("Could not read high addr\n");
            cpu.fail();
        }
        cpu.pc++;
        addr = (((uint16_t) high) << 8) | (uint16_t) low;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        assembly = "a";
        cpu.cycles += 4;
        func(byte, assembly, addr);
        break;

        case 0x11: // (indirect), Y
        cpu.pc++;
        if (!cpu_read(cpu.pc, &low)) {
            printf("Could not read addr\n");
            cpu.fail();
        }
        if (!cpu_read(cpu.pc + 1, &high)) {
            printf("Could not read addr\n");
            cpu.fail();
        }
        addr = (((uint16_t) high) << 8) | (uint16_t) low + cpu.y;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.pc++;
        assembly = "(d),y";
        cpu.cycles += (((uint16_t) low - cpu.y) <= 0xFF) ? 5 : 6; // add 1 to cycles if page boundary is crossed
        func(byte, assembly, addr);
        break;

        case 0x15: // zeropage, X
        cpu.pc++;
        if (!cpu_read((cpu.pc + cpu.x), &low)) {
            printf("Could not read addr\n");
            cpu.fail();
        }

        addr = (((uint16_t) high) << 8) | (uint16_t) low;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.pc++;
        assembly = "d,x";
        cpu.cycles += 4;
        func(byte, assembly, addr);
        break;

        case 0x19: // absolute, Y
        cpu.pc++;
        if (!cpu_read(cpu.pc, &low)) {
            printf("Could not read low addr\n");
            cpu.fail();
        }
        cpu.pc++;
        if (!cpu_read(cpu.pc, &high)) {
            printf("Could not read high addr\n");
            cpu.fail();
        }
        cpu.pc++;
        addr = ((((uint16_t) high) << 8) | (uint16_t) low) + cpu.y;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        assembly = "a,y";
        cpu.cycles += (((uint16_t) (addr - cpu.y) % 0xFF) + cpu.y <= 0xFF) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        func(byte, assembly, addr);
        break;

        case 0x1D: // absolute, X
        cpu.pc++;
        if (!cpu_read(cpu.pc, &low)) {
            printf("Could not read low addr\n");
            cpu.fail();
        }
        cpu.pc++;
        if (!cpu_read(cpu.pc, &high)) {
            printf("Could not read high addr\n");
            cpu.fail();
        }
        cpu.pc++;
        addr = ((((uint16_t) high) << 8) | (uint16_t) low) + cpu.x;
        if (!cpu_read(addr, &byte)) {
            printf("Could not read byte\n");
            cpu.fail();
        }
        cpu.cycles += (((uint16_t) (addr - cpu.x) % 0xFF) + cpu.x <= 0xFF) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        assembly = "a,x";
        func(byte, assembly, addr);
        break;
    }
}

void handleRMW(uint8_t opcode) {

}