#include <stdio.h>
#include "rom.h"
#include "cpu.h"
#include "logger.h" // Debugging

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
    uint8_t byte = 0;
    READ_BYTE_FROM_ADDR(cpu.pc, cpu.low)
    cpu.low += offset;
    SET_ADDR(*addr, 0)
    if (cpu.opcode != 0x84 &&
        cpu.opcode != 0x94 &&
        cpu.opcode != 0x85 &&
        cpu.opcode != 0x95 &&
        cpu.opcode != 0x86 &&
        cpu.opcode != 0x96) // STA, STX, STY shouldn't read byte. zeropage + zeropage, x
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
    uint8_t byte = 0;
    READ_BYTE_PC(cpu.low)
    READ_BYTE_PC(cpu.high)
    SET_ADDR(*addr, offset)
    if (cpu.opcode != 0x8D &&
        cpu.opcode != 0x8C &&
        cpu.opcode != 0x8E) // STA, STX, STY shouldn't read byte
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
    cpu.low = byte;
    sprintf(cpu.asm_args, "#%02X", byte);
    //assembly = "#i";
    cpu.cycles += 2;
    cpu.asm_argc = 2;
    return byte;
}

void compare(uint8_t *first, uint8_t *second ) {
    uint8_t result = *first - *second;
    SET_CARRY_FLAG(*first >= *second)
    SET_ZERO_FLAG(result)
    SET_NEG_FLAG(result)
    //printf("Comparing %X and %X\n", *first, *second);
}

void push(uint8_t *byte) {
    if (cpu.sp == 0x00) {
        printf("Stack Overflow!\n");
        cpu.fail();
    }
    cpu_write((0x100 + (uint16_t) cpu.sp), byte);
    cpu.sp--;
}

void pull(uint8_t *byte) {
    if (cpu.sp == 0xFF) {
        printf("Stack Underflow!\n");
        cpu.fail();
    }
    cpu.sp++;
    cpu_read(0x100 + ((uint16_t) cpu.sp), byte);
}

/* Control Instructions */
/*
These instructions mostly handle interrupts and
subroutines
*/
void column0x00() {
    uint8_t low;
    uint8_t hi;
    switch (cpu.opcode) {
        case 0x00: // BRK Force Break 
        // This instruction seems rather complicated
        // So we will implement it later
        cpu.asm_argc = 1;
        cpu.instruction = "BRK";
        cpu.cycles += 7;
        cpu.status |= irq;
        printf("BRK currently unimplemented\n");
        cpu.fail();
        
        break;

        case 0x20: // JSR abs Jump to New Location Saving Return Address
        // Check if high byte of PC is also pushed to stack
        // idk if this is right
        cpu.asm_argc = 1;
        cpu.instruction = "JSR";
        cpu.asm_argc = 3;
        READ_WORD_PC()
        sprintf(cpu.asm_args, "$%02X%02X", cpu.high, cpu.low);
        uint16_t returnAddr = cpu.pc - 1;
        low = returnAddr & 0xFF;
        hi = (returnAddr >> 8) & 0xFF;
        push(&hi);
        push(&low);
        cpu.cycles += 6;
        cpu.pc = (((uint16_t) cpu.high) << 8) | cpu.low;
        //printf("JSR sorta implemented\n");
        //cpu.fail();
        break;

        case 0x40: // RTI Return from Interrupt
        // pull SR, pull PC
        cpu.instruction = "RTI";
        cpu.asm_argc = 1;
        cpu.cycles += 6;
        pull(&cpu.status);
        pull(&low);
        pull(&hi);
	    cpu.status &= ~b_flag;
	    cpu.status |= always_on_flag;
        // How to pull PC?
        cpu.pc = (((uint16_t) hi) << 8) | low;
        //printf("RTI sorta implemented\n");
        //cpu.fail();
        break;

        case 0x60: // RTS Return from Subroutine
        // pull PC, PC+1 -> PC
        pull(&low);
        pull(&hi);
        cpu.instruction = "RTS";
        cpu.asm_argc = 1;
        cpu.cycles += 6;
        cpu.pc = (((uint16_t) hi) << 8) | low;
        cpu.pc++;
        //printf("RTS unimplemented\n");
        //cpu.fail();
        break;

        case 0xA0: // LDY #%X
        cpu.instruction = "LDY";
        cpu.cycles += 2;
        cpu.asm_argc = 2;
        READ_BYTE_PC(cpu.low)
        cpu.y = cpu.low;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        sprintf(cpu.asm_args, "#%02X", cpu.low);
        break;

        case 0xC0: // CPY #%X
        cpu.instruction = "CPY";
        cpu.cycles += 2;
        cpu.asm_argc = 2;
        READ_BYTE_PC(cpu.low)
        compare(&cpu.y, &cpu.low);
        sprintf(cpu.asm_args, "#%02X", cpu.low);
        break;
        
        case 0xE0: // CPX #%X
        cpu.instruction = "CPX";
        cpu.cycles += 2;
        cpu.asm_argc = 2;
        READ_BYTE_PC(cpu.low)
        compare(&cpu.x, &cpu.low);
        sprintf(cpu.asm_args, "#%02X", cpu.low);
        break;
        default:
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;
    }
}

