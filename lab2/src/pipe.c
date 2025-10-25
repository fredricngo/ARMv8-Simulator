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

int HLT_FLAG = 0;
int HLT_NEXT = 0;
int RUN_BIT;


int64_t bit_extension(int32_t immediate, int start, int end){
    /*
    This function receives an immediate (int32_t) and: 
    (1) Checks if the MSB is set to 1 (signed int)
    (2) if (1), extends to a uint64_t for arithmetic.
    */

    int width = end - start + 1;
    if ((immediate >> (width - 1)) & 1) {
        uint64_t mask = ~((1ULL << width) - 1); 
        return (int64_t) (immediate | mask);
    } else {
        return (int64_t) immediate; 
    }
}

uint32_t extract_bits(uint32_t instruction, int start, int end){
    /* Given an instruction type, returns a section from start: end (inclusive)*/

    int width = end - start + 1;
    uint32_t mask = (1U << width) - 1;
    return (instruction >> start) & mask;
}

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
    RUN_BIT = TRUE;
}


void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
    pipe_stage_fetch();

    HLT_FLAG = HLT_NEXT;

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
    Pipe_Op in = MEM_to_WB_PREV;
    if (in.NOP) return;

    switch (in.INSTRUCTION) {
        case ADD_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ADDS_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case AND_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case LDURB:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LSL_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case MOVZ:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case HLT:
            RUN_BIT = 0;
            stat_inst_retire++;
            break;
    }
}

void pipe_stage_mem()
{
    Pipe_Op in = EX_to_MEM_PREV;
    if (in.NOP) { MEM_to_WB_CURRENT.NOP = 1; return; }

    MEM_to_WB_CURRENT = in;

    switch (in.INSTRUCTION) {
        case ADD_IMM:
            break;
        case ADDS_IMM:
            break; 
        case AND_SHIFTR:
            break; 
        case LDURB:
        {
            uint32_t word = mem_read_32(MEM_to_WB_CURRENT.MEM_ADDRESS & ~3);
            int byte_offset = MEM_to_WB_CURRENT.MEM_ADDRESS & 3;
            uint8_t byte_data = (word >> (byte_offset * 8)) & 0xFF;
            MEM_to_WB_CURRENT.MEM_DATA = (uint64_t) byte_data;
            break;
        }
        case LSL_IMM:
            break;
        case MOVZ:
            break;
        case HLT:
            break;
    }
}

void pipe_stage_execute()
{
    Pipe_Op in = DE_to_EX_PREV;
    if (in.NOP) { EX_to_MEM_CURRENT.NOP = 1; return; }

    EX_to_MEM_CURRENT = in;

    switch (in.INSTRUCTION) {
        case ADD_IMM:
            EX_to_MEM_CURRENT.result = in.RN_VAL + in.IMM;
            break;
        case ADDS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            pipe.FLAG_Z = (EX_to_MEM_CURRENT.result == 0) ? 1 : 0;
            pipe.FLAG_N = (EX_to_MEM_CURRENT.result < 0) ? 1 : 0;
            break;
        
        case AND_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            break;
        case LSL_IMM:
        {
            uint32_t actual_shift = (64 - in.IMM) % 64;
            EX_to_MEM_CURRENT.result = in.RN_VAL << actual_shift;
            break;
        }
        case LDURB:
        {
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        }
        case MOVZ:
            EX_to_MEM_CURRENT.result = in.IMM;
            break;
        case HLT:
            break;
    }
}


void pipe_stage_decode()
{
    Pipe_Op in = IF_to_DE_PREV;
    if (in.NOP) { DE_to_EX_CURRENT.NOP = 1; return; }

    uint64_t current_instruction = in.raw_instruction;

    //ADD(EXTENDED) - K

    // ADD_IMM
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_IMM;
        DE_to_EX_CURRENT.RD_REG  = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG  = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL  = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.IMM     = extract_bits(current_instruction, 10, 21);
        return;
    }

    //ADDS(EXTENDED) - K

    //ADDS(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB1)){
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_IMM; 
        DE_to_EX_CURRENT.RD_REG= extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 10, 21);
        return;
    }

    //CBNZ - K

    //CBZ - F

    //AND(SHIFTED REG) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        DE_to_EX_CURRENT.INSTRUCTION = AND_SHIFTR; 
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG); 
    }

    //ANDS(SHIFTED REGISTER) - K

    //EOR(SHIFTED REG) - K

    //ORR (SHIFTED REG) - F - orr.s

    //LDUR (32-AND 64-BIT) - K

    //LDURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C2)){
        DE_to_EX_CURRENT.INSTRUCTION = LDURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20); 
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20); 
        return; 
    }

    //LDURH - K

    //LSL(IMM) - F
    if ((!(extract_bits(current_instruction, 22, 31) ^ 0x34D)) && ((extract_bits(current_instruction, 10, 15) != 0x3F))){
 
        DE_to_EX_CURRENT.INSTRUCTION = LSL_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.IMM = extract_bits(current_instruction, 16, 21);
        DE_to_EX_CURRENT.READ_MEM = 1;
        return; 
    }

    //LSR (IMM) - K

    // MOVZ
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)) {
        DE_to_EX_CURRENT.INSTRUCTION = MOVZ;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.IMM    = (uint32_t)extract_bits(current_instruction, 5, 20);
        return;
    }

    //STUR (32-AND 64-BIT VAR) - K

    //STURB - F

    //STURH - K

    //SUB(IMM) - F

    //SUB (EXT) - F

    //SUBS - K

    //SUBS (EXT) K :)

    //MUL - F - mul.s

    // HLT
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)) {
        DE_to_EX_CURRENT.INSTRUCTION = HLT;
        HLT_NEXT = 1;
        return;
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
    static Pipe_Op fetched_instruction;
    
    if (!HLT_FLAG) {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        fetched_instruction.raw_instruction = mem_read_32(pipe.PC);
        fetched_instruction.PC = pipe.PC;
        fetched_instruction.NOP = 0;
        fetched_instruction.INSTRUCTION = UNKNOWN;
        pipe.PC = pipe.PC + 4; 
    } else {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        fetched_instruction.NOP = 1;
        fetched_instruction.INSTRUCTION = UNKNOWN;
    }

    IF_to_DE_CURRENT = fetched_instruction;
}