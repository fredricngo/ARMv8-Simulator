#include <stdio.h>
#include "shell.h"

typedef enum {
    UNKNOWN = 0,
    ADD_EXT,
    ADD_IMM, 
    ADDS_EXT, 
    ADDS_IMM,
    CBNZ, 
    CBZ, 
    AND_SHIFTR,
    ANDS_SHIFTR,
    EOR_SHIFTR, 
    ORR_SHIFTR, 
    LDUR,
    LDURB, 
    LDURH, 
    LSL_IMM,
    LSR_IMM, 
    MOVZ,
    STUR, 
    STURB, 
    STURH, 
    SUB_EXT, 
    SUB_IMM,
    SUBS_EXT, 
    SUBS_IMM, 
    MUL, 
    HLT, 
    CMP_EXT, 
    CMP_IMM, 
    BR,
    B, 
    BEQ, 
    BNE, 
    BGT, 
    BLT, 
    BGE, 
    BLE
} instruction_type_t;

instruction_type_t instruction_type;
uint32_t current_instruction;
uint32_t rd, rn, rm;
uint32_t rt; 
uint32_t immediate; 
uint32_t shift_amount;

uint32_t extract_bits(uint32_t instruction, int start, int end){
    //Given an instruction type, returns a section from start:end (inclusive)
    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

void fetch()
{
    //Given the instruction address, we retrieve from memory using mem_read_32
    current_instruction = mem_read_32(CURRENT_STATE.PC);
}

void decode()
{
    //Extract the operands using extract_bits, need to check opcodes and specific bit orientations
    //Need to update the registers
    instruction_type = UNKNOWN;

    //ADD(EXTENDED) - K

    //ADD(IMM)
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)){
        instruction_type = ADD_IMM;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        immediate = extract_bits(current_instruction, 10, 21);
    }

    //ADDS(EXTENDED) - K

    //ADDS(IMM) - F

    //CBNZ - K

    //CBZ - F
 
    //AND(SHIFTED REG)
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        instruction_type = AND_SHIFTR;
        rd = extract_bits(current_instruction, 0, 4);
        rn = extract_bits(current_instruction, 5, 9);
        rm = extract_bits(current_instruction, 16, 20);
    }

    //ANDS(SHIFTED REGISTER) - K

    //EOR(SHIFTED REG) - K

    //ORR (SHIFTED REG) - F

    //LDUR (32-AND 64-BIT) - K

    //LDURB - F

    //LDURH - K

    //LSL(IMM) - F

    //LSR (IMM) - K

    //MOVZ: !!Shift not required!! 
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)){
        instruction_type = MOVZ;
        rd = extract_bits(current_instruction, 0, 4);
        immediate = extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR) - K

    //STURB - F

    //STURH - K

    //SUB(IMM) - F

    //SUB (EXT) - F

    //SUBS - K

    //MUL - F

    //HLT:
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)){
        instruction_type = HLT;
    }
    //CMP - K

    //BR - K

    //B - F

    //BEQ - K

    //BNE - F

    //BGT - K

    //BLT - F

    //BGE - K
    
    //BLE - F
}

void execute()
{
    uint32_t result;
    //Checks the global instruction parameters, then performs the relevant operations
    switch(instruction_type) {

        //ADD(EXTENDED)

        case ADD_IMM: 
            result = CURRENT_STATE.REGS[rn] + immediate;
            NEXT_STATE.REGS[rd] = (uint64_t) result;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ADDS(EXTENDED)

        //ADDS(IMM) 

        //AND(SHIFTED REG)
        case AND_SHIFTR:
            result = CURRENT_STATE.REGS[rn] & CURRENT_STATE.REGS[rm];
            NEXT_STATE.REGS[rd] = (uint64_t) result;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;

        //ANDS(SHIFTED REGISTER)

        //CBNZ

        //CBZ

        //EOR(SHIFTED REG)

        //ORR (SHIFTED REG)

        //LDUR (32-AND 64-BIT)

        //LDURB 

        //LDURH 

        //LSL(IMM)

        //LSR (IMM)

        case MOVZ: // haven't implemented shift
            //we update NEW_STATE registers
            //We move the immediate into the register
            //only need to implement 64-bit variant, no shift needed
            NEXT_STATE.REGS[rd] = (uint64_t) immediate;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
        
        //STUR (32-AND 64-BIT VAR)

        //STURB

        //STURH

        //SUB(IMM)

        //SUB (EXT)

        //SUBS

        //MUL
        
        case HLT:
            //if HLT, we just set RUN_BIT to 0
            RUN_BIT = 0;
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
            break;
            
        
        //CMP

        //BR

        //B

        //BEQ

        //BNE

        //BGT

        //BLT

        //BGE
        
        //BLE
        
    }
    
}

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    fetch();
    decode();
    execute();

}