void column0x04(uint8_t *byte, uint16_t *addr) {
    cpu.asm_argc = 2;
    cpu.cycles += 3;
    switch (cpu.opcode) {
        case 0x24: // BIT Test Bits in Memory with Accumulator
        /*
        bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
        the zero-flag is set to the result of operand AND accumulator.
        */
        cpu.instruction = "BIT";
        if (((*byte) & overflow) > 0)
            cpu.status |= overflow;
        else
            cpu.status &= ~overflow;
        if (((*byte) & negative) > 0)
            cpu.status |= negative;
        else
            cpu.status &= ~negative;
        uint8_t result = cpu.a & *byte;
        SET_ZERO_FLAG(result)
        break;

        case 0x84: // STY
        // Store Index Y in Memory
        // Check
        cpu.instruction = "STY";
        cpu_write(*addr, &cpu.y);
        break;

        case 0xA4: // LDY
        cpu.instruction = "LDY";
        cpu.y = *byte;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        break;

        case 0xC4: // CPY
        cpu.instruction = "CPY";
        compare(&cpu.y, byte);
        break;
        
        case 0xE4: // CPX
        cpu.instruction = "CPX";
        compare(&cpu.x, byte);
        break;
        default:
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;
    }
    sprintf(cpu.asm_args, "$%02X", cpu.low);
}

/*
These instructions seem to mostly effect the stack and registers
They all have implied args
Pushing to stack takes 3 cycles
Pulling from stack takes 4 cycles
Simple ALU ops on registers takes 2 cycles
*/

void column0x08() {
    cpu.asm_argc = 1;
    switch (cpu.opcode) {
        case 0x08: // PHP Push Processor Status on Stack
        cpu.instruction = "PHP";
        cpu.cycles += 3;
        uint8_t pushed = cpu.status | always_on_flag | b_flag; // Check
        push(&pushed); // Check B_flag
        break;

        case 0x28: // PLP Pull Processor Status from Stack
        cpu.instruction = "PLP";
        cpu.cycles += 4;
        pull(&cpu.status);
        cpu.status &= ~b_flag; // CPU ignores b_flag when pulling
        cpu.status |= always_on_flag; // Need to have flag always on
        break;

        case 0x48: // PHA Push Accumulator on Stack
        cpu.instruction = "PHA";
        cpu.cycles += 3;
        push(&cpu.a);
        break;

        case 0x68: // PLA Pull Accumulator from Stack
        cpu.instruction = "PLA";
        cpu.cycles += 4;
        pull(&cpu.a);
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
        break;

        case 0x88: // DEY Decrement Index Y by One
        cpu.instruction = "DEY";
        cpu.cycles += 2;
        cpu.y--;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        break;

        case 0xA8: // TAY Transfer Accumulator to Index Y
        cpu.instruction = "TAY";
        cpu.cycles += 2;
        cpu.y = cpu.a;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        break;

        case 0xC8: // INY Increment Index Y by One
        cpu.instruction = "INY";
        cpu.cycles += 2;
        cpu.y++;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        break;
        
        case 0xE8: // INX Increment Index X by One
        cpu.instruction = "INX";
        cpu.cycles += 2;
        cpu.x++;
        SET_NEG_FLAG(cpu.x)
        SET_ZERO_FLAG(cpu.x)
        break;
    }
}

