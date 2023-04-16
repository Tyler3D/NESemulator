#include <stdio.h>
#include "rom.h"
#include "cpu.h"

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

/*
The following functions help get target bytes to modify
when executing various 6502 instructions.
cpu.pc is expected to be reading the opcode and on completion,
cpu.pc will point to the next opcode.

Might have to modify to account for interrupts
*/


/*
Implements zeropage addressing on the 6502
Returns byte from first page (high byte == 0x00)
Offset variable allows Zeropage, X and Zeropage, Y where you add X or Y to addr

Still need to account for cycles when adding to addr
When we overflow the low byte and increase the high byte,
it takes another clock cycle.

Current solution is to manually check if offset is messed up like this:
((((uint16_t) low - cpu.y) <= 0xFF)) ? 5 : 6;

But this function could probably implement it as well
*/
//(*idputs(int (*puts)(const char *)))(const char *)
int getAddressingMode(uint8_t opcode) {
    return NULL;
}

uint8_t zeropage(uint8_t *low, uint8_t *high, uint16_t *addr, uint8_t offset) {
    uint8_t byte;
    cpu.pc++;
    if (!cpu_read(cpu.pc + offset, low)) {
        printf("Could not read addr\n");
        cpu.fail();
    }

    *addr = (((uint16_t) *high) << 8) | (uint16_t) *low;
    if (!cpu_read(*addr, &byte)) {
        printf("Could not read byte\n");
        cpu.fail();
    }

    cpu.pc++;
    return byte;
}

/*
Implements Absolute addressing on the 6502
Returns byte from address $high low
Reads bytes in little endian form
Offset variable allows Absolute, X and Absolute, Y where you add X or Y to addr

Still need to account for cycles when adding to addr
When we overflow the low byte and increase the high byte,
it takes another clock cycle.

Current solution is to manually check if offset is messed up like this:
((((uint16_t) low - cpu.y) <= 0xFF)) ? 5 : 6;

But this function could probably implement it as well
*/

uint8_t absolute(uint8_t *low, uint8_t *high, uint16_t *addr, uint8_t offset) {
    uint8_t byte;
    cpu.pc++;
    if (!cpu_read(cpu.pc, low)) {
        printf("Could not read low addr\n");
        cpu.fail();
    }
    cpu.pc++;
    if (!cpu_read(cpu.pc, high)) {
        printf("Could not read high addr\n");
        cpu.fail();
    }
    cpu.pc++;
    *addr = ((((uint16_t) *high) << 8) | (uint16_t) *low) + offset;
    if (!cpu_read(*addr, &byte)) {
        printf("Could not read byte\n");
        cpu.fail();
    }
    return byte;
}

/*
This function implements reading immediate bytes from memory
From what I can tell all of these ops use 2 cycles
So cycles is modified here
*/

uint8_t immediate(char *assembly) {
    uint8_t byte;
    cpu.pc++;
    if (!cpu_read(cpu.pc, &byte)) {
        printf("Could not read byte\n");
        cpu.fail();
    }
    cpu.pc++;
    assembly = "#i";
    cpu.cycles += 2;
    return byte;
}

void compare(uint8_t *first, uint8_t *second ) {
    uint8_t result = *first - *second;
    SET_CARRY_FLAG(*first, -1 * (uint16_t) *second, 0)
    SET_ZERO_FLAG(result)
    SET_NEG_FLAG(result)
}

void branch(uint8_t *opcode) {
    
}


