/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* global pipeline state */
Pipe_State pipe;
Pipe_Op IF_to_DE_CURRENT;
Pipe_Op DE_to_EX_CURRENT;
Pipe_Op EX_to_MEM_CURRENT;
Pipe_Op MEM_to_WB_CURRENT;

Pipe_Op IF_to_DE_PREV;
Pipe_Op DE_to_EX_PREV;
Pipe_Op EX_to_MEM_PREV;
Pipe_Op MEM_to_WB_PREV;

int RUN_BIT;
int HLT_FLAG = 0;

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
	RUN_BIT = true;
}

uint32_t extract_bits(uint32_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}


void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
	pipe_stage_fetch();
    IF_to_DE_PREV = IF_to_DE_CURRENT;
    DE_to_EX_PREV = DE_to_EX_CURRENT;
    EX_to_MEM_PREV = EX_to_MEM_CURRENT;
    MEM_to_WB_PREV = MEM_to_WB_CURRENT;
}


void write_register(int reg_num, int64_t value){
    if (reg_num != 31){
        pipe.REGS[reg_num] = value; 
    }
}

int64_t read_register(int reg_num){
    if (reg_num == 31){
        return 0;
    }
    return pipe.REGS[reg_num];
}

void pipe_stage_wb()
{
    MEM_to_WB_CURRENT = EX_to_MEM_PREV;
    if (MEM_to_WB_CURRENT.NOP){
        return;
    }

    switch(MEM_to_WB_CURRENT.INSTRUCTION){

        case ADD_IMM:
        {
            write_register(MEM_to_WB_CURRENT.RD_REG, MEM_to_WB_CURRENT.result);
            break; 
        }
        case MOVZ:
            write_register(MEM_to_WB_CURRENT.RD_REG, MEM_to_WB_CURRENT.result);
            break;
        case HLT:
            RUN_BIT = 0;
            break;

    }

    
}

void pipe_stage_mem()
{
    EX_to_MEM_CURRENT = DE_to_EX_PREV;
    if (EX_to_MEM_CURRENT.NOP){
        return;
    }
    switch(EX_to_MEM_CURRENT.INSTRUCTION){

        case ADD_IMM:
            break;

        case MOVZ:
            break;
        case HLT:
            break;
    }
}

void pipe_stage_execute()
{
    DE_to_EX_CURRENT = IF_to_DE_PREV;
    if (DE_to_EX_CURRENT.NOP){
        return;
    }
    switch(DE_to_EX_CURRENT.INSTRUCTION){

        case ADD_IMM:
        {
            DE_to_EX_CURRENT.result = DE_to_EX_CURRENT.RN_VAL + DE_to_EX_CURRENT.IMM;
            break; 
        }
        case MOVZ:
            DE_to_EX_CURRENT.result = DE_to_EX_CURRENT.IMM;
            break;

        case HLT:
            break;
            
    }
}

void pipe_stage_decode()
{
    if (IF_to_DE_CURRENT.NOP){
        return;
    }
    uint64_t current_instruction = IF_to_DE_CURRENT.raw_instruction;

    //ADD(EXTENDED) - K

    //ADD(IMM) - F

    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)){
        IF_to_DE_CURRENT.INSTRUCTION = ADD_IMM;
        IF_to_DE_CURRENT.RD_REG  = extract_bits(current_instruction, 0, 4);
        IF_to_DE_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        IF_to_DE_CURRENT.RN_VAL = read_register(IF_to_DE_CURRENT.RN_REG);
        IF_to_DE_CURRENT.IMM = extract_bits(current_instruction, 10, 21);
    }


    //ADDS(EXTENDED) - K

    //ADDS(IMM) - F

    //CBNZ - K

    //CBZ - F

    //AND(SHIFTED REG) - F

    //ANDS(SHIFTED REGISTER) - K

    //EOR(SHIFTED REG) - K

    //ORR (SHIFTED REG) - F - orr.s

    //LDUR (32-AND 64-BIT) - K

    //LDURB - F

    //LDURH - K

    //LSL(IMM) - F

    //LSR (IMM) - K

    //MOVZ - F
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)){
        IF_to_DE_CURRENT.INSTRUCTION = MOVZ;
        IF_to_DE_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        IF_to_DE_CURRENT.IMM = (uint32_t) extract_bits(current_instruction, 5, 20);
        }


    //STUR (32-AND 64-BIT VAR) - K

    //STURB - F

    //STURH - K

    //SUB(IMM) - F

    //SUB (EXT) - F

    //SUBS - K

    //SUBS (EXT) K :)

    //MUL - F - mul.s

    //HLT:
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)){
        IF_to_DE_CURRENT.INSTRUCTION = HLT;
        HLT_FLAG = 1;
    }

    //CMP (EXT) K

    // CMP (IMM) K

    //BR - K

    //B - F

    //BEQ - K

    //BNE, BLT, BLE - F

    //BGT - K

    //BGE K
}

void pipe_stage_fetch()
{  
    if (!HLT_FLAG) {
        IF_to_DE_CURRENT.raw_instruction = mem_read_32(pipe.PC);
        pipe.PC = pipe.PC + 4; 
    } else {
        IF_to_DE_CURRENT.NOP = 1;
    }   
}