void column0x0C(uint8_t *byte, uint16_t *addr) {
    cpu.asm_argc = 3;
    sprintf(cpu.asm_args, "$%02X%02X", cpu.high, cpu.low);
    switch (cpu.opcode) {
        case 0x2C: // BIT
        cpu.instruction = "BIT";
        cpu.cycles += 4;
        if (((*byte) & overflow) > 0)
            cpu.status |= overflow;
        else
            cpu.status &= ~overflow;
        if (((*byte) & negative) > 0)
            cpu.status |= negative;
        else
            cpu.status &= ~negative;
        uint8_t result = cpu.a & *byte;
        SET_ZERO_FLAG(result)
        break;

        case 0x4C: // JMP abs
        cpu.instruction = "JMP";
        cpu.cycles += 3;
        cpu.pc = (((uint16_t) cpu.high) << 8) | cpu.low;
        break;

        case 0x6C: // JMP ind
        cpu.instruction = "JMP";
        cpu.cycles += 5;
        uint8_t low = *byte;
        uint8_t hi;
        READ_BYTE_FROM_ADDR((((*addr & 0xFF) == 0xFF) ? (*addr & 0xFF00) : *addr + 1), hi)
        cpu.pc = (((uint16_t) hi) << 8) | low;
        sprintf(cpu.asm_args, "$(%02X%02X)", cpu.high, cpu.low);
        return;
        //printf("JMP ind not implemented\n");
        //cpu.fail();
        break;

        case 0x8C: // STY
        cpu.instruction = "STY";
        cpu.cycles += 4;
        cpu_write(*addr, &cpu.y);
        break;

        case 0xAC: // LDY
        cpu.instruction = "LDY";
        cpu.cycles += 4;
        cpu.y = *byte;
        SET_NEG_FLAG(cpu.y)
        SET_ZERO_FLAG(cpu.y)
        break;

        case 0xCC: // CPY
        cpu.instruction = "CPY";
        cpu.cycles += 4;
        compare(&cpu.y, byte);
        break;
        
        case 0xEC: // CPX
        cpu.instruction = "CPX";
        cpu.cycles += 4;
        compare(&cpu.x, byte);
        break;
        default:
        printf("Broken Instruction 0x%x\n", cpu.opcode);
        cpu.fail();
        break;
    }
    sprintf(cpu.asm_args, "$%02X%02X", cpu.high, cpu.low);
}

/*
These instructions are always of 2 bytes length and perform in 2 CPU cycles, 
if the branch is not taken (the condition resolving to 'false'), 
and 3 cycles, if the branch is taken (when the condition is true). 
If a branch is taken and the target is on a different page, this adds another CPU cycle (4 in total).
*/