/*ALU Instructions*/
void ORA(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("ORA %s\n", assembly);
    cpu.a = cpu.a | *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void AND(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("AND %s\n", assembly);
    cpu.a = cpu.a & *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void EOR(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("EOR %s\n", assembly);
    cpu.a = cpu.a ^ *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void ADC(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    printf("ADC %s\n", assembly);
    SET_CARRY_FLAG(cpu.a, *byte, carryBit)
    SET_OVERFLOW_FLAG(cpu.a, *byte, carryBit)
    cpu.a = cpu.a + *byte + carryBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void LDA(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("LDA %s\n", assembly);
    cpu.a = *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void STA(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("STA %s\n", assembly);
    if (!immediate) // If not STA #i which is NOP
        cpu_write(*addr, &cpu.a);
}

void CMP(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("CMP %s\n", assembly);
    compare(&cpu.a, byte);
}

void SBC(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("SBC %s\n", assembly);
    uint16_t borrowBit = ((cpu.status & carry) > 0) ? 0 : 1;
    SET_CARRY_FLAG(cpu.a, -1 * (uint16_t) *byte, -1 * borrowBit)
    SET_OVERFLOW_FLAG(cpu.a, -1 * (uint16_t) *byte, -1 * borrowBit) // Jank see if it works

    cpu.a = cpu.a - *byte - borrowBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}


/*RMW Instructions*/
void ASL(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("ASL %s\n", assembly);
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = *byte << 1;
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void LSR(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("LSR %s\n", assembly);
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = *byte >> 1;
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void ROL(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("ROL %s\n", assembly);
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = (*byte << 1) | carryBit;
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void ROR(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    printf("ROR %s\n", assembly);
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = (*byte >> 1) | (carryBit << 7);
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void STX(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    if (immediate) {
        printf("TXA\n");
        cpu.a = cpu.x;
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
    } else {
        printf("STX %s\n", assembly);
        cpu_write(*addr, &cpu.x);
    }
}

void LDX(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    if (immediate) {
        printf("TAX\n");
        cpu.x = cpu.a;
    } else {
        printf("LDX %s\n", assembly);
        cpu.x = *byte;
    }
    SET_ZERO_FLAG(cpu.x)
    SET_NEG_FLAG(cpu.x)
}

void INC(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    if (immediate) {
        printf("NOP impl\n");
    } else {
        printf("INC %s\n", assembly);
        (*byte)++;
        SET_ZERO_FLAG(*byte)
        SET_NEG_FLAG(*byte)
        cpu_write(*addr, byte);
    }
}

void DEC(uint8_t *byte, char *assembly, uint16_t *addr, bool immediate) {
    if (immediate) {
        printf("DEX\n");
        cpu.x--;
    } else {
        printf("DEC %s\n", assembly);
        (*byte)--;
    }
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    cpu_write(*addr, byte);
}

/* Will isolate Control assembly instructions */
void handleControl(uint8_t opcode) {
    uint8_t high = 0;
    uint8_t low = 0;
    uint16_t addr;
    uint8_t byte;
    char *assembly = NULL;
    void (*func)(uint8_t *, char *, uint16_t *, bool);

    switch (opcode) {
        case 0x00: // BRK impl

        break;

        case 0x08: // PHP impl

        break;

        case 0x10: // BPL rel

        break;

        case 0x18: // CLC impl

        break;

        case 0x20: // JSR abs

        break;

        case 0x24: // BIT zpg

        break;

        case 0x28: // PLP impl

        break;

        case 0x2C: // BIT abs

        break;

        case 0x30: // BMI rel

        break;

        case 0x38: // SEC impl

        break;

        case 0x40: // RTI impl

        break;

        case 0x48: // PHA impl

        break;

        case 0x4C: // JMP abs

        break;
        
        case 0x50: // BVC rel

        break;

        case 0x58: // CLI impl

        break;

        case 0x60: // RTS impl

        break;

        case 0x68: // PLA impl

        break;

        case 0x6C: // JMP ind

        break;

        case 0x70: // BVS rel

        break;

        case 0x78: // SEI impl

        break;

        case 0x84: // STY zpg

        break;

        case 0x88: // DEY impl

        break;

        case 0x8C: // STY abs

        break;

        case 0x90: // BCC rel

        break;

        case 0x94: // STY zpg,X

        break;

        case 0x98: // TYA impl

        break;

        case 0xA0: // LDY #

        break;

        case 0xA4: // LDY zpg

        break;

        case 0xA8: // TAY impl

        break;

        case 0xAC: // LDY abs

        break;

        case 0xB0: // BCS rel

        break;
    }

    switch (opcode % 0x20) {
        case 0x00: // impl, abs, #

        break;

        case 0x04: // zeropage
        byte = zeropage(&low, &high, &addr, 0);
        break;

        case 0x08: // impl

        break;

        case 0x0C: // abs
        byte = absolute(&low, &high, &addr, 0);
        break;

        case 0x10: // rel

        break;

        case 0x14: // zeropage, X
        byte = zeropage(&low, &high, &addr, cpu.x);
        break;

        case 0x18: // impl

        break;

        case 0x1C: // absolute, X
        byte = absolute(&low, &high, &addr, cpu.x);
        break;

        default:
            printf("Broken Instruction 0x%x\n", opcode);
            cpu.fail();
        break;
    }
}

/* Will isolate ALU assembly instructions */
void handleALU(uint8_t opcode) {
    uint8_t high = 0;
    uint8_t low = 0;
    uint16_t addr;
    uint8_t byte;
    char *assembly = NULL;
    void (*func)(uint8_t *, char *, uint16_t *, bool);

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
        func(&byte, assembly, &addr, false);
        break;

        case 0x05: // zeropage
        byte = zeropage(&low, &high, &addr, 0);
        assembly = "d";
        cpu.cycles += 3;
        func(&byte, assembly, &addr, false);
        break;

        case 0x09: // immediate
        byte = immediate(assembly);
        func(&byte, assembly, &addr, true);
        break;

        case 0x0D: // absolute
        byte = absolute(&low, &high, &addr, 0);
        assembly = "a";
        cpu.cycles += 4;
        func(&byte, assembly, &addr, false);
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
        assembly = "(d),y"; // STA always does 6 cycles
        cpu.cycles += ((((uint16_t) low + cpu.y) <= 0xFF) && func != &STA) ? 5 : 6; // add 1 to cycles if page boundary is crossed
        func(&byte, assembly, &addr, false);
        break;

        case 0x15: // zeropage, X
        byte = zeropage(&low, &high, &addr, cpu.x);
        assembly = "d,x";
        cpu.cycles += 4;
        func(&byte, assembly, &addr, false);
        break;

        case 0x19: // absolute, Y
        byte = absolute(&low, &high, &addr, cpu.y);
        assembly = "a,y"; // // STA always does 5 cycles
        cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.y) && func != &STA) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        func(&byte, assembly, &addr, false);
        break;

        case 0x1D: // absolute, X
        byte = absolute(&low, &high, &addr, cpu.x); // STA always does 5 cycles
        cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.x) && func != &STA) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        assembly = "a,x";
        func(&byte, assembly, &addr, false);
        break;
        
        default:
            printf("Broken Instruction 0x%x\n", opcode);
            cpu.fail();
        break;
    }
}

/* Will isolate RMW assembly instructions */
void handleRMW(uint8_t opcode) {
    uint8_t high = 0;
    uint8_t low = 0;
    uint16_t addr;
    uint8_t byte;
    char *assembly = NULL;
    void (*func)(uint8_t *, char *, uint16_t *, bool);
    if (opcode < 0x20) // ASL
        func = &ASL;
    else if (opcode < 0x40) // ROL
        func = &ROL;
    else if (opcode < 0x60) // LSR
        func = &LSR;
    else if (opcode < 0x80) // ROR
        func = &ROR;
    else if (opcode < 0xA0) // STX
        func = &STX;
    else if (opcode < 0xC0) // LDX
        func = &LDX;
    else if (opcode < 0xE0) // DEC
        func = &DEC;
    else // INC
        func = &INC;
   switch (opcode % 0x20) {
        case 0x02: // immediate #i Only works on LDX
        if (opcode == 0xA2) {
            uint8_t byte = immediate(assembly);
            LDX(&byte, "#i", 0, false);
        } else {
            printf("Broken Instruction 0x%x\n", opcode);
            cpu.fail();
        }
        break;
        
        case 0x06: // zeropage
        byte = zeropage(&low, &high, &addr, 0);
        assembly = "d";
        cpu.cycles += (func == &LDX) ? 3 : 5; // LDX is only 3 cycles
        func(&byte, assembly, &addr, false);
        break;

        case 0x0A: // Works on registers
        cpu.cycles += 2;
        if (opcode < 0x80)
            assembly = "A";
        else
            assembly = "impl";
        func(&byte, assembly, &addr, true);
        break;

        case 0x0E: // absolute
        byte = absolute(&low, &high, &addr, 0);
        assembly = "a";
        cpu.cycles += (func == &LDX || func == &STX) ? 4 : 6; // LDX and STX are only 4 cycles
        func(&byte, assembly, &addr, false);
        break;

        case 0x12: // Fail
        printf("Broken Instruction 0x%x\n", opcode);
        cpu.fail();
        break;

        case 0x16: // zeropage, X or Y
        if (opcode == 0x96 || opcode == 0xB6) { // STX and LDX use zeropage, Y
            byte = zeropage(&low, &high, &addr, cpu.y);
            assembly = "d,y";
        } else {
            byte = zeropage(&low, &high, &addr, cpu.x);
            assembly = "d,x";
        }
        cpu.cycles += (func == &LDX || func == &STX) ? 4 : 6; // LDX and STX are only 4 cycles
        func(&byte, assembly, &addr, false);
        break;

        case 0x1A: // impl
        // Only TXS and TSX
        if (opcode == 0x9A) { 
            // TXS
            cpu.pc++;
            printf("TSX\n");
            cpu.x = cpu.sp;
            cpu.cycles += 2;
            
        } else if (opcode == 0xBA) {
            // TSX
            cpu.pc++;
            printf("TSX\n");
            cpu.sp = cpu.x;
            cpu.cycles += 2;
        } else {
            printf("Broken Instruction 0x%x\n", opcode);
            cpu.fail();
        }
        break;

        case 0x1E: // absolute, X or absolute, Y
        // Need to figure out how many cycles
        if (opcode == 0xBE) {// LDX uses absolute, Y
            byte = absolute(&low, &high, &addr, cpu.y);
            cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.y)) ? 4 : 5;
            assembly = "a,y";
        } else {
            byte = absolute(&low, &high, &addr, cpu.x);
            cpu.cycles += 7;
            assembly = "a,x";
        }
        func(&byte, assembly, &addr, false);
        break;
        
        default:
            printf("Broken Instruction 0x%x\n", opcode);
            cpu.fail();
        break;
   }
}