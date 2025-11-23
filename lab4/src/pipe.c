
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

//STALLING LOGIC
Pipe_Op SAVED_INSTRUCTION;
int INSTRUCTION_SAVED = 0;
uint64_t REFETCH_ADDR  = 0;
int REFETCH_PC = 0;
int REFETCH_PC_NEXT = 0;
int UPDATE_EX = 0;
int UPDATE_EX_NEXT = 0;

int HLT_FLAG = 0;
int HLT_NEXT = 0;
int RUN_BIT;
uint64_t NEXT_PC;

int CLEAR_DE = 0;
int SQUASH = 0;
int BRANCH = 0;
int BRANCH_NEXT = 0;


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

void unconditional_branching(void) {
    if (!(MEM_to_WB_CURRENT.BR_TARGET == IF_to_DE_CURRENT.PC)) {
        // || !(MEM_to_WB_CURRENT.BR_TARGET == pipe.PC)
        SQUASH = 1;
        stat_squash ++;
        CLEAR_DE = 1;
    }
}

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;
    RUN_BIT = TRUE;
    NEXT_PC = pipe.PC;
	bp_t_init();
	pipe.bp = &bp;
}


void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
	pipe_stage_execute();
	pipe_stage_decode();
    pipe_stage_fetch();

    pipe.PC = NEXT_PC;

    REFETCH_PC = REFETCH_PC_NEXT;
    REFETCH_PC_NEXT = 0;
    UPDATE_EX = UPDATE_EX_NEXT;
    UPDATE_EX_NEXT = 0;
    BRANCH = BRANCH_NEXT;
    BRANCH_NEXT = 0;

    HLT_FLAG = HLT_NEXT;

    CLEAR_DE = 0;

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
    if (in.NOP || in.INSTRUCTION == UNKNOWN) return;

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
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break; 
        case ADDS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case AND_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break; 
        case ANDS_SHIFTR:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
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
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case SUBS_EXT:
            write_register(in.RD_REG, in.result);
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
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
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case CMP_IMM:
            pipe.FLAG_Z = in.FLAG_Z;
            pipe.FLAG_N = in.FLAG_N;
            stat_inst_retire++;
            break;
        case BR:
            stat_inst_retire++;
            break;
        case B:
            stat_inst_retire++;
            break;
        case BEQ:
            stat_inst_retire++;
            break;
        case BNE:
            stat_inst_retire++;
            break;
        case BLT:
            stat_inst_retire++;
            break;
        case BLE:
            stat_inst_retire++;
            break;
        case BGT:
            stat_inst_retire++;
            break;
        case BGE:
            stat_inst_retire++;
            break;
        case CBNZ:
            stat_inst_retire++;
            break;
        case CBZ:
            stat_inst_retire++;
            break; 
        case ORR_SHIFTR:
            write_register(in.RD_REG, in.result);
            stat_inst_retire++;
            break;
    }
}

void pipe_stage_mem()
{
    memset(&MEM_to_WB_CURRENT, 0, sizeof(MEM_to_WB_CURRENT));
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
        case BR:
            break;
        case B:
            break;
        case BEQ:
            break;
        case BNE:
            break;
        case BLT:
            break;
        case BLE:
            break;
        case BGT:
            break;
        case BGE:
            break;
        case CBNZ:
            break;
        case CBZ:
            break;
        case ORR_SHIFTR:
            break;
}
}

