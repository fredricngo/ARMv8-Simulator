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

    //ADD(EXTENDED)

    //ADD(IMM)

    //CBNZ

    //CBZ

    //AND(SHIFTED REG)

    //ANDS(SHIFTED REGISTER)

    //EOR(SHIFTED REG)

    //ORR (SHIFTED REG)

    //LDUR (32-AND 64-BIT)

    //LDURB 

    //LDURH 

    //LSL(IMM)

    //LSR (IMM)

    //MOVZ: !!Shift not required!! 
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)){
        instruction_type = MOVZ;
        rd = extract_bits(current_instruction, 0, 4);
        immediate = extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR)

    //STURB

    //STURH

    //SUB

    //SUBS

    //MUL

    //HLT

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

void execute()
{
    //Checks the global instruction parameters, then performs the relevant operations
    switch(instruction_type) {

        case MOVZ:
            if (extract_bits(current_instruction, 31, 31)){
                //we update NEW_STATE registers
                //We move the immediate into the register
                //only need to implement 64-bit variant, no shift needed
                NEXT_STATE.REGS[rd] = (uint64_t) immediate;
            }
            break;
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
