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
Pipe_Reg_IFtoDE if_to_de_reg;      // Changed names!
Pipe_Reg_DEtoEX de_to_ex_reg;
Pipe_Reg_EXtoMEM ex_to_mem_reg;
Pipe_Reg_MEMtoWB mem_to_wb_reg;

int RUN_BIT;

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
    switch(mem_to_wb_reg.INSTRUCTION){

        case ADD_IMM:
        {
            write_register(mem_to_wb_reg.RD_REG, mem_to_wb_reg.result);
            break; 
        }
        case MOVZ:
            write_register(mem_to_wb_reg.RD_REG, mem_to_wb_reg.result);
            break;
    }

    
}

void pipe_stage_mem()
{
    switch(ex_to_mem_reg.INSTRUCTION){

        case ADD_IMM:
        { //pass water through, no memory access needed at this stage
            mem_to_wb_reg.RD_REG = ex_to_mem_reg.RD_REG;
            mem_to_wb_reg.result = ex_to_mem_reg.result;

        }

        case MOVZ:
            mem_to_wb_reg.RD_REG = ex_to_mem_reg.RD_REG;
            mem_to_wb_reg.result = ex_to_mem_reg.result; 
            break;
    }
    mem_to_wb_reg.INSTRUCTION = ex_to_mem_reg.INSTRUCTION;
}

void pipe_stage_execute()
{
    switch(de_to_ex_reg.INSTRUCTION){

        case ADD_IMM:
        {
            ex_to_mem_reg.RD_REG = de_to_ex_reg.RD_REG;
            ex_to_mem_reg.RN_REG = de_to_ex_reg.RN_REG;
            ex_to_mem_reg.result = de_to_ex_reg.RN_VALUE + de_to_ex_reg.IMM;
            break; 
        }
        case MOVZ:
            ex_to_mem_reg.RD_REG = de_to_ex_reg.RD_REG;
            ex_to_mem_reg.result = de_to_ex_reg.IMM;
            break;

        case HLT:
            RUN_BIT = 0;
            break;
            
    }
    ex_to_mem_reg.INSTRUCTION = de_to_ex_reg.INSTRUCTION; 
    
}

void pipe_stage_decode()
{
    uint64_t current_instruction = if_to_de_reg.raw_instruction;

    //ADD(EXTENDED) - K

    //ADD(IMM) - F

    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)){
        de_to_ex_reg.INSTRUCTION = ADD_IMM;
        de_to_ex_reg.RD_REG  = extract_bits(current_instruction, 0, 4);
        de_to_ex_reg.RN_REG = extract_bits(current_instruction, 5, 9);
        de_to_ex_reg.RN_VALUE = read_register(de_to_ex_reg.RN_REG);
        de_to_ex_reg.IMM = extract_bits(current_instruction, 10, 21);
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
        de_to_ex_reg.INSTRUCTION = MOVZ;
        de_to_ex_reg.RD_REG = extract_bits(current_instruction, 0, 4);
        de_to_ex_reg.IMM = (uint32_t) extract_bits(current_instruction, 5, 20);
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
        de_to_ex_reg.INSTRUCTION = HLT;
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
    if_to_de_reg.raw_instruction = mem_read_32(pipe.PC);
    pipe.PC = pipe.PC + 4; 
}