void pipe_stage_execute()
{
    memset(&EX_to_MEM_CURRENT, 0, sizeof(EX_to_MEM_CURRENT));
    Pipe_Op in = DE_to_EX_PREV;

    if (UPDATE_EX) {
        in = SAVED_INSTRUCTION;
        UPDATE_EX = 0;
    } else {
    }
    if (in.NOP) { EX_to_MEM_CURRENT.NOP = 1; return; }

    EX_to_MEM_CURRENT = in;


    // Forward RN_VAL
    if (in.READS_RN) {
        if (EX_to_MEM_PREV.WRITES_REG && 
            EX_to_MEM_PREV.RD_REG == in.RN_REG && 
            EX_to_MEM_PREV.RD_REG != 31) {
            EX_to_MEM_CURRENT.RN_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG && 
                 MEM_to_WB_PREV.RD_REG == in.RN_REG && 
                 MEM_to_WB_PREV.RD_REG != 31) {
            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.result;
        } 
        else if (MEM_to_WB_PREV.LOAD && 
                 MEM_to_WB_PREV.RT_REG == in.RN_REG && 
                 MEM_to_WB_PREV.RT_REG != 31) {
            EX_to_MEM_CURRENT.RN_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Forward RM_VAL
    if (in.READS_RM) {
        if (EX_to_MEM_PREV.WRITES_REG && 
            EX_to_MEM_PREV.RD_REG == in.RM_REG && 
            EX_to_MEM_PREV.RD_REG != 31) {
            EX_to_MEM_CURRENT.RM_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG && 
                MEM_to_WB_PREV.RD_REG == in.RM_REG && 
                MEM_to_WB_PREV.RD_REG != 31) {
            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.result;
        }
        else if (MEM_to_WB_PREV.LOAD && 
                 MEM_to_WB_PREV.RT_REG == in.RM_REG && 
                 MEM_to_WB_PREV.RT_REG != 31) {
            EX_to_MEM_CURRENT.RM_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Forward for STORE
    if (in.STORE) {
    if (EX_to_MEM_PREV.WRITES_REG &&
        EX_to_MEM_PREV.RD_REG == in.RT_REG &&
        in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = EX_to_MEM_PREV.result;
    }
    else if (MEM_to_WB_PREV.WRITES_REG &&
             MEM_to_WB_PREV.RD_REG == in.RT_REG &&
             in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.result;
    }
    else if (MEM_to_WB_PREV.LOAD &&
             MEM_to_WB_PREV.RT_REG == in.RT_REG &&
             in.RT_REG != 31) {
        EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.MEM_DATA;
    }
}

    if (in.INSTRUCTION == CBNZ || in.INSTRUCTION == CBZ) {
        if (EX_to_MEM_PREV.WRITES_REG &&
            EX_to_MEM_PREV.RD_REG == in.RT_REG &&
            in.RT_REG != 31) {

            EX_to_MEM_CURRENT.RT_VAL = EX_to_MEM_PREV.result;
        }
        else if (MEM_to_WB_PREV.WRITES_REG &&
                MEM_to_WB_PREV.RD_REG == in.RT_REG &&
                in.RT_REG != 31) {
            EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.result;
        }
        else if (MEM_to_WB_PREV.LOAD &&
                MEM_to_WB_PREV.RT_REG == in.RT_REG &&
                in.RT_REG != 31) {
            EX_to_MEM_CURRENT.RT_VAL = MEM_to_WB_PREV.MEM_DATA;
        }
    }

    // Add this before your switch statement in pipe_stage_execute()

    // Forward flags if needed
    int use_forwarded_flags = 0;
    int forwarded_flag_n = 0;
    int forwarded_flag_z = 0;

    // Check if we need flags and if there's a flag-setting instruction to forward from
    if (in.INSTRUCTION == BGT || in.INSTRUCTION == BLT || in.INSTRUCTION == BLE || 
        in.INSTRUCTION == BGE || in.INSTRUCTION == BEQ || in.INSTRUCTION == BNE) {
        
        // Check if EX stage has a flag-setting instruction
        if (EX_to_MEM_CURRENT.INSTRUCTION == CMP_EXT || EX_to_MEM_CURRENT.INSTRUCTION == CMP_IMM ||
            EX_to_MEM_CURRENT.INSTRUCTION == SUBS_EXT || EX_to_MEM_CURRENT.INSTRUCTION == SUBS_IMM ||
            EX_to_MEM_CURRENT.INSTRUCTION == ADDS_EXT || EX_to_MEM_CURRENT.INSTRUCTION == ADDS_IMM ||
            EX_to_MEM_CURRENT.INSTRUCTION == ANDS_SHIFTR) {
            use_forwarded_flags = 1;
            forwarded_flag_n = EX_to_MEM_CURRENT.FLAG_N;
            forwarded_flag_z = EX_to_MEM_CURRENT.FLAG_Z;
        }
        // Check if MEM stage has a flag-setting instruction
        else if (EX_to_MEM_PREV.INSTRUCTION == CMP_EXT || EX_to_MEM_PREV.INSTRUCTION == CMP_IMM ||
                EX_to_MEM_PREV.INSTRUCTION == SUBS_EXT || EX_to_MEM_PREV.INSTRUCTION == SUBS_IMM ||
                EX_to_MEM_PREV.INSTRUCTION == ADDS_EXT || EX_to_MEM_PREV.INSTRUCTION == ADDS_IMM ||
                EX_to_MEM_PREV.INSTRUCTION == ANDS_SHIFTR) {
            use_forwarded_flags = 1;
            forwarded_flag_n = EX_to_MEM_PREV.FLAG_N;
            forwarded_flag_z = EX_to_MEM_PREV.FLAG_Z;
        }
    }

    switch (in.INSTRUCTION) {
        case ADD_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            break;
        case ADD_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case ADDS_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;
        case ADDS_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;
        
        case AND_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            break;
        case ANDS_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL & EX_to_MEM_CURRENT.RM_VAL);
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;
        
        case EOR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL ^ EX_to_MEM_CURRENT.RM_VAL);
            break;
        case LSL_IMM:
        {
            uint32_t actual_shift = (64 - EX_to_MEM_CURRENT.SHAM) % 64;
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL << actual_shift;
            break;
        }
        case LSR_IMM:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL >> EX_to_MEM_CURRENT.SHAM;
            break;
        }
        case LDURB:
        {
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        }
        case MOVZ:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.IMM;
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
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break;
        }
        case SUBS_EXT:
        {
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = (EX_to_MEM_CURRENT.result >> 63) & 1;
            break; 
        }
        case HLT:
            break;
        case STUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case STUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case STURB:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case STURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case LDUR_32:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case LDUR_64:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case LDURH:
            EX_to_MEM_CURRENT.MEM_ADDRESS = EX_to_MEM_CURRENT.RN_VAL + EX_to_MEM_CURRENT.IMM;
            break;
        case CMP_EXT:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.RM_VAL;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = ((EX_to_MEM_CURRENT.result) >> 63) & 1;
            break;
        case CMP_IMM:
            EX_to_MEM_CURRENT.result = EX_to_MEM_CURRENT.RN_VAL - EX_to_MEM_CURRENT.IMM;
            EX_to_MEM_CURRENT.FLAG_Z = (EX_to_MEM_CURRENT.result == 0);
            EX_to_MEM_CURRENT.FLAG_N = ((EX_to_MEM_CURRENT.result) >> 63) & 1;
            break;
        case B:
            EX_to_MEM_CURRENT.BR_TARGET = in.PC + (in.IMM << 2);
            break;
        case BR:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.RN_VAL;
            break;
        case BEQ:
            EX_to_MEM_CURRENT.BR_TARGET = in.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_Z;
            break;
        case BNE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_Z;
            break;
        case BLT:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_N;
            break;
        case BLE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = EX_to_MEM_PREV.FLAG_N || EX_to_MEM_PREV.FLAG_Z;
            break;
        case BGT:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_N && !EX_to_MEM_PREV.FLAG_Z;
            break;
        case BGE:
            EX_to_MEM_CURRENT.BR_TARGET = EX_to_MEM_CURRENT.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = !EX_to_MEM_PREV.FLAG_N;
            break;
        case CBNZ:
            EX_to_MEM_CURRENT.BR_TARGET = in.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = (EX_to_MEM_CURRENT.RT_VAL != 0);
            break;
        
        case CBZ:
            EX_to_MEM_CURRENT.BR_TARGET = in.PC + (in.IMM << 2);
            EX_to_MEM_CURRENT.BR_TAKEN = (EX_to_MEM_CURRENT.RT_VAL == 0);
            break; 

        case ORR_SHIFTR:
            EX_to_MEM_CURRENT.result = (EX_to_MEM_CURRENT.RN_VAL | EX_to_MEM_CURRENT.RM_VAL);
            break;
        default:
            break;
		}
		if (EX_to_MEM_CURRENT.UBRANCH || EX_to_MEM_CURRENT.CBRANCH) {
            // printf("BRANCH_EXECUTE: PC=0x%lx, PREDICTED=0x%lx, ACTUAL=0x%lx, TAKEN=%d, BTB_MISS=%d\n",
                // EX_to_MEM_CURRENT.PC, EX_to_MEM_CURRENT.PREDICTED_PC, 
                // EX_to_MEM_CURRENT.BR_TARGET, EX_to_MEM_CURRENT.BR_TAKEN, EX_to_MEM_CURRENT.BTB_MISS);
            
            bp_update(&EX_to_MEM_CURRENT);

            uint64_t correct_pc;
            if (EX_to_MEM_CURRENT.UBRANCH){
                correct_pc = EX_to_MEM_CURRENT.BR_TARGET;
            } else {
                correct_pc = EX_to_MEM_CURRENT.BR_TAKEN ? EX_to_MEM_CURRENT.BR_TARGET : EX_to_MEM_CURRENT.PC + 4;
            }

            if (EX_to_MEM_CURRENT.PREDICTED_PC != correct_pc) {
                // ("MISPREDICTION: correcting PC from 0x%lx to 0x%lx\n", 
                    // pipe.PC, correct_pc);
                NEXT_PC = correct_pc;
                stat_squash++;
                CLEAR_DE = 1;
                memset(&IF_to_DE_CURRENT, 0, sizeof(IF_to_DE_CURRENT));
                IF_to_DE_CURRENT.NOP = 1;
            } else {
                // printf("PREDICTION_CORRECT\n");
            }
            
            // if (EX_to_MEM_CURRENT.PREDICTED_PC != EX_to_MEM_CURRENT.BR_TARGET) {
            //     printf("MISPREDICTION: correcting PC from 0x%lx to 0x%lx\n", 
            //         pipe.PC, EX_to_MEM_CURRENT.BR_TARGET);
            //     NEXT_PC = EX_to_MEM_CURRENT.BR_TARGET;
            //     //pipe_pc = EX_to_MEM_CURRENT.BR_TARGET;
            //     stat_squash++;
            //     CLEAR_DE = 1;
            //     memset(&IF_to_DE_CURRENT, 0, sizeof(IF_to_DE_CURRENT));
            //     IF_to_DE_CURRENT.NOP = 1;
            // } else if (EX_to_MEM_CURRENT.BTB_MISS) {
            //     printf("BTB_MISS_CORRECTION: PC to 0x%lx\n", EX_to_MEM_CURRENT.BR_TARGET);
            //     NEXT_PC = EX_to_MEM_CURRENT.BR_TARGET;
            //     //pipe_pc = EX_to_MEM_CURRENT.BR_TARGET;
            //     stat_squash++;
            //     CLEAR_DE = 1;
            //     memset(&IF_to_DE_CURRENT, 0, sizeof(IF_to_DE_CURRENT));
            //     IF_to_DE_CURRENT.NOP = 1;
            // } else {
            //     printf("PREDICTION_CORRECT\n");
            // }
        }
} 

