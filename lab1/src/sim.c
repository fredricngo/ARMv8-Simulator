#include <stdio.h>
#include "shell.h"

uint32_t extract_bits(uint32_t instruction, int start, int end){
    //Given an instruction type, returns a section from start:end (inclusive)
    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

void fetch()
{
    //Given the instruction address, we retrieve from memory using mem_read_32
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);

    //Extract the operands using extract_bits, need to check opcodes and specific bit orientations

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

    //MOVZ: shift
    if (!(extract_bits(instruction, 23, 30) ^ 0xA5)){
        uint32_t movz_rd = extract_bits(instruction, 0, 4);
        uint32_t imm = extract_bits(instruction, 5, 20);
        uint32_t hw = extract_bits(instruction, 21, 22);
        uint32_t opc = extract_bits(instruction, 29, 30); 
        uint32_t sf = extract_bits(instruction, 31, 31);
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

void decode()
{

}

void execute()
{

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