void branch() {
    READ_BYTE_PC(cpu.low)
    uint16_t targetPC = cpu.pc + ((int8_t) cpu.low);
    sprintf(cpu.asm_args, "$%02X", targetPC);
    cpu.asm_argc = 2;
    switch (cpu.opcode) {
        case 0x10: // BPL branch on plus (negative clear)
        cpu.instruction = "BPL";
        if (IS_FLAG_OFF(negative))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x30: // BMI branch on minus (negative set)
        cpu.instruction = "BMI";
        if (IS_FLAG_ON(negative))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x50: // BVC branch on overflow clear
        cpu.instruction = "BVC";
        if (IS_FLAG_OFF(overflow))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x70: // BVS branch on overflow set
        cpu.instruction = "BVS";
        if (IS_FLAG_ON(overflow))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0x90: // BCC branch on carry clear
        cpu.instruction = "BCC";
        if (IS_FLAG_OFF(carry))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xB0: // BCS branch on carry set
        cpu.instruction = "BCS";
        if (IS_FLAG_ON(carry))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xD0: // BNE branch on not equal (zero clear)
        cpu.instruction = "BNE";
        if (IS_FLAG_OFF(zero))
            goto actuallyBranch;
        goto branchFail;
        break;

        case 0xF0: // BEQ branch on equal (zero set)
        cpu.instruction = "BEQ";
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
    if (GET_PAGE_NUM(cpu.pc) != GET_PAGE_NUM(targetPC))
        cpu.cycles += 4;
    else
        cpu.cycles += 3;
    cpu.pc = targetPC;
    return;
}

void column0x18() {
    cpu.asm_argc = 1;
    cpu.cycles += 2;
    switch (cpu.opcode) {
        case 0x18: // CLC
        cpu.instruction = "CLC";
        cpu.status &= ~carry;
        break;

        case 0x38: // SEC
        cpu.instruction = "SEC";
        cpu.status |= carry;
        break;

        case 0x58: // CLI
        cpu.instruction = "CLI";
        cpu.status &= ~irq;
        break;

        case 0x78: // SEI
        cpu.instruction = "SEI";
        cpu.status |= irq;
        break;

        case 0x98: // TYA
        cpu.instruction = "TYA";
        cpu.a = cpu.y;
        SET_ZERO_FLAG(cpu.a);
        SET_NEG_FLAG(cpu.a);
        break;

        case 0xB8: // CLV
        cpu.instruction = "CLV";
        cpu.status &= ~overflow;
        break;

        case 0xD8: // CLD
        cpu.instruction = "CLD";
        cpu.status &= ~decimal;
        break;
        
        case 0xF8: // SED
        cpu.instruction = "SED";
        cpu.status |= decimal;
        break;
    }
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
    uint16_t result = (uint16_t) cpu.a + (uint16_t) *byte + carryBit;
    //printf("ADC %s\n", cpu.instruction);
    cpu.instruction = "ADC";
    SET_CARRY_FLAG((uint16_t) cpu.a + (uint16_t) *byte + (uint16_t) carryBit > 0xFF)
    //SET_CARRY_FLAG(cpu.a, *byte, carryBit)
    SET_OVERFLOW_FLAG(cpu.a, *byte, carryBit)
    cpu.a = result & 0xFF;
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
    uint16_t result = (uint16_t) cpu.a - *byte - borrowBit;
    printf("CPU.A: %X, BYTE: %X, RESULT: %X, STATUS: %X\n", cpu.a, *byte, result, cpu.status);
    SET_CARRY_FLAG(((uint16_t) cpu.a >= borrowBit + *byte))
    //SET_CARRY_FLAG((result & 0xFF00) > 0)
    //SET_CARRY_FLAG(cpu.a, *byte, carryBit)
    SET_OVERFLOW_FLAG(cpu.a, -1 * *byte, -1 * borrowBit)
    printf("CPU.A: %X, BYTE: %X, RESULT: %X, STATUS: %X\n", cpu.a, *byte, result, cpu.status);
    cpu.a = result & 0xFF;
    SET_ZERO_FLAG(cpu.a)
    SET_NEG_FLAG(cpu.a)
}


/*RMW Instructions*/
void ASL(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ASL %s\n", cpu.instruction);
    cpu.instruction = "ASL";
    if (immediate) {
        SET_CARRY_FLAG((cpu.a & 0x80) > 0)
        cpu.a = cpu.a << 1;
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
    }
    else {
        SET_CARRY_FLAG((*byte & 0x80) > 0)
        *byte = *byte << 1;
        SET_ZERO_FLAG(*byte)
        SET_NEG_FLAG(*byte)
        cpu_write(*addr, byte);
    }
}

void LSR(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("LSR %s\n", cpu.instruction);
    cpu.instruction = "LSR";
    //SET_NEG_FLAG(*byte)
    cpu.status &= ~negative;
    if (immediate) {
        SET_CARRY_FLAG((cpu.a & 0x01) > 0)
        cpu.a = cpu.a >> 1;
        SET_ZERO_FLAG(cpu.a)
    } else {
        SET_CARRY_FLAG((*byte & 0x01) > 0)
        *byte = *byte >> 1;
        SET_ZERO_FLAG(*byte)
        cpu_write(*addr, byte);
    }
}

void ROL(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ROL %s\n", cpu.instruction);
    cpu.instruction = "ROL";
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    if (immediate) {
        SET_CARRY_FLAG((cpu.a & 0x80) > 0)
        cpu.a = (cpu.a << 1) | carryBit;
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
    }
    else {
        SET_CARRY_FLAG((*byte & 0x80) > 0)
        *byte = (*byte << 1) | carryBit;
        SET_ZERO_FLAG(*byte)
        SET_NEG_FLAG(*byte)
        cpu_write(*addr, byte);
    }
}

void ROR(uint8_t *byte, uint16_t *addr, bool immediate) {
    //printf("ROR %s\n", cpu.instruction);
    cpu.instruction = "ROR";
    uint16_t carryBit = ((cpu.status & carry) > 0) ? 1 : 0;
    //SET_CARRY_FLAG(*byte, 0, 0)
    if (immediate) {
        SET_CARRY_FLAG(cpu.a & 0x01)
        cpu.a = (cpu.a >> 1) | (carryBit << 7);
        SET_ZERO_FLAG(cpu.a)
        SET_NEG_FLAG(cpu.a)
    }
    else {
        SET_CARRY_FLAG(*byte & 0x01)
        *byte = (*byte >> 1) | (carryBit << 7);
        SET_ZERO_FLAG(*byte)
        SET_NEG_FLAG(*byte)
        cpu_write(*addr, byte);
    }
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
        SET_ZERO_FLAG(cpu.x)
        SET_NEG_FLAG(cpu.x)
        return;
    }
    cpu.instruction = "DEC";
    //printf("DEC %s\n", cpu.instruction);
    (*byte)--;
    cpu_write(*addr, byte);
    SET_ZERO_FLAG(*byte)
    SET_NEG_FLAG(*byte)
}