void pipe_stage_decode()
{
    memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));

    Pipe_Op in = IF_to_DE_PREV;
    if (in.NOP) { DE_to_EX_CURRENT.NOP = 1; return; }

    if (CLEAR_DE || HLT_FLAG) {
        DE_to_EX_CURRENT.NOP = 1;
        // CLEAR_DE = 0;
        return;
    }
    uint64_t current_instruction = in.raw_instruction;

    DE_to_EX_CURRENT.PC = in.PC; 
    DE_to_EX_CURRENT.PREDICTED_PC = in.PREDICTED_PC;
    DE_to_EX_CURRENT.BTB_MISS     = in.BTB_MISS;
    DE_to_EX_CURRENT.GHR_XOR_PC   = in.GHR_XOR_PC;

    //ADD(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x458)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    // ADD_IMM
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x91)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADD_IMM;
        DE_to_EX_CURRENT.RD_REG  = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG  = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL  = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.IMM  = extract_bits(current_instruction, 10, 21);
    }

    //ADDS(EXTENDED) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x558)) {
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ADDS(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB1)){
        DE_to_EX_CURRENT.INSTRUCTION = ADDS_IMM; 
        DE_to_EX_CURRENT.RD_REG= extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = immediate;
    }

    //CBNZ - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB5)) {
        DE_to_EX_CURRENT.INSTRUCTION = CBNZ;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    //CBZ - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xB4)){
    DE_to_EX_CURRENT.INSTRUCTION = CBZ;
    DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
    DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
    uint32_t immediate = extract_bits(current_instruction, 5, 23);
    DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
    DE_to_EX_CURRENT.CBRANCH = 1;
}


    //AND(SHIFTED REG) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x8A)){
        DE_to_EX_CURRENT.INSTRUCTION = AND_SHIFTR; 
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG); 
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ANDS(SHIFTED REGISTER) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x750)) {
        DE_to_EX_CURRENT.INSTRUCTION = ANDS_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    //EOR(SHIFTED REG) - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xCA)) {
        DE_to_EX_CURRENT.INSTRUCTION = EOR_SHIFTR;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //ORR (SHIFTED REG) - F - orr.s
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xAA)){
    DE_to_EX_CURRENT.INSTRUCTION = ORR_SHIFTR;
    DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
    DE_to_EX_CURRENT.WRITES_REG = 1;
    DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
    DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
    DE_to_EX_CURRENT.READS_RN = 1;
    DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
    DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
    DE_to_EX_CURRENT.READS_RM = 1;
}

    //LDUR (32-AND 64-BIT) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LDURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C2)){
        DE_to_EX_CURRENT.INSTRUCTION = LDURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20); 
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20); 
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LDURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C2)) {
        DE_to_EX_CURRENT.INSTRUCTION = LDURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.READ_MEM = 1;
        DE_to_EX_CURRENT.LOAD = 1;
    }

    //LSL(IMM) - F
    if ((!(extract_bits(current_instruction, 22, 31) ^ 0x34D)) && ((extract_bits(current_instruction, 10, 15) != 0x3F))){
 
        DE_to_EX_CURRENT.INSTRUCTION = LSL_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
    }

    //LSR (IMM) - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x34D) && !(extract_bits(current_instruction, 10, 15) ^ 0x3F)){
        DE_to_EX_CURRENT.INSTRUCTION = LSR_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.SHAM = extract_bits(current_instruction, 16, 21);
    }

    // MOVZ
    if (!(extract_bits(current_instruction, 23, 30) ^ 0xA5)) {
        DE_to_EX_CURRENT.INSTRUCTION = MOVZ;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.IMM    = (uint32_t)extract_bits(current_instruction, 5, 20);
    }

    //STUR (32-AND 64-BIT VAR) - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x5C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_32;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x7C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STUR_64;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //STURB - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x1C0)){
        DE_to_EX_CURRENT.INSTRUCTION = STURB;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 12, 20);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //STURH - K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x3C0)) {
        DE_to_EX_CURRENT.INSTRUCTION = STURH;
        DE_to_EX_CURRENT.RT_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.RT_VAL = read_register(DE_to_EX_CURRENT.RT_REG);
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 12, 20);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 8);
        DE_to_EX_CURRENT.STORE = 1;
    }

    //SUB(IMM) - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0xD1)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 10, 21); 
    }

    //SUB (EXT) - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x658)){
        DE_to_EX_CURRENT.INSTRUCTION = SUB_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    //SUBS - K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3C4) && (extract_bits(current_instruction, 0, 4) != 31)){
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_IMM;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
    }

    //SUBS (EXT) K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758) && (extract_bits(current_instruction, 0, 4) != 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = SUBS_EXT;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    //MUL - F
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x4D8)){
        DE_to_EX_CURRENT.INSTRUCTION = MUL;
        DE_to_EX_CURRENT.RD_REG = extract_bits(current_instruction, 0, 4);
        DE_to_EX_CURRENT.WRITES_REG = 1;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9); 
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG  = extract_bits(current_instruction, 16, 20); 
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }


    // HLT
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6a2)) {
        DE_to_EX_CURRENT.INSTRUCTION = HLT;
        HLT_NEXT = 1;
    }
    //CMP (EXT) K
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x758)  && (extract_bits(current_instruction, 0, 4) == 31)) {
        DE_to_EX_CURRENT.INSTRUCTION = CMP_EXT;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        DE_to_EX_CURRENT.RM_REG = extract_bits(current_instruction, 16, 20);
        DE_to_EX_CURRENT.RM_VAL = read_register(DE_to_EX_CURRENT.RM_REG);
        DE_to_EX_CURRENT.READS_RM = 1;
    }

    // CMP (IMM) K
    if (!(extract_bits(current_instruction, 22, 31) ^ 0x3c4) && (extract_bits(current_instruction, 0, 4) == 31)){
        DE_to_EX_CURRENT.INSTRUCTION = CMP_IMM;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1;
        uint32_t immediate = extract_bits(current_instruction, 10, 21);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 11);
    }

    //BR - K -
    if (!(extract_bits(current_instruction, 21, 31) ^ 0x6B0)){
        DE_to_EX_CURRENT.INSTRUCTION = BR;
        DE_to_EX_CURRENT.RN_REG = extract_bits(current_instruction, 5, 9);
        DE_to_EX_CURRENT.RN_VAL = read_register(DE_to_EX_CURRENT.RN_REG);
        DE_to_EX_CURRENT.READS_RN = 1; 
        DE_to_EX_CURRENT.UBRANCH = 1; 
    }


    //B - F
    if (!(extract_bits(current_instruction, 26, 31) ^ 0x5)){
        DE_to_EX_CURRENT.INSTRUCTION = B;
        uint32_t immediate = extract_bits(current_instruction, 0, 25);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 25);
        DE_to_EX_CURRENT.UBRANCH = 1;
    }

    //BEQ - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && (extract_bits(current_instruction, 0, 3) == 0x0)){
        DE_to_EX_CURRENT.INSTRUCTION = BEQ;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }


    //BNE, BLT, BLE - F
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54)){
        unsigned int cond_code = extract_bits(current_instruction, 0, 3);
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM  = bit_extension(immediate, 5, 23);
        DE_to_EX_CURRENT.CBRANCH = 1;

        switch (cond_code){
            case 0x1:
                DE_to_EX_CURRENT.INSTRUCTION = BNE;
                break;
            case 0xB:
                DE_to_EX_CURRENT.INSTRUCTION = BLT;
                break;
            case 0xD:
                DE_to_EX_CURRENT.INSTRUCTION = BLE;
                break;
        }
    }

    //BGT - K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && (extract_bits(current_instruction, 0, 3) == 0xC)){
        DE_to_EX_CURRENT.INSTRUCTION = BGT;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    //BGE K
    if (!(extract_bits(current_instruction, 24, 31) ^ 0x54) && (extract_bits(current_instruction, 0, 3) == 0xA)){
        DE_to_EX_CURRENT.INSTRUCTION = BGE;
        uint32_t immediate = extract_bits(current_instruction, 5, 23);
        DE_to_EX_CURRENT.IMM = bit_extension(immediate, 0, 18);
        DE_to_EX_CURRENT.CBRANCH = 1;
    }

    if (DE_to_EX_PREV.LOAD && !DE_to_EX_PREV.NOP) {
        int load_target = DE_to_EX_PREV.RT_REG;
        int need_stall = 0;
        
        if (DE_to_EX_CURRENT.READS_RN && DE_to_EX_CURRENT.RN_REG == load_target && load_target != 31) {
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.READS_RM && DE_to_EX_CURRENT.RM_REG == load_target && load_target != 31) {
            need_stall = 1;
        }
        if (DE_to_EX_CURRENT.STORE && DE_to_EX_CURRENT.RT_REG == load_target && load_target != 31) {
            need_stall = 1;
        }
        
        if (need_stall) {
            SAVED_INSTRUCTION = DE_to_EX_CURRENT;
            memset(&DE_to_EX_CURRENT, 0, sizeof(DE_to_EX_CURRENT));
            DE_to_EX_CURRENT.NOP = 1;

            REFETCH_ADDR     = IF_to_DE_PREV.PC + 4;
            REFETCH_PC_NEXT  = 1;

            CLEAR_DE = 1;
            return;
        }

    }
}

