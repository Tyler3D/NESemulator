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
cpu.pc is expected to be reading the cpu.opcode and on completion,
cpu.pc will point to the next cpu.opcode.

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
int getAddressingMode() {
    return 0;
}

uint8_t zeropage(uint16_t *addr, uint8_t offset) {
    uint8_t byte;
    READ_BYTE_FROM_ADDR(cpu.pc + offset, cpu.low)
    SET_ADDR(*addr, 0)
    READ_BYTE_FROM_ADDR(*addr, byte)
    cpu.pc++;
    cpu.asm_argc = 2;
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

uint8_t absolute(uint16_t *addr, uint8_t offset) {
    uint8_t byte;
    READ_BYTE_PC(cpu.low)
    READ_BYTE_PC(cpu.high)
    SET_ADDR(*addr, offset)
    READ_BYTE_FROM_ADDR(*addr, byte)
    cpu.asm_argc = 3;
    return byte;
}

/*
This function implements reading immediate bytes from memory
From what I can tell all of these ops use 2 cycles
So cycles is modified here
*/

uint8_t immediate() {
    uint8_t byte;
    READ_BYTE_PC(byte)
    sprintf(cpu.asm_args, "#%X", byte);
    //assembly = "#i";
    cpu.cycles += 2;
    cpu.asm_argc = 2;
    return byte;
}

void compare(uint8_t *first, uint8_t *second ) {
    uint8_t result = *first - *second;
    SET_CARRY_FLAG(*first, -1 * (uint16_t) *second, 0)
    SET_ZERO_FLAG(result)
    SET_NEG_FLAG(result)
}

/*
These instructions are always of 2 bytes length and perform in 2 CPU cycles, 
if the branch is not taken (the condition resolving to 'false'), 
and 3 cycles, if the branch is taken (when the condition is true). 
If a branch is taken and the target is on a different page, this adds another CPU cycle (4 in total).
*/

void branch() {
    READ_BYTE_PC(cpu.low)
    int8_t target;
    switch (cpu.opcode) {
        case 0x10: // BPL branch on plus (negative clear)
        if (IS_FLAG_OFF(negative))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x30: // BMI branch on minus (negative set)
        if (IS_FLAG_ON(negative))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x50: // BVC branch on overflow clear
        if (IS_FLAG_OFF(overflow))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x70: // BVS branch on overflow set
        if (IS_FLAG_ON(overflow))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x90: // BCC branch on carry clear
        if (IS_FLAG_OFF(carry))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xB0: // BCS branch on carry set
        if (IS_FLAG_ON(carry))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xD0: // BNE branch on not equal (zero clear)
        if (IS_FLAG_OFF(zero))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xF0: // BEQ branch on equal (zero set)
        if (IS_FLAG_ON(zero))
            goto actuallyBranch;
        goto branchFail;
        break;
        default:
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;
    }

    branchFail:
    cpu.cycles += 2;
    return;

    actuallyBranch:
    target = (int8_t) cpu.low;
    if (GET_PAGE_NUM(cpu.pc) != GET_PAGE_NUM(cpu.pc + target))
        cpu.cycles += 4;
    else
        cpu.cycles += 3;
    return;
}


/*ALU Instructions*/
void ORA(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ORA %s\n", cpu.instruction);
    cpu.instruction = "ORA";
    cpu.a = cpu.a | *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void AND(uint8_t *byte, uint16_t *addr, bool immediate) {
    //log_state(4,)
    //printf("AND %s\n", cpu.instruction);
    cpu.instruction = "AND";
    cpu.a = cpu.a & *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void EOR(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("EOR %s\n", cpu.instruction);
    cpu.instruction = "EOR";
    cpu.a = cpu.a ^ *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void ADC(uint8_t *byte, uint16_t *addr, bool immediate) {
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    //printf("ADC %s\n", cpu.instruction);
    cpu.instruction = "ADC";
    SET_CARRY_FLAG(cpu.a, *byte, carryBit)
    SET_OVERFLOW_FLAG(cpu.a, *byte, carryBit)
    cpu.a = cpu.a + *byte + carryBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void LDA(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("LDA %s\n", cpu.instruction);
    cpu.instruction = "LDA";
    cpu.a = *byte;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}

void STA(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("STA %s\n", cpu.instruction);
    cpu.instruction = "STA";
    if (!immediate) {// If not STA #i which is NOP
        cpu_write(*addr, &cpu.a);
    }
}

void CMP(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("CMP %s\n", cpu.instruction);
    cpu.instruction = "CMP";
    compare(&cpu.a, byte);
}

void SBC(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("SBC %s\n", cpu.instruction);
    cpu.instruction = "SBC";
    uint16_t borrowBit = ((cpu.status & carry) > 0) ? 0 : 1;
    SET_CARRY_FLAG(cpu.a, -1 * (uint16_t) *byte, -1 * borrowBit)
    SET_OVERFLOW_FLAG(cpu.a, -1 * (uint16_t) *byte, -1 * borrowBit) // Jank see if it works

    cpu.a = cpu.a - *byte - borrowBit;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}


/*RMW Instructions*/
void ASL(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ASL %s\n", cpu.instruction);
    cpu.instruction = "ASL";
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = *byte << 1;
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void LSR(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("LSR %s\n", cpu.instruction);
    cpu.instruction = "LSR";
    SET_CARRY_FLAG(*byte, 0, 0)
    *byte = *byte >> 1;
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
    if (immediate)
        cpu.a = *byte;
    else
        cpu_write(*addr, byte);
}

void ROL(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ROL %s\n", cpu.instruction);
    cpu.instruction = "ROL";
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

void ROR(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ROR %s\n", cpu.instruction);
    cpu.instruction = "ROR";
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

void STX(uint8_t *byte, uint16_t *addr, bool immediate) {
    if (immediate) {
        //printf("TXA\n");
        cpu.instruction = "TXA";
        cpu.a = cpu.x;
        cpu.asm_argc = 1;
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
    } else {
        cpu.instruction = "STX";
        //printf("STX %s\n", cpu.instruction);
        cpu_write(*addr, &cpu.x);
    }
}

void LDX(uint8_t *byte, uint16_t *addr, bool immediate) {
    if (immediate) {
        cpu.instruction = "TAX";
        //printf("TAX\n");
        cpu.asm_argc = 1;
        cpu.x = cpu.a;
    } else {
        cpu.instruction = "LDX";
        //printf("LDX %s\n", cpu.instruction);
        cpu.x = *byte;
    }
    SET_ZERO_FLAG(cpu.x)
    SET_NEG_FLAG(cpu.x)
}

void INC(uint8_t *byte, uint16_t *addr, bool immediate) {
    if (immediate) {
        cpu.instruction = "NOP";
        cpu.asm_argc = 1;
        //printf("NOP impl\n");
    } else {
        cpu.instruction = "INC";
        //printf("INC %s\n", cpu.instruction);
        (*byte)++;
        SET_ZERO_FLAG(*byte)
        SET_NEG_FLAG(*byte)
        cpu_write(*addr, byte);
    }
}

void DEC(uint8_t *byte, uint16_t *addr, bool immediate) {
    if (immediate) {
        cpu.instruction = "DEX";
        cpu.asm_argc = 1;
        //printf("DEX\n");
        cpu.x--;
    } else {
        cpu.instruction = "DEC";
        //printf("DEC %s\n", cpu.instruction);
        (*byte)--;
        cpu_write(*addr, byte);
    }
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
}

/* Will isolate Control assembly instructions */
void handleControl() {
    uint16_t addr;
    uint8_t byte;

    switch (cpu.opcode % 0x20) {
        case 0x00: // impl, abs, # first half interrupt stuff/second half cmp

        break;

        case 0x04: // zeropage
        byte = zeropage(&addr, 0);
        break;

        case 0x08: // impl Mostly Stack and register stuff

        break;

        case 0x0C: // abs
        byte = absolute(&addr, 0);
        break;

        case 0x10: // rel branching
        branch();
        break;

        case 0x14: // zeropage, X
        byte = zeropage(&addr, cpu.x);
        break;

        case 0x18: // impl mostly flag stuff

        break;

        case 0x1C: // absolute, X
        byte = absolute(&addr, cpu.x);
        break;

        default:
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        break;
    }
}

/* Will isolate ALU assembly instructions */
void handleALU() {
    uint16_t addr;
    uint8_t byte;
    void (*func)(uint8_t *, uint16_t *, bool);

    if (cpu.opcode < 0x20) // ORA
        func = &ORA;
    else if (cpu.opcode < 0x40) // AND
        func = &AND;
    else if (cpu.opcode < 0x60) // EOR
        func = &EOR;
    else if (cpu.opcode < 0x80) // ADC
        func = &ADC;
    else if (cpu.opcode < 0xA0) // STA
        func = &STA;
    else if (cpu.opcode < 0xC0) // LDA
        func = &LDA;
    else if (cpu.opcode < 0xE0) // CMP
        func = &CMP;
    else
        func = &SBC;

    switch(cpu.opcode % 0x20) {
        case 0x01: // (indirect, X)
        //cpu.pc++;
        READ_BYTE_FROM_ADDR(cpu.pc + cpu.x, cpu.low)
        READ_BYTE_FROM_ADDR(cpu.pc + cpu.x + 1, cpu.high)
        SET_ADDR(addr, 0)
        READ_BYTE_FROM_ADDR(addr, byte)
        cpu.pc++;
        sprintf(cpu.asm_args, "($%X,x)", cpu.low);
        //assembly = "(d,x)";
        cpu.asm_argc = 2;
        cpu.cycles += 6;
        func(&byte, &addr, false);
        break;

        case 0x05: // zeropage
        byte = zeropage(&addr, 0);
        sprintf(cpu.asm_args, "$%X", cpu.low);
        //assembly = "d";
        cpu.cycles += 3;
        func(&byte, &addr, false);
        break;

        case 0x09: // immediate
        byte = immediate();
        func(&byte, &addr, true);
        break;

        case 0x0D: // absolute
        byte = absolute(&addr, 0);
        sprintf(cpu.asm_args, "$%X%X", cpu.high, cpu.low);
        //assembly = "a";
        cpu.cycles += 4;
        func(&byte, &addr, false);
        break;

        case 0x11: // (indirect), Y
        //cpu.pc++;
        READ_WORD(cpu.pc)
        SET_ADDR(addr, cpu.y)
        READ_BYTE_FROM_ADDR(addr, byte)
        cpu.pc++;
        sprintf(cpu.asm_args, "($%X),y", cpu.low);
        cpu.asm_argc = 3;
        //assembly = "(d),y"; // STA always does 6 cycles
        cpu.cycles += ((((uint16_t) cpu.low + cpu.y) <= 0xFF) && func != &STA) ? 5 : 6; // add 1 to cycles if page boundary is crossed
        func(&byte, &addr, false);
        break;

        case 0x15: // zeropage, X
        byte = zeropage(&addr, cpu.x);
        sprintf(cpu.asm_args, "$%X,x", cpu.low);
        //assembly = "d,x";
        cpu.cycles += 4;
        func(&byte, &addr, false);
        break;

        case 0x19: // absolute, Y
        byte = absolute(&addr, cpu.y);
        sprintf(cpu.asm_args, "$%X%X,y", cpu.high, cpu.low);
        //assembly = "a,y"; // // STA always does 5 cycles
        cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.y) && func != &STA) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        func(&byte, &addr, false);
        break;

        case 0x1D: // absolute, X
        byte = absolute(&addr, cpu.x); // STA always does 5 cycles
        cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.x) && func != &STA) ? 4 : 5; // add 1 to cycles if page boundary is crossed
        sprintf(cpu.asm_args, "$%X%X,x", cpu.high, cpu.low);
        //assembly = "a,x";
        func(&byte, &addr, false);
        break;
        
        default:
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;
    }
}

/* Will isolate RMW assembly instructions */
void handleRMW() {
    cpu.low = 0;
    cpu.high = 0;
    uint16_t addr;
    uint8_t byte;
    void (*func)(uint8_t *, uint16_t *, bool);
    if (cpu.opcode < 0x20) // ASL
        func = &ASL;
    else if (cpu.opcode < 0x40) // ROL
        func = &ROL;
    else if (cpu.opcode < 0x60) // LSR
        func = &LSR;
    else if (cpu.opcode < 0x80) // ROR
        func = &ROR;
    else if (cpu.opcode < 0xA0) // STX
        func = &STX;
    else if (cpu.opcode < 0xC0) // LDX
        func = &LDX;
    else if (cpu.opcode < 0xE0) // DEC
        func = &DEC;
    else // INC
        func = &INC;
   switch (cpu.opcode % 0x20) {
        case 0x02: // immediate #i Only works on LDX
        if (cpu.opcode == 0xA2) {
            uint8_t byte = immediate();
            sprintf(cpu.asm_args, "#%X", byte);
            LDX(&byte, 0, false);
            cpu.asm_argc = 2;
        } else {
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        }
        break;
        
        case 0x06: // zeropage
        byte = zeropage(&addr, 0);
        sprintf(cpu.asm_args, "$%X", cpu.low);
        //assembly = "d";
        cpu.cycles += (func == &LDX) ? 3 : 5; // LDX is only 3 cycles
        func(&byte, &addr, false);
        break;

        case 0x0A: // Works on registers
        cpu.cycles += 2;
        if (cpu.opcode < 0x80)
            cpu.asm_args = "A";
        else
            cpu.asm_args = "impl";
        cpu.asm_argc = 1;
        func(&byte, &addr, true);
        break;

        case 0x0E: // absolute
        byte = absolute(&addr, 0);
        sprintf(cpu.asm_args, "$%X%X", cpu.high, cpu.low);
        //assembly = "a";
        cpu.cycles += (func == &LDX || func == &STX) ? 4 : 6; // LDX and STX are only 4 cycles
        func(&byte, &addr, false);
        break;

        case 0x12: // Fail
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;

        case 0x16: // zeropage, X or Y
        if (cpu.opcode == 0x96 || cpu.opcode == 0xB6) { // STX and LDX use zeropage, Y
            byte = zeropage(&addr, cpu.y);
            sprintf(cpu.asm_args, "$%X,y", cpu.low);
            //assembly = "d,y";
        } else {
            byte = zeropage(&addr, cpu.x);
            sprintf(cpu.asm_args, "$%X,x", cpu.low);
            //assembly = "d,x";
        }
        cpu.cycles += (func == &LDX || func == &STX) ? 4 : 6; // LDX and STX are only 4 cycles
        func(&byte, &addr, false);
        break;

        case 0x1A: // impl
        // Only TXS and TSX
        if (cpu.opcode == 0x9A) { 
            // TXS
            cpu.pc++;
            //printf("TXS\n");
            cpu.instruction = "TXS";
            cpu.x = cpu.sp;
            cpu.cycles += 2;
            
        } else if (cpu.opcode == 0xBA) {
            // TSX
            cpu.pc++;
            //printf("TSX\n");
            cpu.instruction = "TSX";
            cpu.sp = cpu.x;
            cpu.cycles += 2;
        } else {
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        }
        cpu.asm_argc = 1;
        break;

        case 0x1E: // absolute, X or absolute, Y
        // Need to figure out how many cycles
        if (cpu.opcode == 0xBE) {// LDX uses absolute, Y
            byte = absolute(&addr, cpu.y);
            cpu.cycles += (CHECK_PAGE_BOUNDARY(addr, cpu.y)) ? 4 : 5;
            sprintf(cpu.asm_args, "$%X%X,y", cpu.high, cpu.low);
            //assembly = "a,y";
        } else {
            byte = absolute(&addr, cpu.x);
            cpu.cycles += 7;
            sprintf(cpu.asm_args, "$%X%X,x", cpu.high, cpu.low);
            //assembly = "a,x";
        }
        cpu.asm_argc = 3;
        func(&byte, &addr, false);
        break;
        
        default:
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        break;
   }
}