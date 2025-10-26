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
        case ADD_EXT:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case ADD_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ADDS_IMM:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = (in.result >> 63) & 1;
            stat_inst_retire++;
            break; 
        case ADDS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = (in.result >> 63) & 1;
            stat_inst_retire++;
            break;
        case AND_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ANDS_SHIFTR:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = (in.result >> 63) & 1;
            stat_inst_retire++;
            break;
        case EOR_SHIFTR:
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
        case LSR_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case MOVZ:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case MUL:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
        case SUB_IMM:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++; 
            break;
        case SUB_EXT:
            write_register(in.RD_REG, in.result); 
            stat_inst_retire++;
            break; 
        case SUBS_IMM:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = (in.result >> 63) & 1;
            stat_inst_retire++;
            break;
        case SUBS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = (in.result >> 63) & 1;
            stat_inst_retire++;
            break;
        case HLT:
            RUN_BIT = 0;
            stat_inst_retire++;
            break;
        case STUR_32:
            stat_inst_retire++;
            break;
        case STUR_64:
            stat_inst_retire++;
            break;
        case STURB:
            stat_inst_retire++;
            break;
        case STURH:
            stat_inst_retire++;
            break; 
        case LDUR_32:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LDUR_64:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case LDURH:
            write_register(in.RT_REG, in.MEM_DATA);
            stat_inst_retire++;
            break;
        case CMP_EXT:
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = ((in.result) >> 63) & 1;
            stat_inst_retire++;
            break;
        case CMP_IMM:
            pipe.FLAG_Z = (in.result == 0);
            pipe.FLAG_N = ((in.result) >> 63) & 1;
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
        case ADD_EXT:
            break;
        case ADD_IMM:
            break;
        case ADDS_IMM:
            break; 
        case ADDS_EXT:
            break;
        case AND_SHIFTR:
            break; 
        case ANDS_SHIFTR:
            break;
        case EOR_SHIFTR:
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
        case LSR_IMM:
            break;
        case MOVZ:
            break;
        case MUL:
            break; 
        case SUB_IMM:
            break; 
        case SUB_EXT:
            break; 
        case SUBS_IMM:
            break;
        case SUBS_EXT:
            break;
        case HLT:
            break;
        case STUR_32:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFFFFFF));
            break;
        case STUR_64:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFFFFFF));
            mem_write_32(in.MEM_ADDRESS + 4, (uint32_t)(in.RT_VAL >> 32));
            break;
        case STURB:
        {
            uint32_t word = mem_read_32(in.MEM_ADDRESS & ~3);
            int byte_offset = in.MEM_ADDRESS & 3;
            uint32_t mask = 0xFF << (byte_offset * 8);
            uint32_t new_word = (word & ~mask) | ((in.RT_VAL & 0xFF) << (byte_offset * 8));
            mem_write_32(in.MEM_ADDRESS & ~3, new_word);
            break;
        }
        case STURH:
            mem_write_32(in.MEM_ADDRESS, (uint32_t)(in.RT_VAL & 0xFFFF));
            break;
        case LDUR_32:
            MEM_to_WB_CURRENT.MEM_DATA = (int32_t) mem_read_32(in.MEM_ADDRESS);
            break;
        case LDUR_64:
        {
            uint32_t low_word = mem_read_32(in.MEM_ADDRESS);
            uint32_t high_word = mem_read_32(in.MEM_ADDRESS + 4);
            MEM_to_WB_CURRENT.MEM_DATA = ((uint64_t)high_word << 32) | low_word;
            break;
        }
        case LDURH:
            MEM_to_WB_CURRENT.MEM_DATA = (int16_t) mem_read_32(in.MEM_ADDRESS);
            break;
        case CMP_EXT:
            break;
        case CMP_IMM:
            break;
    }
}