void pipe_stage_fetch()
{  
    static Pipe_Op fetched_instruction;
    static int cycle_count = 0;
    cycle_count++;
    // printf("FETCH: PC=0x%lx, HLT=%d\n", pipe.PC, HLT_FLAG);

    if (!HLT_FLAG) {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));

        // pick the right PC to fetch from
        uint64_t fetch_pc = pipe.PC;
        if (CLEAR_DE) {
            // execute just told us to go somewhere else
            fetch_pc = NEXT_PC;
        }

        uint32_t raw_inst = mem_read_32(fetch_pc);
        fetched_instruction.raw_instruction = raw_inst;
        fetched_instruction.PC = fetch_pc;
        bp_predict(&fetched_instruction);
        fetched_instruction.NOP = 0;
        fetched_instruction.INSTRUCTION = UNKNOWN;

        // printf("FETCHED: PC=0x%lx, inst=0x%x, predicted_pc=0x%lx\n",
            //    fetched_instruction.PC, raw_inst, fetched_instruction.PREDICTED_PC);

        // only let fetch advance PC if we are NOT in the middle of a squash
        if (!CLEAR_DE && !SQUASH) {
            NEXT_PC = fetched_instruction.PREDICTED_PC;
        }
    } else {
        memset(&fetched_instruction, 0, sizeof(Pipe_Op));
        fetched_instruction.NOP = 1;
        fetched_instruction.INSTRUCTION = UNKNOWN;
        // printf("FETCH_NOP: HLT active\n");
    }

    if (!CLEAR_DE){
        IF_to_DE_CURRENT = fetched_instruction;
    }

    
}