/* Will isolate Control assembly instructions */
void handleControl() {
    uint16_t addr;
    uint8_t byte;

    switch (cpu.opcode % 0x20) {
        case 0x00: // impl, abs, # first half interrupt stuff/second half cmp
        column0x00();
        break;

        case 0x04: // zeropage
        byte = zeropage(&addr, 0);
        column0x04(&byte, &addr);
        break;

        case 0x08: // impl Mostly Stack and register stuff
        column0x08();
        break;

        case 0x0C: // abs
        byte = absolute(&addr, 0);
        column0x0C(&byte, &addr);
        break;

        case 0x10: // rel branching
        branch();
        break;

        case 0x14: // zeropage, X
        byte = zeropage(&addr, cpu.x);
        if (cpu.opcode == 0x94) {
            // STY
            cpu.instruction = "STY";
            cpu.asm_argc = 2;
            sprintf(cpu.asm_args, "$%02X,x", cpu.low);
            cpu.cycles += 4;
            cpu_write(addr, &cpu.y);
            //printf("STY not implemented\n");
            //cpu.fail();
        } else if (cpu.opcode == 0xB4) {
            // LDY
            cpu.cycles += 4;
            cpu.y = byte;
            SET_NEG_FLAG(cpu.y)
            SET_ZERO_FLAG(cpu.y)
        } else {
            printf("Broken instruction 0x%X\n", cpu.opcode);
            cpu.fail();
        }
        break;

        case 0x18: // impl mostly flag stuff
        column0x18();
        break;

        case 0x1C: // absolute, X
        if (cpu.opcode == 0xBC) {
            //LDY
            byte = absolute(&addr, cpu.x);
            sprintf(cpu.asm_args, "$%X%X,x", cpu.high, cpu.low);
            if ((GET_PAGE_NUM(addr - cpu.x) != GET_PAGE_NUM(addr)))
                cpu.cycles += 1;
            cpu.cycles += 4;
            cpu.y = byte;
            SET_NEG_FLAG(cpu.y)
            SET_ZERO_FLAG(cpu.y)
        } else {
            printf("Broken instruction 0x%X\n", cpu.opcode);
            cpu.fail();
        }
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
        READ_BYTE_FROM_ADDR(cpu.pc, cpu.low)
        sprintf(cpu.asm_args, "($%02X,x)", cpu.low);
        cpu.low += cpu.x;
        SET_ADDR(addr, 0)
        READ_BYTE_FROM_ADDR(addr, cpu.low)
        READ_BYTE_FROM_ADDR(((addr & 0xFF) == 0xFF) ? (addr & 0xFF00) : addr + 1, cpu.high)
        SET_ADDR(addr, 0)
        if (func != &STA)
            READ_BYTE_FROM_ADDR(addr, byte)
        cpu.pc++;
        //assembly = "(d,x)";
        cpu.asm_argc = 2;
        cpu.cycles += 6;
        func(&byte, &addr, false);
        break;

        case 0x05: // zeropage
        byte = zeropage(&addr, 0);
        sprintf(cpu.asm_args, "$%02X", cpu.low);
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
        sprintf(cpu.asm_args, "$%02X%02X", cpu.high, cpu.low);
        //assembly = "a";
        cpu.cycles += 4;
        func(&byte, &addr, false);
        break;

        case 0x11: // (indirect), Y
        //cpu.pc++;
        READ_BYTE_FROM_ADDR(cpu.pc, cpu.low)
        sprintf(cpu.asm_args, "($%02X),y", cpu.low);
        SET_ADDR(addr, 0)
        READ_BYTE_FROM_ADDR(addr, cpu.low)
        READ_BYTE_FROM_ADDR( ((addr & 0xFF) == 0xFF) ? (addr & 0xFF00) : addr + 1, cpu.high)
        SET_ADDR(addr, 0)
        if ((GET_PAGE_NUM(addr + cpu.y) != GET_PAGE_NUM(addr)) || func == STA) // STA always does a lot of cycles
            cpu.cycles += 1;
        addr += cpu.y;
        if (func != STA)
            READ_BYTE_FROM_ADDR(addr, byte)
        cpu.pc++;
        
        cpu.asm_argc = 3;
        //assembly = "(d),y"; // STA always does 6 cycles
        cpu.cycles += 5;
        func(&byte, &addr, false);
        break;

        case 0x15: // zeropage, X
        byte = zeropage(&addr, cpu.x);
        sprintf(cpu.asm_args, "$%02X,x", cpu.low - cpu.x);
        //assembly = "d,x";
        cpu.cycles += 4;
        func(&byte, &addr, false);
        break;

        case 0x19: // absolute, Y
        byte = absolute(&addr, cpu.y);
        sprintf(cpu.asm_args, "$%02X%02X,y", cpu.high, cpu.low);
        //assembly = "a,y"; // // STA always does 5 cycles
        if ((GET_PAGE_NUM(addr - cpu.y) != GET_PAGE_NUM(addr)) || func == STA) // STA always does a lot of cycles
            cpu.cycles += 1;
        cpu.cycles +=  4; // add 1 to cycles if page boundary is crossed
        func(&byte, &addr, false);
        break;

        case 0x1D: // absolute, X
        byte = absolute(&addr, cpu.x); // STA always does 5 cycles
        if ((GET_PAGE_NUM(addr - cpu.x) != GET_PAGE_NUM(addr)) || func == STA) // STA always does a lot of cycles
            cpu.cycles += 1;
        cpu.cycles += 4; // add 1 to cycles if page boundary is crossed
        sprintf(cpu.asm_args, "$%02X%02X,x", cpu.high, cpu.low);
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
            sprintf(cpu.asm_args, "#%02X", byte);
            LDX(&byte, 0, false);
            cpu.asm_argc = 2;
        } else {
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        }
        break;
        
        case 0x06: // zeropage
        byte = zeropage(&addr, 0);
        sprintf(cpu.asm_args, "$%02X", cpu.low);
        //assembly = "d";
        cpu.cycles += (func == &LDX || func == &STX) ? 3 : 5; // LDX is only 3 cycles
        func(&byte, &addr, false);
        break;

        case 0x0A: // Works on registers
        cpu.cycles += 2;
        cpu.asm_argc = 1;
        func(&byte, &addr, true);
        break;

        case 0x0E: // absolute
        byte = absolute(&addr, 0);
        sprintf(cpu.asm_args, "$%02X%02X", cpu.high, cpu.low);
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
            sprintf(cpu.asm_args, "$%02X,y", cpu.low - cpu.y);
            //assembly = "d,y";
        } else {
            byte = zeropage(&addr, cpu.x);
            sprintf(cpu.asm_args, "$%02X,x", cpu.low - cpu.x);
            //assembly = "d,x";
        }
        cpu.cycles += (func == &LDX || func == &STX) ? 4 : 6; // LDX and STX are only 4 cycles
        func(&byte, &addr, false);
        break;

        case 0x1A: // impl
        // Only TXS and TSX
        if (cpu.opcode == 0x9A) { 
            // TXS
            //printf("TXS\n");
            cpu.instruction = "TXS";
            cpu.sp = cpu.x;
            cpu.cycles += 2;
            
        } else if (cpu.opcode == 0xBA) {
            // TSX
            //printf("TSX\n");
            cpu.instruction = "TSX";
            cpu.x = cpu.sp;
            cpu.cycles += 2;
            SET_ZERO_FLAG(cpu.x)
            SET_NEG_FLAG(cpu.x)
        } else {
            printf("Broken Instruction 0x%x\n", cpu.opcode);
            cpu.fail();
        }
        cpu.asm_argc = 1;
        break;

        case 0x1E: // absolute, X or absolute, Y
        // Need to figure out how many cycles
        if (cpu.opcode == 0xBE) { // LDX uses absolute, Y
            byte = absolute(&addr, cpu.y);
            if ((GET_PAGE_NUM(addr - cpu.y) != GET_PAGE_NUM(addr)))
                cpu.cycles += 1;
            cpu.cycles += 4;
            sprintf(cpu.asm_args, "$%02X%02X,y", cpu.high, cpu.low);
            //assembly = "a,y";
        } else {
            byte = absolute(&addr, cpu.x);
            cpu.cycles += 7;
            sprintf(cpu.asm_args, "$%02X%02X,x", cpu.high, cpu.low);
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