void pipe_stage_execute()
{
    Pipe_Op in = DE_to_EX_PREV;
    if (in.NOP) { EX_to_MEM_CURRENT.NOP = 1; return; }

    EX_to_MEM_CURRENT = in;

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            EX_to_MEM_CURRENT.result = in.RN_VAL + in.RM_VAL;
            break;
        case ADD_IMM:
            EX_to_MEM_CURRENT.result = in.RN_VAL + in.IMM;
            break;
        case ADDS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case ADDS_EXT:
            EX_to_MEM_CURRENT.result = in.RN_VAL + in.RM_VAL;
            break;
        
        case AND_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            break;
        case ANDS_SHIFTR:
            EX_to_MEM_CURRENT.result = (in.RN_VAL & in.RM_VAL);
            break;
        case EOR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL ^ EX_to_MEM_CURRENT.RM_VAL);
            break;
        case LSL_IMM:
        {
            uint32_t actual_shift = (64 - in.SHAM) % 64;
            EX_to_MEM_CURRENT.result = in.RN_VAL << actual_shift;
            break;
        }
        case LSR_IMM:
        {
            EX_to_MEM_CURRENT.result = in.RN_VAL >> in.SHAM;
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
        case MUL:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL * EX_to_MEM_CURRENT.RM_VAL;
            break; 
        }
        case SUB_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            break;
        }
        case SUB_EXT:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            break; 
        }
        case SUBS_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            break;
        }
        case SUBS_EXT:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            break; 
        }
        case HLT:
            break;
        case STUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case STUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case STURB:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case STURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case LDUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case LDUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case LDURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = in.RN_VAL + in.IMM;
            break;
        case CMP_EXT:
            EX_to_MEM_CURRENT.result = in.RN_VAL - in.RM_VAL;
            break;
        case CMP_IMM:
            EX_to_MEM_CURRENT.result = in.RN_VAL - in.IMM;
            break;
    }
}


void pipe_stage_decode()
{
    Pipe_Op in = IF_to_DE_PREV;
    if (in.NOP) { DE_to_EX_CURRENT.NOP = 1; return; }
    
    uint64_t current_instruction = in.raw_instruction;

    //ADD(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x458)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }

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
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x558)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }

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
        return;
    }

    //ANDS(SHIFTED REGISTER) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x750)) {
        DE_to_EX_CURRENT.INSTRUCTION = ANDS_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }


    //EOR(SHIFTED REG) - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xCA)) {
        DE_to_EX_CURRENT.INSTRUCTION = EOR_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }

    //ORR (SHIFTED REG) - F - orr.s

    //LDUR (32-AND 64-BIT) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
    }

    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
    }

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
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        return;
    }

    //LSL(IMM) - F
    if ((!(extract_bits(current_instruction, 22, 31) ^ 0x34D)) && ((extract_bits(current_instruction, 10, 15) != 0x3F))){
 
        DE_to_EX_CURRENT.INSTRUCTION = LSL_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
        DE_to_EX_CURRENT.READ_MEM = 1;
        return; 
    }

    //LSR (IMM) - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x34D) && !(extract_bits(current_instruction, 10, 15) ^ 0x3F)){
        DE_to_EX_CURRENT.INSTRUCTION = LSR_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
        DE_to_EX_CURRENT.READ_MEM = 1;
        return;
    }

    // MOVZ
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)) {
        DE_to_EX_CURRENT.INSTRUCTION = MOVZ;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.IMM    = (uint32_t)extract_bits(current_instruction, 5, 20);
        return;
    }

    //STUR (32-AND 64-BIT VAR) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        return;
    }
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        return;
    }

    //STURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C0)){
        DE_to_EX_CURRENT.INSTRUCTION = STURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20);
        return;
    }

    //STURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
    }

    //SUB(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xD1)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 10, 21); 
        return;
    }

    //SUB (EXT) - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x658)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }

    //SUBS - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3C4) && (extract_bits(current_instruction, 0, 4) != 31)){
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
        return;
    }

    //SUBS (EXT) K :)
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758) && (extract_bits(current_instruction, 0, 4) != 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }


    //MUL - F - mul.s
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x4D8)){
        DE_to_EX_CURRENT.INSTRUCTION = MUL;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }


    // HLT
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)) {
        DE_to_EX_CURRENT.INSTRUCTION = HLT;
        HLT_NEXT = 1;
        return;
    }
    //CMP (EXT) K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758)  && (extract_bits(current_instruction, 0, 4) == 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = CMP_EXT;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        return;
    }

    // CMP (IMM) K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3c4) && (extract_bits(current_instruction, 0, 4) == 31)){
        DE_to_EX_CURRENT.INSTRUCTION = CMP_IMM;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
        return;
    }

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